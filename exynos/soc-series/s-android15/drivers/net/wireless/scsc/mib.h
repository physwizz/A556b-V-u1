/****************************************************************************
 *
 * Copyright (c) 2014 - 2024 Samsung Electronics Co., Ltd. All rights reserved
 *
 ****************************************************************************/

/* Note: this is an auto-generated file. */

#ifndef SLSI_MIB_H__
#define SLSI_MIB_H__

#ifdef __cplusplus
extern "C" {
#endif

struct slsi_mib_data {
	u32 dataLength;
	u8  *data;
};

#define SLSI_MIB_MAX_INDEXES 2U

#define SLSI_MIB_TYPE_BOOL    0
#define SLSI_MIB_TYPE_UINT    1
#define SLSI_MIB_TYPE_INT     2
#define SLSI_MIB_TYPE_OCTET   3U
#define SLSI_MIB_TYPE_NONE    4

struct slsi_mib_value {
	u8 type;
	union {
		bool                 boolValue;
		s32                  intValue;
		u32                  uintValue;
		struct slsi_mib_data octetValue;
	} u;
};

struct slsi_mib_entry {
	u16                   psid;
	u16                   index[SLSI_MIB_MAX_INDEXES]; /* 0 = no Index */
	struct slsi_mib_value value;
};

struct slsi_mib_get_entry {
	u16 psid;
	u16 index[SLSI_MIB_MAX_INDEXES]; /* 0 = no Index */
};

#define SLSI_MIB_STATUS_SUCCESS                     0x0000
#define SLSI_MIB_STATUS_UNKNOWN_PSID                0x0001
#define SLSI_MIB_STATUS_INVALID_INDEX               0x0002
#define SLSI_MIB_STATUS_OUT_OF_RANGE                0x0003
#define SLSI_MIB_STATUS_WRITE_ONLY                  0x0004
#define SLSI_MIB_STATUS_READ_ONLY                   0x0005
#define SLSI_MIB_STATUS_UNKNOWN_INTERFACE_TAG       0x0006
#define SLSI_MIB_STATUS_INVALID_NUMBER_OF_INDICES   0x0007
#define SLSI_MIB_STATUS_ERROR                       0x0008
#define SLSI_MIB_STATUS_UNSUPPORTED_ON_INTERFACE    0x0009
#define SLSI_MIB_STATUS_UNAVAILABLE                 0x000A
#define SLSI_MIB_STATUS_NOT_FOUND                   0x000B
#define SLSI_MIB_STATUS_INCOMPATIBLE                0x000C
#define SLSI_MIB_STATUS_OUT_OF_MEMORY               0x000D
#define SLSI_MIB_STATUS_TO_MANY_REQUESTED_VARIABLES 0x000E
#define SLSI_MIB_STATUS_NOT_TRIED                   0x000F
#define SLSI_MIB_STATUS_FAILURE                     0xFFFF

/*******************************************************************************
 *
 * NAME
 *  slsi_mib_encode_get Functions
 *
 * DESCRIPTION
 *  For use when getting data from the Wifi Stack.
 *  These functions append the encoded data to the "buffer".
 *
 *  index == 0 where there is no index required
 *
 * EXAMPLE
 *  {
 *      static const struct slsi_mib_get_entry getValues[] = {
 *          { PSID1, { 0, 0 } },
 *          { PSID2, { 3, 0 } },
 *      };
 *      struct slsi_mib_data buffer;
 *      slsi_mib_encode_get_list(&buffer,
 *                              sizeof(getValues) / sizeof(struct slsi_mib_get_entry),
 *                              getValues);
 *  }
 *  or
 *  {
 *      struct slsi_mib_data buffer = {0, NULL};
 *      slsi_mib_encode_get(&buffer, PSID1, 0);
 *      slsi_mib_encode_get(&buffer, PSID2, 3);
 *  }
 * RETURN
 *  SlsiResult: See SLSI_MIB_STATUS_*
 *
 *******************************************************************************/
void slsi_mib_encode_get(struct slsi_mib_data *buffer, u16 psid, u16 index);
int slsi_mib_encode_get_list(struct slsi_mib_data *buffer, u16 psidsLength, const struct slsi_mib_get_entry *psids);

/*******************************************************************************
 *
 * NAME
 *  SlsiWifiMibdEncode Functions
 *
 * DESCRIPTION
 *  For use when getting data from the Wifi Stack.
 *
 *  index == 0 where there is no index required
 *
 * EXAMPLE
 *  {
 *      static const struct slsi_mib_get_entry getValues[] = {
 *          { PSID1, { 0, 0 } },
 *          { PSID2, { 3, 0 } },
 *      };
 *      struct slsi_mib_data buffer = rxMibData; # Buffer with encoded Mib Data
 *
 *      getValues = slsi_mib_decode_get_list(&buffer,
 *                                      sizeof(getValues) / sizeof(struct slsi_mib_get_entry),
 *                                      getValues);
 *
 *      print("PSID1 = %d\n", getValues[0].u.uintValue);
 *      print("PSID2.3 = %s\n", getValues[1].u.boolValue?"TRUE":"FALSE");
 *
 *      kfree(getValues);
 *
 *  }
 *  or
 *  {
 *      u8* buffer = rxMibData; # Buffer with encoded Mib Data
 *      size_t offset=0;
 *      struct slsi_mib_entry value;
 *
 *      offset += slsi_mib_decode(&buffer[offset], &value);
 *      print("PSID1 = %d\n", value.u.uintValue);
 *
 *      offset += slsi_mib_decode(&buffer[offset], &value);
 *      print("PSID2.3 = %s\n", value.u.boolValue?"TRUE":"FALSE");
 *
 *  }
 *
 *******************************************************************************/
size_t slsi_mib_decode(struct slsi_mib_data *buffer, struct slsi_mib_entry *value);
struct slsi_mib_value *slsi_mib_decode_get_list(struct slsi_mib_data *buffer, u16 psidsLength, const struct slsi_mib_get_entry *psids);

/*******************************************************************************
 *
 * NAME
 *  slsi_mib_encode Functions
 *
 * DESCRIPTION
 *  For use when setting data in the Wifi Stack.
 *  These functions append the encoded data to the "buffer".
 *
 *  index == 0 where there is no index required
 *
 * EXAMPLE
 *  {
 *      u8 octets[2] = {0x00, 0x01};
 *      struct slsi_mib_data buffer = {0, NULL};
 *      slsi_mib_encode_bool(&buffer, PSID1, TRUE, 0);                     # Boolean set with no index
 *      slsi_mib_encode_int(&buffer, PSID2, -1234, 1);                     # Signed Integer set with on index 1
 *      slsi_mib_encode_uint(&buffer, PSID2, 1234, 3);                     # Unsigned Integer set with on index 3
 *      slsi_mib_encode_octet(&buffer, PSID3, sizeof(octets), octets, 0);  # Octet set with no index
 *  }
 *  or
 *  {
 # Unsigned Integer set with on index 3
 #      struct slsi_mib_data buffer = {0, NULL};
 #      struct slsi_mib_entry value;
 #      value.psid = psid;
 #      value.index[0] = 3;
 #      value.index[1] = 0;
 #      value.value.type = SLSI_MIB_TYPE_UINT;
 #      value.value.u.uintValue = 1234;
 #      slsi_mib_encode(buffer, &value);
 #  }
 # RETURN
 #  See SLSI_MIB_STATUS_*
 #
 *******************************************************************************/
u16 slsi_mib_encode(struct slsi_mib_data *buffer, struct slsi_mib_entry *value);
u16 slsi_mib_encode_bool(struct slsi_mib_data *buffer, u16 psid, bool value, u16 index);
u16 slsi_mib_encode_int(struct slsi_mib_data *buffer, u16 psid, s32 value, u16 index);
u16 slsi_mib_encode_uint(struct slsi_mib_data *buffer, u16 psid, u32 value, u16 index);
u16 slsi_mib_encode_octet(struct slsi_mib_data *buffer, u16 psid, size_t dataLength, const u8 *data, u16 index);

/*******************************************************************************
 *
 * NAME
 *  SlsiWifiMib Low level Encode/Decode functions
 *
 *******************************************************************************/
size_t slsi_mib_encode_uint32(u8 *buffer, u32 value);
size_t slsi_mib_encode_int32(u8 *buffer, s32 signedValue);
size_t slsi_mib_encode_octet_str(u8 *buffer, struct slsi_mib_data *octetValue);

size_t slsi_mib_decodeUint32(u8 *buffer, u32 *value);
size_t slsi_mib_decodeInt32(u8 *buffer, s32 *value);
size_t slsi_mib_decodeUint64(u8 *buffer, u64 *value);
size_t slsi_mib_decodeInt64(u8 *buffer, s64 *value);
size_t slsi_mib_decode_octet_str(u8 *buffer, struct slsi_mib_data *octetValue);

/*******************************************************************************
 *
 * NAME
 *  SlsiWifiMib Helper Functions
 *
 *******************************************************************************/

/* Find a the offset to psid data in an encoded buffer
 * {
 *      struct slsi_mib_data buffer = rxMibData;                 # Buffer with encoded Mib Data
 *      struct slsi_mib_get_entry value = {PSID1, {0x01, 0x00}};   # Find value for PSID1.1
 *      u8* mibdata = slsi_mib_find(&buffer, &value);
 *      if(mibdata) {print("Mib Data for PSID1.1 Found\n");
 *  }
 */
u8 *slsi_mib_find(struct slsi_mib_data *buffer, const struct slsi_mib_get_entry *entry);

/* Append data to a Buffer */
void slsi_mib_buf_append(struct slsi_mib_data *dst, size_t bufferLength, u8 *buffer);

/*******************************************************************************
 *
 * PSID Definitions
 *
 *******************************************************************************/

/*******************************************************************************
 * NAME          : Dot11TdlsPeerUapsdIndicationWindow
 * PSID          : 53 (0x0035)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * UNITS         : beacon intervals
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 1
 * DESCRIPTION   :
 *  The minimum time after the last TPU SP, before a RAME_TPU_SP indication
 *  can be issued.
 *******************************************************************************/
#define SLSI_PSID_DOT11_TDLS_PEER_UAPSD_INDICATION_WINDOW 0x0035

/*******************************************************************************
 * NAME          : Dot11AssociationSaQueryMaximumTimeout
 * PSID          : 100 (0x0064)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       : 1000
 * DESCRIPTION   :
 *  Timeout (in TUs) before giving up on a Peer that has not responded to a
 *  SA Query frame.
 *******************************************************************************/
#define SLSI_PSID_DOT11_ASSOCIATION_SA_QUERY_MAXIMUM_TIMEOUT 0x0064

/*******************************************************************************
 * NAME          : Dot11AssociationSaQueryRetryTimeout
 * PSID          : 101 (0x0065)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       : 201
 * DESCRIPTION   :
 *  Timeout (in TUs) before trying a Query Request frame.
 *******************************************************************************/
#define SLSI_PSID_DOT11_ASSOCIATION_SA_QUERY_RETRY_TIMEOUT 0x0065

/*******************************************************************************
 * NAME          : Dot11FilsActivated
 * PSID          : 102 (0x0066)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  FILS Implementation
 *******************************************************************************/
#define SLSI_PSID_DOT11_FILS_ACTIVATED 0x0066

/*******************************************************************************
 * NAME          : Dot11MinPowerCapabilityOverride
 * PSID          : 113 (0x0071)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt16
 * MIN           : -127
 * MAX           : 127
 * DEFAULT       :
 * DESCRIPTION   :
 *  Golden Certification MIB don't delete, change PSID or name: Override for
 *  the advertised power in an Association Request.
 *******************************************************************************/
#define SLSI_PSID_DOT11_MIN_POWER_CAPABILITY_OVERRIDE 0x0071

/*******************************************************************************
 * NAME          : Dot11MaxNetworkPowerCapabilityOverride
 * PSID          : 114 (0x0072)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt16
 * MIN           : -127
 * MAX           : 127
 * DEFAULT       :
 * DESCRIPTION   :
 *  Golden Certification MIB don't delete, change PSID or name: Override for
 *  the regulatory domain max power used in calculating network power cap.
 *******************************************************************************/
#define SLSI_PSID_DOT11_MAX_NETWORK_POWER_CAPABILITY_OVERRIDE 0x0072

/*******************************************************************************
 * NAME          : Dot11RtsThreshold
 * PSID          : 121 (0x0079)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * UNITS         : octet
 * MIN           : 0
 * MAX           : 65536
 * DEFAULT       : 65536
 * DESCRIPTION   :
 *  Size of an MPDU, below which an RTS/CTS handshake shall not be performed,
 *  except as RTS/CTS is used as a cross modulation protection mechanism as
 *  defined in 9.10. An RTS/CTS handshake shall be performed at the beginning
 *  of any frame exchange sequence where the MPDU is of type Data or
 *  Management, the MPDU has an individual address in the Address1 field, and
 *  the length of the MPDU is greater than this threshold. (For additional
 *  details, refer to Table 21 in 9.7.) Setting larger than the maximum MSDU
 *  size shall have the effect of turning off the RTS/CTS handshake for
 *  frames of Data or Management type transmitted by this STA. Setting to
 *  zero shall have the effect of turning on the RTS/CTS handshake for all
 *  frames of Data or Management type transmitted by this STA.
 *******************************************************************************/
#define SLSI_PSID_DOT11_RTS_THRESHOLD 0x0079

/*******************************************************************************
 * NAME          : Dot11ShortRetryLimit
 * PSID          : 122 (0x007A)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 1
 * MAX           : 255
 * DEFAULT       : 32
 * DESCRIPTION   :
 *  Maximum number of transmission attempts of a frame, the length of which
 *  is less than or equal to dot11RTSThreshold, that shall be made before a
 *  failure condition is indicated.
 *******************************************************************************/
#define SLSI_PSID_DOT11_SHORT_RETRY_LIMIT 0x007A

/*******************************************************************************
 * NAME          : Dot11LongRetryLimit
 * PSID          : 123 (0x007B)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 1
 * MAX           : 255
 * DEFAULT       : 4
 * DESCRIPTION   :
 *  Maximum number of transmission attempts of a frame, the length of which
 *  is greater than dot11RTSThreshold, that shall be made before a failure
 *  condition is indicated.
 *******************************************************************************/
#define SLSI_PSID_DOT11_LONG_RETRY_LIMIT 0x007B

/*******************************************************************************
 * NAME          : Dot11FragmentationThreshold
 * PSID          : 124 (0x007C)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 256
 * MAX           : 11500
 * DEFAULT       : 3000
 * DESCRIPTION   :
 *  Current maximum size, in octets, of the MPDU that may be delivered to the
 *  security encapsulation. This maximum size does not apply when an MSDU is
 *  transmitted using an HT-immediate or HTdelayed Block Ack agreement, or
 *  when an MSDU or MMPDU is carried in an AMPDU that does not contain a VHT
 *  single MPDU. Fields added to the frame by security encapsulation are not
 *  counted against the limit specified. Except as described above, an MSDU
 *  or MMPDU is fragmented when the resulting frame has an individual address
 *  in the Address1 field, and the length of the frame is larger than this
 *  threshold, excluding security encapsulation fields. The default value is
 *  the lesser of 11500 or the aMPDUMaxLength or the aPSDUMaxLength of the
 *  attached PHY and the value never exceeds the lesser of 11500 or the
 *  aMPDUMaxLength or the aPSDUMaxLength of the attached PHY.
 *******************************************************************************/
#define SLSI_PSID_DOT11_FRAGMENTATION_THRESHOLD 0x007C

/*******************************************************************************
 * NAME          : Dot11TxopDurationRtsThreshold
 * PSID          : 125 (0x007D)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * UNITS         : octet
 * MIN           : 0
 * MAX           : 1023
 * DEFAULT       : 1023
 * DESCRIPTION   :
 *  In an HE BSS, the use of RTS/CTS can be TXOP duration-based.Any Data PPDU
 *  transmitted from the STA whose TXOP duration is greater or equal to the
 *  duration calculated above should pre-pend with RTS. Value is in units of
 *  32us
 *******************************************************************************/
#define SLSI_PSID_DOT11_TXOP_DURATION_RTS_THRESHOLD 0x007D

/*******************************************************************************
 * NAME          : Dot11RtsSuccessCount
 * PSID          : 146 (0x0092)
 * PER INTERFACE?: NO
 * TYPE          : UINT64
 * MIN           : 0
 * MAX           : 18446744073709551615
 * DEFAULT       :
 * DESCRIPTION   :
 *  This counter shall increment when a CTS is received in response to an
 *  RTS.
 *******************************************************************************/
#define SLSI_PSID_DOT11_RTS_SUCCESS_COUNT 0x0092

/*******************************************************************************
 * NAME          : Dot11AckFailureCount
 * PSID          : 148 (0x0094)
 * PER INTERFACE?: NO
 * TYPE          : UINT64
 * MIN           : 0
 * MAX           : 18446744073709551615
 * DEFAULT       :
 * DESCRIPTION   :
 *  This counter shall increment when an ACK is not received when expected.
 *******************************************************************************/
#define SLSI_PSID_DOT11_ACK_FAILURE_COUNT 0x0094

/*******************************************************************************
 * NAME          : Dot11MulticastReceivedFrameCount
 * PSID          : 150 (0x0096)
 * PER INTERFACE?: NO
 * TYPE          : UINT64
 * MIN           : 0
 * MAX           : 18446744073709551615
 * DEFAULT       :
 * DESCRIPTION   :
 *  This counter shall increment when a MSDU is received with the multicast
 *  bit set in the destination MAC address.
 *******************************************************************************/
#define SLSI_PSID_DOT11_MULTICAST_RECEIVED_FRAME_COUNT 0x0096

/*******************************************************************************
 * NAME          : Dot11FcsErrorCount
 * PSID          : 151 (0x0097)
 * PER INTERFACE?: NO
 * TYPE          : UINT64
 * MIN           : 0
 * MAX           : 18446744073709551615
 * DEFAULT       :
 * DESCRIPTION   :
 *  This counter shall increment when an FCS error is detected in a received
 *  MPDU.
 *******************************************************************************/
#define SLSI_PSID_DOT11_FCS_ERROR_COUNT 0x0097

/*******************************************************************************
 * NAME          : Dot11ManufacturerProductVersion
 * PSID          : 183 (0x00B7)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 300
 * DEFAULT       :
 * DESCRIPTION   :
 *  Printable string used to identify the manufacturer&apos;s product version
 *  of the resource.
 *******************************************************************************/
#define SLSI_PSID_DOT11_MANUFACTURER_PRODUCT_VERSION 0x00B7

/*******************************************************************************
 * NAME          : UnifiMlmeScanChannelMaxScanTime
 * PSID          : 2001 (0x07D1)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 14
 * MAX           : 14
 * DEFAULT       : { 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00 }
 * DESCRIPTION   :
 *  Test only: overrides max_scan_time. 0 indicates not used.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MLME_SCAN_CHANNEL_MAX_SCAN_TIME 0x07D1

/*******************************************************************************
 * NAME          : UnifiMlmeScanChannelProbeInterval
 * PSID          : 2002 (0x07D2)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 14
 * MAX           : 14
 * DEFAULT       : { 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00 }
 * DESCRIPTION   :
 *  Test only: overrides probe interval. 0 indicates not used.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MLME_SCAN_CHANNEL_PROBE_INTERVAL 0x07D2

/*******************************************************************************
 * NAME          : UnifiMlmeScanChannelRule
 * PSID          : 2003 (0x07D3)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 4
 * MAX           : 4
 * DEFAULT       : { 0X00, 0X01, 0X00, 0X01 }
 * DESCRIPTION   :
 *  Rules for channel scanners.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MLME_SCAN_CHANNEL_RULE 0x07D3

/*******************************************************************************
 * NAME          : UnifiMlmeDataReferenceTimeout
 * PSID          : 2005 (0x07D5)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65534
 * DEFAULT       :
 * DESCRIPTION   :
 *  Maximum time, in TU, allowed for the data in data references
 *  corresponding to MLME primitives to be made available to the firmware.
 *  The special value 0 specifies an infinite timeout.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MLME_DATA_REFERENCE_TIMEOUT 0x07D5

/*******************************************************************************
 * NAME          : UnifiMlmeScanHighRssiThreshold
 * PSID          : 2008 (0x07D8)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt16
 * MIN           : -128
 * MAX           : 127
 * DEFAULT       : -90
 * DESCRIPTION   :
 *  Minimum RSSI, in dB, for a scan indication to be kept.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MLME_SCAN_HIGH_RSSI_THRESHOLD 0x07D8

/*******************************************************************************
 * NAME          : UnifiMlmeScanDeltaRssiThreshold
 * PSID          : 2010 (0x07DA)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 1
 * MAX           : 255
 * DEFAULT       : 20
 * DESCRIPTION   :
 *  Magnitude of the change in RSSI for which a scan result will be issued.
 *  In dB.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MLME_SCAN_DELTA_RSSI_THRESHOLD 0x07DA

/*******************************************************************************
 * NAME          : UnifiMlmeScanMaximumResults
 * PSID          : 2015 (0x07DF)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 100
 * DESCRIPTION   :
 *  Max number of scan results, per sps, which will be stored before the
 *  oldest result is discarded, irrespective of its age. The value 0
 *  specifies no maximum.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MLME_SCAN_MAXIMUM_RESULTS 0x07DF

/*******************************************************************************
 * NAME          : UnifiChannelBusyThreshold
 * PSID          : 2018 (0x07E2)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 1
 * MAX           : 100
 * DEFAULT       : 25
 * DESCRIPTION   :
 *  The threshold in percentage of CCA busy time when a channel would be
 *  considered busy
 *******************************************************************************/
#define SLSI_PSID_UNIFI_CHANNEL_BUSY_THRESHOLD 0x07E2

/*******************************************************************************
 * NAME          : UnifiMacSequenceNumberRandomisationActivated
 * PSID          : 2020 (0x07E4)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Enabling Sequence Number Randomisation to be applied for Probe Requests
 *  when scanning. Note: Randomisation only happens, if mac address gets
 *  randomised.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MAC_SEQUENCE_NUMBER_RANDOMISATION_ACTIVATED 0x07E4

/*******************************************************************************
 * NAME          : UnifiFirmwareBuildId
 * PSID          : 2021 (0x07E5)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  Numeric build identifier for this firmware build. This should normally be
 *  displayed in decimal. The textual build identifier is available via the
 *  standard dot11manufacturerProductVersion MIB attribute.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_FIRMWARE_BUILD_ID 0x07E5

/*******************************************************************************
 * NAME          : UnifiChipVersion
 * PSID          : 2022 (0x07E6)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  Numeric identifier for the UniFi silicon revision (as returned by the
 *  GBL_CHIP_VERSION hardware register). Other than being different for each
 *  design variant (but not for alternative packaging options), the
 *  particular values returned do not have any significance.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_CHIP_VERSION 0x07E6

/*******************************************************************************
 * NAME          : UnifiFirmwarePatchBuildId
 * PSID          : 2023 (0x07E7)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  Numeric build identifier for the patch set that has been applied to this
 *  firmware image. This should normally be displayed in decimal. For a
 *  patched ROM build there will be two build identifiers, the first will
 *  correspond to the base ROM image, the second will correspond to the patch
 *  set that has been applied.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_FIRMWARE_PATCH_BUILD_ID 0x07E7

/*******************************************************************************
 * NAME          : UnifiExtendedCapabilitiesSoftAp
 * PSID          : 2024 (0x07E8)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 8
 * MAX           : 10
 * DEFAULT       : { 0X01, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X40, 0X00, 0X00 }
 * DESCRIPTION   :
 *  Extended capabilities for SoftAp. Bit field definition and coding follows
 *  IEEE 802.11 Extended Capability Information Element, with spare subfields
 *  for capabilities that are independent from chip/firmware implementation.
 *  The size of the caps is limited to 8 octets as per RQMT-1878 and should
 *  not be updated without MX request. The only exception to this rule is
 *  except for when unifiFtmResponderActivated is true or when
 *  unifiTWTActivated is true with TWT Responder bit set as part of
 *  UNIFITWTCONTROLFLAGS, then size should be 10 octets.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_EXTENDED_CAPABILITIES_SOFT_AP 0x07E8

/*******************************************************************************
 * NAME          : UnifiMaxNumAntennaToUse
 * PSID          : 2025 (0x07E9)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       : 0X0202
 * DESCRIPTION   :
 *  Golden Certification MIB don't delete, change PSID or name: Specify the
 *  maximum number of antenna that will be used for each band. Lower 8 bits =
 *  2GHz, Mid 8 bits = 5Ghz, Higher 8 bits = 6Ghz. Limited by maximum
 *  supported by underlying hardware. WARNING: Changing this value after
 *  system start-up will have no effect.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MAX_NUM_ANTENNA_TO_USE 0x07E9

/*******************************************************************************
 * NAME          : UnifiHtCapabilitiesSoftAp
 * PSID          : 2028 (0x07EC)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 21
 * MAX           : 21
 * DEFAULT       : { 0XEF, 0X0A, 0X17, 0XFF, 0XFF, 0X00, 0X00, 0X01, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00 }
 * DESCRIPTION   :
 *  Golden Certification MIB don't delete, change PSID or name. HT
 *  capabilities for softAP to disallow MIMO when customer want to control
 *  MIMO/ SISO mode (see SC-508764-DD for further details). NOTE: Greenfield
 *  has been disabled due to interoperability issues with setting SGI.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_HT_CAPABILITIES_SOFT_AP 0x07EC

/*******************************************************************************
 * NAME          : UnifiSoftAp40MhzOn24g
 * PSID          : 2029 (0x07ED)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Enables 40MHz operation on 2.4GHz band for SoftAP.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_SOFT_AP40_MHZ_ON24G 0x07ED

/*******************************************************************************
 * NAME          : UnifiBasicCapabilities
 * PSID          : 2030 (0x07EE)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 0X1730
 * DESCRIPTION   :
 *  The 16-bit field follows the coding of IEEE 802.11 Capability
 *  Information.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_BASIC_CAPABILITIES 0x07EE

/*******************************************************************************
 * NAME          : UnifiExtendedCapabilities
 * PSID          : 2031 (0x07EF)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 10
 * MAX           : 12
 * DEFAULT       : { 0X05, 0X00, 0X08, 0X00, 0X00, 0X00, 0X00, 0X40, 0X80, 0X20, 0X00, 0X00 }
 * DESCRIPTION   :
 *  Extended capabilities. Bit field definition and coding follows IEEE
 *  802.11 Extended Capability Information Element, with spare subfields for
 *  capabilities that are independent from chip/firmware implementation.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_EXTENDED_CAPABILITIES 0x07EF

/*******************************************************************************
 * NAME          : UnifiHtCapabilities
 * PSID          : 2032 (0x07F0)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 21
 * MAX           : 21
 * DEFAULT       : { 0XEF, 0X0A, 0X17, 0XFF, 0XFF, 0X00, 0X00, 0X01, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00 }
 * DESCRIPTION   :
 *  Golden Certification MIB don't delete, change PSID or name. HT
 *  capabilities of the chip. See SC-503520-SP for further details. NOTE:
 *  Greenfield has been disabled due to interoperability issues wuth SGI.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_HT_CAPABILITIES 0x07F0

/*******************************************************************************
 * NAME          : Unifi24G40MhzChannels
 * PSID          : 2035 (0x07F3)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Golden Certification MIB don't delete, change PSID or name: Enables 40Mz
 *  wide channels in the 2.4G band for STA. This can only be changed before
 *  STA is coonected.
 *******************************************************************************/
#define SLSI_PSID_UNIFI24_G40_MHZ_CHANNELS 0x07F3

/*******************************************************************************
 * NAME          : UnifiExtendedCapabilitiesDisabled
 * PSID          : 2036 (0x07F4)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Golden Certification MIB don't delete, change PSID or name: Suppress
 *  extended capabilities IE being sent in the association request. Please
 *  note that this may fix IOP issues with Aruba APs in WMMAC. Singed Decimal
 *******************************************************************************/
#define SLSI_PSID_UNIFI_EXTENDED_CAPABILITIES_DISABLED 0x07F4

/*******************************************************************************
 * NAME          : beaconDriftSupport
 * PSID          : 2037 (0x07F5)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  This mib helps to schedule vif properly to listen for beacons when there
 *  are larger drifts in the beacons received from the connected AP.
 *******************************************************************************/
#define SLSI_PSID_BEACON_DRIFT_SUPPORT 0x07F5

/*******************************************************************************
 * NAME          : UnifiSupportedDataRates
 * PSID          : 2041 (0x07F9)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * UNITS         : 500 kbps
 * MIN           : 2
 * MAX           : 16
 * DEFAULT       : { 0X02, 0X04, 0X0B, 0X0C, 0X12, 0X16, 0X18, 0X24, 0X30, 0X48, 0X60, 0X6C }
 * DESCRIPTION   :
 *  Golden Certification MIB don't delete, change PSID or name: Defines the
 *  supported non-HT data rates. It is encoded as N+1 octets where the first
 *  octet is N and the subsequent octets each describe a single supported
 *  rate.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_SUPPORTED_DATA_RATES 0x07F9

/*******************************************************************************
 * NAME          : UnifiRadioMeasurementActivated
 * PSID          : 2043 (0x07FB)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  When TRUE Radio Measurements are supported.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_RADIO_MEASUREMENT_ACTIVATED 0x07FB

/*******************************************************************************
 * NAME          : UnifiRadioMeasurementCapabilities
 * PSID          : 2044 (0x07FC)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 5
 * MAX           : 5
 * DEFAULT       : { 0X73, 0X0A, 0X01, 0X00, 0X04 }
 * DESCRIPTION   :
 *  RM Enabled capabilities of the chip. See SC-503520-SP for further
 *  details.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_RADIO_MEASUREMENT_CAPABILITIES 0x07FC

/*******************************************************************************
 * NAME          : UnifiVhtActivated
 * PSID          : 2045 (0x07FD)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Activate VHT mode.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_VHT_ACTIVATED 0x07FD

/*******************************************************************************
 * NAME          : UnifiHtActivated
 * PSID          : 2046 (0x07FE)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Activate HT mode.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_HT_ACTIVATED 0x07FE

/*******************************************************************************
 * NAME          : UnifiRoamingActivated
 * PSID          : 2049 (0x0801)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Activate Roaming functionality
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAMING_ACTIVATED 0x0801

/*******************************************************************************
 * NAME          : UnifiRoamRssiScanTrigger
 * PSID          : 2050 (0x0802)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt8
 * UNITS         : dbm
 * MIN           : -128
 * MAX           : 127
 * DEFAULT       : -75
 * DESCRIPTION   :
 *  The RSSI value, in dBm, below which roaming scan shall start.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_RSSI_SCAN_TRIGGER 0x0802

/*******************************************************************************
 * NAME          : UnifiRoamCuScanTriggerHysteresis
 * PSID          : 2051 (0x0803)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 1
 * MAX           : 255
 * DEFAULT       : 10
 * DESCRIPTION   :
 *  Hysteresis value that subtracted from unifiRoamCUScanTrigger will result
 *  in the CU value that will stop eventual CU scans. This is an SCSC
 *  addition to ensure scans are not started/ stopped in an excessive manner.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_CU_SCAN_TRIGGER_HYSTERESIS 0x0803

/*******************************************************************************
 * NAME          : UnifiRoamBackgroundScanPeriod
 * PSID          : 2052 (0x0804)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * UNITS         : microseconds
 * MIN           : 1
 * MAX           : 4294967295
 * DEFAULT       : 120000000
 * DESCRIPTION   :
 *  The background scan period for cached channels.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_BACKGROUND_SCAN_PERIOD 0x0804

/*******************************************************************************
 * NAME          : UnifiRoamNchoFullScanPeriod
 * PSID          : 2053 (0x0805)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * UNITS         : microseconds
 * MIN           : 1
 * MAX           : 4294967295
 * DEFAULT       : 120000000
 * DESCRIPTION   :
 *  NCHO: Specifies the period at which the full channel scan shall be run.
 *  For certification and Host use only.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_NCHO_FULL_SCAN_PERIOD 0x0805

/*******************************************************************************
 * NAME          : UnifiRoamScanBand
 * PSID          : 2055 (0x0807)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 1
 * MAX           : 2
 * DEFAULT       : 2
 * DESCRIPTION   :
 *  Indicates whether only intra-band or all-band should be used for roaming
 *  scan. 2 - Roaming across band 1 - Roaming within band
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_SCAN_BAND 0x0807

/*******************************************************************************
 * NAME          : UnifiRoamCuRssiScanTriggerHysteresis
 * PSID          : 2056 (0x0808)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * UNITS         : dbm
 * MIN           : 1
 * MAX           : 255
 * DEFAULT       : 5
 * DESCRIPTION   :
 *  Hysteresis value that added to unifiRoamCURssiScanTrigger will result in
 *  the RSSI value that will stop eventual CU scans. This is an SCSC addition
 *  to ensure scans are not started/ stopped in an excessive manner.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_CU_RSSI_SCAN_TRIGGER_HYSTERESIS 0x0808

/*******************************************************************************
 * NAME          : UnifiRoamNchoScanMaxActiveChannelTime
 * PSID          : 2057 (0x0809)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * UNITS         : TU
 * MIN           : 1
 * MAX           : 65535
 * DEFAULT       : 40
 * DESCRIPTION   :
 *  NCHO: Specifies the maximum time spent active scanning a channel.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_NCHO_SCAN_MAX_ACTIVE_CHANNEL_TIME 0x0809

/*******************************************************************************
 * NAME          : UnifiRoamReScanPeriod
 * PSID          : 2058 (0x080A)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * UNITS         : microseconds
 * MIN           : 1
 * MAX           : 4294967295
 * DEFAULT       : 10000000
 * DESCRIPTION   :
 *  The scan period for re-scanning cached channels.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_RE_SCAN_PERIOD 0x080A

/*******************************************************************************
 * NAME          : UnifiRoamInactiveScanPeriod
 * PSID          : 2059 (0x080B)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * UNITS         : microseconds
 * MIN           : 1
 * MAX           : 4294967295
 * DEFAULT       : 10000000
 * DESCRIPTION   :
 *  The scan period for the inactive scan.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_INACTIVE_SCAN_PERIOD 0x080B

/*******************************************************************************
 * NAME          : UnifiRoamMode
 * PSID          : 2060 (0x080C)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 2
 * DEFAULT       : 1
 * DESCRIPTION   :
 *  Enable/Disable host resume when roaming. 0: Wake up the host all the
 *  time. 1: Only wakeup the host if the AP is not white-listed. 2: Don't
 *  wake up the host.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_MODE 0x080C

/*******************************************************************************
 * NAME          : UnifiRoamRssiScanTriggerReset
 * PSID          : 2061 (0x080D)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt8
 * UNITS         : dbm
 * MIN           : -128
 * MAX           : 127
 * DEFAULT       : -70
 * DESCRIPTION   :
 *  The current channel Averaged RSSI value above which a RSSI triggered
 *  roaming scan shall stop, and the RSSI trigger shall be reset at its
 *  initial value.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_RSSI_SCAN_TRIGGER_RESET 0x080D

/*******************************************************************************
 * NAME          : UnifiRoamRssiScanTriggerStep
 * PSID          : 2062 (0x080E)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * UNITS         : dbm
 * MIN           : 0
 * MAX           : 10
 * DEFAULT       : 5
 * DESCRIPTION   :
 *  The delta to apply to unifiRoamRssiScanTrigger when no eligible candidate
 *  is found.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_RSSI_SCAN_TRIGGER_STEP 0x080E

/*******************************************************************************
 * NAME          : UnifiRoamIdleVariationRssi
 * PSID          : 2063 (0x080F)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * UNITS         : dBm
 * MIN           : 0
 * MAX           : 100
 * DEFAULT       : 8
 * DESCRIPTION   :
 *  Defines the maximum RSSI variation of the current link allowed during
 *  idle roam. This parameter ensures the device is stationary.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_IDLE_VARIATION_RSSI 0x080F

/*******************************************************************************
 * NAME          : UnifiRoamIdleMinRssi
 * PSID          : 2064 (0x0810)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt16
 * UNITS         : dBm
 * MIN           : -127
 * MAX           : 0
 * DEFAULT       : -65
 * DESCRIPTION   :
 *  Defines the minimum RSSI of the current link required to perform an idle
 *  roam.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_IDLE_MIN_RSSI 0x0810

/*******************************************************************************
 * NAME          : UnifiRoamEapTimeout
 * PSID          : 2065 (0x0811)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 200
 * DESCRIPTION   :
 *  Timeout, in ms, for receiving the first EAP/EAPOL frame from the AP
 *  during roaming
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_EAP_TIMEOUT 0x0811

/*******************************************************************************
 * NAME          : UnifiRoamIdleInactiveTime
 * PSID          : 2066 (0x0812)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * UNITS         : seconds
 * MIN           : 0
 * MAX           : 100
 * DEFAULT       : 5
 * DESCRIPTION   :
 *  Defines the time interval during which the link needs to remain idle to
 *  perform an idle roam.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_IDLE_INACTIVE_TIME 0x0812

/*******************************************************************************
 * NAME          : UnifiRoamNchoScanControl
 * PSID          : 2067 (0x0813)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  NCHO: Indicates which control path shall be evaluated in order to
 *  determine which channels should be scanned for NCHO scans.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_NCHO_SCAN_CONTROL 0x0813

/*******************************************************************************
 * NAME          : UnifiRoamNchoDfsScanMode
 * PSID          : 2068 (0x0814)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 2
 * DEFAULT       : 1
 * DESCRIPTION   :
 *  NCHO: Specifies how DFS channels should be scanned for roaming. For
 *  certification and Host use only.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_NCHO_DFS_SCAN_MODE 0x0814

/*******************************************************************************
 * NAME          : UnifiRoamNchoScanHomeTime
 * PSID          : 2069 (0x0815)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * UNITS         : TU
 * MIN           : 40
 * MAX           : 65535
 * DEFAULT       : 45
 * DESCRIPTION   :
 *  NCHO: The time, in TU, to spend NOT scanning during a HomeAway scan.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_NCHO_SCAN_HOME_TIME 0x0815

/*******************************************************************************
 * NAME          : UnifiRoamNchoScanHomeAwayTime
 * PSID          : 2070 (0x0816)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * UNITS         : TU
 * MIN           : 40
 * MAX           : 65535
 * DEFAULT       : 100
 * DESCRIPTION   :
 *  NCHO: The time, in TU, to spend scanning during a HomeAway scan.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_NCHO_SCAN_HOME_AWAY_TIME 0x0816

/*******************************************************************************
 * NAME          : UnifiRoamIdleInactiveCount
 * PSID          : 2071 (0x0817)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * UNITS         : number_of_packets
 * MIN           : 0
 * MAX           : 100
 * DEFAULT       : 5
 * DESCRIPTION   :
 *  The number of packets over which the link is considered not idle.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_IDLE_INACTIVE_COUNT 0x0817

/*******************************************************************************
 * NAME          : UnifiRoamNchoScanNProbe
 * PSID          : 2072 (0x0818)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 2
 * DESCRIPTION   :
 *  NCHO: The number of ProbeRequest frames per channel.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_NCHO_SCAN_NPROBE 0x0818

/*******************************************************************************
 * NAME          : UnifiRoamIdleBand
 * PSID          : 2073 (0x0819)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 5
 * DEFAULT       : 1
 * DESCRIPTION   :
 *  A bitmask specifying if Idle Roaming is allowed depending on the band of
 *  the existing connection. There are also other conditions which will be
 *  checked in order to trigger Idle Roaming. Bit 0 - if set Idle Roaming is
 *  allowed in 2.4GHz band. Bit 1 - if set Idle Roaming is allowed in 5GHz
 *  band. So, the following values are allowed and have following meaning , 0
 *  - Idle Roaming is not allowed, 1 - Idle Roaming is allowed only for
 *  2.4GHz 2 - Idle Roaming is allowed only for 5GHz, 3 - Idle Roaming is
 *  allowed for both 2.4GHz and 5 GHz. Note, there is Idle Roaming for 6GHz,
 *  as part of the conditions for Idle Mode to be triggered is to have cached
 *  channels in higher band.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_IDLE_BAND 0x0819

/*******************************************************************************
 * NAME          : UnifiRoamIdleApSelectDeltaFactor
 * PSID          : 2074 (0x081A)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 100
 * DEFAULT       :
 * DESCRIPTION   :
 *  Delta value applied to the score of the currently connected AP to
 *  determine candidates' eligibility threshold for Idle period triggered
 *  roaming scans.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_IDLE_AP_SELECT_DELTA_FACTOR 0x081A

/*******************************************************************************
 * NAME          : UnifiRoamNchoRssiDelta
 * PSID          : 2075 (0x081B)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * UNITS         : dBm
 * MIN           : 0
 * MAX           : 127
 * DEFAULT       : 10
 * DESCRIPTION   :
 *  NCHO: Specifies the RSSI delta at a potential candidate is deemed
 *  eligible.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_NCHO_RSSI_DELTA 0x081B

/*******************************************************************************
 * NAME          : UnifiApOlbcDuration
 * PSID          : 2076 (0x081C)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 300
 * DESCRIPTION   :
 *  How long, in milliseconds, the AP enables reception of BEACON frames to
 *  perform Overlapping Legacy BSS Condition(OLBC). If set to 0 then OLBC is
 *  disabled.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_AP_OLBC_DURATION 0x081C

/*******************************************************************************
 * NAME          : UnifiApOlbcInterval
 * PSID          : 2077 (0x081D)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 2000
 * DESCRIPTION   :
 *  How long, in milliseconds, between periods of receiving BEACON frames to
 *  perform Overlapping Legacy BSS Condition(OLBC). This value MUST exceed
 *  the OBLC duration MIB unifiApOlbcDuration. If set to 0 then OLBC is
 *  disabled.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_AP_OLBC_INTERVAL 0x081D

/*******************************************************************************
 * NAME          : UnifiDnsSupportActivated
 * PSID          : 2078 (0x081E)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  This MIB activates support for transmitting DNS frame via MLME.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_DNS_SUPPORT_ACTIVATED 0x081E

/*******************************************************************************
 * NAME          : UnifiOffchannelProcedureTimeout
 * PSID          : 2079 (0x081F)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 1000
 * DESCRIPTION   :
 *  Maximum timeout in ms the Offchannel FSM will wait until the procedure is
 *  completed
 *******************************************************************************/
#define SLSI_PSID_UNIFI_OFFCHANNEL_PROCEDURE_TIMEOUT 0x081F

/*******************************************************************************
 * NAME          : UnifiFrameResponseTimeout
 * PSID          : 2080 (0x0820)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 500
 * DEFAULT       : 200
 * DESCRIPTION   :
 *  Timeout, in TU, to wait for a frame(Auth, Assoc, ReAssoc) after TX Cfm
 *  trasnmission_status == Successful.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_FRAME_RESPONSE_TIMEOUT 0x0820

/*******************************************************************************
 * NAME          : UnifiConnectionFailureTimeout
 * PSID          : 2081 (0x0821)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 20000
 * DEFAULT       : 2000
 * DESCRIPTION   :
 *  Timeout, in TU, for a frame retry before giving up.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_CONNECTION_FAILURE_TIMEOUT 0x0821

/*******************************************************************************
 * NAME          : UnifiConnectingProbeTimeout
 * PSID          : 2082 (0x0822)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 100
 * DEFAULT       : 10
 * DESCRIPTION   :
 *  How long, in TU, to wait for a ProbeRsp when syncronising before
 *  resending a ProbeReq
 *******************************************************************************/
#define SLSI_PSID_UNIFI_CONNECTING_PROBE_TIMEOUT 0x0822

/*******************************************************************************
 * NAME          : UnifiDisconnectTimeout
 * PSID          : 2083 (0x0823)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 3000
 * DEFAULT       : 1500
 * DESCRIPTION   :
 *  Timeout, in milliseconds, to perform a disconnect or disconnect all STAs
 *  (triggered by MLME_DISCONNECT-REQ or MLME_DISCONNECT-REQ
 *  00:00:00:00:00:00) before responding with MLME-DISCONNECT-IND and
 *  aborting the disconnection attempt. This is particulary important when a
 *  SoftAP is attempting to disconnect associated stations which might have
 *  "silently" left the ESS.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_DISCONNECT_TIMEOUT 0x0823

/*******************************************************************************
 * NAME          : UnifiFrameResponseCfmFailureTimeout
 * PSID          : 2085 (0x0825)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 40
 * DESCRIPTION   :
 *  Timeout, in TU, to wait to retry a frame after TX Cfm trasnmission_status
 *  != Successful.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_FRAME_RESPONSE_CFM_FAILURE_TIMEOUT 0x0825

/*******************************************************************************
 * NAME          : UnifiForceActiveDuration
 * PSID          : 2086 (0x0826)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 1000
 * DEFAULT       : 200
 * DESCRIPTION   :
 *  How long, in milliseconds, the firmware temporarily extends PowerSave for
 *  STA as a workaround for wonky APs such as D-link.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_FORCE_ACTIVE_DURATION 0x0826

/*******************************************************************************
 * NAME          : UnifiMlmeScanStopIfLessThanXFrames
 * PSID          : 2088 (0x0828)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 4
 * DESCRIPTION   :
 *  Stop scanning on a channel if less than X Beacons or Probe Responses are
 *  received.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MLME_SCAN_STOP_IF_LESS_THAN_XFRAMES 0x0828

/*******************************************************************************
 * NAME          : UnifiApAssociationTimeout
 * PSID          : 2089 (0x0829)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 2000
 * DESCRIPTION   :
 *  SoftAP: Permitted time for a station to complete associatation with FW
 *  acting as AP in milliseconds.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_AP_ASSOCIATION_TIMEOUT 0x0829

/*******************************************************************************
 * NAME          : UnifiRoamingInP2pActivated
 * PSID          : 2090 (0x082A)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Activate Roaming in P2P.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAMING_IN_P2P_ACTIVATED 0x082A

/*******************************************************************************
 * NAME          : UnifiHostNumAntennaControlActivated
 * PSID          : 2091 (0x082B)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Host has a control of number of antenna to use
 *******************************************************************************/
#define SLSI_PSID_UNIFI_HOST_NUM_ANTENNA_CONTROL_ACTIVATED 0x082B

/*******************************************************************************
 * NAME          : UnifiRoamNchoRssiTrigger
 * PSID          : 2092 (0x082C)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt8
 * UNITS         : dBm
 * MIN           : -128
 * MAX           : 127
 * DEFAULT       : -75
 * DESCRIPTION   :
 *  The current channel Averaged RSSI value below which a NCHO scan shall
 *  start.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_NCHO_RSSI_TRIGGER 0x082C

/*******************************************************************************
 * NAME          : UnifiRoamNchoRssiTriggerHysteresis
 * PSID          : 2093 (0x082D)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * UNITS         : dBm
 * MIN           : 0
 * MAX           : 127
 * DEFAULT       : 5
 * DESCRIPTION   :
 *  Hysteresis value that added to unifiRoamNchoRssiTrigger will result in
 *  the RSSI value that will stop eventual NCHO scans.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_NCHO_RSSI_TRIGGER_HYSTERESIS 0x082D

/*******************************************************************************
 * NAME          : UnifiPeerBandwidth
 * PSID          : 2094 (0x082E)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  The bandwidth used with peer station prior it disconnects
 *******************************************************************************/
#define SLSI_PSID_UNIFI_PEER_BANDWIDTH 0x082E

/*******************************************************************************
 * NAME          : UnifiCurrentPeerNss
 * PSID          : 2095 (0x082F)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  The number of spatial streams used with peer station prior it
 *  disconnects: BIG DATA
 *******************************************************************************/
#define SLSI_PSID_UNIFI_CURRENT_PEER_NSS 0x082F

/*******************************************************************************
 * NAME          : UnifiPeerTxDataRate
 * PSID          : 2096 (0x0830)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  The tx rate that was used for transmissions prior disconnection
 *******************************************************************************/
#define SLSI_PSID_UNIFI_PEER_TX_DATA_RATE 0x0830

/*******************************************************************************
 * NAME          : UnifiPeerRssi
 * PSID          : 2097 (0x0831)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt16
 * MIN           : -32768
 * MAX           : 32767
 * DEFAULT       :
 * DESCRIPTION   :
 *  The recorded RSSI from peer station prior it disconnects
 *******************************************************************************/
#define SLSI_PSID_UNIFI_PEER_RSSI 0x0831

/*******************************************************************************
 * NAME          : UnifiMlmeStationInactivityTimeout
 * PSID          : 2098 (0x0832)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 6
 * DESCRIPTION   :
 *  Timeout, in seconds, for instigating ConnectonFailure procedures. Setting
 *  it to less than 3 seconds may result in frequent disconnection or roaming
 *  with the AP. Disable with Zero. Values lower than
 *  INACTIVITY_MINIMUM_TIMEOUT becomes INACTIVITY_MINIMUM_TIMEOUT. This value
 *  is read only once when an interface is added.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MLME_STATION_INACTIVITY_TIMEOUT 0x0832

/*******************************************************************************
 * NAME          : UnifiMlmeCliInactivityTimeout
 * PSID          : 2099 (0x0833)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 10
 * DESCRIPTION   :
 *  Timeout, in seconds, for instigating ConnectonFailure procedures. Zero
 *  value disables the feature. Any value written lower than
 *  INACTIVITY_MINIMUM_TIMEOUT becomes INACTIVITY_MINIMUM_TIMEOUT. This value
 *  is read only once when an interface is added.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MLME_CLI_INACTIVITY_TIMEOUT 0x0833

/*******************************************************************************
 * NAME          : UnifiMlmeStationInitialKickTimeout
 * PSID          : 2100 (0x0834)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 50
 * DESCRIPTION   :
 *  Golden Certification MIB don't delete, change PSID or name: Timeout, in
 *  milliseconds, for sending the AP a NULL frame to kick off the EAPOL
 *  exchange. This value is read only once when an interface is added.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MLME_STATION_INITIAL_KICK_TIMEOUT 0x0834

/*******************************************************************************
 * NAME          : UnifiInitialConnectionFailureTimeout
 * PSID          : 2102 (0x0836)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 20000
 * DEFAULT       : 3000
 * DESCRIPTION   :
 *  Timeout, in TU, for a frame retry before giving up for initial
 *  connection.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_INITIAL_CONNECTION_FAILURE_TIMEOUT 0x0836

/*******************************************************************************
 * NAME          : unifRetryAuthForSpecialAps
 * PSID          : 2103 (0x0837)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 20000
 * DEFAULT       : 500
 * DESCRIPTION   :
 *  How long, in milliseconds, before ConnectionFailureTimeout to send
 *  authentication frame for special aps.
 *******************************************************************************/
#define SLSI_PSID_UNIF_RETRY_AUTH_FOR_SPECIAL_APS 0x0837

/*******************************************************************************
 * NAME          : UnifiAssocComebackTimeout
 * PSID          : 2104 (0x0838)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 20000
 * DEFAULT       : 3000
 * DESCRIPTION   :
 *  Timeout, in TU, for a association comeback limit before giving up.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ASSOC_COMEBACK_TIMEOUT 0x0838

/*******************************************************************************
 * NAME          : UnifiMlmeInactivityNewAlgorithmActivated
 * PSID          : 2105 (0x0839)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  If enabled, the new algorithm which uses the timestamp of Sync frames
 *  (Beacon and Probe Response) would be used in logic to evaluate
 *  inactivity. If disabled, the old inactivity mechanism of any RX/TX
 *  timestamp and transmission of Null/QoS Null frame would be used in
 *  inactivity logic.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MLME_INACTIVITY_NEW_ALGORITHM_ACTIVATED 0x0839

/*******************************************************************************
 * NAME          : UnifiRoamBackgroundScanActivated
 * PSID          : 2106 (0x083A)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  If false, the FW does not trigger the roaming scan by background scan
 *  timer.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_BACKGROUND_SCAN_ACTIVATED 0x083A

/*******************************************************************************
 * NAME          : UnifiUartConfigure
 * PSID          : 2110 (0x083E)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  UART configuration using the values of the other unifiUart* attributes.
 *  The value supplied for this attribute is ignored.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_UART_CONFIGURE 0x083E

/*******************************************************************************
 * NAME          : UnifiUartPios
 * PSID          : 2111 (0x083F)
 * PER INTERFACE?: NO
 * TYPE          : unifiUartPios
 * MIN           : 0
 * MAX           : 255
 * DEFAULT       :
 * DESCRIPTION   :
 *  Specification of which PIOs should be connected to the UART. Currently
 *  defined values are: 1 - UART not used; all PIOs are available for other
 *  uses. 2 - Data transmit and receive connected to PIO[12] and PIO[14]
 *  respectively. No hardware handshaking lines. 3 - Data and handshaking
 *  lines connected to PIO[12:15].
 *******************************************************************************/
#define SLSI_PSID_UNIFI_UART_PIOS 0x083F

/*******************************************************************************
 * NAME          : UnifiCrystalFrequencyTrim
 * PSID          : 2141 (0x085D)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 63
 * DEFAULT       : 31
 * DESCRIPTION   :
 *  The IEEE 802.11 standard requires a frequency accuracy of either +/- 20
 *  ppm or +/- 25 ppm depending on the physical layer being used. If
 *  UniFi&apos;s frequency reference is a crystal then this attribute should
 *  be used to tweak the oscillating frequency to compensate for design- or
 *  device-specific variations. Each step change trims the frequency by
 *  approximately 2 ppm.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_CRYSTAL_FREQUENCY_TRIM 0x085D

/*******************************************************************************
 * NAME          : UnifiExternalClockDetect
 * PSID          : 2146 (0x0862)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  If UniFi is running with an external fast clock source, i.e.
 *  unifiExternalFastClockRequest is set, it is common for this clock to be
 *  shared with other devices. Setting to true causes UniFi to detect when
 *  the clock is present (presumably in response to a request from another
 *  device), and to perform any pending activities at that time rather than
 *  requesting the clock again some time later. This is likely to reduce
 *  overall system power consumption by reducing the total time that the
 *  clock needs to be active.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_EXTERNAL_CLOCK_DETECT 0x0862

/*******************************************************************************
 * NAME          : UnifiExternalFastClockRequest
 * PSID          : 2149 (0x0865)
 * PER INTERFACE?: NO
 * TYPE          : unifiExternalFastClockRequest
 * MIN           : 0
 * MAX           : 255
 * DEFAULT       : 1
 * DESCRIPTION   :
 *  It is possible to supply UniFi with an external fast reference clock, as
 *  an alternative to using a crystal. If such a clock is used then it is
 *  only required when UniFi is active. A signal can be output on PIO[2] or
 *  if the version of UniFi in use is the UF602x or later, any PIO may be
 *  used (see unifiExternalFastClockRequestPIO) to indicate when UniFi
 *  requires a fast clock. Setting makes this signal become active and
 *  determines the type of signal output. 0 - No clock request. 1 - Non
 *  inverted, totem pole. 2 - Inverted, totem pole. 3 - Open drain. 4 - Open
 *  source.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_EXTERNAL_FAST_CLOCK_REQUEST 0x0865

/*******************************************************************************
 * NAME          : UnifiWatchdogTimeout
 * PSID          : 2152 (0x0868)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * UNITS         : ms
 * MIN           : 1
 * MAX           : 65535
 * DEFAULT       : 1500
 * DESCRIPTION   :
 *  Maximum time the background may be busy or locked out for. If this time
 *  is exceeded, UniFi will reset. If this key is set to 65535 then the
 *  watchdog will be disabled.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_WATCHDOG_TIMEOUT 0x0868

/*******************************************************************************
 * NAME          : UnifiScanParametersPublic
 * PSID          : 2153 (0x0869)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 4
 * MAX           : 4
 * DEFAULT       :
 * DESCRIPTION   :
 *  Each row of the table contains 2 entries for a scan: first entry when
 *  there is 0 registered VIFs, second - when there is 1 or more registered
 *  VIFs. index2 = 1 octet 0 ~ 1 - Probe Interval in Time Units (uint16) /
 *  Zero VIF octet 2 ~ 3 - Probe Interval in Time Units (uint16) / Non-Zero
 *  VIF index2 = 2 octet 0 ~ 1 - Max Active Channel Time in Time Units
 *  (uint16) / Zero VIF octet 2 ~ 3 - Max Active Channel Time in Time Units
 *  (uint16) / Non-Zero VIF index2 = 3 octet 0 ~ 1 - Max Passive Channel Time
 *  in Time Units (uint16) / Zero VIF octet 2 ~ 3 - Max Passive Channel Time
 *  in Time Units (uint16) / Non-Zero VIF index2 = 4 octet 0 ~ 1 - Scan Flags
 *  (uint16) (see unifiScanFlags) / Zero VIF octet 2 ~ 3 - Scan Flags
 *  (uint16) (see unifiScanFlags) / Non-Zero VIF bit 0 - Enable Early Channel
 *  Exit (bool) bit 1 - Disable Scan (bool) bit 2 - Enable NCHO (bool) bit 3
 *  - Enable MAC Randomization (bool) bit 4 - Clear Scan Records cache
 *  between cycles (bool)
 *******************************************************************************/
#define SLSI_PSID_UNIFI_SCAN_PARAMETERS_PUBLIC 0x0869

/*******************************************************************************
 * NAME          : UnifiOverrideEdcaParamActivated
 * PSID          : 2155 (0x086B)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Activate override of STA edca config parameters with
 *  unifiOverrideEDCAParam. default: True - for volcano, and False - for
 *  others
 *******************************************************************************/
#define SLSI_PSID_UNIFI_OVERRIDE_EDCA_PARAM_ACTIVATED 0x086B

/*******************************************************************************
 * NAME          : UnifiOverrideEdcaParam
 * PSID          : 2156 (0x086C)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 4
 * MAX           : 4
 * DEFAULT       :
 * DESCRIPTION   :
 *  EDCA Parameters to be used if unifiOverrideEDCAParamActivated is true,
 *  indexed by unifiAccessClassIndex octet 0 - AIFSN octet 1 - [7:4] ECW MAX
 *  [3:0] ECW MIN octet 2 ~ 3 - TXOP[7:0] TXOP[15:8] in 32 usec units for
 *  both non-HT and HT connections.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_OVERRIDE_EDCA_PARAM 0x086C

/*******************************************************************************
 * NAME          : UnifiScanParametersPrivate
 * PSID          : 2157 (0x086D)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 2
 * MAX           : 2
 * DEFAULT       :
 * DESCRIPTION   :
 *  Each row of the table contains 2 entries for a scan type: first entry
 *  when there is 0 registered VIFs, second - when there is 1 or more
 *  registered VIFs. index2 = 1 octet 0 - Scan Priority (uint8) / Zero VIF
 *  octet 1 - Scan Priority (uint8) / Non-Zero VIF index2 = 2 octet 0 - Scan
 *  Policy (uint8) / Zero VIF octet 1 - Scan Policy (uint8) / Non-Zero VIF
 *  index2 = 3 octet 0 - Scan Groups (uint8) (see unifiScanGroups) / Zero VIF
 *  octet 1 - Scan Groups (uint8) (see unifiScanGroups) / Non-Zero VIF bit 0
 *  - Roaming scan group (bool) bit 1 - Roaming scan on known channel group
 *  (bool) bit 2 - P2P scan group (bool) bit 3 - Wifi Sharing allowed group
 *  (bool) bit 4 - NAN scan group (bool)
 *******************************************************************************/
#define SLSI_PSID_UNIFI_SCAN_PARAMETERS_PRIVATE 0x086D

/*******************************************************************************
 * NAME          : UnifiExternalFastClockRequestPio
 * PSID          : 2158 (0x086E)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 15
 * DEFAULT       : 9
 * DESCRIPTION   :
 *  If an external fast reference clock is being supplied to UniFi as an
 *  alternative to a crystal (see unifiExternalFastClockRequest) and the
 *  version of UniFi in use is the UF602x or later, any PIO may be used as
 *  the external fast clock request output from UniFi. key determines the PIO
 *  to use.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_EXTERNAL_FAST_CLOCK_REQUEST_PIO 0x086E

/*******************************************************************************
 * NAME          : UnifiModemDesenseEnabled
 * PSID          : 2160 (0x0870)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Enable dynamic desensing of the CCK and OFDM modems based on RSSI for STA
 *  VIFs.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MODEM_DESENSE_ENABLED 0x0870

/*******************************************************************************
 * NAME          : UnifiModemDesenseOffset
 * PSID          : 2164 (0x0874)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt16
 * UNITS         : dBm
 * MIN           : 0
 * MAX           : 127
 * DEFAULT       : 20
 * DESCRIPTION   :
 *  The offset value for the CCK and 2G4 OFDM modem desensing. When the CCK
 *  and/or 2G4 OFDM modems are desensed, the desense value will be set to the
 *  averaged STA-AP RSSI less this parameter value. Only used when
 *  unifiModemDesenseEnabled is set to true.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MODEM_DESENSE_OFFSET 0x0874

/*******************************************************************************
 * NAME          : UnifiModemDesenseCap
 * PSID          : 2165 (0x0875)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt16
 * UNITS         : dBm
 * MIN           : -128
 * MAX           : 0
 * DEFAULT       : -65
 * DESCRIPTION   :
 *  The upper limit for dynamic desensing of the CCK and 2G4 OFDM modem. The
 *  CCK and/or 2G4 OFDM modems will not be desensed to a value in excess of
 *  this parameter. Only used when unifiModemDesenseEnabled is set to true.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MODEM_DESENSE_CAP 0x0875

/*******************************************************************************
 * NAME          : UnifiModemDesenseOffset5g
 * PSID          : 2169 (0x0879)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt16
 * UNITS         : dBm
 * MIN           : 0
 * MAX           : 127
 * DEFAULT       : 20
 * DESCRIPTION   :
 *  The offset value for the CCK and OFDM modem desensing. When the 5G OFDM
 *  modem is desensed, the desense value will be set to the averaged STA-AP
 *  RSSI less this parameter value. Only used when unifiModemDesenseEnabled
 *  is set to true.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MODEM_DESENSE_OFFSET5G 0x0879

/*******************************************************************************
 * NAME          : UnifiModemDesenseCap5g
 * PSID          : 2170 (0x087A)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt16
 * UNITS         : dBm
 * MIN           : -128
 * MAX           : 0
 * DEFAULT       : -65
 * DESCRIPTION   :
 *  The upper limit for dynamic desensing of the CCK and OFDM modem. The 5G
 *  OFDM modem will not be desensed to a value in excess of this parameter.
 *  Only used when unifiModemDesenseEnabled is set to true.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MODEM_DESENSE_CAP5G 0x087A

/*******************************************************************************
 * NAME          : UnifiModemDesenseOption
 * PSID          : 2171 (0x087B)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 7
 * DESCRIPTION   :
 *  Select which of the modems will be desensed. Only valid if
 *  unifiModemDesenseEnabled is set to true. Set the corresponding bit to
 *  enable 0: CCK desense 1: 2G4 OFDM desense 2: 5G OFDM desense 3: JD
 *  settings adjust for C1 4: Suppress during connect 5: 6G OFDM desense 6:
 *  Desense based on minimum RSSI (instead of avg RSSI) Also see
 *  unifiDesenseSelection
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MODEM_DESENSE_OPTION 0x087B

/*******************************************************************************
 * NAME          : UnifiRssiThresholdForBbbSync
 * PSID          : 2172 (0x087C)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt16
 * MIN           : -32768
 * MAX           : 32767
 * DEFAULT       : 128
 * DESCRIPTION   :
 *  Below this (dBm) threshold, a different set of bbb_sync_ratio and
 *  bbb_num_syms values are used for modem desense by default it is disabled
 *  by keeping a very high value of 128dbm
 *******************************************************************************/
#define SLSI_PSID_UNIFI_RSSI_THRESHOLD_FOR_BBB_SYNC 0x087C

/*******************************************************************************
 * NAME          : UnifiModemDesenseOffset6g
 * PSID          : 2173 (0x087D)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt16
 * UNITS         : dBm
 * MIN           : 0
 * MAX           : 127
 * DEFAULT       : 20
 * DESCRIPTION   :
 *  The offset value for the CCK and OFDM modem desensing. When the 6G OFDM
 *  modem is desensed, the desense value will be set to the averaged STA-AP
 *  RSSI less this parameter value. Only used when unifiModemDesenseEnabled
 *  is set to true.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MODEM_DESENSE_OFFSET6G 0x087D

/*******************************************************************************
 * NAME          : UnifiModemDesenseCap6g
 * PSID          : 2174 (0x087E)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt16
 * UNITS         : dBm
 * MIN           : -128
 * MAX           : 0
 * DEFAULT       : -65
 * DESCRIPTION   :
 *  The upper limit for dynamic desensing of the CCK and OFDM modem. The 6G
 *  OFDM modem will not be desensed to a value in excess of this parameter.
 *  Only used when unifiModemDesenseEnabled is set to true.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MODEM_DESENSE_CAP6G 0x087E

/*******************************************************************************
 * NAME          : UnifiDynamicItoEnable
 * PSID          : 2175 (0x087F)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Switches Dynamic ITO update feature. When ADPS works, this mib should be
 *  set to false to avoid the conflict
 *******************************************************************************/
#define SLSI_PSID_UNIFI_DYNAMIC_ITO_ENABLE 0x087F

/*******************************************************************************
 * NAME          : UnifiPowerSaveDisabledDuration
 * PSID          : 2177 (0x0881)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  Duration of WLAN FW not in Power save mode in seconds
 *******************************************************************************/
#define SLSI_PSID_UNIFI_POWER_SAVE_DISABLED_DURATION 0x0881

/*******************************************************************************
 * NAME          : UnifiPowerSaveEntryCounter
 * PSID          : 2178 (0x0882)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  Count of WLAN FW going into Power save mode
 *******************************************************************************/
#define SLSI_PSID_UNIFI_POWER_SAVE_ENTRY_COUNTER 0x0882

/*******************************************************************************
 * NAME          : UnifiForceShortFullScanConfig
 * PSID          : 2179 (0x0883)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 0X0001
 * DESCRIPTION   :
 *  Controls if and when full Wifi Scans are forced to become short. 3
 *  separate modes defined, not designed to be OR'd together. 0 = Do not turn
 *  full scans into short 1 = Force full scans into short while a USPBO is
 *  attached to a vif, and the AP that we are connected to is classified as
 *  Leaky (STA) 2 = Force full scans into short whenever Macrame is informed
 *  by coex that significant BT activity is present See
 *  coex_mac_update_bt_activity_state()
 *******************************************************************************/
#define SLSI_PSID_UNIFI_FORCE_SHORT_FULL_SCAN_CONFIG 0x0883

/*******************************************************************************
 * NAME          : UnifiAllowCtsChaining
 * PSID          : 2180 (0x0884)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Enable or disable the ability to support blackouts with > 32ms duration
 *  when CTS is being used. This is implemented by sending (chaining) CTS
 *  requests near the end of the existing reservation. false : Do now allow
 *  chained CTS when blackouts > 32ms when CTS is requested true : Allow
 *  chained CTS when blackouts > 32ms is requested.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ALLOW_CTS_CHAINING 0x0884

/*******************************************************************************
 * NAME          : UnifiSchedScanPassiveDuration
 * PSID          : 2181 (0x0885)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 0X806A
 * DESCRIPTION   :
 *  Defines if passive scan schedules can be broken down. If enabled and the
 *  passive scan schedule entry cannot fit in an available schedule slot,
 *  then this will define the minimum schedule duration for which a passive
 *  scan can be considered as a candidate for scheduling. Should the
 *  available schedule be granted, the scan schedule entry duration will be
 *  reduced to the value specified by this MIB entry. Bits 0-14: Smallest
 *  slot duration for which partitioning a passive scan schedule can be
 *  partitioned. Expressed in TUs. Bit 15: 1 - Scan breakdown enabled. 0 -
 *  Scan breakdown disabled. Default setting for this MIB entry is scan
 *  breakdown enabled and slot duration set to 106 TUs.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_SCHED_SCAN_PASSIVE_DURATION 0x0885

/*******************************************************************************
 * NAME          : UnifiSchedScanActiveDuration
 * PSID          : 2182 (0x0886)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 0X8019
 * DESCRIPTION   :
 *  Defines if active scan schedules can be broken down. If enabled and the
 *  active scan schedule entry cannot fit in an available schedule slot, then
 *  this will define the minimum schedule duration for which an active scan
 *  can be considered as a candidate for scheduling. Should the available
 *  schedule be granted, the scan schedule entry duration will be reduced to
 *  the value specified by this MIB entry. Bits 0-14: Smallest slot duration
 *  for which partitioning an active scan schedule can be partitioned.
 *  Expressed in TUs. Bit 15: 1 - Scan breakdown enabled. 0 - Scan breakdown
 *  disabled. Default setting for this MIB entry is scan breakdown enabled
 *  and slot duration set to 25 TUs.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_SCHED_SCAN_ACTIVE_DURATION 0x0886

/*******************************************************************************
 * NAME          : UnifiMaxTotalVifClearTime
 * PSID          : 2184 (0x0888)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * UNITS         : microseconds
 * MIN           : 5000
 * MAX           : 2147483647
 * DEFAULT       : 25000
 * DESCRIPTION   :
 *  This entry controls the maximum amount of time that a STA or an AP VIF
 *  may dedicate to its clearing process: this includes (1) announcing its
 *  absense (by sending a CTS or a NULL frame); (2) (for the STA case only)
 *  Extra listen time for peer AP training or leaky purposes; and (3)
 *  signalling delays between CP and DP to pause the VIF.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MAX_TOTAL_VIF_CLEAR_TIME 0x0888

/*******************************************************************************
 * NAME          : UnifiEnforceMaxSupportedAntenna
 * PSID          : 2186 (0x088A)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Setting this mib will enforce max supported antenna as per platform
 *  configuration This is different from unifiSupportMaxRequiredAntenna which
 *  enforces max antenna for cases like STBC whereas this will enable it even
 *  for non-STBC e.g. to enable/test CDD. Enabling this mib will have power
 *  consumption cost for SISO connection - as 2 antenna may be enabled. This
 *  mib can not be changed at run time and currently implemented for STA type
 *  VIF only (STA/GC)
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ENFORCE_MAX_SUPPORTED_ANTENNA 0x088A

/*******************************************************************************
 * NAME          : UnifiWlanLowPowerDebugControl
 * PSID          : 2187 (0x088B)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 0X0000
 * DESCRIPTION   :
 *  Control WLAN low power debug within firmware with bit flags. 1 defines
 *  enabled, 0 defines disabled. bit 0: Deep sleep bit 1: mifless bit 2: Diet
 *  mode bit 8: enable STA idle exit stats bit 9: enable Diet/legacy mifless
 *  exit stats bit 10:enable AP idle/mifless exit stats
 *******************************************************************************/
#define SLSI_PSID_UNIFI_WLAN_LOW_POWER_DEBUG_CONTROL 0x088B

/*******************************************************************************
 * NAME          : UnifiWlanLowPowerDebugTimeout
 * PSID          : 2188 (0x088C)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 200
 * DESCRIPTION   :
 *  Duration of timeout in milliseconds if certain wlan low power opeartion
 *  does'nt happen
 *******************************************************************************/
#define SLSI_PSID_UNIFI_WLAN_LOW_POWER_DEBUG_TIMEOUT 0x088C

/*******************************************************************************
 * NAME          : UnifiLpControl
 * PSID          : 2190 (0x088E)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       : 0X00000001
 * DESCRIPTION   :
 *  Controls wlan low power interface with Infra. Refer to unifiLPControlBits
 *  for the full set of bit masks. b'0: Enable Deep Sleep b'1: Enable Mifless
 *  v2 b'2: Delay Deep Sleep (time of delay specified by unifiLPControlDelay)
 *  b'3: Delay Mifless v2 (time of delay specified by unifiLPControlDelay)
 *  b'29: Deep Sleep disallowed when any STA vif present b'30: Deep Sleep
 *  disallowed when any NAN vif present
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LP_CONTROL 0x088E

/*******************************************************************************
 * NAME          : UnifiLpControlDelay
 * PSID          : 2191 (0x088F)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  Specifies the delay applied in seconds before unifiLPControl controls are
 *  applied
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LP_CONTROL_DELAY 0x088F

/*******************************************************************************
 * NAME          : UnifiRssiTypeToUse
 * PSID          : 2193 (0x0891)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  Define which type of RSSI averaging will be used internally, e.g. for
 *  determining roaming thresholds. If the value is set to 0, then the
 *  average RSSI for all frame types will be used. If the value is set to 1,
 *  then the maximum RSSI between the beacon-only and non-beacon-only
 *  averages will be used. If the value is set to 2, then the average RSSI
 *  for only beacon frame types will be used. Only used for VIFs of type STA.
 *  Also see unifiRSSIAvgType.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_RSSI_TYPE_TO_USE 0x0891

/*******************************************************************************
 * NAME          : UnifiNumRssiAverageSample
 * PSID          : 2194 (0x0892)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 16
 * DESCRIPTION   :
 *  The amount of samples used for calculating the average RSSI value.
 *  WARNING: Changing this value after system start-up will have no effect.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NUM_RSSI_AVERAGE_SAMPLE 0x0892

/*******************************************************************************
 * NAME          : UnifiChannelUtilizationBeaconIntervals
 * PSID          : 2195 (0x0893)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       : 50
 * DESCRIPTION   :
 *  The amount of samples used for calculating channel utilization. WARNING:
 *  Changing this value after system start-up will have no effect.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_CHANNEL_UTILIZATION_BEACON_INTERVALS 0x0893

/*******************************************************************************
 * NAME          : UnifiRxDataRate
 * PSID          : 2196 (0x0894)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  The bit rate of the last received frame on this VIF.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_RX_DATA_RATE 0x0894

/*******************************************************************************
 * NAME          : UnifiRssiPerRadio
 * PSID          : 2197 (0x0895)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt16
 * MIN           : -32768
 * MAX           : 32767
 * DEFAULT       :
 * DESCRIPTION   :
 *  Read per antenna(radio) rssi value
 *******************************************************************************/
#define SLSI_PSID_UNIFI_RSSI_PER_RADIO 0x0895

/*******************************************************************************
 * NAME          : UnifiPeerRxRetryCount
 * PSID          : 2198 (0x0896)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  The number of retry packets from peer station
 *******************************************************************************/
#define SLSI_PSID_UNIFI_PEER_RX_RETRY_COUNT 0x0896

/*******************************************************************************
 * NAME          : UnifiPeerRxMulticastCount
 * PSID          : 2199 (0x0897)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  The number of multicast and broadcast packets received from peer station
 *******************************************************************************/
#define SLSI_PSID_UNIFI_PEER_RX_MULTICAST_COUNT 0x0897

/*******************************************************************************
 * NAME          : UnifiRssi
 * PSID          : 2200 (0x0898)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt16
 * UNITS         : dBm
 * MIN           : -32768
 * MAX           : 32767
 * DEFAULT       :
 * DESCRIPTION   :
 *  Running average of the Received Signal Strength Indication (RSSI) for
 *  packets received by UniFi&apos;s radio. The value should only be treated
 *  as an indication of the signal strength; it is not an accurate
 *  measurement. The result is only meaningful if the unifiRxExternalGain
 *  attribute is set to the correct calibration value. If UniFi is part of a
 *  BSS, only frames originating from devices in the BSS are reported (so far
 *  as this can be determined). The average is reset when UniFi joins or
 *  starts a BSS or is reset.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_RSSI 0x0898

/*******************************************************************************
 * NAME          : UnifiLastBssRssi
 * PSID          : 2201 (0x0899)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt16
 * MIN           : -32768
 * MAX           : 32767
 * DEFAULT       :
 * DESCRIPTION   :
 *  Last BSS RSSI. See unifiRSSI description.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LAST_BSS_RSSI 0x0899

/*******************************************************************************
 * NAME          : UnifiSnr
 * PSID          : 2202 (0x089A)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt16
 * MIN           : -32768
 * MAX           : 32767
 * DEFAULT       :
 * DESCRIPTION   :
 *  Provides a running average of the Signal to Noise Ratio (dB) for packets
 *  received by UniFi&apos;s radio.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_SNR 0x089A

/*******************************************************************************
 * NAME          : UnifiLastBssSnr
 * PSID          : 2203 (0x089B)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt16
 * MIN           : -32768
 * MAX           : 32767
 * DEFAULT       :
 * DESCRIPTION   :
 *  Last BSS SNR. See unifiSNR description.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LAST_BSS_SNR 0x089B

/*******************************************************************************
 * NAME          : UnifiSwTxTimeout
 * PSID          : 2204 (0x089C)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * UNITS         : second
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 5
 * DESCRIPTION   :
 *  Maximum time in seconds for a frame to be queued in firmware, ready to be
 *  sent, but not yet actually pumped to hardware.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_SW_TX_TIMEOUT 0x089C

/*******************************************************************************
 * NAME          : UnifiDelayedWakeUpFlushThreshold
 * PSID          : 2205 (0x089D)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 190
 * DEFAULT       : 120
 * DESCRIPTION   :
 *  Count of free entries in to-host HIP queue as flush threshold for Delayed
 *  WU(Wake-Up) feature in RQMT-1807, above which to-host HIP flush will be
 *  performed raising to-host interrupt. The maximum threshold value is 190
 *  considering queue length (HIP4: 256, HIP5: 256/512) receiving two
 *  consecutive ampdus.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_DELAYED_WAKE_UP_FLUSH_THRESHOLD 0x089D

/*******************************************************************************
 * NAME          : UnifiRateStatsRxSuccessCount
 * PSID          : 2206 (0x089E)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  The number of successful receptions of complete management and data
 *  frames at the rate indexed by unifiRateStatsIndex.This number will wrap
 *  to zero after the range is exceeded.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_RATE_STATS_RX_SUCCESS_COUNT 0x089E

/*******************************************************************************
 * NAME          : UnifiRateStatsTxSuccessCount
 * PSID          : 2207 (0x089F)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  The number of successful (acknowledged) unicast transmissions of complete
 *  data or management frames the rate indexed by unifiRateStatsIndex. This
 *  number will wrap to zero after the range is exceeded.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_RATE_STATS_TX_SUCCESS_COUNT 0x089F

/*******************************************************************************
 * NAME          : UnifiTxDataRate
 * PSID          : 2208 (0x08A0)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  The bit rate currently in use for transmissions of unicast data frames;
 *  On an infrastructure BSS, this is the data rate used in communicating
 *  with the associated access point, if there is none, an error is returned
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TX_DATA_RATE 0x08A0

/*******************************************************************************
 * NAME          : UnifiSnrExtraOffsetCck
 * PSID          : 2209 (0x08A1)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt16
 * UNITS         : dB
 * MIN           : -32768
 * MAX           : 32767
 * DEFAULT       : 8
 * DESCRIPTION   :
 *  This offset is added to SNR values received at 802.11b data rates. This
 *  accounts for differences in the RF pathway between 802.11b and 802.11g
 *  demodulators. The offset applies to values of unifiSNR as well as SNR
 *  values in scan indications. Not used in 5GHz mode.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_SNR_EXTRA_OFFSET_CCK 0x08A1

/*******************************************************************************
 * NAME          : UnifiRssiMaxAveragingPeriod
 * PSID          : 2210 (0x08A2)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * UNITS         : TU
 * MIN           : 1
 * MAX           : 65535
 * DEFAULT       : 3000
 * DESCRIPTION   :
 *  Limits the period over which the value of unifiRSSI is averaged. If no
 *  more than unifiRSSIMinReceivedFrames frames have been received in the
 *  period, then the value of unifiRSSI is reset to the value of the next
 *  measurement and the rolling average is restarted. This ensures that the
 *  value is timely (although possibly poorly averaged) when little data is
 *  being received.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_RSSI_MAX_AVERAGING_PERIOD 0x08A2

/*******************************************************************************
 * NAME          : UnifiRssiMinReceivedFrames
 * PSID          : 2211 (0x08A3)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 1
 * MAX           : 65535
 * DEFAULT       : 2
 * DESCRIPTION   :
 *  See the description of unifiRSSIMaxAveragingPeriod for how the
 *  combination of attributes is used.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_RSSI_MIN_RECEIVED_FRAMES 0x08A3

/*******************************************************************************
 * NAME          : UnifiRateStatsRate
 * PSID          : 2212 (0x08A4)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * UNITS         : 500 kbps
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  The rate corresponding to the current table entry. The value is rounded
 *  to the nearest number of units where necessary. Most rates do not require
 *  rounding, but when short guard interval is in effect the rates are no
 *  longer multiples of the base unit. Note that there may be two occurrences
 *  of the value 130: the first corresponds to MCS index 7, and the second,
 *  if present, to MCS index 6 with short guard interval.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_RATE_STATS_RATE 0x08A4

/*******************************************************************************
 * NAME          : UnifiLastBssTxDataRate
 * PSID          : 2213 (0x08A5)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  Last BSS Tx DataRate. See unifiTxDataRate description.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LAST_BSS_TX_DATA_RATE 0x08A5

/*******************************************************************************
 * NAME          : UnifiDiscardedFrameCount
 * PSID          : 2214 (0x08A6)
 * PER INTERFACE?: NO
 * TYPE          : UINT64
 * MIN           : 0
 * MAX           : 18446744073709551615
 * DEFAULT       :
 * DESCRIPTION   :
 *  This is a counter that indicates the number of data and management frames
 *  that have been processed by the UniFi hardware but were discarded before
 *  being processed by the firmware. It does not include frames not processed
 *  by the hardware because they were not addressed to the local device, nor
 *  does it include frames discarded by the firmware in the course of normal
 *  MAC processing (which include, for example, frames in an appropriate
 *  encryption state and multicast frames not requested by the host).
 *  Typically this counter indicates lost data frames for which there was no
 *  buffer space; however, other cases may cause the counter to increment,
 *  such as receiving a retransmitted frame that was already successfully
 *  processed. Hence this counter should not be treated as a reliable guide
 *  to lost frames. The counter wraps to 0 after 65535.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_DISCARDED_FRAME_COUNT 0x08A6

/*******************************************************************************
 * NAME          : UnifiMacrameDebugStats
 * PSID          : 2215 (0x08A7)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  MACRAME debug stats readout key. Use set to write a debug readout, then
 *  read the same key to get the actual readout.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MACRAME_DEBUG_STATS 0x08A7

/*******************************************************************************
 * NAME          : UnifiMacrameLpExitStats
 * PSID          : 2216 (0x08A8)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  MACRAME LP exit stats readout key. Use set to write a stats readout, then
 *  read the same key to get the actual readout.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MACRAME_LP_EXIT_STATS 0x08A8

/*******************************************************************************
 * NAME          : UnifiCurrentTsfTime
 * PSID          : 2218 (0x08AA)
 * PER INTERFACE?: NO
 * TYPE          : UINT64
 * MIN           : 0
 * MAX           : 18446744073709551615
 * DEFAULT       :
 * DESCRIPTION   :
 *  Get TSF time (last 32 bits) for the specified VIF. VIF index can't be 0
 *  as that is treated as global VIF For station VIF - Correct BSS TSF wil
 *  only be reported after MLME-CONNECT.indication(success) indication to
 *  host. Note that if MAC Hardware is switched off then TSF returned is
 *  estimated value
 *******************************************************************************/
#define SLSI_PSID_UNIFI_CURRENT_TSF_TIME 0x08AA

/*******************************************************************************
 * NAME          : UnifiBaRxEnableTid
 * PSID          : 2219 (0x08AB)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       : 0X5555
 * DESCRIPTION   :
 *  Configure Block Ack RX on a per-TID basis. Bit mask is two bits per TID
 *  (B1 = Not Used, B0 = enable).
 *******************************************************************************/
#define SLSI_PSID_UNIFI_BA_RX_ENABLE_TID 0x08AB

/*******************************************************************************
 * NAME          : UnifiBaTxEnableTid
 * PSID          : 2221 (0x08AD)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       : 0X0557
 * DESCRIPTION   :
 *  Golden Certification MIB don't delete, change PSID or name: Configure
 *  Block Ack TX on a per-TID basis. Bit mask is two bits per TID (B1 =
 *  autosetup, B0 = enable).
 *******************************************************************************/
#define SLSI_PSID_UNIFI_BA_TX_ENABLE_TID 0x08AD

/*******************************************************************************
 * NAME          : UnifiTrafficThresholdToSetupBa
 * PSID          : 2222 (0x08AE)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       : 100
 * DESCRIPTION   :
 *  Sets the default Threshold (as packet count) to setup BA agreement per
 *  TID.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TRAFFIC_THRESHOLD_TO_SETUP_BA 0x08AE

/*******************************************************************************
 * NAME          : UnifiDplaneTxAmsduHwCapability
 * PSID          : 2223 (0x08AF)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  Returns 0 if A-MSDU size limited to 4K. Returns 1 is A-MSDU size is
 *  limited to 8K. This value is chip specific and limited by HW.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_DPLANE_TX_AMSDU_HW_CAPABILITY 0x08AF

/*******************************************************************************
 * NAME          : UnifiDplaneTxAmsduSubframeCountMax
 * PSID          : 2224 (0x08B0)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 1
 * MAX           : 5
 * DEFAULT       : 3
 * DESCRIPTION   :
 *  Defines the maximum number of A-MSDU sub-frames per A-MSDU. A value of 1
 *  indicates A-MSDU aggregation has been disabled
 *******************************************************************************/
#define SLSI_PSID_UNIFI_DPLANE_TX_AMSDU_SUBFRAME_COUNT_MAX 0x08B0

/*******************************************************************************
 * NAME          : UnifiBaConfig
 * PSID          : 2225 (0x08B1)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  Block Ack Configuration. It is composed of A-MSDU supported, TX MPDU per
 *  A-MPDU, RX Buffer size, TX Buffer size and Block Ack Timeout. Indexed by
 *  unifiAccessClassIndex.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_BA_CONFIG 0x08B1

/*******************************************************************************
 * NAME          : UnifiBaTxMaxNumber
 * PSID          : 2226 (0x08B2)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 0X10
 * DESCRIPTION   :
 *  Block Ack Configuration. Maximum number of BAs. Limited by HW.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_BA_TX_MAX_NUMBER 0x08B2

/*******************************************************************************
 * NAME          : UnifiMoveBKtoBe
 * PSID          : 2227 (0x08B3)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       :
 * DESCRIPTION   :
 *  Deprecated. Golden Certification MIB don't delete, change PSID or name
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MOVE_BKTO_BE 0x08B3

/*******************************************************************************
 * NAME          : UnifiBeaconReceived
 * PSID          : 2228 (0x08B4)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  Access point beacon received count from connected AP
 *******************************************************************************/
#define SLSI_PSID_UNIFI_BEACON_RECEIVED 0x08B4

/*******************************************************************************
 * NAME          : UnifiAcRetries
 * PSID          : 2229 (0x08B5)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  It represents the number of retransmitted frames under each ac priority
 *  (indexed by unifiAccessClassIndex). This number will wrap to zero after
 *  the range is exceeded.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_AC_RETRIES 0x08B5

/*******************************************************************************
 * NAME          : UnifiRadioOnTime
 * PSID          : 2230 (0x08B6)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  msecs the radio is awake (32 bits number accruing over time). On
 *  multi-radio platforms an index to the radio instance is required
 *******************************************************************************/
#define SLSI_PSID_UNIFI_RADIO_ON_TIME 0x08B6

/*******************************************************************************
 * NAME          : UnifiRadioTxTime
 * PSID          : 2231 (0x08B7)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  msecs the radio is transmitting (32 bits number accruing over time). On
 *  multi-radio platforms an index to the radio instance is required
 *******************************************************************************/
#define SLSI_PSID_UNIFI_RADIO_TX_TIME 0x08B7

/*******************************************************************************
 * NAME          : UnifiRadioRxTime
 * PSID          : 2232 (0x08B8)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  msecs the radio is in active receive (32 bits number accruing over time).
 *  On multi-radio platforms an index to the radio instance is required
 *******************************************************************************/
#define SLSI_PSID_UNIFI_RADIO_RX_TIME 0x08B8

/*******************************************************************************
 * NAME          : UnifiRadioScanTime
 * PSID          : 2233 (0x08B9)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  msecs the radio is awake due to all scan (32 bits number accruing over
 *  time). On multi-radio platforms an index to the radio instance is
 *  required
 *******************************************************************************/
#define SLSI_PSID_UNIFI_RADIO_SCAN_TIME 0x08B9

/*******************************************************************************
 * NAME          : UnifiPsLeakyAp
 * PSID          : 2234 (0x08BA)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  AP evaluation state
 *******************************************************************************/
#define SLSI_PSID_UNIFI_PS_LEAKY_AP 0x08BA

/*******************************************************************************
 * NAME          : UnifiTqamActivated
 * PSID          : 2235 (0x08BB)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Activate Vendor VHT IE for 256-QAM mode on 2.4GHz.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TQAM_ACTIVATED 0x08BB

/*******************************************************************************
 * NAME          : UnifiRadioOnTimeNan
 * PSID          : 2236 (0x08BC)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  msecs the radio is awake due to NAN operations (32 bits number accruing
 *  over time). On multi-radio platforms an index to the radio instance is
 *  required
 *******************************************************************************/
#define SLSI_PSID_UNIFI_RADIO_ON_TIME_NAN 0x08BC

/*******************************************************************************
 * NAME          : UnifiNqamActivated
 * PSID          : 2237 (0x08BD)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Activate 1024-QAM mode(Nitro QAM) on 2.4GHz/5GHz.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NQAM_ACTIVATED 0x08BD

/*******************************************************************************
 * NAME          : UnifiPhyEventLogEnable
 * PSID          : 2238 (0x08BE)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 1
 * DESCRIPTION   :
 *  Enable or Disable PHY Event Log - Disable: zero, Enable: None zero
 *******************************************************************************/
#define SLSI_PSID_UNIFI_PHY_EVENT_LOG_ENABLE 0x08BE

/*******************************************************************************
 * NAME          : UnifiOutputRadioInfoToKernelLog
 * PSID          : 2239 (0x08BF)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Print messages about the radio status to the Android Kernel Log. See
 *  document SC-508266-TC.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_OUTPUT_RADIO_INFO_TO_KERNEL_LOG 0x08BF

/*******************************************************************************
 * NAME          : UnifiNoAckActivationCount
 * PSID          : 2240 (0x08C0)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  The number of frames that are discarded due to HW No-ack activated during
 *  test. This number will wrap to zero after the range is exceeded.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NO_ACK_ACTIVATION_COUNT 0x08C0

/*******************************************************************************
 * NAME          : UnifiRxFcsErrorCount
 * PSID          : 2241 (0x08C1)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  The number of received frames that are discarded due to bad FCS (CRC).
 *  This number will wrap to zero after the range is exceeded.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_RX_FCS_ERROR_COUNT 0x08C1

/*******************************************************************************
 * NAME          : UnifiPeerFrameRxCounters
 * PSID          : 2242 (0x08C2)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  Peer frame RX Counters used by the host.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_PEER_FRAME_RX_COUNTERS 0x08C2

/*******************************************************************************
 * NAME          : UnifiPeerFrameTxCounters
 * PSID          : 2243 (0x08C3)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  Peer frame TX Counters used by the host.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_PEER_FRAME_TX_COUNTERS 0x08C3

/*******************************************************************************
 * NAME          : UnifiBeaconsReceivedPercentage
 * PSID          : 2245 (0x08C5)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  Percentage of beacons received, calculated as received / expected. The
 *  percentage is scaled to an integer value between 0 (0%) and 1000 (100%).
 *******************************************************************************/
#define SLSI_PSID_UNIFI_BEACONS_RECEIVED_PERCENTAGE 0x08C5

/*******************************************************************************
 * NAME          : UnifiArpDetectActivated
 * PSID          : 2246 (0x08C6)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Activate feature support for Enhanced ARP Detect. This is required by
 *  Volcano.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ARP_DETECT_ACTIVATED 0x08C6

/*******************************************************************************
 * NAME          : UnifiArpDetectResponseCounter
 * PSID          : 2247 (0x08C7)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  Counter used to track ARP Response frame for Enhanced ARP Detect. This is
 *  required by Volcano.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ARP_DETECT_RESPONSE_COUNTER 0x08C7

/*******************************************************************************
 * NAME          : UnifiReadTxPacketStatsFromStaVif
 * PSID          : 2248 (0x08C8)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Read TX packet stats from VIF to avoid resetting of the counters during
 *  roaming - this is a Volcano requirement
 *******************************************************************************/
#define SLSI_PSID_UNIFI_READ_TX_PACKET_STATS_FROM_STA_VIF 0x08C8

/*******************************************************************************
 * NAME          : UnifiEnableMgmtPacketStats
 * PSID          : 2249 (0x08C9)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Consider management packets for Tx RX stats counters
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ENABLE_MGMT_PACKET_STATS 0x08C9

/*******************************************************************************
 * NAME          : UnifiSwToHwQueueStats
 * PSID          : 2250 (0x08CA)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  The timing statistics of packets being queued between SW-HW
 *******************************************************************************/
#define SLSI_PSID_UNIFI_SW_TO_HW_QUEUE_STATS 0x08CA

/*******************************************************************************
 * NAME          : UnifiHostToSwQueueStats
 * PSID          : 2251 (0x08CB)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  The timing statistics of packets being queued between HOST-SW
 *******************************************************************************/
#define SLSI_PSID_UNIFI_HOST_TO_SW_QUEUE_STATS 0x08CB

/*******************************************************************************
 * NAME          : UnifiQueueStatsEnable
 * PSID          : 2252 (0x08CC)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Enables recording timing statistics of packets being queued between
 *  HOST-SW-HW
 *******************************************************************************/
#define SLSI_PSID_UNIFI_QUEUE_STATS_ENABLE 0x08CC

/*******************************************************************************
 * NAME          : UnifiTxDataConfirm
 * PSID          : 2253 (0x08CD)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       :
 * DESCRIPTION   :
 *  Allows to request on a per access class basis that an MA_UNITDATA.confirm
 *  be generated after each packet transfer. The default value is applied for
 *  all ACs.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TX_DATA_CONFIRM 0x08CD

/*******************************************************************************
 * NAME          : UnifiThroughputDebug
 * PSID          : 2254 (0x08CE)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  is used to access throughput related counters that can help diagnose
 *  throughput problems. The index of the MIB will access different counters,
 *  as described in SC-506328-DD. Setting any index for a VIF to any value,
 *  clears all DPLP debug stats for the MAC instance used by the VIF. This is
 *  useful mainly for debugging LAA or small scale throughput issues that
 *  require short term collection of the statistics.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_THROUGHPUT_DEBUG 0x08CE

/*******************************************************************************
 * NAME          : UnifiLoadDpdLut
 * PSID          : 2255 (0x08CF)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 147
 * MAX           : 147
 * DEFAULT       :
 * DESCRIPTION   :
 *  Write a static DPD LUT to the FW, read DPD LUT from hardware
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LOAD_DPD_LUT 0x08CF

/*******************************************************************************
 * NAME          : UnifiDpdMasterSwitch
 * PSID          : 2256 (0x08D0)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  Enables Digital Pre-Distortion
 *******************************************************************************/
#define SLSI_PSID_UNIFI_DPD_MASTER_SWITCH 0x08D0

/*******************************************************************************
 * NAME          : UnifiDpdPredistortGains
 * PSID          : 2257 (0x08D1)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 14
 * MAX           : 14
 * DEFAULT       :
 * DESCRIPTION   :
 *  DPD pre-distort gains. Takes a range of frequencies, where f_min &lt;=
 *  f_channel &lt; f_max. The format is [freq_min_msb, freq_min_lsb,
 *  freq_max_msb, freq_max_lsb, DPD policy bitmap, bandwidth_bitmap,
 *  power_trim_enable, OFDM0_gain, OFDM1_gain, CCK_gain, TR_gain, CCK PSAT
 *  gain, OFDM PSAT gain].
 *******************************************************************************/
#define SLSI_PSID_UNIFI_DPD_PREDISTORT_GAINS 0x08D1

/*******************************************************************************
 * NAME          : UnifiOverrideDpdLut
 * PSID          : 2258 (0x08D2)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 147
 * MAX           : 147
 * DEFAULT       :
 * DESCRIPTION   :
 *  Write a DPD LUT directly to the HW
 *******************************************************************************/
#define SLSI_PSID_UNIFI_OVERRIDE_DPD_LUT 0x08D2

/*******************************************************************************
 * NAME          : UnifiRadioLongTrimPowerCap
 * PSID          : 2259 (0x08D3)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt16
 * UNITS         : dBm
 * MIN           : -128
 * MAX           : 127
 * DEFAULT       :
 * DESCRIPTION   :
 *  When a long running trim is active we will be in Reduced Power Mode. RICE
 *  will automatically limit the power for frames other than 11ax triggered
 *  frames. This MIB specifies the power cap. MACRAME/DPLANE/MLME may need to
 *  reconfgure 11ax triggered frame operation to also not exceed this cap.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_RADIO_LONG_TRIM_POWER_CAP 0x08D3

/*******************************************************************************
 * NAME          : UnifiRestrictScoTxopSize
 * PSID          : 2265 (0x08D9)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  If non 0 value, during an SCO/ESCO USBPO, restrict the TXOP Limit to
 *  value (in steps of 32us). 0 = Do not restrict.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_RESTRICT_SCO_TXOP_SIZE 0x08D9

/*******************************************************************************
 * NAME          : UnifiUspboTxAggregateBufferSize
 * PSID          : 2266 (0x08DA)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 8
 * DESCRIPTION   :
 *  Control BA TX agreements and Restrict the Tx BA Buffer size during
 *  non-LTE USPBO. 0 = Tx BA agreements will be suppressed during USPBO. 1 -
 *  256 = Tx BA agreements will be preserved during USPBO, buffer size
 *  limited to this value. 0xffff = Tx BA agreements will be preserved during
 *  USPBO, buffer size limited to default value.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_USPBO_TX_AGGREGATE_BUFFER_SIZE 0x08DA

/*******************************************************************************
 * NAME          : UnifiNonLteUspboConfig
 * PSID          : 2267 (0x08DB)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 0X00
 * DESCRIPTION   :
 *  Deprecated. Configuration bitmap for non-LTE BlockACK control. bit0: If
 *  the bit value is 1, then preserve RX BA agreements during USPBO. If the
 *  bit value is 0, then RX BA agreements will be suppressed during USPBO
 *  bit1: If the bit value is 1, then preserve TX BA agreements during USPBO.
 *  If the bit value is 0, then TX BA agreements will be suppressed during
 *  USPBO bit2-bit15: reserved
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NON_LTE_USPBO_CONFIG 0x08DB

/*******************************************************************************
 * NAME          : UnifiSetResponseRate
 * PSID          : 2268 (0x08DC)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  Forces the response rate to the one of the selected or lower than rx
 *  frame A bitmask of following rates MAC_BASIC_RATE11A_06MBPS_MASK =
 *  0x0001, MAC_BASIC_RATE11A_09MBPS_MASK = 0x0002,
 *  MAC_BASIC_RATE11A_12MBPS_MASK = 0x0004, MAC_BASIC_RATE11A_18MBPS_MASK =
 *  0x0008, MAC_BASIC_RATE11A_24MBPS_MASK = 0x0010,
 *  MAC_BASIC_RATE11A_36MBPS_MASK = 0x0020, MAC_BASIC_RATE11A_48MBPS_MASK =
 *  0x0040, MAC_BASIC_RATE11A_54MBPS_MASK = 0x0080,
 *  MAC_BASIC_RATE11B_01MBPS_MASK = 0x0100, MAC_BASIC_RATE11B_02MBPS_MASK =
 *  0x0200, MAC_BASIC_RATE11B_5M5BPS_CCK_MASK = 0x0400,
 *  MAC_BASIC_RATE11B_11MBPS_CCK_MASK = 0x0800
 *******************************************************************************/
#define SLSI_PSID_UNIFI_SET_RESPONSE_RATE 0x08DC

/*******************************************************************************
 * NAME          : UnifiMacBeaconTimeout
 * PSID          : 2270 (0x08DE)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * UNITS         : microseconds
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 128
 * DESCRIPTION   :
 *  The maximum time in microseconds we want to stall TX data when expecting
 *  a beacon at EBRT time as a station.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MAC_BEACON_TIMEOUT 0x08DE

/*******************************************************************************
 * NAME          : UnifiBlockScanAfterNumSchedVif
 * PSID          : 2272 (0x08E0)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  Block Scan requests from having medium time after a specified amount of
 *  sync VIFs are schedulable. A value of 0 disables the functionality.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_BLOCK_SCAN_AFTER_NUM_SCHED_VIF 0x08E0

/*******************************************************************************
 * NAME          : UnifiStaUsesOneAntennaWhenIdle
 * PSID          : 2274 (0x08E2)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Allow the platform to downgrade antenna usage for STA VIFs to 1 if the
 *  VIF is idle. Only valid for multi-radio platforms.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_STA_USES_ONE_ANTENNA_WHEN_IDLE 0x08E2

/*******************************************************************************
 * NAME          : UnifiStaUsesMultiAntennasDuringConnect
 * PSID          : 2275 (0x08E3)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Allow the platform to use multiple antennas for STA VIFs during the
 *  connect phase. Only valid for multi-radio platforms.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_STA_USES_MULTI_ANTENNAS_DURING_CONNECT 0x08E3

/*******************************************************************************
 * NAME          : UnifiPreferredAntennaBitmap
 * PSID          : 2278 (0x08E6)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 3
 * DEFAULT       :
 * DESCRIPTION   :
 *  Golden Certification MIB don't delete, change PSID or name: Specify the
 *  preferred antenna(s) to use. A value of 0 means that the FW will decide
 *  on the antenna(s) to use. Only valid for multi-radio platforms. S630: -
 *  bit0 : RADIO_0_A (2G) or RADIO_0_B (5G/6G) - bit1 : RADIO_1_A (2G) or
 *  RADIO_1_B (5G/6G) S62x or earlier than S630: - bit0 : RADIO_0_A - bit1 :
 *  RADIO_1_A
 *******************************************************************************/
#define SLSI_PSID_UNIFI_PREFERRED_ANTENNA_BITMAP 0x08E6

/*******************************************************************************
 * NAME          : UnifiMaxConcurrentMaCs
 * PSID          : 2279 (0x08E7)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 2
 * DESCRIPTION   :
 *  Golden Certification MIB don't delete, change PSID or name: Specify the
 *  maximum number of MACs that may be used for the platform. For multi-MAC
 *  platforms that value *could* be greater than 1. WARNING: Changing this
 *  value after system start-up will have no effect.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MAX_CONCURRENT_MA_CS 0x08E7

/*******************************************************************************
 * NAME          : UnifiLoadDpdLutPerRadio
 * PSID          : 2280 (0x08E8)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 147
 * MAX           : 147
 * DEFAULT       :
 * DESCRIPTION   :
 *  Write a static DPD LUT to the FW, read DPD LUT from hardware (for devices
 *  that support multiple radios)
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LOAD_DPD_LUT_PER_RADIO 0x08E8

/*******************************************************************************
 * NAME          : UnifiOverrideDpdLutPerRadio
 * PSID          : 2281 (0x08E9)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 147
 * MAX           : 147
 * DEFAULT       :
 * DESCRIPTION   :
 *  Write a DPD LUT directly to the HW (for devices that support multiple
 *  radios)
 *******************************************************************************/
#define SLSI_PSID_UNIFI_OVERRIDE_DPD_LUT_PER_RADIO 0x08E9

/*******************************************************************************
 * NAME          : UnifiRoamingLowLatencySoftRoamingAllowed
 * PSID          : 2282 (0x08EA)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Allow Soft Roaming functionality for some chipsets when the low latency
 *  mode is enabled.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAMING_LOW_LATENCY_SOFT_ROAMING_ALLOWED 0x08EA

/*******************************************************************************
 * NAME          : UnifiRoamingSoftRoamingActivatedWhenLcdIsOff
 * PSID          : 2284 (0x08EC)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Activate Soft Roaming functionality when the LCD is off.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAMING_SOFT_ROAMING_ACTIVATED_WHEN_LCD_IS_OFF 0x08EC

/*******************************************************************************
 * NAME          : UnifiRoamingSendDeauthWhenRoamingBack
 * PSID          : 2285 (0x08ED)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Send a deauthentication frame before sending an authentication frame when
 *  roaming back.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAMING_SEND_DEAUTH_WHEN_ROAMING_BACK 0x08ED

/*******************************************************************************
 * NAME          : UnifiRoamBtmDisregardSelectionFactor
 * PSID          : 2286 (0x08EE)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Disregard the selection factor for BTM candidates.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_BTM_DISREGARD_SELECTION_FACTOR 0x08EE

/*******************************************************************************
 * NAME          : UnifiTestSetMaxBandwidth
 * PSID          : 2287 (0x08EF)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  Golden Certification MIB don't delete, change PSID or name. Set the
 *  maximum bandwidth for a vif Setting it to 0 uses the default bandwidth as
 *  selected by firmware. channel_bw_20_mhz = 20, channel_bw_40_mhz = 40,
 *  channel_bw_80_mhz = 80, channel_bw_160_mhz = 160, channel_bw_320_mhz =
 *  176
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TEST_SET_MAX_BANDWIDTH 0x08EF

/*******************************************************************************
 * NAME          : UnifiLowLatencyScanNProbe
 * PSID          : 2288 (0x08F0)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 1
 * MAX           : 65535
 * DEFAULT       : 2
 * DESCRIPTION   :
 *  Low Latency Scan: The number of ProbeRequest frames per channel.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LOW_LATENCY_SCAN_NPROBE 0x08F0

/*******************************************************************************
 * NAME          : UnifiRoamEteActivated
 * PSID          : 2289 (0x08F1)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  This MIB activates support for calculation of estimated throughput. This
 *  sorts all candidates by descending order of the throughput.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_ETE_ACTIVATED 0x08F1

/*******************************************************************************
 * NAME          : UnifiRoamMpduss
 * PSID          : 2290 (0x08F2)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 1
 * MAX           : 255
 * DEFAULT       : 0X08
 * DESCRIPTION   :
 *  minimum MPDU start spacing of the MPDUs receiver. Limited by HW.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_MPDUSS 0x08F2

/*******************************************************************************
 * NAME          : UnifiRoamBaWinSize
 * PSID          : 2291 (0x08F3)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 255
 * DEFAULT       : 0X20
 * DESCRIPTION   :
 *  Block Ack Configuration. Maximum number of BAs for receiving a-mpdu
 *  frame. Limited by HW.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_BA_WIN_SIZE 0x08F3

/*******************************************************************************
 * NAME          : UnifiRoamNchoCachedScanPeriod
 * PSID          : 2292 (0x08F4)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * UNITS         : microseconds
 * MIN           : 1
 * MAX           : 4294967295
 * DEFAULT       : 10000000
 * DESCRIPTION   :
 *  NCHO: Specifies the period at which the cached channel scan shall be run.
 *  For certification and Host use only.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_NCHO_CACHED_SCAN_PERIOD 0x08F4

/*******************************************************************************
 * NAME          : UnifiRoamCuActivated
 * PSID          : 2294 (0x08F6)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  If false, the FW shall ignore the channel utilisation of QBSS IE in
 *  received Beacon frames.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_CU_ACTIVATED 0x08F6

/*******************************************************************************
 * NAME          : UnifiRoamCuFactor
 * PSID          : 2295 (0x08F7)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 3
 * MAX           : 3
 * DEFAULT       :
 * DESCRIPTION   :
 *  Bi dimensional octet string table for allocating CUfactor to CU values.
 *  First index is the radio band, and the second will be CU table entry. The
 *  tables define the maximum CU value to which the values do apply(MAX CU),
 *  an OFFSET and an A value for the equation: CUfactor = OFFSET - A*(CU)/10
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_CU_FACTOR 0x08F7

/*******************************************************************************
 * NAME          : UnifiBeaconCu
 * PSID          : 2296 (0x08F8)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt16
 * MIN           : -32768
 * MAX           : 32767
 * DEFAULT       :
 * DESCRIPTION   :
 *  Provides a channel utilization of peer, value 255=100% channel
 *  utilization.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_BEACON_CU 0x08F8

/*******************************************************************************
 * NAME          : UnifiRoamBtmForceAbridged
 * PSID          : 2297 (0x08F9)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Force abridged scan, no matter the abridged bit is set or not.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_BTM_FORCE_ABRIDGED 0x08F9

/*******************************************************************************
 * NAME          : UnifiRoamRssiBoost
 * PSID          : 2298 (0x08FA)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt16
 * MIN           : -32768
 * MAX           : 32767
 * DEFAULT       :
 * DESCRIPTION   :
 *  The value in dBm of the RSSI boost for each band
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_RSSI_BOOST 0x08FA

/*******************************************************************************
 * NAME          : UnifiRoamTargetRssiBeaconLost
 * PSID          : 2299 (0x08FB)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt16
 * MIN           : -127
 * MAX           : 32767
 * DEFAULT       : -75
 * DESCRIPTION   :
 *  RSSI threshold under which candidate APs are not deemed eligible for
 *  Beacon Lost scans.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_TARGET_RSSI_BEACON_LOST 0x08FB

/*******************************************************************************
 * NAME          : UnifiRoamCuLocal
 * PSID          : 2300 (0x08FC)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 255
 * DEFAULT       :
 * DESCRIPTION   :
 *  Channel utilisation for the STA VIF, value 255=100% channel utilisation.
 *  - used for roaming
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_CU_LOCAL 0x08FC

/*******************************************************************************
 * NAME          : UnifiRoamTargetRssiEmergency
 * PSID          : 2301 (0x08FD)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt16
 * MIN           : -127
 * MAX           : 32767
 * DEFAULT       : -75
 * DESCRIPTION   :
 *  RSSI threshold under which candidate APs are not deemed eligible for
 *  emergency roaming scans.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_TARGET_RSSI_EMERGENCY 0x08FD

/*******************************************************************************
 * NAME          : UnifiRoamApSelectDeltaFactor
 * PSID          : 2302 (0x08FE)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 100
 * DEFAULT       : 20
 * DESCRIPTION   :
 *  How much higher, in percentage points, does a candidate's score needs to
 *  be in order be considered an eligible candidate? A "0" value renders all
 *  candidates eligible. Please note this applies only to soft roams.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_AP_SELECT_DELTA_FACTOR 0x08FE

/*******************************************************************************
 * NAME          : UnifiRoamCuWeight
 * PSID          : 2303 (0x08FF)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 100
 * DEFAULT       : 35
 * DESCRIPTION   :
 *  Weight of CUfactor, in percentage points, in AP selection algorithm.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_CU_WEIGHT 0x08FF

/*******************************************************************************
 * NAME          : UnifiRoamBtmApSelectDeltaFactor
 * PSID          : 2304 (0x0900)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 100
 * DEFAULT       :
 * DESCRIPTION   :
 *  Delta value applied to the score of the currently connected AP to
 *  determine candidates' eligibility threshold for BTM triggered roaming
 *  scans
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_BTM_AP_SELECT_DELTA_FACTOR 0x0900

/*******************************************************************************
 * NAME          : UnifiRoamRssiweight
 * PSID          : 2305 (0x0901)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 100
 * DEFAULT       : 65
 * DESCRIPTION   :
 *  Weight of RSSI factor, in percentage points, in AP selection algorithm.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_RSSIWEIGHT 0x0901

/*******************************************************************************
 * NAME          : UnifiRoamRssiFactor
 * PSID          : 2306 (0x0902)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 2
 * MAX           : 2
 * DEFAULT       :
 * DESCRIPTION   :
 *  Table allocating RSSIfactor to RSSI values range for each band.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_RSSI_FACTOR 0x0902

/*******************************************************************************
 * NAME          : UnifiRoamCuRssiScanTrigger
 * PSID          : 2307 (0x0903)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt16
 * MIN           : -32768
 * MAX           : 32767
 * DEFAULT       :
 * DESCRIPTION   :
 *  The current channel Averaged RSSI value below which a soft roaming scan
 *  shall initially start, providing high channel utilisation (see
 *  unifiRoamCUScanTrigger). This is a table indexed by frequency band.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_CU_RSSI_SCAN_TRIGGER 0x0903

/*******************************************************************************
 * NAME          : UnifiRoamCuScanTrigger
 * PSID          : 2308 (0x0904)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  BSS Load / Channel Utilisation trigger above which a soft roaming scan
 *  shall initially start. This is a table indexed by frequency band.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_CU_SCAN_TRIGGER 0x0904

/*******************************************************************************
 * NAME          : UnifiRoamBssLoadMonitoringFrequency
 * PSID          : 2309 (0x0905)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 100
 * DEFAULT       : 1
 * DESCRIPTION   :
 *  How often, in seconds, should the BSS load be monitored? - used for
 *  roaming
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_BSS_LOAD_MONITORING_FREQUENCY 0x0905

/*******************************************************************************
 * NAME          : UnifiRoamBlacklistSize
 * PSID          : 2310 (0x0906)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * UNITS         : entries
 * MIN           : 0
 * MAX           : 100
 * DEFAULT       : 5
 * DESCRIPTION   :
 *  Do not remove! Read by the host! And then passed up to the framework.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_BLACKLIST_SIZE 0x0906

/*******************************************************************************
 * NAME          : UnifiCuMeasurementInterval
 * PSID          : 2311 (0x0907)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 1
 * MAX           : 1000
 * DEFAULT       : 500
 * DESCRIPTION   :
 *  The interval in ms to perform the channel usage update
 *******************************************************************************/
#define SLSI_PSID_UNIFI_CU_MEASUREMENT_INTERVAL 0x0907

/*******************************************************************************
 * NAME          : UnifiCurrentBssNss
 * PSID          : 2312 (0x0908)
 * PER INTERFACE?: NO
 * TYPE          : unifiAntennaMode
 * MIN           : 0
 * MAX           : 255
 * DEFAULT       :
 * DESCRIPTION   :
 *  Specifies current AP antenna mode: BIG DATA 0 = SISO, 1 = MIMO (2x2), 2 =
 *  MIMO (3x3), 3 = MIMO (4x4)
 *******************************************************************************/
#define SLSI_PSID_UNIFI_CURRENT_BSS_NSS 0x0908

/*******************************************************************************
 * NAME          : UnifiApMimoUsed
 * PSID          : 2313 (0x0909)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  AP uses MU-MIMO
 *******************************************************************************/
#define SLSI_PSID_UNIFI_AP_MIMO_USED 0x0909

/*******************************************************************************
 * NAME          : UnifiRoamEapolTimeout
 * PSID          : 2314 (0x090A)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 100
 * DEFAULT       : 10
 * DESCRIPTION   :
 *  Maximum time, in seconds, allowed for an offloaded Eapol (4 way
 *  handshake).
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_EAPOL_TIMEOUT 0x090A

/*******************************************************************************
 * NAME          : UnifiRoamingCount
 * PSID          : 2315 (0x090B)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  Number of roams
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAMING_COUNT 0x090B

/*******************************************************************************
 * NAME          : UnifiRoamingAkm
 * PSID          : 2316 (0x090C)
 * PER INTERFACE?: NO
 * TYPE          : unifiRoamingAKM
 * MIN           : 0
 * MAX           : 255
 * DEFAULT       :
 * DESCRIPTION   :
 *  specifies current AKM 0 = None 1 = OKC 2 = FT (FT_1X) 3 = CCKM 4 =
 *  Adaptive 11r 5 = WPA3 SAE-FT
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAMING_AKM 0x090C

/*******************************************************************************
 * NAME          : UnifiCurrentBssBandwidth
 * PSID          : 2317 (0x090D)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  Current bandwidth the STA is operating on channel_bw_20_mhz = 20,
 *  channel_bw_40_mhz = 40, channel_bw_80_mhz = 80, channel_bw_160_mhz = 160,
 *  channel_bw_320_mhz = 176
 *******************************************************************************/
#define SLSI_PSID_UNIFI_CURRENT_BSS_BANDWIDTH 0x090D

/*******************************************************************************
 * NAME          : UnifiCurrentBssChannelFrequency
 * PSID          : 2318 (0x090E)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  Centre frequency for the connected channel
 *******************************************************************************/
#define SLSI_PSID_UNIFI_CURRENT_BSS_CHANNEL_FREQUENCY 0x090E

/*******************************************************************************
 * NAME          : UnifiRoamInactiveCount
 * PSID          : 2319 (0x090F)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * UNITS         : number_of_packets
 * MIN           : 0
 * MAX           : 1000
 * DEFAULT       : 5
 * DESCRIPTION   :
 *  The number of packets over which the link is considered not idle.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_INACTIVE_COUNT 0x090F

/*******************************************************************************
 * NAME          : UnifiLoggerEnabled
 * PSID          : 2320 (0x0910)
 * PER INTERFACE?: NO
 * TYPE          : unifiWifiLogger
 * MIN           : 0
 * MAX           : 255
 * DEFAULT       : 1
 * DESCRIPTION   :
 *  Enable reporting of the following events for Android logging: - firmware
 *  connectivity events - fate of management frames sent by the host through
 *  the MLME SAP It can take the following values: - 0: reporting for non
 *  mandetory triggers disabled. EAPOL, security, btm frames and roam
 *  triggers are reported. - 1: partial reporting is enabled. Beacons frames
 *  will not be reported. - 2: full reporting is enabled. Beacons frames are
 *  included.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LOGGER_ENABLED 0x0910

/*******************************************************************************
 * NAME          : UnifiRoamEmergencyActivated
 * PSID          : 2321 (0x0911)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  If false, the FW does not trigger the roaming scan by emergency
 *  disconnection(deauthentication or disassociation).
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_EMERGENCY_ACTIVATED 0x0911

/*******************************************************************************
 * NAME          : UnifiRoamApSelectMinDeltaFactor
 * PSID          : 2322 (0x0912)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 10000
 * DEFAULT       : 1500
 * DESCRIPTION   :
 *  The minimum score delta with which a candidate needs to be better to be
 *  considered as an eligible candidate. This prevents ping pong roaming.
 *  Please note this applies only to soft roams.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_AP_SELECT_MIN_DELTA_FACTOR 0x0912

/*******************************************************************************
 * NAME          : UnifiRoamBtmActivated
 * PSID          : 2323 (0x0913)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  If false, the FW does not trigger the roaming scan by BTM request.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_BTM_ACTIVATED 0x0913

/*******************************************************************************
 * NAME          : UnifiStaVifLinkNss
 * PSID          : 2324 (0x0914)
 * PER INTERFACE?: NO
 * TYPE          : unifiAntennaMode
 * MIN           : 0
 * MAX           : 255
 * DEFAULT       :
 * DESCRIPTION   :
 *  STA Vif (Not P2P) while connected to an AP and does not apply to TDLS
 *  links. Specifies the max number of NSS that the link can use: BIG DATA 0
 *  = SISO, 1 = MIMO (2x2), 2 = MIMO (3x3), 3 = MIMO (4x4)
 *******************************************************************************/
#define SLSI_PSID_UNIFI_STA_VIF_LINK_NSS 0x0914

/*******************************************************************************
 * NAME          : UnifiCurrentBssAssocMode
 * PSID          : 2325 (0x0915)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  Connection status association mode
 *******************************************************************************/
#define SLSI_PSID_UNIFI_CURRENT_BSS_ASSOC_MODE 0x0915

/*******************************************************************************
 * NAME          : UnifiFrameRxCounters
 * PSID          : 2326 (0x0916)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  Frame RX Counters used by the host. These are required by MCD.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_FRAME_RX_COUNTERS 0x0916

/*******************************************************************************
 * NAME          : UnifiFrameTxCounters
 * PSID          : 2327 (0x0917)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  Frame TX Counters used by the host. These are required by MCD.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_FRAME_TX_COUNTERS 0x0917

/*******************************************************************************
 * NAME          : UnifiAcNoAcks
 * PSID          : 2328 (0x0918)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  It represents the number of frames without ACK for each AC priority
 *  (indexed by unifiAccessClassIndex). This number will wrap to zero after
 *  the range is exceeded.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_AC_NO_ACKS 0x0918

/*******************************************************************************
 * NAME          : UnifiLaaNssSpeculationIntervalSlotTime
 * PSID          : 2330 (0x091A)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 300
 * DESCRIPTION   :
 *  For Link Adaptation Algorithm. It defines the repeatable amount of time,
 *  in ms, that firmware will start to send speculation frames for spatial
 *  streams.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LAA_NSS_SPECULATION_INTERVAL_SLOT_TIME 0x091A

/*******************************************************************************
 * NAME          : UnifiLaaNssSpeculationIntervalSlotMaxNum
 * PSID          : 2331 (0x091B)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 5
 * DESCRIPTION   :
 *  For Link Adaptation Algorithm. It defines the maximum number of
 *  speculation time slot for spatial stream.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LAA_NSS_SPECULATION_INTERVAL_SLOT_MAX_NUM 0x091B

/*******************************************************************************
 * NAME          : UnifiLaaBwSpeculationIntervalSlotTime
 * PSID          : 2332 (0x091C)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 300
 * DESCRIPTION   :
 *  For Link Adaptation Algorithm. It defines the repeatable amount of time,
 *  in ms, that firmware will start to send speculation frames for bandwidth.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LAA_BW_SPECULATION_INTERVAL_SLOT_TIME 0x091C

/*******************************************************************************
 * NAME          : UnifiLaaBwSpeculationIntervalSlotMaxNum
 * PSID          : 2333 (0x091D)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 8
 * DESCRIPTION   :
 *  For Link Adaptation Algorithm. It defines the maximum number of
 *  speculation time slot for bandwidth.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LAA_BW_SPECULATION_INTERVAL_SLOT_MAX_NUM 0x091D

/*******************************************************************************
 * NAME          : UnifiLaaMcsSpeculationIntervalSlotTime
 * PSID          : 2334 (0x091E)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 100
 * DESCRIPTION   :
 *  For Link Adaptation Algorithm. It defines the repeatable amount of time,
 *  in ms, that firmware will start to send speculation frames for MCS or
 *  rate index.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LAA_MCS_SPECULATION_INTERVAL_SLOT_TIME 0x091E

/*******************************************************************************
 * NAME          : UnifiLaaMcsSpeculationIntervalSlotMaxNum
 * PSID          : 2335 (0x091F)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 10
 * DESCRIPTION   :
 *  For Link Adaptation Algorithm. It defines the maximum number of
 *  speculation time slot for MCS or rate index.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LAA_MCS_SPECULATION_INTERVAL_SLOT_MAX_NUM 0x091F

/*******************************************************************************
 * NAME          : UnifiLaaGiSpeculationIntervalSlotTime
 * PSID          : 2336 (0x0920)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 100
 * DESCRIPTION   :
 *  For Link Adaptation Algorithm. It defines the repeatable amount of time,
 *  in ms, that firmware will start to send speculation frames for guard
 *  interval.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LAA_GI_SPECULATION_INTERVAL_SLOT_TIME 0x0920

/*******************************************************************************
 * NAME          : UnifiLaaGiSpeculationIntervalSlotMaxNum
 * PSID          : 2337 (0x0921)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 50
 * DESCRIPTION   :
 *  For Link Adaptation Algorithm. It defines the maximum number of
 *  speculation time slot for guard interval.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LAA_GI_SPECULATION_INTERVAL_SLOT_MAX_NUM 0x0921

/*******************************************************************************
 * NAME          : UnifiLaaTxNssAllowedMode
 * PSID          : 2338 (0x0922)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  If non-zero,it constraites NSS mode used in LAA. Note that this MIB
 *  should be set before any connection is made. - 0 : This MIB is not used.
 *  LAA can use all possible NSS modes - 0x1 : Only 1SS mode can be used
 *  (only bit 0 is set) - 0x2 : Only 2SS mode can be used (only bit 1 is set)
 *  - 0x3 : Both 1SS and 2SS modes can be used (both bit 0 and 1 are set)
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LAA_TX_NSS_ALLOWED_MODE 0x0922

/*******************************************************************************
 * NAME          : UnifiDplaneTxSinkDelayUs
 * PSID          : 2339 (0x0923)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  Configure frame burst gap in DPLANE TX Sink mode. Typical value is 90 us.
 *  Zero value disables TX sink mode.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_DPLANE_TX_SINK_DELAY_US 0x0923

/*******************************************************************************
 * NAME          : UnifiDisableCca
 * PSID          : 2340 (0x0924)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Test only: Disable CCA detection in transmission
 *******************************************************************************/
#define SLSI_PSID_UNIFI_DISABLE_CCA 0x0924

/*******************************************************************************
 * NAME          : UnifiCprofScopeBitmap
 * PSID          : 2341 (0x0925)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       : 0XFFFFFFFF
 * DESCRIPTION   :
 *  CPROF Scope IDs in bitmap to monitor.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_CPROF_SCOPE_BITMAP 0x0925

/*******************************************************************************
 * NAME          : UnifiAmsduAggregationDistribution
 * PSID          : 2342 (0x0926)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  Count of the number of MSDUs per AMSDU. It is a table. Index 0 : Count of
 *  MPDUs where MSDUs can't be aggregated. Index 1 ~ 6 : Count of the MSDUs
 *  per AMSDU.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_AMSDU_AGGREGATION_DISTRIBUTION 0x0926

/*******************************************************************************
 * NAME          : UnifiAmpdutxopDistribution
 * PSID          : 2343 (0x0927)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  Count of the number of MSDUs per AMSDU. It is a table. Index 0 - 31 :
 *  Count of number of AMPDUs in the TXOP (0=1 ...)
 *******************************************************************************/
#define SLSI_PSID_UNIFI_AMPDUTXOP_DISTRIBUTION 0x0927

/*******************************************************************************
 * NAME          : UnifiDebugIgnoreHighImportance
 * PSID          : 2345 (0x0929)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  DPLP will ignore the TRANSMISSION_CONTROL_HIGH_IMPORTANT_FRAME bit
 *******************************************************************************/
#define SLSI_PSID_UNIFI_DEBUG_IGNORE_HIGH_IMPORTANCE 0x0929

/*******************************************************************************
 * NAME          : UnifiLaaTxDiversityBeamformEnabled
 * PSID          : 2350 (0x092E)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  For Link Adaptation Algorithm. It is used to enable or disable TX
 *  beamformer functionality.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LAA_TX_DIVERSITY_BEAMFORM_ENABLED 0x092E

/*******************************************************************************
 * NAME          : UnifiLaaTxDiversityBeamformMinMcs
 * PSID          : 2351 (0x092F)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 2
 * DESCRIPTION   :
 *  For Link Adaptation Algorithm. TX Beamform is applied when MCS is same or
 *  larger than this threshold value.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LAA_TX_DIVERSITY_BEAMFORM_MIN_MCS 0x092F

/*******************************************************************************
 * NAME          : UnifiLaaTxDiversityFixMode
 * PSID          : 2352 (0x0930)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  For Link Adaptation Algorithm. It is used to fix TX diversity mode. With
 *  two antennas available and only one spatial stream used, then one of the
 *  following modes can be selected: - 0 : Not fixed. Tx diversity mode is
 *  automatically selected by LAA. - 1 : CDD fixed mode - 2 : Beamforming
 *  fixed mode - 3 : STBC fixed mode
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LAA_TX_DIVERSITY_FIX_MODE 0x0930

/*******************************************************************************
 * NAME          : UnifiLaaAmpduInhibitTimeout
 * PSID          : 2353 (0x0931)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 20
 * DESCRIPTION   :
 *  When fallback rate for an A-MPDU retransmission is non-(V)HT, we need to
 *  disable A-MPDUs to transmit the MPDU(s) at that rate. LAA does this, and
 *  re-enabled A-MPDUs based on this timeout. Note that the timeout is
 *  examined only when the current rate changes, which signifies some change
 *  in the conditions or state machine. If set to 0, A-MPDUs will restored as
 *  soon as a rate with MCS>1 or BW>20 is installed.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LAA_AMPDU_INHIBIT_TIMEOUT 0x0931

/*******************************************************************************
 * NAME          : UnifiLaaInhibitRtsCount
 * PSID          : 2354 (0x0932)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 32
 * DESCRIPTION   :
 *  When RTS protection for an A-MPDU transmission fails
 *  UnifiLaaInhibitRtsCount successive times, we disable RTS.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LAA_INHIBIT_RTS_COUNT 0x0932

/*******************************************************************************
 * NAME          : UnifiLaaRtsInhibitTimeout
 * PSID          : 2355 (0x0933)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 20
 * DESCRIPTION   :
 *  When RTS protection for an A-MPDU transmission fails
 *  UnifiLaaInhibitRtsCount successive times, we disable RTS. RTS is later
 *  re-enabled based on this timeout. Note that RTS may be controlled by
 *  other means, orthogonally to this feature, e.g. Coex may have disabled
 *  it. If set to 0, RTS will be restored only after a re-association.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LAA_RTS_INHIBIT_TIMEOUT 0x0933

/*******************************************************************************
 * NAME          : UnifiLaaProtectionConfigOverride
 * PSID          : 2356 (0x0934)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 6
 * DESCRIPTION   :
 *  Overrides the default Protection configuration. Only valid flags are
 *  DPIF_PEER_INFO_PROTECTION_TXOP_AMPDU and
 *  DPIF_PEER_INFO_PROTECTION_ALLOWED. Default allows protection code to work
 *  out the rules. If DPIF_PEER_INFO_PROTECTION_ALLOWED is unset, all
 *  protections are disabled. If DPIF_PEER_INFO_PROTECTION_TXOP_AMPDU is
 *  unset then, the first A-MPDU in the TxOp is no longer protected.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LAA_PROTECTION_CONFIG_OVERRIDE 0x0934

/*******************************************************************************
 * NAME          : UnifiLaaSpeculationMaxTime
 * PSID          : 2357 (0x0935)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 5000
 * DESCRIPTION   :
 *  For Link Adaptation Algorithm. It defines the maximum time interval (in
 *  milliseconds) between speculating. So if the interval is being scaled due
 *  to not being scheduled all the time, the speculation interval will not
 *  exceed this value, even when backing off.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LAA_SPECULATION_MAX_TIME 0x0935

/*******************************************************************************
 * NAME          : UnifiRateStatsRtsErrorCount
 * PSID          : 2358 (0x0936)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  The number of successive RTS failures.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_RATE_STATS_RTS_ERROR_COUNT 0x0936

/*******************************************************************************
 * NAME          : UnifiRtsDurationProtectsFullTxop
 * PSID          : 2359 (0x0937)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  When set to true the RTS duration protects full TXOP. If false then it
 *  only covers from the RTS to the first BA, which may be followed by other
 *  bursted AMPDUs
 *******************************************************************************/
#define SLSI_PSID_UNIFI_RTS_DURATION_PROTECTS_FULL_TXOP 0x0937

/*******************************************************************************
 * NAME          : UnifiLaaAmpduInhibitRtsAlso
 * PSID          : 2360 (0x0938)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  When inhibiting A-MPDU also disable RTS
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LAA_AMPDU_INHIBIT_RTS_ALSO 0x0938

/*******************************************************************************
 * NAME          : UnifiLoadMemDpdLut
 * PSID          : 2361 (0x0939)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 250
 * MAX           : 350
 * DEFAULT       :
 * DESCRIPTION   :
 *  Write a static DPD LUT to the FW, read DPD LUT from hardware for devices
 *  that support Memory DPD
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LOAD_MEM_DPD_LUT 0x0939

/*******************************************************************************
 * NAME          : UnifiCsrOnlyEifsDuration
 * PSID          : 2362 (0x093A)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * UNITS         : microseconds
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 12
 * DESCRIPTION   :
 *  Specifies time that is used for EIFS. A value of 0 causes the build in
 *  value to be used.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_CSR_ONLY_EIFS_DURATION 0x093A

/*******************************************************************************
 * NAME          : UnifiOverrideMemDpdLut
 * PSID          : 2363 (0x093B)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 140
 * MAX           : 550
 * DEFAULT       :
 * DESCRIPTION   :
 *  Write a DPD LUT directly to the HW for devices that support Memory DPD
 *******************************************************************************/
#define SLSI_PSID_UNIFI_OVERRIDE_MEM_DPD_LUT 0x093B

/*******************************************************************************
 * NAME          : UnifiOverrideDefaultBetxopForHt
 * PSID          : 2364 (0x093C)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 171
 * DESCRIPTION   :
 *  When set to non-zero value then this will override the BE TXOP for 11n
 *  and higher modulations (in 32 usec units) to the value specified here.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_OVERRIDE_DEFAULT_BETXOP_FOR_HT 0x093C

/*******************************************************************************
 * NAME          : UnifiOverrideDefaultBetxop
 * PSID          : 2365 (0x093D)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 78
 * DESCRIPTION   :
 *  Golden Certification MIB don't delete, change PSID or name: When set to
 *  non-zero value then this will override the BE TXOP for 11g (in 32 usec
 *  units) to the value specified here.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_OVERRIDE_DEFAULT_BETXOP 0x093D

/*******************************************************************************
 * NAME          : UnifiRxabbTrimSettings
 * PSID          : 2366 (0x093E)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  Various settings to change RX ABB filter trim behavior.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_RXABB_TRIM_SETTINGS 0x093E

/*******************************************************************************
 * NAME          : UnifiRadioTrimsEnable
 * PSID          : 2367 (0x093F)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       : 0XFFFFFFF0
 * DESCRIPTION   :
 *  A bitmap for enabling/disabling trims at runtime. Check unifiEnabledTrims
 *  enum for description of the possible values.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_RADIO_TRIMS_ENABLE 0x093F

/*******************************************************************************
 * NAME          : UnifiRadioCcaThresholds
 * PSID          : 2368 (0x0940)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 255
 * DEFAULT       :
 * DESCRIPTION   :
 *  The wideband CCA ED thresholds so that the CCA-ED triggers at the
 *  regulatory value of -62 dBm.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_RADIO_CCA_THRESHOLDS 0x0940

/*******************************************************************************
 * NAME          : UnifiHardwarePlatform
 * PSID          : 2369 (0x0941)
 * PER INTERFACE?: NO
 * TYPE          : unifiHardwarePlatform
 * MIN           : 0
 * MAX           : 255
 * DEFAULT       :
 * DESCRIPTION   :
 *  Hardware platform. This is necessary so we can apply tweaks to specific
 *  revisions, even though they might be running the same baseband and RF
 *  chip combination. Check unifiHardwarePlatform enum for description of the
 *  possible values.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_HARDWARE_PLATFORM 0x0941

/*******************************************************************************
 * NAME          : UnifiForceChannelBw
 * PSID          : 2370 (0x0942)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  Test only: Force channel bandwidth to specified value. This can also be
 *  used to allow emulator/silicon back to back connection to communicate at
 *  bandwidth other than default (20 MHz) Setting it to 0 uses the default
 *  bandwidth as selected by firmware. The change will be applied at next
 *  radio state change opportunity channel_bw_20_mhz = 20, channel_bw_40_mhz
 *  = 40, channel_bw_80_mhz = 80
 *******************************************************************************/
#define SLSI_PSID_UNIFI_FORCE_CHANNEL_BW 0x0942

/*******************************************************************************
 * NAME          : UnifiDpdTrainingDuration
 * PSID          : 2371 (0x0943)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 10
 * DESCRIPTION   :
 *  Duration of DPD training (in ms).
 *******************************************************************************/
#define SLSI_PSID_UNIFI_DPD_TRAINING_DURATION 0x0943

/*******************************************************************************
 * NAME          : UnifiTxFtrimSettings
 * PSID          : 2372 (0x0944)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 255
 * DEFAULT       :
 * DESCRIPTION   :
 *  Hardware specific transmitter frequency compensation settings
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TX_FTRIM_SETTINGS 0x0944

/*******************************************************************************
 * NAME          : UnifiTxPowerTrimCommonConfig
 * PSID          : 2374 (0x0946)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 3
 * MAX           : 255
 * DEFAULT       :
 * DESCRIPTION   :
 *  Common transmitter power trim settings
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TX_POWER_TRIM_COMMON_CONFIG 0x0946

/*******************************************************************************
 * NAME          : UnifiIqDebugEnabled
 * PSID          : 2375 (0x0947)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Send IQ capture data to host for IQ debug
 *******************************************************************************/
#define SLSI_PSID_UNIFI_IQ_DEBUG_ENABLED 0x0947

/*******************************************************************************
 * NAME          : UnifiFtmOneCrossTwoSupport
 * PSID          : 2376 (0x0948)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Enable FTM 1*2 support
 *******************************************************************************/
#define SLSI_PSID_UNIFI_FTM_ONE_CROSS_TWO_SUPPORT 0x0948

/*******************************************************************************
 * NAME          : UnifiRoamFtOverDsEnabled
 * PSID          : 2378 (0x094A)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Enable roaming by FT over DS if BSS support it.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_FT_OVER_DS_ENABLED 0x094A

/*******************************************************************************
 * NAME          : UnifiRoamFtOverDsMinRssi
 * PSID          : 2379 (0x094B)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt16
 * UNITS         : dBm
 * MIN           : -128
 * MAX           : 127
 * DEFAULT       : -70
 * DESCRIPTION   :
 *  Defines the minimum RSSI of the roaming required to try 'FT over the DS'.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_FT_OVER_DS_MIN_RSSI 0x094B

/*******************************************************************************
 * NAME          : UnifiAlwaysIncludeOperatingModeNotification
 * PSID          : 2380 (0x094C)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Golden Certification MIB don't delete, change PSID or name: Always
 *  include OMN IE in the Assoc request frames.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ALWAYS_INCLUDE_OPERATING_MODE_NOTIFICATION 0x094C

/*******************************************************************************
 * NAME          : UnifiSisoOnSingleNssActivated
 * PSID          : 2381 (0x094D)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  When activated, avoid MIMO+OMN and connect as SISO.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_SISO_ON_SINGLE_NSS_ACTIVATED 0x094D

/*******************************************************************************
 * NAME          : UnifiMaxTdlsClient
 * PSID          : 2382 (0x094E)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 255
 * DEFAULT       : 4
 * DESCRIPTION   :
 *  Maximum number of clients Supported in TDLS.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MAX_TDLS_CLIENT 0x094E

/*******************************************************************************
 * NAME          : UnifiAutoConnectionTestActivated
 * PSID          : 2383 (0x094F)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Golden Certification MIB don't delete. Enable auto response for
 *  connection without radio. This MIB will NOT take effect unless the build
 *  is HUTS or SWAT build
 *******************************************************************************/
#define SLSI_PSID_UNIFI_AUTO_CONNECTION_TEST_ACTIVATED 0x094F

/*******************************************************************************
 * NAME          : UnifiRoamWtcActivated
 * PSID          : 2384 (0x0950)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  If false, the FW shall ignore the Cisco VSIE(WTC) in received BTM Request
 *  frames.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_WTC_ACTIVATED 0x0950

/*******************************************************************************
 * NAME          : UnifiAutoConnectionTestBssid
 * PSID          : 2385 (0x0951)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 6
 * MAX           : 6
 * DEFAULT       : { 0X24, 0X4C, 0X0F, 0X10, 0X22, 0X4D }
 * DESCRIPTION   :
 *  Golden Certification MIB don't delete. Select the BSSID for the AP when
 *  auto response is enabled
 *******************************************************************************/
#define SLSI_PSID_UNIFI_AUTO_CONNECTION_TEST_BSSID 0x0951

/*******************************************************************************
 * NAME          : UnifiMscsActivated
 * PSID          : 2386 (0x0952)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  This MIB activates support for MSCS Procedure
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MSCS_ACTIVATED 0x0952

/*******************************************************************************
 * NAME          : UnifiApDeauthTransmitLifetime
 * PSID          : 2387 (0x0953)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 2500
 * DESCRIPTION   :
 *  Lifetime of Deauth frame sent from AP in unit of ms.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_AP_DEAUTH_TRANSMIT_LIFETIME 0x0953

/*******************************************************************************
 * NAME          : UnifiBaRxReductionCpOnly
 * PSID          : 2388 (0x0954)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Do not update the Rx buffer reduction due to coex internally. Only
 *  negotiate it with AP. Added for InterOp, ref: SOC-172279.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_BA_RX_REDUCTION_CP_ONLY 0x0954

/*******************************************************************************
 * NAME          : UnifiQosMapActivated
 * PSID          : 2389 (0x0955)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  This MIB activates support for DSCP-to-UP mapping using QoS Map
 *******************************************************************************/
#define SLSI_PSID_UNIFI_QOS_MAP_ACTIVATED 0x0955

/*******************************************************************************
 * NAME          : UnifiRoamSkipFtEmergencyRoam
 * PSID          : 2390 (0x0956)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Skip roaming for FT 1x and PSK security in case of emergency roaming when
 *  we received deauthenitcation/Disassociation frames from the AP.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_SKIP_FT_EMERGENCY_ROAM 0x0956

/*******************************************************************************
 * NAME          : UnifiQsfsVersion
 * PSID          : 2391 (0x0957)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  This MIB shall be manually incremented by 1 when and only when there is
 *  any change to any of the MIB attributes specified in the tables in
 *  SC-510742-ME.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_QSFS_VERSION 0x0957

/*******************************************************************************
 * NAME          : UnifiHeActivatedP2pGc
 * PSID          : 2392 (0x0958)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Enables HE mode for P2P client.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_HE_ACTIVATED_P2P_GC 0x0958

/*******************************************************************************
 * NAME          : UnifiHeActivatedP2pGo
 * PSID          : 2393 (0x0959)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Enables HE mode for P2P GO. This MIB is controlled by Host.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_HE_ACTIVATED_P2P_GO 0x0959

/*******************************************************************************
 * NAME          : UnifiEnableCoexLowLatency
 * PSID          : 2401 (0x0961)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Enable requests to BT to modify behaviour to support WLAN Low Latency
 *  operation. If enabled, WLAN shall request Low Latency supporting
 *  operation from BT if Low Latency is requested by the host or if
 *  CTS-To-Self is being used to protect Leaky APs See also
 *  unifiUseCtsForLeakyAp.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ENABLE_COEX_LOW_LATENCY 0x0961

/*******************************************************************************
 * NAME          : UnifiCoexApiLoopbackTest
 * PSID          : 2402 (0x0962)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  Perform a loopback test over the WLAN-BT interface.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_COEX_API_LOOPBACK_TEST 0x0962

/*******************************************************************************
 * NAME          : UnifiLteEnableLaaCoex
 * PSID          : 2403 (0x0963)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Enables LAA Coex support
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LTE_ENABLE_LAA_COEX 0x0963

/*******************************************************************************
 * NAME          : UnifiReadWlbtToCpMailbox
 * PSID          : 2404 (0x0964)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 12
 * MAX           : 12
 * DEFAULT       :
 * DESCRIPTION   :
 *  Readback the WLBT->Cellular section of the Mailbox(registers 17-19) FOR
 *  TEST PURPOSES ONLY
 *******************************************************************************/
#define SLSI_PSID_UNIFI_READ_WLBT_TO_CP_MAILBOX 0x0964

/*******************************************************************************
 * NAME          : UniFiLaaOverride
 * PSID          : 2405 (0x0965)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  Control CON_STATUS[5ghz_active] for test purposes
 *******************************************************************************/
#define SLSI_PSID_UNI_FI_LAA_OVERRIDE 0x0965

/*******************************************************************************
 * NAME          : UnifiCoexFleximacBlackoutConfigFlags
 * PSID          : 2406 (0x0966)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 0X0003
 * DESCRIPTION   :
 *  Configuration flags for fleximac blackout support
 *******************************************************************************/
#define SLSI_PSID_UNIFI_COEX_FLEXIMAC_BLACKOUT_CONFIG_FLAGS 0x0966

/*******************************************************************************
 * NAME          : UnifiCoexShortPerioidicBlackoutAttachControl
 * PSID          : 2407 (0x0967)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       : 0X70006
 * DESCRIPTION   :
 *  Configures whether short-duration periodic blackouts of specified type
 *  should be attached to VIFs
 *******************************************************************************/
#define SLSI_PSID_UNIFI_COEX_SHORT_PERIOIDIC_BLACKOUT_ATTACH_CONTROL 0x0967

/*******************************************************************************
 * NAME          : UnifiCoexModingControlFlags
 * PSID          : 2408 (0x0968)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 0X0001
 * DESCRIPTION   :
 *  Configuration flags for coex moding
 *******************************************************************************/
#define SLSI_PSID_UNIFI_COEX_MODING_CONTROL_FLAGS 0x0968

/*******************************************************************************
 * NAME          : UnifiCoexUwbEnable
 * PSID          : 2410 (0x096A)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Force Enable UWB Coex at service start
 *******************************************************************************/
#define SLSI_PSID_UNIFI_COEX_UWB_ENABLE 0x096A

/*******************************************************************************
 * NAME          : UnifiCoexUwbChannelRange
 * PSID          : 2411 (0x096B)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 4
 * MAX           : 4
 * DEFAULT       : {0X00, 0X00, 0X00, 0X00 }
 * DESCRIPTION   :
 *  Specifies range of 6GHz channels and 5GHz Channels which must not be used
 *  for Tx during UWB ranging
 *******************************************************************************/
#define SLSI_PSID_UNIFI_COEX_UWB_CHANNEL_RANGE 0x096B

/*******************************************************************************
 * NAME          : UnifiCoexUwbPrepareTimeMs
 * PSID          : 2412 (0x096C)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 255
 * DEFAULT       : 10
 * DESCRIPTION   :
 *  The time, in ms, between the UWB_WLAN_IND going high and the start of UWB
 *  ranging
 *******************************************************************************/
#define SLSI_PSID_UNIFI_COEX_UWB_PREPARE_TIME_MS 0x096C

/*******************************************************************************
 * NAME          : UnifiCoexUwbMaxGrantDurationMs
 * PSID          : 2413 (0x096D)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 255
 * DEFAULT       : 30
 * DESCRIPTION   :
 *  The maximum time, in ms, that the UWB device will require the medium for,
 *  after the UWB device gains the medium PREPARE TIME after
 *******************************************************************************/
#define SLSI_PSID_UNIFI_COEX_UWB_MAX_GRANT_DURATION_MS 0x096D

/*******************************************************************************
 * NAME          : UnifiCoexUwbSimulateUwbWlanInd
 * PSID          : 2414 (0x096E)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       :
 * DESCRIPTION   :
 *  Simulate setting a value on UWB_WLAN_IND and calling GPIO interrupt to
 *  UWB code FOR TEST PURPOSES ONLY
 *******************************************************************************/
#define SLSI_PSID_UNIFI_COEX_UWB_SIMULATE_UWB_WLAN_IND 0x096E

/*******************************************************************************
 * NAME          : UnifiCoexTxTrafficThresholdPc
 * PSID          : 2415 (0x096F)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 61
 * DESCRIPTION   :
 *  Maximum WLAN transmit duty cycle threshold below which the dedicated BLE
 *  Scan time shall be increased.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_COEX_TX_TRAFFIC_THRESHOLD_PC 0x096F

/*******************************************************************************
 * NAME          : UnifiCoexDebugStats
 * PSID          : 2416 (0x0970)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  Read Coex Debug Stats. Use set to select a debug parameter, then read
 *  this same key to get the actual parameter. 0 Diet mode: BT activity set
 *  count. 1 Diet mode: BT arbitration wins. 2 Diet mode: how many BT
 *  transaction IPCs have been received. 3 Diet mode: BT activity applied
 *  count. 4 full mode: how many BT transactions have been reported as
 *  processed by FM. 5 full mode: valid BT activity count. 6 Diet mode: BT
 *  arbitration draw count. 7 Diet mode: BT arbitration not needed count. 8
 *  Diet mode: how many times the allowed signals have changed uint32_t
 *  full_mode_allowed_change_count;
 *******************************************************************************/
#define SLSI_PSID_UNIFI_COEX_DEBUG_STATS 0x0970

/*******************************************************************************
 * NAME          : UnifiCoexDebugOverrideBt
 * PSID          : 2425 (0x0979)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Enables overriding of all BT activities by WLAN.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_COEX_DEBUG_OVERRIDE_BT 0x0979

/*******************************************************************************
 * NAME          : UnifiLteNrBand79InformRadioFw
 * PSID          : 2429 (0x097D)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  "Cellular Coex: When WLAN Coex is informed by Cellular that NR band 79 is
 *  enabled or disabled, inform Radio FW to take any required action.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LTE_NR_BAND79_INFORM_RADIO_FW 0x097D

/*******************************************************************************
 * NAME          : UnifiLteMailbox
 * PSID          : 2430 (0x097E)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 68
 * MAX           : 68
 * DEFAULT       :
 * DESCRIPTION   :
 *  Set modem status to simulate lte status updates. See SC-508826-SP for API
 *  description. Defined as array of uint32 represented by the octet string
 *  FOR TEST PURPOSES ONLY
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LTE_MAILBOX 0x097E

/*******************************************************************************
 * NAME          : UnifiLteMwsSignal
 * PSID          : 2431 (0x097F)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  Set modem status to simulate lte status updates. See SC-508826-SP for API
 *  description. See unifiLteSignalsBitField for enum bitmap. Also re-read
 *  MIBs for channel avoidance. FOR TEST PURPOSES ONLY
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LTE_MWS_SIGNAL 0x097F

/*******************************************************************************
 * NAME          : UnifiLteEnableChannelAvoidance
 * PSID          : 2432 (0x0980)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Enables channel avoidance scheme for LTE Coex
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LTE_ENABLE_CHANNEL_AVOIDANCE 0x0980

/*******************************************************************************
 * NAME          : UnifiLteEnablePowerBackoff
 * PSID          : 2433 (0x0981)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Enables power backoff scheme for LTE Coex
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LTE_ENABLE_POWER_BACKOFF 0x0981

/*******************************************************************************
 * NAME          : UnifiLteEnableTimeDomain
 * PSID          : 2434 (0x0982)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Enables TDD scheme for LTE Coex
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LTE_ENABLE_TIME_DOMAIN 0x0982

/*******************************************************************************
 * NAME          : UnifiLteEnableLteCoex
 * PSID          : 2435 (0x0983)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Enables LTE Coex support
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LTE_ENABLE_LTE_COEX 0x0983

/*******************************************************************************
 * NAME          : UnifiLteBand40PowerBackoffChannels
 * PSID          : 2436 (0x0984)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 2
 * MAX           : 2
 * DEFAULT       : { 0X01, 0X02 }
 * DESCRIPTION   :
 *  Defines channels to which power backoff shall be applied when LTE
 *  operating on Band40.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LTE_BAND40_POWER_BACKOFF_CHANNELS 0x0984

/*******************************************************************************
 * NAME          : UnifiLteBand40PowerBackoffRsrpLow
 * PSID          : 2437 (0x0985)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt16
 * UNITS         : dBm
 * MIN           : -140
 * MAX           : -77
 * DEFAULT       : -100
 * DESCRIPTION   :
 *  WLAN Power Reduction shall be applied when RSRP of LTE operating on band
 *  40 falls below this level
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LTE_BAND40_POWER_BACKOFF_RSRP_LOW 0x0985

/*******************************************************************************
 * NAME          : UnifiLteBand40PowerBackoffRsrpHigh
 * PSID          : 2438 (0x0986)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt16
 * UNITS         : dBm
 * MIN           : -140
 * MAX           : -77
 * DEFAULT       : -95
 * DESCRIPTION   :
 *  WLAN Power Reduction shall be restored when RSRP of LTE operating on band
 *  40 climbs above this level
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LTE_BAND40_POWER_BACKOFF_RSRP_HIGH 0x0986

/*******************************************************************************
 * NAME          : UnifiLteBand40PowerBackoffRsrpAveragingAlpha
 * PSID          : 2439 (0x0987)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * UNITS         : percentage
 * MIN           : 0
 * MAX           : 255
 * DEFAULT       : 50
 * DESCRIPTION   :
 *  Weighting applied when calculaing the average RSRP when considering Power
 *  Back Off Specifies the percentage weighting (alpha) to give to the most
 *  recent value when calculating the moving average. ma_new = alpha *
 *  new_sample + (1-alpha) * ma_old.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LTE_BAND40_POWER_BACKOFF_RSRP_AVERAGING_ALPHA 0x0987

/*******************************************************************************
 * NAME          : UnifiLteSetChannel
 * PSID          : 2440 (0x0988)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  Enables LTE Coex support
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LTE_SET_CHANNEL 0x0988

/*******************************************************************************
 * NAME          : UnifiLteSetPowerBackoff
 * PSID          : 2441 (0x0989)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  MIB to force WLAN Power Backoff for LTE COEX testing purposes
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LTE_SET_POWER_BACKOFF 0x0989

/*******************************************************************************
 * NAME          : UnifiLteSetTddDebugMode
 * PSID          : 2442 (0x098A)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  MIB to enable LTE TDD COEX simulation for testing purposes
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LTE_SET_TDD_DEBUG_MODE 0x098A

/*******************************************************************************
 * NAME          : UnifiLteBand40AvoidChannels
 * PSID          : 2443 (0x098B)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 2
 * MAX           : 2
 * DEFAULT       : { 0X01, 0X0A }
 * DESCRIPTION   :
 *  MIB to define WLAN channels to avoid when LTE Band 40 in use.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LTE_BAND40_AVOID_CHANNELS 0x098B

/*******************************************************************************
 * NAME          : UnifiLteBand41AvoidChannels
 * PSID          : 2444 (0x098C)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 2
 * MAX           : 2
 * DEFAULT       : { 0X04, 0X0E }
 * DESCRIPTION   :
 *  MIB to define WLAN channels to avoid when LTE Band 41 in use.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LTE_BAND41_AVOID_CHANNELS 0x098C

/*******************************************************************************
 * NAME          : UnifiLteBand7AvoidChannels
 * PSID          : 2445 (0x098D)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 2
 * MAX           : 2
 * DEFAULT       : { 0X09, 0X0E }
 * DESCRIPTION   :
 *  MIB to define WLAN channels to avoid when LTE Band 7 in use.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LTE_BAND7_AVOID_CHANNELS 0x098D

/*******************************************************************************
 * NAME          : UnifiLteBand40FreqThreshold
 * PSID          : 2446 (0x098E)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 2350
 * MAX           : 2400
 * DEFAULT       : 2380
 * DESCRIPTION   :
 *  MIB describing the Band 40 Frequency Threshold
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LTE_BAND40_FREQ_THRESHOLD 0x098E

/*******************************************************************************
 * NAME          : UnifiLteBand41FreqThreshold
 * PSID          : 2447 (0x098F)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 2496
 * MAX           : 2546
 * DEFAULT       : 2500
 * DESCRIPTION   :
 *  MIB describing the Band 41 Frequency Threshold
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LTE_BAND41_FREQ_THRESHOLD 0x098F

/*******************************************************************************
 * NAME          : UnifiLteBand79FreqThreshold
 * PSID          : 2448 (0x0990)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 4950
 * MAX           : 5000
 * DEFAULT       : 5000
 * DESCRIPTION   :
 *  MIB describing the Band 79 Frequency Threshold
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LTE_BAND79_FREQ_THRESHOLD 0x0990

/*******************************************************************************
 * NAME          : UnifiLteBand41PowerBackoffChannels
 * PSID          : 2449 (0x0991)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 2
 * MAX           : 2
 * DEFAULT       : { 0X0C, 0X0D }
 * DESCRIPTION   :
 *  Defines channels to which power backoff shall be applied when LTE
 *  operating on Band41.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LTE_BAND41_POWER_BACKOFF_CHANNELS 0x0991

/*******************************************************************************
 * NAME          : UnifiLteMultiBandAvoidChannelsFallback
 * PSID          : 2450 (0x0992)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 2
 * MAX           : 2
 * DEFAULT       : { 0X06, 0X06 }
 * DESCRIPTION   :
 *  Fallback allowed channels when LTE/NR Band (N)40 and (N)41/(N)7/(N)90 in
 *  use simultaneously.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LTE_MULTI_BAND_AVOID_CHANNELS_FALLBACK 0x0992

/*******************************************************************************
 * NAME          : UnifiLteBand79Enable
 * PSID          : 2451 (0x0993)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Enable cellular coexistence schemes for band (n)79
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LTE_BAND79_ENABLE 0x0993

/*******************************************************************************
 * NAME          : UnifiLteBand79PowerBackoffChannels
 * PSID          : 2452 (0x0994)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 2
 * MAX           : 2
 * DEFAULT       : { 0X00, 0X00 }
 * DESCRIPTION   :
 *  Defines channels to which power backoff shall be applied when NR
 *  operating on Band n79.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LTE_BAND79_POWER_BACKOFF_CHANNELS 0x0994

/*******************************************************************************
 * NAME          : UnifiLteBand79AvoidChannels
 * PSID          : 2453 (0x0995)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 2
 * MAX           : 2
 * DEFAULT       : { 0X00, 0X00 }
 * DESCRIPTION   :
 *  MIB to define WLAN channels to avoid when NR Band 79 in use.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LTE_BAND79_AVOID_CHANNELS 0x0995

/*******************************************************************************
 * NAME          : UnifiLeakyApConfig
 * PSID          : 2454 (0x0996)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 0X0009
 * DESCRIPTION   :
 *  Bitmap to configure leaky AP behavior bit 0: Set dont send NULL with
 *  CTS2SELF when bad AP is detected (deprecated) bit 1: Force all connected
 *  STA VIFs to behave as if they are connected to a BAD AP bit 2: Enable
 *  Retraining when AP is classified as GOOD bit 3: Enable Good Behavour
 *  threshold adjustment when throughput is high bit 4: Force all connected
 *  STA VIFs to behave as if they are connected to a GOOD AP bit 5: Force all
 *  connected STA VIFs to behave as if they are connected to a LEAKY AP
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LEAKY_AP_CONFIG 0x0996

/*******************************************************************************
 * NAME          : UnifiUseCtsForLeakyAp
 * PSID          : 2455 (0x0997)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Enable or disable the use of CTS-to-self frames by STA connected to AP
 *  that leaks frames after PM=1. If enabled, coex will also modify BT
 *  behaviour to ensure BT opportunities are short enough to be protected by
 *  CTS-To-Self. See also unifiEnableCoexLowLatency.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_USE_CTS_FOR_LEAKY_AP 0x0997

/*******************************************************************************
 * NAME          : UnifiUspboRxAggregateBufferSize
 * PSID          : 2456 (0x0998)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 4
 * DESCRIPTION   :
 *  Control BA RX agreements and Restrict the Rx BA Buffer size during
 *  non-LTE USPBO. 0 = Rx BA agreements will be suppressed during USPBO. 1 -
 *  256 = Rx BA agreements will be preserved during USPBO, buffer size
 *  limited to this value. 0xffff = Rx BA agreements will be preserved during
 *  USPBO, buffer size limited to default value
 *******************************************************************************/
#define SLSI_PSID_UNIFI_USPBO_RX_AGGREGATE_BUFFER_SIZE 0x0998

/*******************************************************************************
 * NAME          : UnifiAntennaSelectionOption
 * PSID          : 2459 (0x099B)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  Select how antennas will be allocated for SISO/RSDB/RSTB scenarios. Only
 *  valid for multi-radio platforms. WARNING: Changing this value after
 *  system start-up will have no effect. If the value is set to 0, antenna0
 *  is used for 2G4 and antenna1 is used for 5G or 6G. If the value is set to
 *  1, antenna1 is used for 2G4 and antenna0 is used for 5G or 6G. If the
 *  value is set to 2, the antenna with the strongest RSSI average will be
 *  preferred over the other antenna. Also see unifiAntennaSelection. Please
 *  refer to SC-509923-DD document for the detail.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ANTENNA_SELECTION_OPTION 0x099B

/*******************************************************************************
 * NAME          : UnifiSupportMaxRequiredAntenna
 * PSID          : 2460 (0x099C)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Setting this MIB key to 'false' will use the number of antennas based on
 *  the peer device's advertised Nss. Setting this MIB key to 'true' will use
 *  the number of antennas based on the peers's max required antennas. An
 *  example where this is useful is when a peer advertises support for 1ss
 *  and STBC, thus 2 antennas must be used. For multi-antenna platforms only.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_SUPPORT_MAX_REQUIRED_ANTENNA 0x099C

/*******************************************************************************
 * NAME          : UnifiBeaconRxPeriodUpdate
 * PSID          : 2465 (0x09A1)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Enables beacon rx period update, if AP is sending wrong beacon period.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_BEACON_RX_PERIOD_UPDATE 0x09A1

/*******************************************************************************
 * NAME          : UnifiSisoSwitchWaitTime
 * PSID          : 2466 (0x09A2)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 20
 * DESCRIPTION   :
 *  The amount of time in beacon intervals to evaluate whether an AP has
 *  responded correctly to a STA VIF attempting to switch from SISO to MIMO
 *  based on a host request. Only used if unifiHostNumAntennaControlActivated
 *  is set to true.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_SISO_SWITCH_WAIT_TIME 0x09A2

/*******************************************************************************
 * NAME          : UnifiMiflessApMinAwakeInterval
 * PSID          : 2467 (0x09A3)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * UNITS         : ms
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       : 100
 * DESCRIPTION   :
 *  For Mifless AP low power mode this is the minimum time period in
 *  milliseconds before VIF re-enters low power mode after exiting due to
 *  DPHP RX
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MIFLESS_AP_MIN_AWAKE_INTERVAL 0x09A3

/*******************************************************************************
 * NAME          : UnifiMacUsagePolicy
 * PSID          : 2468 (0x09A4)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  MAC usage policy table is not HW capability table but the FW selection of
 *  MAC to use
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MAC_USAGE_POLICY 0x09A4

/*******************************************************************************
 * NAME          : UnifiMacPrefsOrder
 * PSID          : 2469 (0x09A5)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 1
 * MAX           : 3
 * DEFAULT       :
 * DESCRIPTION   :
 *  MAC preference order
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MAC_PREFS_ORDER 0x09A5

/*******************************************************************************
 * NAME          : UnifiBlackoutSimulation
 * PSID          : 2470 (0x09A6)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  For Test Purposes, create specific blackouts for single vif attachment
 *  Uses unifiBlackoutSimConfigurationType to define BO type and VIF Type
 *  Currently allows only single BO type to be applied at a time vifs should
 *  be schedulable before the mib is changed. All combinations are allowed,
 *  however some will be not sensible (etc attach NDL blackout to a STA) 0 =
 *  Turn off all test blackouts. For BO Types 0x0001 = A2DP One shot blackout
 *  sent periodically 0x0002 = ESCO Blackout 0x0004 = NDL availability
 *  reduction, one periodic 32ms blackout, 50% duty cycle For VIF Types
 *  0x0400 = STA Vif 0x0800 = AP Vif 0x1000 = NDL Vif 0x2000 = SCAN Vif For
 *  Announcement Forcing (If neither bits are set CP will decide) 0x4000 =
 *  Force Puncturing 0x8000 = Force Announcement Clearing the value should
 *  stop the test.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_BLACKOUT_SIMULATION 0x09A6

/*******************************************************************************
 * NAME          : UnifiBlackoutForceTxOnly
 * PSID          : 2471 (0x09A7)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 3
 * DESCRIPTION   :
 *  Setting individual bits will make any blackout requests of that type into
 *  TX only blackouts. Only applicable to coex blackouts. 0x00 = Do not force
 *  any blackouts to become TX only 0x01 = Force coex single shot blackouts
 *  to become TX only 0x02 = Force coex non-uspbo periodic blackouts to
 *  become TX only
 *******************************************************************************/
#define SLSI_PSID_UNIFI_BLACKOUT_FORCE_TX_ONLY 0x09A7

/*******************************************************************************
 * NAME          : UnifiSaeDwellTime
 * PSID          : 2472 (0x09A8)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * UNITS         : TimeUnits
 * MIN           : 1
 * MAX           : 65535
 * DEFAULT       : 976
 * DESCRIPTION   :
 *  Dwell time for SAE Auth frames.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_SAE_DWELL_TIME 0x09A8

/*******************************************************************************
 * NAME          : UnifiAplpControl
 * PSID          : 2473 (0x09A9)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       : 0X00000009
 * DESCRIPTION   :
 *  Control AP Low Power behaviour, controlled as a bit mask. Refer to
 *  unifiAPLPControlBits for the full set of bit masks. b'0: Enable AP Idle
 *  mode b'1: Use one antenna when all peers are idle b'2: Enable Mifless v1
 *  b'3: Suppress Deep Sleep if AP vif exists
 *******************************************************************************/
#define SLSI_PSID_UNIFI_APLP_CONTROL 0x09A9

/*******************************************************************************
 * NAME          : UnifiUseHostListenInterval
 * PSID          : 2476 (0x09AC)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * UNITS         : DTIM intervals
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  Listen interval of beacons when in single-vif as set by host.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_USE_HOST_LISTEN_INTERVAL 0x09AC

/*******************************************************************************
 * NAME          : UnifiDynamicItoControl
 * PSID          : 2478 (0x09AE)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 0X000E
 * DESCRIPTION   :
 *  Control DynamicITO algorithm with bit flags. 1 defines enabled,with 0
 *  showing the case disabled. bit 0: Dynamic ITO based on Beacon RX missed
 *  condition or struggling to receive beacons bit 1: Dynamic ITO based on
 *  Traffic condition (continuous or occasional) bit 2: Dynamic ITO based on
 *  Channel condition (Free/busy) bit 3: Dynamic ITO based on how fast AP
 *  responding to data
 *******************************************************************************/
#define SLSI_PSID_UNIFI_DYNAMIC_ITO_CONTROL 0x09AE

/*******************************************************************************
 * NAME          : UnifiMaxBeaconListenWindowTime
 * PSID          : 2479 (0x09AF)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * UNITS         : microseconds
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  The maximum beacon listen window time we can limit on.This is the
 *  difference between listen end and listen start time.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MAX_BEACON_LISTEN_WINDOW_TIME 0x09AF

/*******************************************************************************
 * NAME          : UnifiApScanAbsenceDuration
 * PSID          : 2480 (0x09B0)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * UNITS         : beacon intervals
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 2
 * DESCRIPTION   :
 *  Duration of the Absence time to use when protecting AP VIFs from scan
 *  operations. A value of 0 disables the feature.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_AP_SCAN_ABSENCE_DURATION 0x09B0

/*******************************************************************************
 * NAME          : UnifiApScanAbsencePeriod
 * PSID          : 2481 (0x09B1)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * UNITS         : beacon intervals
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 4
 * DESCRIPTION   :
 *  Period of the Absence/Presence times cycles to use when protecting AP
 *  VIFs from scan operations.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_AP_SCAN_ABSENCE_PERIOD 0x09B1

/*******************************************************************************
 * NAME          : UnifiMlmestaKeepAliveTimeoutCheck
 * PSID          : 2485 (0x09B5)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 100
 * DEFAULT       : 5
 * DESCRIPTION   :
 *  DO NOT SET TO A VALUE HIGHER THAN THE TIMEOUT. How long before keepalive
 *  timeout to start polling, in seconds. This value is read only once when
 *  an interface is added.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MLMESTA_KEEP_ALIVE_TIMEOUT_CHECK 0x09B5

/*******************************************************************************
 * NAME          : UnifiMlmeapKeepAliveTimeoutCheck
 * PSID          : 2486 (0x09B6)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 100
 * DEFAULT       : 5
 * DESCRIPTION   :
 *  DO NOT SET TO A VALUE HIGHER THAN THE TIMEOUT. How long before keepalive
 *  timeout to start polling, in seconds. This value is read only once when
 *  an interface is added.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MLMEAP_KEEP_ALIVE_TIMEOUT_CHECK 0x09B6

/*******************************************************************************
 * NAME          : UnifiMlmegoKeepAliveTimeoutCheck
 * PSID          : 2487 (0x09B7)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 100
 * DEFAULT       : 5
 * DESCRIPTION   :
 *  DO NOT SET TO A VALUE HIGHER THAN THE TIMEOUT. How long before keepalive
 *  timeout to start polling, in seconds. This value is read only once when
 *  an interface is added.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MLMEGO_KEEP_ALIVE_TIMEOUT_CHECK 0x09B7

/*******************************************************************************
 * NAME          : UnifiBssMaxIdlePeriod
 * PSID          : 2488 (0x09B8)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * UNITS         : second
 * MIN           : 0
 * MAX           : 300
 * DEFAULT       : 300
 * DESCRIPTION   :
 *  BSS Idle MAX Period. Used to cap the value coming from BSS Max Idle
 *  Period IE, in seconds
 *******************************************************************************/
#define SLSI_PSID_UNIFI_BSS_MAX_IDLE_PERIOD 0x09B8

/*******************************************************************************
 * NAME          : UnifiIgmpOffloadActivated
 * PSID          : 2489 (0x09B9)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Activate IGMP IPv4 Offloading.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_IGMP_OFFLOAD_ACTIVATED 0x09B9

/*******************************************************************************
 * NAME          : UnifiGoctWindowDelay
 * PSID          : 2492 (0x09BC)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * UNITS         : seconds
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  Delay to apply Client Traffic Window in seconds for oppertunistic power
 *  save in P2PGO.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_GOCT_WINDOW_DELAY 0x09BC

/*******************************************************************************
 * NAME          : UnifiFastPowerSaveTimeoutAggressive
 * PSID          : 2494 (0x09BE)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * UNITS         : microseconds
 * MIN           : 0
 * MAX           : 2147483647
 * DEFAULT       :
 * DESCRIPTION   :
 *  UniFi implements a proprietary power management mode called Fast Power
 *  Save that balances network performance against power consumption. In this
 *  mode UniFi delays entering power save mode until it detects that there
 *  has been no exchange of data for the duration of time specified. The
 *  unifiFastPowerSaveTimeOutAggressive aims to improve the power consumption
 *  by setting a aggressive time when channel is not busy for the Fast Power
 *  Save Timeout. If set with a value above unifiFastPowerSaveTimeOut it will
 *  default to unifiFastPowerSaveTimeOut. Setting it to zero disables the
 *  feature
 *******************************************************************************/
#define SLSI_PSID_UNIFI_FAST_POWER_SAVE_TIMEOUT_AGGRESSIVE 0x09BE

/*******************************************************************************
 * NAME          : UnifiFastPowerSaveTimeout
 * PSID          : 2500 (0x09C4)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * UNITS         : microseconds
 * MIN           : 0
 * MAX           : 2147483647
 * DEFAULT       : 200000
 * DESCRIPTION   :
 *  Golden Certification MIB don't delete, change PSID or name: UniFi
 *  implements a proprietary power management mode called Fast Power Save
 *  that balances network performance against power consumption. In this mode
 *  UniFi delays entering power save mode until it detects that there has
 *  been no exchange of data for the duration of time specified.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_FAST_POWER_SAVE_TIMEOUT 0x09C4

/*******************************************************************************
 * NAME          : UnifiFastPowerSaveTimeoutSmall
 * PSID          : 2501 (0x09C5)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * UNITS         : microseconds
 * MIN           : 0
 * MAX           : 2147483647
 * DEFAULT       : 50000
 * DESCRIPTION   :
 *  UniFi implements a proprietary power management mode called Fast Power
 *  Save that balances network performance against power consumption. In this
 *  mode UniFi delays entering power save mode until it detects that there
 *  has been no exchange of data for the duration of time specified. The
 *  unifiFastPowerSaveTimeOutSmall aims to improve the power consumption by
 *  setting a lower bound for the Fast Power Save Timeout. If set with a
 *  value above unifiFastPowerSaveTimeOut it will default to
 *  unifiFastPowerSaveTimeOut.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_FAST_POWER_SAVE_TIMEOUT_SMALL 0x09C5

/*******************************************************************************
 * NAME          : UnifiMlmestaKeepAliveTimeout
 * PSID          : 2502 (0x09C6)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 2147
 * DEFAULT       : 30
 * DESCRIPTION   :
 *  Golden Certification MIB don't delete, change PSID or name: Timeout
 *  before disconnecting in seconds. 0 = Disabled. Capped to greater than 6
 *  seconds. This value is read only once when an interface is added.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MLMESTA_KEEP_ALIVE_TIMEOUT 0x09C6

/*******************************************************************************
 * NAME          : UnifiMlmeapKeepAliveTimeout
 * PSID          : 2503 (0x09C7)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 2147
 * DEFAULT       : 10
 * DESCRIPTION   :
 *  Timeout before disconnecting in seconds. 0 = Disabled. Capped to greater
 *  than 6 seconds. This value is read only once when an interface is added.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MLMEAP_KEEP_ALIVE_TIMEOUT 0x09C7

/*******************************************************************************
 * NAME          : UnifiMlmegoKeepAliveTimeout
 * PSID          : 2504 (0x09C8)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 2147
 * DEFAULT       : 10
 * DESCRIPTION   :
 *  Timeout before disconnecting in seconds. 0 = Disabled. Capped to greater
 *  than 6 seconds. This value is read only once when an interface is added.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MLMEGO_KEEP_ALIVE_TIMEOUT 0x09C8

/*******************************************************************************
 * NAME          : UnifiStaRouterAdvertisementMinimumIntervalToForward
 * PSID          : 2505 (0x09C9)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 60
 * MAX           : 4294967285
 * DEFAULT       : 60
 * DESCRIPTION   :
 *  STA Mode: Minimum interval to forward Router Advertisement frames to
 *  Host. Minimum value = 60 secs.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_STA_ROUTER_ADVERTISEMENT_MINIMUM_INTERVAL_TO_FORWARD 0x09C9

/*******************************************************************************
 * NAME          : UnifiRoamConnectionQualityCheckWaitAfterConnect
 * PSID          : 2506 (0x09CA)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * UNITS         : ms
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 200
 * DESCRIPTION   :
 *  The amount of time a STA will wait after connection before starting to
 *  check the MLME-installed connection quality trigger thresholds
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_CONNECTION_QUALITY_CHECK_WAIT_AFTER_CONNECT 0x09CA

/*******************************************************************************
 * NAME          : UnifiApBeaconMaxDrift
 * PSID          : 2507 (0x09CB)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * UNITS         : microseconds
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 0XFFFF
 * DESCRIPTION   :
 *  The maximum drift in microseconds we will allow for each beacon sent when
 *  we're trying to move it to get a 50% duty cycle between GO and STA in
 *  multiple VIF scenario. We'll delay our TX beacon by a maximum of this
 *  value until we reach our target TBTT. We have 3 possible cases for this
 *  value: a) ap_beacon_max_drift = 0x0000 - Feature disabled b)
 *  ap_beacon_max_drift between 0x0001 and 0xFFFE - Each time we transmit the
 *  beacon we'll move it a little bit forward but never more than this. (Not
 *  implemented yet) c) ap_beacon_max_drift = 0xFFFF - Move the beacon to the
 *  desired position in one shot.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_AP_BEACON_MAX_DRIFT 0x09CB

/*******************************************************************************
 * NAME          : UnifiBssMaxIdlePeriodActivated
 * PSID          : 2508 (0x09CC)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  If set STA will configure keep-alive with options specified in a received
 *  BSS max idle period IE
 *******************************************************************************/
#define SLSI_PSID_UNIFI_BSS_MAX_IDLE_PERIOD_ACTIVATED 0x09CC

/*******************************************************************************
 * NAME          : UnifiVifIdleMonitorTime
 * PSID          : 2509 (0x09CD)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * UNITS         : second
 * MIN           : 0
 * MAX           : 1800
 * DEFAULT       : 1
 * DESCRIPTION   :
 *  In Fast Power Save mode, the STA will decide whether it is idle based on
 *  monitoring its traffic class. If the traffic class is continuously
 *  "occasional" for equal or longer than the specified value (in seconds),
 *  then the VIF is marked as idle. Traffic class monitoring is based on the
 *  interval specified in the "unifiTrafficAnalysisPeriod" MIB
 *******************************************************************************/
#define SLSI_PSID_UNIFI_VIF_IDLE_MONITOR_TIME 0x09CD

/*******************************************************************************
 * NAME          : UnifiDisableLegacyPowerSave
 * PSID          : 2510 (0x09CE)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Golden Certification MIB don't delete, change PSID or name: This affects
 *  Station VIF power save behaviour. Setting it to true will disable legacy
 *  power save (i.e. we wil use fast power save to retrieve data) Note that
 *  actually disables full power save mode (i.e sending trigger to retrieve
 *  frames which will be PS-POLL for legacy and QOS-NULL for UAPSD)
 *******************************************************************************/
#define SLSI_PSID_UNIFI_DISABLE_LEGACY_POWER_SAVE 0x09CE

/*******************************************************************************
 * NAME          : UnifiDebugForceActive
 * PSID          : 2511 (0x09CF)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Golden Certification MIB don't delete, change PSID or name: Force station
 *  power save mode to be active (when scheduled). VIF scheduling, coex and
 *  other non-VIF specific reasons could still force power save on the VIF.
 *  Applies to all VIFs of type station (includes P2P client). Changes to the
 *  mib will only get applied after next host/mlme power management request.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_DEBUG_FORCE_ACTIVE 0x09CF

/*******************************************************************************
 * NAME          : UnifiStationActivityIdleTime
 * PSID          : 2512 (0x09D0)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * UNITS         : milliseconds
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       : 500
 * DESCRIPTION   :
 *  Time since last station activity when it can be considered to be idle.
 *  Only used in SoftAP mode when determining if all connected stations are
 *  idle (not active).
 *******************************************************************************/
#define SLSI_PSID_UNIFI_STATION_ACTIVITY_IDLE_TIME 0x09D0

/*******************************************************************************
 * NAME          : UnifiDmsActivated
 * PSID          : 2513 (0x09D1)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Activate Directed Multicast Service (DMS)
 *******************************************************************************/
#define SLSI_PSID_UNIFI_DMS_ACTIVATED 0x09D1

/*******************************************************************************
 * NAME          : UnifiPowerManagementDelayTimeout
 * PSID          : 2514 (0x09D2)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * UNITS         : microseconds
 * MIN           : 0
 * MAX           : 2147483647
 * DEFAULT       : 15000
 * DESCRIPTION   :
 *  When UniFi enters power save mode it signals the new state by setting the
 *  power management bit in the frame control field of a NULL frame. It then
 *  remains active for the period since the previous unicast reception, or
 *  since the transmission of the NULL frame, whichever is later. This entry
 *  controls the maximum time during which UniFi will continue to listen for
 *  data. This allows any buffered data on a remote device to be cleared.
 *  Specifies an upper limit on the timeout. UniFi internally implements a
 *  proprietary algorithm to adapt the timeout depending upon the
 *  situation.This is used by firmware when current station VIF is only
 *  station VIF which can be scheduled
 *******************************************************************************/
#define SLSI_PSID_UNIFI_POWER_MANAGEMENT_DELAY_TIMEOUT 0x09D2

/*******************************************************************************
 * NAME          : UnifiApsdServicePeriodTimeout
 * PSID          : 2515 (0x09D3)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * UNITS         : microseconds
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 20000
 * DESCRIPTION   :
 *  During Unscheduled Automated Power Save Delivery (U-APSD), UniFi may
 *  trigger a service period in order to fetch data from the access point.
 *  The service period is normally terminated by a frame from the access
 *  point with the EOSP (End Of Service Period) flag set, at which point
 *  UniFi returns to sleep. However, if the access point is temporarily
 *  inaccessible, UniFi would stay awake indefinitely. Specifies a timeout
 *  starting from the point where the trigger frame has been sent. If the
 *  timeout expires and no data has been received from the access point,
 *  UniFi will behave as if the service period had been ended normally and
 *  return to sleep. This timeout takes precedence over
 *  unifiPowerSaveExtraListenTime if both would otherwise be applicable.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_APSD_SERVICE_PERIOD_TIMEOUT 0x09D3

/*******************************************************************************
 * NAME          : UnifiConcurrentPowerManagementDelayTimeout
 * PSID          : 2516 (0x09D4)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * UNITS         : microseconds
 * MIN           : 0
 * MAX           : 2147483647
 * DEFAULT       : 5000
 * DESCRIPTION   :
 *  When UniFi enters power save mode it signals the new state by setting the
 *  power management bit in the frame control field of a NULL frame. It then
 *  remains active for the period since the previous unicast reception, or
 *  since the transmission of the NULL frame, whichever is later. This entry
 *  controls the maximum time during which UniFi will continue to listen for
 *  data. This allows any buffered data on a remote device to be cleared.
 *  This is same as unifiPowerManagementDelayTimeout but this value is
 *  considered only when we are doing multivif operations and other VIFs are
 *  waiting to be scheduled.Note that firmware automatically chooses one of
 *  unifiPowerManagementDelayTimeout and
 *  unifiConcurrentPowerManagementDelayTimeout depending upon the current
 *  situation.It is sensible to set unifiPowerManagementDelayTimeout to be
 *  always more thanunifiConcurrentPowerManagementDelayTimeout.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_CONCURRENT_POWER_MANAGEMENT_DELAY_TIMEOUT 0x09D4

/*******************************************************************************
 * NAME          : UnifiStationQosInfo
 * PSID          : 2517 (0x09D5)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 255
 * DEFAULT       :
 * DESCRIPTION   :
 *  QoS capability for a non-AP Station, and is encoded as per IEEE 802.11
 *  QoS Capability.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_STATION_QOS_INFO 0x09D5

/*******************************************************************************
 * NAME          : UnifiListenIntervalSkippingDtim
 * PSID          : 2518 (0x09D6)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * UNITS         : DTIM intervals
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       : 0X000A89AA
 * DESCRIPTION   :
 *  Listen interval of beacons when in single-vif power saving mode,receiving
 *  DTIMs is enabled and idle mode disabled. No DTIMs are skipped during MVIF
 *  operation. A maximum of the listen interval beacons are skipped, which
 *  may be less than the number of DTIMs that can be skipped. The value is a
 *  lookup table for DTIM counts. Each 4bits, in LSB order, represent DTIM1,
 *  DTIM2, DTIM3, DTIM4, DTIM5, (unused). This key is only used for STA VIF,
 *  connected to an AP. For P2P group client intervals, refer to
 *  unifiP2PListenIntervalSkippingDTIM, PSID=2523.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LISTEN_INTERVAL_SKIPPING_DTIM 0x09D6

/*******************************************************************************
 * NAME          : UnifiListenInterval
 * PSID          : 2519 (0x09D7)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt16
 * MIN           : 0
 * MAX           : 100
 * DEFAULT       : 10
 * DESCRIPTION   :
 *  Association request listen interval parameter in beacon intervals. Not
 *  used for any other purpose.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LISTEN_INTERVAL 0x09D7

/*******************************************************************************
 * NAME          : UnifiLegacyPsPollTimeout
 * PSID          : 2520 (0x09D8)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * UNITS         : microseconds
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 15000
 * DESCRIPTION   :
 *  Time we try to stay awake after sending a PS-POLL to receive data.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LEGACY_PS_POLL_TIMEOUT 0x09D8

/*******************************************************************************
 * NAME          : UnifiBeaconSkippingControl
 * PSID          : 2521 (0x09D9)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       : 0X00010101
 * DESCRIPTION   :
 *  Control beacon skipping behaviour within firmware with bit flags. 1
 *  defines enabled, with 0 showing the case disabled. If beacon skipping is
 *  enabled, further determine if DTIM beacons can be skipped, or only
 *  non-DTIM beacons. The following applies: bit 0: station skipping on host
 *  suspend bit 1: station skipping on host awake bit 2: station skipping on
 *  LCD on bit 3: station skipping with multivif bit 4: station skipping with
 *  BT active. bit 8: station skip dtim on host suspend bit 9: station skip
 *  dtim on host awake bit 10: station skip dtim on LCD on bit 11: station
 *  skip dtim on multivif bit 12: station skip dtim with BT active bit 16:
 *  p2p-gc skipping on host suspend bit 17: p2p-gc skipping on host awake bit
 *  18: p2p-gc skipping on LCD on bit 19: p2p-gc skipping with multivif bit
 *  20: p2p-gc skipping with BT active bit 24: p2p-gc skip dtim on host
 *  suspend bit 25: p2p-gc skip dtim on host awake bit 26: p2p-gc skip dtim
 *  on LCD on bit 27: p2p-gc skip dtim on multivif bit 28: p2p-gc skip dtim
 *  with BT active
 *******************************************************************************/
#define SLSI_PSID_UNIFI_BEACON_SKIPPING_CONTROL 0x09D9

/*******************************************************************************
 * NAME          : UnifiTogglePowerDomain
 * PSID          : 2522 (0x09DA)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Toggle WLAN power domain when entering dorm mode (deep sleep). When
 *  entering deep sleep and this value it true, then the WLAN power domain is
 *  disabled for the deep sleep duration. When false, the power domain is
 *  left turned on. This is to work around issues with WLAN rx, and is
 *  considered temporary until the root cause is found and fixed.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TOGGLE_POWER_DOMAIN 0x09DA

/*******************************************************************************
 * NAME          : UnifiP2PListenIntervalSkippingDtim
 * PSID          : 2523 (0x09DB)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * UNITS         : DTIM intervals
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       : 0X00000002
 * DESCRIPTION   :
 *  Listen interval of beacons when in single-vif, P2P client power saving
 *  mode,receiving DTIMs and idle mode disabled. No DTIMs are skipped during
 *  MVIF operation. A maximum of (listen interval - 1) beacons are skipped,
 *  which may be less than the number of DTIMs that can be skipped. The value
 *  is a lookup table for DTIM counts. Each 4bits, in LSB order, represent
 *  DTIM1, DTIM2, DTIM3, DTIM4, DTIM5, (unused). This key is only used for
 *  P2P group client. For STA connected to an AP, refer to
 *  unifiListenIntervalSkippingDTIM, PSID=2518.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_P2_PLISTEN_INTERVAL_SKIPPING_DTIM 0x09DB

/*******************************************************************************
 * NAME          : UnifiFragmentationDuration
 * PSID          : 2524 (0x09DC)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * UNITS         : microseconds
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  A limit on transmission time for a data frame. If the data payload would
 *  take longer than unifiFragmentationDuration to transmit, UniFi will
 *  attempt to fragment the frame to ensure that the data portion of each
 *  fragment is within the limit. The limit imposed by the fragmentation
 *  threshold is also respected, and no more than 16 fragments may be
 *  generated. If the value is zero no limit is imposed. The value may be
 *  changed dynamically during connections. Note that the limit is a
 *  guideline and may not always be respected. In particular, the data rate
 *  is finalised after fragmentation in order to ensure responsiveness to
 *  conditions, the calculation is not performed to high accuracy, and octets
 *  added during encryption are not included in the duration calculation.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_FRAGMENTATION_DURATION 0x09DC

/*******************************************************************************
 * NAME          : UnifiRoamSendInitFrameToGetNeighbors
 * PSID          : 2525 (0x09DD)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Golden Certification MIB don't delete, change PSID or name: The firmware
 *  can send a Neighbor request/ BTM query to get AP's neighbors. This frame
 *  upsets certification setups. Setting this MIB to false inhibits sending a
 *  Neighbor request/ BTM query to get AP's neighbors.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_SEND_INIT_FRAME_TO_GET_NEIGHBORS 0x09DD

/*******************************************************************************
 * NAME          : UnifiPowerManagementDelayTimeoutLeaky
 * PSID          : 2528 (0x09E0)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * UNITS         : microseconds
 * MIN           : 0
 * MAX           : 2147483647
 * DEFAULT       : 5000
 * DESCRIPTION   :
 *  When UniFi enters power save mode it signals the new state by setting the
 *  power management bit in the frame control field of a NULL frame. It then
 *  remains active for the period since the previous unicast reception, or
 *  since the transmission of the NULL frame, whichever is later. This entry
 *  controls the maximum time during which UniFi will continue to listen for
 *  data. This allows any buffered data on a remote device to be cleared.
 *  Specifies an upper limit on the timeout. UniFi internally implements a
 *  proprietary algorithm to adapt the timeout depending upon the
 *  situation.This is used by firmware when current station VIF is only
 *  station VIF which can be scheduled, but this case is specifically for a
 *  STA when its connected AP has been detected as being a Leaky AP. In that
 *  case this value will be used instead of the usual
 *  unifiPowerManagementDelayTimeout
 *******************************************************************************/
#define SLSI_PSID_UNIFI_POWER_MANAGEMENT_DELAY_TIMEOUT_LEAKY 0x09E0

/*******************************************************************************
 * NAME          : UnifiDtimWaitTimeout
 * PSID          : 2529 (0x09E1)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * UNITS         : microseconds
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 50000
 * DESCRIPTION   :
 *  If UniFi is in power save and receives a Traffic Indication Map from its
 *  associated access point with a DTIM indication, it will wait a maximum
 *  time given by this attribute for succeeding broadcast or multicast
 *  traffic, or until it receives such traffic with the &apos;more data&apos;
 *  flag clear. Any reception of broadcast or multicast traffic with the
 *  &apos;more data&apos; flag set, or any reception of unicast data, resets
 *  the timeout. The timeout can be turned off by setting the value to zero;
 *  in that case UniFi will remain awake indefinitely waiting for broadcast
 *  or multicast data. Otherwise, the value should be larger than that of
 *  unifiPowerSaveExtraListenTime.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_DTIM_WAIT_TIMEOUT 0x09E1

/*******************************************************************************
 * NAME          : UnifiListenIntervalMaxTime
 * PSID          : 2530 (0x09E2)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * UNITS         : TU
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 1000
 * DESCRIPTION   :
 *  Maximum number length of time, in Time Units (1TU = 1024us), that can be
 *  used as a beacon listen interval. This will limit how many beacons maybe
 *  skipped, and affects the DTIM beacon skipping count; DTIM skipping (if
 *  enabled) will be such that skipped count = (unifiListenIntervalMaxTime /
 *  DTIM_period).
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LISTEN_INTERVAL_MAX_TIME 0x09E2

/*******************************************************************************
 * NAME          : UnifiScanMaxProbeTransmitLifetime
 * PSID          : 2531 (0x09E3)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 1
 * MAX           : 4294967295
 * DEFAULT       : 64
 * DESCRIPTION   :
 *  In TU. If non-zero, used during active scans as the maximum lifetime for
 *  probe requests. It is the elapsed time after the initial transmission at
 *  which further attempts to transmit the probe are terminated.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_SCAN_MAX_PROBE_TRANSMIT_LIFETIME 0x09E3

/*******************************************************************************
 * NAME          : UnifiPowerSaveTransitionPacketThreshold
 * PSID          : 2532 (0x09E4)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 10
 * DESCRIPTION   :
 *  Golden Certification MIB don't delete, change PSID or name:If VIF has
 *  this many packets queued/transmitted/received in last
 *  unifiFastPowerSaveTransitionPeriod then firmware may decide to come out
 *  of aggressive power save mode. This is applicable to STA/CLI and AP/GO
 *  VIFs. Note that this is only a guideline. Firmware internal factors may
 *  override this MIB. Also see unifiTrafficAnalysisPeriod and
 *  unifiAggressivePowerSaveTransitionPeriod.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_POWER_SAVE_TRANSITION_PACKET_THRESHOLD 0x09E4

/*******************************************************************************
 * NAME          : UnifiProbeResponseLifetime
 * PSID          : 2533 (0x09E5)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 100
 * DESCRIPTION   :
 *  Lifetime of proberesponse frame in unit of ms.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_PROBE_RESPONSE_LIFETIME 0x09E5

/*******************************************************************************
 * NAME          : UnifiProbeResponseMaxRetry
 * PSID          : 2534 (0x09E6)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 255
 * DEFAULT       : 5
 * DESCRIPTION   :
 *  Number of retries of probe response frame.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_PROBE_RESPONSE_MAX_RETRY 0x09E6

/*******************************************************************************
 * NAME          : UnifiTrafficAnalysisPeriod
 * PSID          : 2535 (0x09E7)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * UNITS         : TU
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 80
 * DESCRIPTION   :
 *  Period in TUs over which firmware counts number of packet
 *  transmitted/queued/received to make decisions like coming out of
 *  aggressive power save mode or setting up BlockAck. This is applicable to
 *  STA/CLI and AP/GO VIFs. Note that this is only a guideline. Firmware
 *  internal factors may override this MIB. Also see
 *  unifiPowerSaveTransitionPacketThreshold,
 *  unifiAggressivePowerSaveTransitionPeriod and
 *  unifiTrafficThresholdToSetupBA.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TRAFFIC_ANALYSIS_PERIOD 0x09E7

/*******************************************************************************
 * NAME          : UnifiAggressivePowerSaveTransitionPeriod
 * PSID          : 2536 (0x09E8)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * UNITS         : TU
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 5
 * DESCRIPTION   :
 *  Defines how many unifiTrafficAnalysisPeriod firmware should wait in which
 *  VIF had received/transmitted/queued less than
 *  unifiPowerSaveTransitionPacketThreshold packets - before entering
 *  aggressive power save mode (when not in aggressive power save mode) This
 *  is applicable to STA/CLI and AP/GO VIFs. Note that this is only a
 *  guideline. Firmware internal factors may override this MIB. Also see
 *  unifiPowerSaveTransitionPacketThreshold and unifiTrafficAnalysisPeriod.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_AGGRESSIVE_POWER_SAVE_TRANSITION_PERIOD 0x09E8

/*******************************************************************************
 * NAME          : UnifiActiveTimeAfterMoreBit
 * PSID          : 2537 (0x09E9)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * UNITS         : TU
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       : 30
 * DESCRIPTION   :
 *  Golden Certification MIB don't delete, change PSID or name: After seeing
 *  the "more" bit set in a message from the AP, the STA will goto active
 *  mode for this duration of time. After this time, traffic information is
 *  evaluated to determine whether the STA should stay active or go to
 *  powersave. Setting this value to 0 means that the described functionality
 *  is disabled.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ACTIVE_TIME_AFTER_MORE_BIT 0x09E9

/*******************************************************************************
 * NAME          : UnifiDefaultDwellTime
 * PSID          : 2538 (0x09EA)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * UNITS         : TimeUnits
 * MIN           : 1
 * MAX           : 65535
 * DEFAULT       : 50
 * DESCRIPTION   :
 *  Dwell time for frames that need a response but have no dwell time
 *  associated.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_DEFAULT_DWELL_TIME 0x09EA

/*******************************************************************************
 * NAME          : UnifiVhtCapabilities
 * PSID          : 2540 (0x09EC)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 12
 * MAX           : 12
 * DEFAULT       : { 0XB1, 0XF2, 0X91, 0X03, 0XFA, 0XFF, 0X00, 0X00, 0XFA, 0XFF, 0X00, 0X00 }
 * DESCRIPTION   :
 *  Golden Certification MIB don't delete, change PSID or name. VHT
 *  capabilities of the chip. see SC-503520-SP.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_VHT_CAPABILITIES 0x09EC

/*******************************************************************************
 * NAME          : UnifiMaxVifScheduleDuration
 * PSID          : 2541 (0x09ED)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * UNITS         : TU
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 50
 * DESCRIPTION   :
 *  Default time for which a non-scan VIF can be scheduled. Applies to
 *  multiVIF scenario. Internal firmware logic or BSS state (e.g. NOA) may
 *  cut short the schedule.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MAX_VIF_SCHEDULE_DURATION 0x09ED

/*******************************************************************************
 * NAME          : UnifiVifLongIntervalTime
 * PSID          : 2542 (0x09EE)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * UNITS         : TU
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       : 60
 * DESCRIPTION   :
 *  When the scheduler expects a VIF to schedule for time longer than this
 *  parameter (specified in TUs), then the VIF may come out of powersave.
 *  Only valid for STA VIFs.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_VIF_LONG_INTERVAL_TIME 0x09EE

/*******************************************************************************
 * NAME          : UnifiDisallowSchedRelinquish
 * PSID          : 2543 (0x09EF)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  When enabled the VIFs will not relinquish their assigned schedules when
 *  they have nothing left to do.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_DISALLOW_SCHED_RELINQUISH 0x09EF

/*******************************************************************************
 * NAME          : UnifiRameDplaneOperationTimeout
 * PSID          : 2544 (0x09F0)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * UNITS         : milliseconds
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       : 1000
 * DESCRIPTION   :
 *  Timeout for requests sent from MACRAME to Data Plane. Any value below
 *  1000ms will be capped at 1000ms.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_RAME_DPLANE_OPERATION_TIMEOUT 0x09F0

/*******************************************************************************
 * NAME          : UnifiDebugKeepRadioOn
 * PSID          : 2545 (0x09F1)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Keep the radio on. For debug purposes only. Setting the value to FALSE
 *  means radio on/off functionality will behave normally. Note that setting
 *  this value to TRUE will automatically disable dorm. The intention is
 * not* for this value to be changed at runtime.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_DEBUG_KEEP_RADIO_ON 0x09F1

/*******************************************************************************
 * NAME          : UnifiForceFixedDurationSchedule
 * PSID          : 2546 (0x09F2)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * UNITS         : TU
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 100
 * DESCRIPTION   :
 *  For schedules with fixed duration e.g. scan, unsync VIF, the schedule
 *  will be forced after this time to avoid VIF starving
 *******************************************************************************/
#define SLSI_PSID_UNIFI_FORCE_FIXED_DURATION_SCHEDULE 0x09F2

/*******************************************************************************
 * NAME          : UnifiGoScanAbsenceDuration
 * PSID          : 2548 (0x09F4)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * UNITS         : beacon intervals
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 2
 * DESCRIPTION   :
 *  Duration of the Absence time to use when protecting P2PGO VIFs from scan
 *  operations. A value of 0 disables the feature.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_GO_SCAN_ABSENCE_DURATION 0x09F4

/*******************************************************************************
 * NAME          : UnifiGoScanAbsencePeriod
 * PSID          : 2549 (0x09F5)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * UNITS         : beacon intervals
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 4
 * DESCRIPTION   :
 *  Period of the Absence/Presence times cycles to use when protecting P2PGO
 *  VIFs from scan operations.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_GO_SCAN_ABSENCE_PERIOD 0x09F5

/*******************************************************************************
 * NAME          : UnifiMaxClient
 * PSID          : 2550 (0x09F6)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 1
 * MAX           : 10
 * DEFAULT       : 10
 * DESCRIPTION   :
 *  Restricts the maximum number of associated STAs for SoftAP. In case of
 *  SoftAP + SoftAP it restricts combined number of peers for all SoftAPs
 *  together.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MAX_CLIENT 0x09F6

/*******************************************************************************
 * NAME          : UnifiGoctWindowDuration
 * PSID          : 2551 (0x09F7)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * UNITS         : TU
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  Client Traffic Window in TU to be set for oppertunistic power save in
 *  P2PGO.This will help reduce the GO power consumption but may adversly
 *  affect P2PGO disconverability.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_GOCT_WINDOW_DURATION 0x09F7

/*******************************************************************************
 * NAME          : UnifiSuppressScanGoNoaAnnounce
 * PSID          : 2552 (0x09F8)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Stop a p2p GO from sending NoA announcements when the GO needs to be
 *  absent for scanning purposes.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_SUPPRESS_SCAN_GO_NOA_ANNOUNCE 0x09F8

/*******************************************************************************
 * NAME          : UnifiMaxP2PClient
 * PSID          : 2553 (0x09F9)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 1
 * MAX           : 10
 * DEFAULT       : 8
 * DESCRIPTION   :
 *  Restricts the maximum number of associated STAs for P2P GO.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MAX_P2_PCLIENT 0x09F9

/*******************************************************************************
 * NAME          : UnifiTdlsInP2pActivated
 * PSID          : 2556 (0x09FC)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Activate TDLS in P2P.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TDLS_IN_P2P_ACTIVATED 0x09FC

/*******************************************************************************
 * NAME          : UnifiTdlsActivated
 * PSID          : 2558 (0x09FE)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Golden Certification MIB don't delete, change PSID or name: Activate
 *  TDLS.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TDLS_ACTIVATED 0x09FE

/*******************************************************************************
 * NAME          : UnifiTdlsTpThresholdPktSecs
 * PSID          : 2559 (0x09FF)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       : 100
 * DESCRIPTION   :
 *  Used for "throughput_threshold_pktsecs" of
 *  RAME-MLME-ENABLE-PEER-TRAFFIC-REPORTING.request.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TDLS_TP_THRESHOLD_PKT_SECS 0x09FF

/*******************************************************************************
 * NAME          : UnifiTdlsRssiThreshold
 * PSID          : 2560 (0x0A00)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt16
 * MIN           : -32768
 * MAX           : 32767
 * DEFAULT       : -75
 * DESCRIPTION   :
 *  FW initiated TDLS Discovery/Setup procedure. If the RSSI of a received
 *  TDLS Discovery Response frame is greater than this value, initiate the
 *  TDLS Setup procedure.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TDLS_RSSI_THRESHOLD 0x0A00

/*******************************************************************************
 * NAME          : UnifiTdlsTpMonitorSecs
 * PSID          : 2562 (0x0A02)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 10
 * DESCRIPTION   :
 *  Measurement period for recording the number of packets sent to a peer
 *  over a TDLS link.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TDLS_TP_MONITOR_SECS 0x0A02

/*******************************************************************************
 * NAME          : Dot11TdlsDiscoveryRequestWindow
 * PSID          : 2565 (0x0A05)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       : 10
 * DESCRIPTION   :
 *  Time to gate Discovery Request frame (in DTIM intervals) after
 *  transmitting a Discovery Request frame.
 *******************************************************************************/
#define SLSI_PSID_DOT11_TDLS_DISCOVERY_REQUEST_WINDOW 0x0A05

/*******************************************************************************
 * NAME          : Dot11TdlsResponseTimeout
 * PSID          : 2566 (0x0A06)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       : 5
 * DESCRIPTION   :
 *  If a valid Setup Response frame is not received within (seconds), the
 *  initiator STA shall terminate the setup procedure and discard any Setup
 *  Response frames.
 *******************************************************************************/
#define SLSI_PSID_DOT11_TDLS_RESPONSE_TIMEOUT 0x0A06

/*******************************************************************************
 * NAME          : Dot11TdlsChannelSwitchActivated
 * PSID          : 2567 (0x0A07)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       :
 * DESCRIPTION   :
 *  Deprecated
 *******************************************************************************/
#define SLSI_PSID_DOT11_TDLS_CHANNEL_SWITCH_ACTIVATED 0x0A07

/*******************************************************************************
 * NAME          : UnifiTdlsWiderBandwidthProhibited
 * PSID          : 2569 (0x0A09)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Wider bandwidth prohibited flag.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TDLS_WIDER_BANDWIDTH_PROHIBITED 0x0A09

/*******************************************************************************
 * NAME          : UnifiStaAndLinkThresholdForReten
 * PSID          : 2570 (0x0A0A)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  Threshold number of stations or link data allocated from retention
 *  memory. Setting it to zero disables allocation to these elements from
 *  non-retention memory. The MIB is cached at init time and will not have
 *  any run-time effects. Bits 7..0 define number of station records to be
 *  allocated from retention memory. Bits 15..8 define number of link data
 *  entries to be allocated from retention memory.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_STA_AND_LINK_THRESHOLD_FOR_RETEN 0x0A0A

/*******************************************************************************
 * NAME          : UnifiTdlsAvailable
 * PSID          : 2571 (0x0A0B)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Get FW status of TDLS state.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TDLS_AVAILABLE 0x0A0B

/*******************************************************************************
 * NAME          : UnifiTdlsKeyLifeTimeInterval
 * PSID          : 2577 (0x0A11)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       : 0X000FFFFF
 * DESCRIPTION   :
 *  Build the Key Lifetime Interval in the TDLS Setup Request frame.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TDLS_KEY_LIFE_TIME_INTERVAL 0x0A11

/*******************************************************************************
 * NAME          : UnifiTdlsTeardownFrameTxTimeout
 * PSID          : 2578 (0x0A12)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 500
 * DESCRIPTION   :
 *  Allowed time in milliseconds for a Teardown frame to be transmitted.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TDLS_TEARDOWN_FRAME_TX_TIMEOUT 0x0A12

/*******************************************************************************
 * NAME          : UnifiTdlsChannelSwitchActivated
 * PSID          : 2579 (0x0A13)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Enable / Disable TDLS channel switch feature
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TDLS_CHANNEL_SWITCH_ACTIVATED 0x0A13

/*******************************************************************************
 * NAME          : UnifiWifiSharingActivated
 * PSID          : 2580 (0x0A14)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Activate WiFi Sharing feature
 *******************************************************************************/
#define SLSI_PSID_UNIFI_WIFI_SHARING_ACTIVATED 0x0A14

/*******************************************************************************
 * NAME          : UnifiWiFiSharingChannels
 * PSID          : 2581 (0x0A15)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 8
 * MAX           : 13
 * DEFAULT       : { 0XFF, 0XDF, 0XFF, 0XFF, 0XFF, 0XFF, 0XFF, 0XFF, 0XFF, 0XFF, 0XFF, 0XFF, 0X07 }
 * DESCRIPTION   :
 *  Default host allowed channels during Wifi Sharing. Defined in a uint64
 *  represented by the octet string. First byte of the octet string maps to
 *  LSB. Bits 0-13 representing 2.4G channels. Bits 14-38 represent 5G
 *  channels. Bit 39 is set to 1 if 6GHz band is supported. Bits 40-97
 *  represent 6G channels. Include 6GHz channels as well. If 6GHz is not
 *  supported, FW will restrict to 2.4G and 5G channels only. Mapping defined
 *  in ChannelisationRules.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_WI_FI_SHARING_CHANNELS 0x0A15

/*******************************************************************************
 * NAME          : UnifiTdlsChannelSwitchTime
 * PSID          : 2582 (0x0A16)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * UNITS         : microseconds
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 30000
 * DESCRIPTION   :
 *  time it takes for a STA sending the Channel Switch Timing element to
 *  switch channels, in units of microseconds
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TDLS_CHANNEL_SWITCH_TIME 0x0A16

/*******************************************************************************
 * NAME          : UnifiWifiSharingChannelSwitchCount
 * PSID          : 2583 (0x0A17)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 3
 * MAX           : 10
 * DEFAULT       : 10
 * DESCRIPTION   :
 *  Channel switch announcement count which will be used in the Channel
 *  announcement IE when using wifi sharing
 *******************************************************************************/
#define SLSI_PSID_UNIFI_WIFI_SHARING_CHANNEL_SWITCH_COUNT 0x0A17

/*******************************************************************************
 * NAME          : UnifiChannelAnnouncementCount
 * PSID          : 2584 (0x0A18)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 10
 * DESCRIPTION   :
 *  Channel switch announcement count which will be used in the Channel
 *  announcement IE
 *******************************************************************************/
#define SLSI_PSID_UNIFI_CHANNEL_ANNOUNCEMENT_COUNT 0x0A18

/*******************************************************************************
 * NAME          : deprecated_unifiRaTestStoredSa
 * PSID          : 2585 (0x0A19)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  Deprecated. Test only: Source address of router contained in virtural
 *  router advertisement packet, specified in chapter '6.2 Forward Received
 *  RA frame to Host' in SC-506393-TE
 *******************************************************************************/
#define SLSI_PSID_DEPRECATED_UNIFI_RA_TEST_STORED_SA 0x0A19

/*******************************************************************************
 * NAME          : deprecated_unifiRaTestStoreFrame
 * PSID          : 2586 (0x0A1A)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  Deprecated. Test only: Virtual router advertisement packet. Specified in
 *  chapter '6.2 Forward Received RA frame to Host' in SC-506393-TE
 *******************************************************************************/
#define SLSI_PSID_DEPRECATED_UNIFI_RA_TEST_STORE_FRAME 0x0A1A

/*******************************************************************************
 * NAME          : Dot11TdlsPeerUapsdBufferStaActivated
 * PSID          : 2587 (0x0A1B)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Activate TDLS peer U-APSD.
 *******************************************************************************/
#define SLSI_PSID_DOT11_TDLS_PEER_UAPSD_BUFFER_STA_ACTIVATED 0x0A1B

/*******************************************************************************
 * NAME          : UnifiWpA3Activated
 * PSID          : 2588 (0x0A1C)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Activate WPA3 support.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_WP_A3_ACTIVATED 0x0A1C

/*******************************************************************************
 * NAME          : UnifiArpOutstandingMax
 * PSID          : 2590 (0x0A1E)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 32
 * DESCRIPTION   :
 *  Maximum number of outstanding ARP frame transmissions in the firmware.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ARP_OUTSTANDING_MAX 0x0A1E

/*******************************************************************************
 * NAME          : UnifiP2PLegacyProbeResponseActivated
 * PSID          : 2599 (0x0A27)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Golden Certification MIB don't delete. When activated, P2P-GO responds to
 *  legacy Probe Request.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_P2_PLEGACY_PROBE_RESPONSE_ACTIVATED 0x0A27

/*******************************************************************************
 * NAME          : UnifiProbeResponseLifetimeP2p
 * PSID          : 2600 (0x0A28)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 500
 * DESCRIPTION   :
 *  Lifetime of proberesponse frame in unit of ms for P2P.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_PROBE_RESPONSE_LIFETIME_P2P 0x0A28

/*******************************************************************************
 * NAME          : UnifiStaChannelSwitchSlowApActivated
 * PSID          : 2601 (0x0A29)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Golden Certification MIB don't delete, change PSID or name: ChanelSwitch:
 *  Activate waiting for a slow AP.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_STA_CHANNEL_SWITCH_SLOW_AP_ACTIVATED 0x0A29

/*******************************************************************************
 * NAME          : UnifiMlmestaPktLifetime
 * PSID          : 2602 (0x0A2A)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 10
 * DEFAULT       : 5
 * DESCRIPTION   :
 *  MLME STA packet lifetime in seconds. Maximum time in seconds for a frame
 *  to be queued before sent out. This value is read only once when an
 *  interface is added. Used along with MlmeSendFrameRequest.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MLMESTA_PKT_LIFETIME 0x0A2A

/*******************************************************************************
 * NAME          : UnifiStaChannelSwitchSlowApMaxTime
 * PSID          : 2604 (0x0A2C)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       : 70
 * DESCRIPTION   :
 *  ChannelSwitch delay for Slow APs. In Seconds.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_STA_CHANNEL_SWITCH_SLOW_AP_MAX_TIME 0x0A2C

/*******************************************************************************
 * NAME          : UnifiStaChannelSwitchSlowApPollInterval
 * PSID          : 2605 (0x0A2D)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 1
 * DESCRIPTION   :
 *  ChannelSwitch polling interval for Slow APs. In Seconds.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_STA_CHANNEL_SWITCH_SLOW_AP_POLL_INTERVAL 0x0A2D

/*******************************************************************************
 * NAME          : UnifiStaChannelSwitchSlowApProcedureTimeoutIncrement
 * PSID          : 2606 (0x0A2E)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 10
 * DESCRIPTION   :
 *  ChannelSwitch procedure timeout increment for Slow APs. In Seconds.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_STA_CHANNEL_SWITCH_SLOW_AP_PROCEDURE_TIMEOUT_INCREMENT 0x0A2E

/*******************************************************************************
 * NAME          : UnifiMlmeScanMaxAerials
 * PSID          : 2607 (0x0A2F)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 1
 * MAX           : 65535
 * DEFAULT       : 1
 * DESCRIPTION   :
 *  Golden Certification MIB don't delete, change PSID or name. Limit the
 *  number of Aerials that Scan will use.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MLME_SCAN_MAX_AERIALS 0x0A2F

/*******************************************************************************
 * NAME          : UnifiIncludePmkidForPsk
 * PSID          : 2610 (0x0A32)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Include PMKID in frame if security type WPA2-PSK.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_INCLUDE_PMKID_FOR_PSK 0x0A32

/*******************************************************************************
 * NAME          : UnifiFilsRnrApLimit
 * PSID          : 2611 (0x0A33)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 255
 * DEFAULT       : 20
 * DESCRIPTION   :
 *  Limits number of RNR AP Info entries (MlmeFilsRnrApInfoEntry) in each
 *  MLME FILS RNR entry(MlmeFilsRnrEntry).
 *******************************************************************************/
#define SLSI_PSID_UNIFI_FILS_RNR_AP_LIMIT 0x0A33

/*******************************************************************************
 * NAME          : UnifiFilsRnrLimit
 * PSID          : 2612 (0x0A34)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 255
 * DEFAULT       : 5
 * DESCRIPTION   :
 *  Limits number of RNR entries (MlmeFilsRnrEntry) in RNR database.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_FILS_RNR_LIMIT 0x0A34

/*******************************************************************************
 * NAME          : UnifiAmsduSubframeRealtekOverrideActivated
 * PSID          : 2615 (0x0A37)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  When connected to Realtek chipset AP, inform Dataplane that actions need
 *  to be taken.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_AMSDU_SUBFRAME_REALTEK_OVERRIDE_ACTIVATED 0x0A37

/*******************************************************************************
 * NAME          : UnifiSuppressStsUpdateBasedOnPeer
 * PSID          : 2616 (0x0A38)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Golden Certification MIB don't delete, change PSID or name: Do not update
 *  STS in VHT caps based on Peer's sounding dimensions.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_SUPPRESS_STS_UPDATE_BASED_ON_PEER 0x0A38

/*******************************************************************************
 * NAME          : UnifiBaRxInternalDeleteDisabled
 * PSID          : 2618 (0x0A3A)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Disable Rx Block Ack from being deleted internally. The BA delete is
 *  still advertised to the peer, but is not deleted internally. This is
 *  applicable only when the delete does not come from the peer.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_BA_RX_INTERNAL_DELETE_DISABLED 0x0A3A

/*******************************************************************************
 * NAME          : UnifiHtApDowngradeByNssActivated
 * PSID          : 2619 (0x0A3B)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Enable HT Ap downgrade when NSS is changed. This is customer specific.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_HT_AP_DOWNGRADE_BY_NSS_ACTIVATED 0x0A3B

/*******************************************************************************
 * NAME          : UnifiHE40in5GRalinkOverrideEnable
 * PSID          : 2620 (0x0A3C)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  When connected to Ralink chipset AP, inform MLME to override support of
 *  40Mhz in 5G of HE phy capabilities when connected on 2.4G.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_HE40IN5_GRALINK_OVERRIDE_ENABLE 0x0A3C

/*******************************************************************************
 * NAME          : UnifiScanMaxOnChipCacheCount
 * PSID          : 2621 (0x0A3D)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 32
 * DEFAULT       : 8
 * DESCRIPTION   :
 *  Maximum count the Scan will maintain the scan result in on-chip cache.
 *  Zero to disable the caching.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_SCAN_MAX_ON_CHIP_CACHE_COUNT 0x0A3D

/*******************************************************************************
 * NAME          : UnifiSchdlpmActivated
 * PSID          : 2622 (0x0A3E)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Enable scheduled PM feature
 *******************************************************************************/
#define SLSI_PSID_UNIFI_SCHDLPM_ACTIVATED 0x0A3E

/*******************************************************************************
 * NAME          : UnifiLastStaConnectedChannel
 * PSID          : 2623 (0x0A3F)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  Primary channel frequency stored by HOST after disconnection from peer
 *  AP(device acting as Mobile Station, not applicable to P2P CLI). In case
 *  of multiple Station VIFs the primary channel of connection which was
 *  terminated latest. 0 if there is no information.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LAST_STA_CONNECTED_CHANNEL 0x0A3F

/*******************************************************************************
 * NAME          : UnifiLinkStatRadioIndex
 * PSID          : 2624 (0x0A40)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  It represents which vif is using which radio in mlo operation. bit [0]:
 *  If bit is enabled/disabled, current vif index is using / not using radio
 *  indexed as 0 bit [1]: If bit is enabled/disabled, current vif index is
 *  using / not using radio indexed as 1 bit [2]: If bit is enabled/disabled,
 *  current vif index is using / not using radio indexed as 2 bit [3]: If bit
 *  is enabled/disabled, current vif index is using / not using radio indexed
 *  as 3 bit [31:4] : reserved
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LINK_STAT_RADIO_INDEX 0x0A40

/*******************************************************************************
 * NAME          : UnifiRoamQuickDisconnectionActivated
 * PSID          : 2630 (0x0A46)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  The FW disconnects the current connection with beacon loss when RSSI is
 *  weak and there are no roaming candidates.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_QUICK_DISCONNECTION_ACTIVATED 0x0A46

/*******************************************************************************
 * NAME          : UnifiRoamQuickDisconnectionTargetRssi
 * PSID          : 2631 (0x0A47)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt8
 * UNITS         : dBm
 * MIN           : -128
 * MAX           : 127
 * DEFAULT       : -83
 * DESCRIPTION   :
 *  RSSI threshold under which connected AP is not deemed eligible for resume
 *  connection.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_QUICK_DISCONNECTION_TARGET_RSSI 0x0A47

/*******************************************************************************
 * NAME          : UnifiRoamingResumeActivated
 * PSID          : 2632 (0x0A48)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Enabled the roaming resume feature by default. It used to roam back to
 *  current AP if assocition is failed with candidate AP..
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAMING_RESUME_ACTIVATED 0x0A48

/*******************************************************************************
 * NAME          : UnifiRoamSkipFullScanOnEmergencyTrigger
 * PSID          : 2633 (0x0A49)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Skip full scan in case of Emergency Trigger type roaming when DUT
 *  received deauthenitcation/Disassociation frames from the AP.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_SKIP_FULL_SCAN_ON_EMERGENCY_TRIGGER 0x0A49

/*******************************************************************************
 * NAME          : UnifiRoamEtpMloBoost
 * PSID          : 2634 (0x0A4A)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt16
 * MIN           : -20
 * MAX           : 20
 * DEFAULT       : 10
 * DESCRIPTION   :
 *  Weight of MLO throughput preference in AP selection algorithm.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_ETP_MLO_BOOST 0x0A4A

/*******************************************************************************
 * NAME          : UnifiBtmEssDisassociationImminentActivated
 * PSID          : 2635 (0x0A4B)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  If false, the FW does not process the ESS disassociation bit present in
 *  the BTM request request mode field.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_BTM_ESS_DISASSOCIATION_IMMINENT_ACTIVATED 0x0A4B

/*******************************************************************************
 * NAME          : UnifiBtmDisassociationScanDelay
 * PSID          : 2639 (0x0A4F)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Delay roaming scan to the expiration of Dissassociation Timer when
 *  disassociation is imminent.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_BTM_DISASSOCIATION_SCAN_DELAY 0x0A4F

/*******************************************************************************
 * NAME          : UnifiRoamBtmQueryTimeout
 * PSID          : 2640 (0x0A50)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * UNITS         : second
 * MIN           : 0
 * MAX           : 10
 * DEFAULT       : 1
 * DESCRIPTION   :
 *  Wait time in seconds for a response to BTM Query.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_BTM_QUERY_TIMEOUT 0x0A50

/*******************************************************************************
 * NAME          : UnifiRoamBsscuTriggerFrequency
 * PSID          : 2641 (0x0A51)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 100
 * DEFAULT       : 10
 * DESCRIPTION   :
 *  How long, in seconds, should the BSS load be high before triggering
 *  roaming?
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_BSSCU_TRIGGER_FREQUENCY 0x0A51

/*******************************************************************************
 * NAME          : UnifiSecuritySkipAllZeroesCheck
 * PSID          : 2642 (0x0A52)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  validate the installation of all keys zero in 4-way handshake. Disable
 *  the MIB while performing vulnerability certification.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_SECURITY_SKIP_ALL_ZEROES_CHECK 0x0A52

/*******************************************************************************
 * NAME          : UnifiRoamRssiFactorFixedMultiplier
 * PSID          : 2643 (0x0A53)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt8
 * MIN           : -128
 * MAX           : 127
 * DEFAULT       :
 * DESCRIPTION   :
 *  Table allocating fixed multiplier used to calculate the RSSIfactorScore
 *  to RSSI values range for each band.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_RSSI_FACTOR_FIXED_MULTIPLIER 0x0A53

/*******************************************************************************
 * NAME          : UnifiRoamNchoScanMaxPassiveChannelTime
 * PSID          : 2644 (0x0A54)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * UNITS         : TU
 * MIN           : 1
 * MAX           : 65535
 * DEFAULT       : 130
 * DESCRIPTION   :
 *  NCHO: Specifies the maximum time spent passive scanning a channel.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_NCHO_SCAN_MAX_PASSIVE_CHANNEL_TIME 0x0A54

/*******************************************************************************
 * NAME          : UnifiRoamTargetRssiBtm
 * PSID          : 2645 (0x0A55)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt16
 * UNITS         : dBm
 * MIN           : -128
 * MAX           : 127
 * DEFAULT       : -83
 * DESCRIPTION   :
 *  Threshold for evaluating BTM roaming target when
 *  unifiRoamBtmDisregardSelectionFactor is set.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_TARGET_RSSI_BTM 0x0A55

/*******************************************************************************
 * NAME          : UnifiRoamETputBoost
 * PSID          : 2646 (0x0A56)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt16
 * UNITS         : percentage
 * MIN           : 0
 * MAX           : 100
 * DEFAULT       :
 * DESCRIPTION   :
 *  The value in percentage of the ETput boost for each band
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_ETPUT_BOOST 0x0A56

/*******************************************************************************
 * NAME          : UnifiRoamBtCoexActivated
 * PSID          : 2647 (0x0A57)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  If false, the FW shall ignore BT connected indication and do not trigger
 *  roaming by BT coex.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_BT_COEX_ACTIVATED 0x0A57

/*******************************************************************************
 * NAME          : UnifiRoamBtCoexTargetRssi
 * PSID          : 2648 (0x0A58)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt16
 * UNITS         : dBm
 * MIN           : -128
 * MAX           : 127
 * DEFAULT       : -75
 * DESCRIPTION   :
 *  Threshold for evaluating BT Coex roaming candidate.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_BT_COEX_TARGET_RSSI 0x0A58

/*******************************************************************************
 * NAME          : UnifiRoamBtCoexApSelectDeltaFactor
 * PSID          : 2649 (0x0A59)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 100
 * DEFAULT       : 10
 * DESCRIPTION   :
 *  Delta value applied to the score of the currently connected AP to
 *  determine candidates' eligibility threshold for BT Coex roaming.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_BT_COEX_AP_SELECT_DELTA_FACTOR 0x0A59

/*******************************************************************************
 * NAME          : UnifiApfActivated
 * PSID          : 2650 (0x0A5A)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  It is used to enable or disable Android Packet Filter(APF).
 *******************************************************************************/
#define SLSI_PSID_UNIFI_APF_ACTIVATED 0x0A5A

/*******************************************************************************
 * NAME          : UnifiApfVersion
 * PSID          : 2651 (0x0A5B)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 4
 * DESCRIPTION   :
 *  APF version currently supported by the FW.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_APF_VERSION 0x0A5B

/*******************************************************************************
 * NAME          : UnifiApfMaxSize
 * PSID          : 2652 (0x0A5C)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 1024
 * DESCRIPTION   :
 *  Max size in bytes supported by FW per VIF. Includes both program len and
 *  data len.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_APF_MAX_SIZE 0x0A5C

/*******************************************************************************
 * NAME          : UnifiApfActiveModeEnabled
 * PSID          : 2653 (0x0A5D)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Indicates if APF is supported in host active mode. Applicable to only
 *  group addressed frames.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_APF_ACTIVE_MODE_ENABLED 0x0A5D

/*******************************************************************************
 * NAME          : UnifiFrameResponseNdmTimeout
 * PSID          : 2654 (0x0A5E)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 3000
 * DESCRIPTION   :
 *  Timeout, in TU, to wait to retry a frame after after TX Cfm
 *  trasnmission_status == Successful.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_FRAME_RESPONSE_NDM_TIMEOUT 0x0A5E

/*******************************************************************************
 * NAME          : UnifiRoamBtCoexScoreWeight
 * PSID          : 2655 (0x0A5F)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 100
 * DEFAULT       : 70
 * DESCRIPTION   :
 *  Score weight of BT Coex factor to only applied 2.4G AP if BT is
 *  connected, in percentage points.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_BT_COEX_SCORE_WEIGHT 0x0A5F

/*******************************************************************************
 * NAME          : UnifiRoamBtCoexEtputWeight
 * PSID          : 2656 (0x0A60)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 100
 * DEFAULT       : 70
 * DESCRIPTION   :
 *  ETPUT weight of BT Coex factor to only applied 2.4G AP if BT is
 *  connected, in percentage points.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_BT_COEX_ETPUT_WEIGHT 0x0A60

/*******************************************************************************
 * NAME          : UnifiRoamBtCoexThresholdTime
 * PSID          : 2657 (0x0A61)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 60
 * DEFAULT       : 10
 * DESCRIPTION   :
 *  If BT coex threshold time does not end, additional BT coex roaming
 *  trigger condition must be ignored.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAM_BT_COEX_THRESHOLD_TIME 0x0A61

/*******************************************************************************
 * NAME          : UnifiSmPowerSaveMode
 * PSID          : 2670 (0x0A6E)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 3
 * DEFAULT       : 3
 * DESCRIPTION   :
 *  Config SM Power Save mode capability Input: 0 : Static 1 : Dynamic 2 : No
 *  Limit 3 : Disabled
 *******************************************************************************/
#define SLSI_PSID_UNIFI_SM_POWER_SAVE_MODE 0x0A6E

/*******************************************************************************
 * NAME          : UnifiHeActivated
 * PSID          : 2700 (0x0A8C)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Enables HE mode.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_HE_ACTIVATED 0x0A8C

/*******************************************************************************
 * NAME          : UnifiHeCapabilities
 * PSID          : 2701 (0x0A8D)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 28
 * MAX           : 32
 * DEFAULT       : { 0X03, 0X08, 0X88, 0X02, 0X00, 0X00, 0X26, 0X70, 0X4E, 0X09, 0XFD, 0X00, 0XAF, 0X0A, 0X00, 0XBD, 0X00, 0XFA, 0XFF, 0XFA, 0XFF, 0X79, 0X1C, 0XC7, 0X71, 0X1C, 0XC7, 0X71 }
 * DESCRIPTION   :
 *  HE capabilities of chip. This includes HE MAC capabilities information,
 *  HE PHY capabilities information, Supported HE-MCS and NSS set, PPE
 *  thresholds(optional) fields.see SC-XXXXXX-SP.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_HE_CAPABILITIES 0x0A8D

/*******************************************************************************
 * NAME          : UnifiRaaTxGiValue
 * PSID          : 2702 (0x0A8E)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 255
 * DEFAULT       :
 * DESCRIPTION   :
 *  HE GI value set by Host".
 *******************************************************************************/
#define SLSI_PSID_UNIFI_RAA_TX_GI_VALUE 0x0A8E

/*******************************************************************************
 * NAME          : UnifiRaaTxLtfValue
 * PSID          : 2703 (0x0A8F)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 255
 * DEFAULT       : 1
 * DESCRIPTION   :
 *  HE LTF value set by host.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_RAA_TX_LTF_VALUE 0x0A8F

/*******************************************************************************
 * NAME          : Unifi256MsduConfig
 * PSID          : 2704 (0x0A90)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  256 MSDU support.
 *******************************************************************************/
#define SLSI_PSID_UNIFI256_MSDU_CONFIG 0x0A90

/*******************************************************************************
 * NAME          : UnifiMbssidActivated
 * PSID          : 2705 (0x0A91)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Activate Multiple Bssid.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MBSSID_ACTIVATED 0x0A91

/*******************************************************************************
 * NAME          : UnifiTxAmsduSupportOverride
 * PSID          : 2706 (0x0A92)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Param to override tx amsdu support
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TX_AMSDU_SUPPORT_OVERRIDE 0x0A92

/*******************************************************************************
 * NAME          : UnifiMbssidRoamingActivated
 * PSID          : 2707 (0x0A93)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Activate Multiple Bssid roaming.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MBSSID_ROAMING_ACTIVATED 0x0A93

/*******************************************************************************
 * NAME          : UnifiHeDynamicSmpsEnable
 * PSID          : 2708 (0x0A94)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Enable/Disable HE dynamic SMPS
 *******************************************************************************/
#define SLSI_PSID_UNIFI_HE_DYNAMIC_SMPS_ENABLE 0x0A94

/*******************************************************************************
 * NAME          : UnifiHe1xltf08giWorkaround
 * PSID          : 2709 (0x0A95)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Param to disable workaround logic for station support of HE 1x ltf and
 *  0.8us gi in its association request frame. Some APs send HE SU PPDUs at
 *  1x ltf and 0.8us gi even though they do not support 1x ltf and 0.8us gi
 *  if peer advertises that it supports HE SU PPDUs at 1x ltf and 0.8us gi.
 *  If this param is set to true and if ap advertises that it does not
 *  support 1x ltf and 0.8us gi, station will disable support for 1x ltf and
 *  0.8us gi in HE capabilities in its association request frame. If this
 *  param is set to false, station will advertise support for 1x ltf and
 *  0.8us gi in HE capabilities in its association request frame based on its
 *  local capability only.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_HE1XLTF08GI_WORKAROUND 0x0A95

/*******************************************************************************
 * NAME          : UnifiOverrideMuedcaParamAcEnable
 * PSID          : 2710 (0x0A96)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 31
 * DEFAULT       : 1
 * DESCRIPTION   :
 *  override STA - AC MU-EDCA using the values in unifiOverrideMUEDCAParamAC
 *  MIB, useful for internal testing. Bit 0 - 0 means disable M-EDCA feature
 *  completely and always use legacy EDCA parameters. Bit 1 - if set to 1
 *  means ignore MUEDCA parameters for BE that is being broadcasted by AP and
 *  overwrite it with MU EDCA parameters for BE specified in MIB variable
 *  unifiOverrideMUEDCAParamAC. Bit 2 - if set to 1 means ignore MUEDCA
 *  parameters for BK that is being broadcasted by AP and overwrite it with
 *  MU EDCA parameters for BK specified in MIB variable
 *  unifiOverrideMUEDCAParamAC. Bit 3 - if set to 1 means ignore MUEDCA
 *  parameters for VI that is being broadcasted by AP and overwrite it with
 *  MU EDCA parameters for VI specified in MIB variable
 *  unifiOverrideMUEDCAParamAC. Bit 4 - if set to 1 means ignore MUEDCA
 *  parameters for VO that is being broadcasted by AP and overwrite it with
 *  MU EDCA parameters for VO specified in MIB variable
 *  unifiOverrideMUEDCAParamAC.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_OVERRIDE_MUEDCA_PARAM_AC_ENABLE 0x0A96

/*******************************************************************************
 * NAME          : UnifiOverrideMuedcaParamAc
 * PSID          : 2711 (0x0A97)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 12
 * MAX           : 12
 * DEFAULT       : {0X02, 0X33, 0X06, 0X02, 0XA4, 0X06, 0X02, 0X44, 0X06, 0X02, 0X32, 0X06}
 * DESCRIPTION   :
 *  Override the MU EDCA parameters. octet 0 - BE AIFS octet 1 - [7:4] ECW
 *  MAX [3:0] ECW MIN octet 2 - MU EDCA timer for BE in 8 TU units. octet 3 -
 *  BK AIFS octet 4 - [7:4] ECW MAX [3:0] ECW MIN octet 5 - MU EDCA timer for
 *  BK in 8 TU units. octet 6 - VI AIFS octet 7 - [7:4] ECW MAX [3:0] ECW MIN
 *  octet 8 - MU EDCA timer for VI in 8 TU units. octet 9 - VO AIFS octet 10
 *  - [7:4] ECW MAX [3:0] ECW MIN octet 11 - MU EDCA timer for VO in 8 TU
 *  units.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_OVERRIDE_MUEDCA_PARAM_AC 0x0A97

/*******************************************************************************
 * NAME          : UnifiMuedcaTimerUnit
 * PSID          : 2712 (0x0A98)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 8
 * DESCRIPTION   :
 *  MU EDCA Timer Unit
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MUEDCA_TIMER_UNIT 0x0A98

/*******************************************************************************
 * NAME          : UnifiUlmuDisableBitMap
 * PSID          : 2713 (0x0A99)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 255
 * DEFAULT       : 2
 * DESCRIPTION   :
 *  Disable UL MU in various modes. Input: 1 (i.e. BIT0) : Disable UL MU if
 *  Bluetooth is on 2 (i.e BIT1) : Disable UL MU if Same band multi-vif
 *  detected 4 (i.e. BIT2) : Disable UL MU if TDLS is active 0: Enable by
 *  default
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ULMU_DISABLE_BIT_MAP 0x0A99

/*******************************************************************************
 * NAME          : UnifiHeErSuDisable
 * PSID          : 2714 (0x0A9A)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Enable/Disable HE ER in SAP/P2P Go mode
 *******************************************************************************/
#define SLSI_PSID_UNIFI_HE_ER_SU_DISABLE 0x0A9A

/*******************************************************************************
 * NAME          : UnifiOmTxNstsValue
 * PSID          : 2715 (0x0A9B)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  Tx NSTS value set by Host
 *******************************************************************************/
#define SLSI_PSID_UNIFI_OM_TX_NSTS_VALUE 0x0A9B

/*******************************************************************************
 * NAME          : UnifiOmRxNssValue
 * PSID          : 2716 (0x0A9C)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  Rx NSS value set by Host
 *******************************************************************************/
#define SLSI_PSID_UNIFI_OM_RX_NSS_VALUE 0x0A9C

/*******************************************************************************
 * NAME          : UnifiOmChanBandwidthValue
 * PSID          : 2717 (0x0A9D)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  Operating channel bandwidth value set by Host
 *******************************************************************************/
#define SLSI_PSID_UNIFI_OM_CHAN_BANDWIDTH_VALUE 0x0A9D

/*******************************************************************************
 * NAME          : UnifiOmUlMuEnable
 * PSID          : 2718 (0x0A9E)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 2
 * DESCRIPTION   :
 *  UL MU/UL MU Data enable/disable
 *******************************************************************************/
#define SLSI_PSID_UNIFI_OM_UL_MU_ENABLE 0x0A9E

/*******************************************************************************
 * NAME          : UnifiOpModeValue
 * PSID          : 2719 (0x0A9F)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 0XDEAD
 * DESCRIPTION   :
 *  Operating Mode control param to add in OMI frame
 *******************************************************************************/
#define SLSI_PSID_UNIFI_OP_MODE_VALUE 0x0A9F

/*******************************************************************************
 * NAME          : UnifiOfdmaNarrowBwRuDisableTime
 * PSID          : 2720 (0x0AA0)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 2929
 * DEFAULT       : 2929
 * DESCRIPTION   :
 *  Minimum Time (3 Minute, 2929TU) that needs to pass since the reception of
 *  last beacon from OBSS AP that disable Narrow BW RU.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_OFDMA_NARROW_BW_RU_DISABLE_TIME 0x0AA0

/*******************************************************************************
 * NAME          : UnifiUlMuMimoSupportOverride
 * PSID          : 2721 (0x0AA1)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Param to override UL MU support
 *******************************************************************************/
#define SLSI_PSID_UNIFI_UL_MU_MIMO_SUPPORT_OVERRIDE 0x0AA1

/*******************************************************************************
 * NAME          : UnifiDlMumimoNcWar
 * PSID          : 2722 (0x0AA2)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Enable DL MUMIMO WAR for dynamic configuration of nc value
 *******************************************************************************/
#define SLSI_PSID_UNIFI_DL_MUMIMO_NC_WAR 0x0AA2

/*******************************************************************************
 * NAME          : UnifiHeUlOfdmaRxDisabledSoftAp
 * PSID          : 2723 (0x0AA3)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Enables uplink ofdma reception in softap mode.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_HE_UL_OFDMA_RX_DISABLED_SOFT_AP 0x0AA3

/*******************************************************************************
 * NAME          : UnifiMbssControlFrameRxSupported
 * PSID          : 2724 (0x0AA4)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Activate support for control frames from MBSS AP.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MBSS_CONTROL_FRAME_RX_SUPPORTED 0x0AA4

/*******************************************************************************
 * NAME          : UnifiPnCheck
 * PSID          : 2725 (0x0AA5)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt16
 * MIN           : -32768
 * MAX           : 32767
 * DEFAULT       : 000
 * DESCRIPTION   :
 *  Changes the packet number to test PN check Input: input a positive or
 *  negitive value to offset PN value
 *******************************************************************************/
#define SLSI_PSID_UNIFI_PN_CHECK 0x0AA5

/*******************************************************************************
 * NAME          : UnifiOfdmaSoftApTxTriggerParams
 * PSID          : 2726 (0x0AA6)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  Configures interval for trigger transmissions and aid for which trigger
 *  has to be tx'd Input: Byte 0-1: Trigger Interval use bits 12 Byte 1 : ack
 *  policy use bits 4 Byte 2 : Aid of the STA for which trigger has to be
 *  tx'd Byte 3 : Trigger Type
 *******************************************************************************/
#define SLSI_PSID_UNIFI_OFDMA_SOFT_AP_TX_TRIGGER_PARAMS 0x0AA6

/*******************************************************************************
 * NAME          : UnifiPsrAndNonSrgObssPdProhibited
 * PSID          : 2727 (0x0AA7)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  PSR_AND_NON_SRG_OBSS_PD_PROHIBITED value.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_PSR_AND_NON_SRG_OBSS_PD_PROHIBITED 0x0AA7

/*******************************************************************************
 * NAME          : UnifiSpatialReuseConfig
 * PSID          : 2728 (0x0AA8)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 21
 * MAX           : 21
 * DEFAULT       : {0X03, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00}
 * DESCRIPTION   :
 *  Spatial Reuse Config to enable/disable Spatial Reuse support and also to
 *  override Spatial Reuse parameters. octet 0 - Bit 0 - 1 to enable spatial
 *  reuse 0 to disable spatial reuse Bit 1 - 1 to Override Spatial Reuse
 *  Paramset octet 1 - SR Control octet 2 - Non-SRG OBSS PD Max Offset octet
 *  3 - SRG OBSS PD Min Offset field octet 4 - SRG OBSS PD Max Offset field
 *  octet 5 to 12 - SRG BSS Color Bitmap field octet 13 - 20 SRG Partial
 *  BSSID Bitmap field
 *******************************************************************************/
#define SLSI_PSID_UNIFI_SPATIAL_REUSE_CONFIG 0x0AA8

/*******************************************************************************
 * NAME          : UnifiReadSrReg
 * PSID          : 2729 (0x0AA9)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  Read value from a register for a specifi Spatial Reuse counter and return
 *  it or reset the counters
 *******************************************************************************/
#define SLSI_PSID_UNIFI_READ_SR_REG 0x0AA9

/*******************************************************************************
 * NAME          : UnifiTwtActivated
 * PSID          : 2731 (0x0AAB)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Enable TWT feature
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TWT_ACTIVATED 0x0AAB

/*******************************************************************************
 * NAME          : UnifiTwtControlFlags
 * PSID          : 2732 (0x0AAC)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 255
 * DEFAULT       : 36
 * DESCRIPTION   :
 *  Control flags for TWT Input: BIT0 : Broadcast TWT Support BIT1 :
 *  Responder TWT Support BIT2 : Requester TWT Support BIT3 : Flexible TWT
 *  Support BIT4 : Enable TWT Service period skip if LCD OFF BIT5 : Enable
 *  aggressive TWT Service period powerdown
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TWT_CONTROL_FLAGS 0x0AAC

/*******************************************************************************
 * NAME          : UnifiTwtIndDefSessIdx
 * PSID          : 2733 (0x0AAD)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 1
 * DESCRIPTION   :
 *  Default individual TWT session index to be used from
 *  unifiTWTIndSessParamTable.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TWT_IND_DEF_SESS_IDX 0x0AAD

/*******************************************************************************
 * NAME          : UnifiTwtStartSession
 * PSID          : 2734 (0x0AAE)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 255
 * DESCRIPTION   :
 *  Start TWT session. Input: Byte 0: Session Idx Byte 1: Value 0: Individual
 *  TWT Value 1: Broadcast TWT
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TWT_START_SESSION 0x0AAE

/*******************************************************************************
 * NAME          : UnifiTwtTeardownSession
 * PSID          : 2735 (0x0AAF)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  Teardown TWT session. Input: Byte 0: flow id(Individual TWT)/ BTWT ID
 *  (Broadcast TWT) Byte 1: Value 0: Individual TWT Value 1: Broadcast TWT
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TWT_TEARDOWN_SESSION 0x0AAF

/*******************************************************************************
 * NAME          : UnifiTwtIndSessParam
 * PSID          : 2736 (0x0AB0)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 7
 * MAX           : 7
 * DEFAULT       :
 * DESCRIPTION   :
 *  Individual TWT session parameters. Supports multiple sessions. Each row
 *  of the table contains session parameters for a individual TWT sessions.
 *  Entry has the following structure: Octet 0: Control field bit 0: NDP
 *  paging indicator bit 1: Responder PM mode bit 2-3: Negotiation type bit
 *  4: TWT information frame disabled. bit 5-7: Reserved Octet 1: bit 0: TWT
 *  request bit 1: Trigger bit 2: Implicit bit 3: Flowtype bit 4: TWT
 *  protection bit 5-7: TWT Setup command Octet 2: bit 0-4: TWT Wake Interval
 *  Exponent bit 5: TWT channel bit 6-7: Reserved Octet 3: Nominal Minimum
 *  TWT wake duration Octet 4-5: TWT Wake Interval Mantissa Octet 6: TWT wake
 *  time offset from next beacon tbtt in milliseconds. Size of each entry is
 *  7 Octets.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TWT_IND_SESS_PARAM 0x0AB0

/*******************************************************************************
 * NAME          : UnifiTwtModeSupportBitMap
 * PSID          : 2737 (0x0AB1)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 255
 * DEFAULT       :
 * DESCRIPTION   :
 *  TWT support in various modes. Input: 1 (i.e. BIT0) : TWT Support in
 *  Active Mode 2 (i.e BIT1) : TWT Support in Host Suspend Mode 4 (i.e. BIT2)
 *  : TWT Support in BT Coex Mode 0: Disable TWT in Active Mode uses Row 1
 *  params in unifiTWTIndSessParamTable TWT in Host Suspend Mode uses Row 2
 *  params in unifiTWTIndSessParamTable and so on
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TWT_MODE_SUPPORT_BIT_MAP 0x0AB1

/*******************************************************************************
 * NAME          : UnifiTwtHostSuspTwtTeardownDelay
 * PSID          : 2738 (0x0AB2)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 100
 * DESCRIPTION   :
 *  Teardown host suspend TWT session after this delay. Delay in msec
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TWT_HOST_SUSP_TWT_TEARDOWN_DELAY 0x0AB2

/*******************************************************************************
 * NAME          : UnifiTwtspInactivityTime
 * PSID          : 2739 (0x0AB3)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 10000
 * DESCRIPTION   :
 *  Inactivity time for terminating current service period in usec
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TWTSP_INACTIVITY_TIME 0x0AB3

/*******************************************************************************
 * NAME          : UnifiTwtResumeDuration
 * PSID          : 2740 (0x0AB4)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  This value is in microseconds and is used to compute TWT information
 *  frame Next TWT subfield value
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TWT_RESUME_DURATION 0x0AB4

/*******************************************************************************
 * NAME          : UnifiTwtOperation
 * PSID          : 2741 (0x0AB5)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 2
 * DEFAULT       :
 * DESCRIPTION   :
 *  Set to 1 for TWT suspend and set to 2 for TWT resume
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TWT_OPERATION 0x0AB5

/*******************************************************************************
 * NAME          : UnifiTwtInfoFrameTx
 * PSID          : 2742 (0x0AB6)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Enable/disable transmission of TWT information frames
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TWT_INFO_FRAME_TX 0x0AB6

/*******************************************************************************
 * NAME          : UnifiTwtBcastSessParam
 * PSID          : 2743 (0x0AB7)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 10
 * MAX           : 10
 * DEFAULT       :
 * DESCRIPTION   :
 *  Broadcast TWT session parameters. Supports multiple sessions. Each row of
 *  the table contains session parameters for a broadcast TWT sessions. Entry
 *  has the following structure: Octet 0: Control field bit 0: NDP paging
 *  indicator bit 1: Responder PM mode bit 2-3: Negotiation type bit 4: TWT
 *  information frame disabled. bit 5-7: Reserved Octet 1: bit 0: TWT request
 *  bit 1: Trigger bit 2: Last Broadcast Param set bit 3: Flowtype bit 4-6:
 *  TWT Setup command bit 7: Reserved Octet 2: bit 0-4: TWT Wake Interval
 *  Exponent bit 5-7: Broadcast TWT Recommendation Octet 3: Nominal Minimum
 *  TWT wake duration Octet 4-5: TWT Wake Interval Mantissa Octet 6-7: Target
 *  Wake time Octet 8: Broadcast TWT Persistence Octet 9: bit 0-4: Broadcast
 *  TWT ID bit 5-7: Reserved Size of each entry is 10 Octets.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TWT_BCAST_SESS_PARAM 0x0AB7

/*******************************************************************************
 * NAME          : UnifiTwtspStartBackoff
 * PSID          : 2744 (0x0AB8)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 4000
 * DESCRIPTION   :
 *  Time unit is in microseconds. Indicates the amount of time by which the
 *  TWT blackout duration is reduced. This is to account for the radio on,
 *  vif schedule and data path resume latencies. Default value is 4000usec.
 *  If this MIB is set to 6000usec then TWT SP starts 2msec earlier and so on
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TWTSP_START_BACKOFF 0x0AB8

/*******************************************************************************
 * NAME          : UnifiHeCapabilitiesSoftAp
 * PSID          : 2745 (0x0AB9)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 28
 * MAX           : 32
 * DEFAULT       : { 0X01, 0X08, 0X00, 0X02, 0X00, 0X00, 0X26, 0X70, 0X02, 0X00, 0X00, 0X00, 0X80, 0X0A, 0X00, 0X8D, 0X00, 0XFA, 0XFF, 0XFA, 0XFF, 0X79, 0X1C, 0XC7, 0X71, 0X1C, 0XC7, 0X71 }
 * DESCRIPTION   :
 *  HE capabilities of chip supported in softap mode. This includes HE MAC
 *  capabilities information, HE PHY capabilities information, Supported
 *  HE-MCS and NSS set,
 *******************************************************************************/
#define SLSI_PSID_UNIFI_HE_CAPABILITIES_SOFT_AP 0x0AB9

/*******************************************************************************
 * NAME          : UnifiHeActivatedSoftAp
 * PSID          : 2746 (0x0ABA)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Enables HE mode.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_HE_ACTIVATED_SOFT_AP 0x0ABA

/*******************************************************************************
 * NAME          : UnifiHeActivatedP2p
 * PSID          : 2747 (0x0ABB)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Deprecated
 *******************************************************************************/
#define SLSI_PSID_UNIFI_HE_ACTIVATED_P2P 0x0ABB

/*******************************************************************************
 * NAME          : UnifiHeEnableDynamicDurationBasedRtsThreshold
 * PSID          : 2748 (0x0ABC)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Enables Dynamic Duration Based RTS Threshold.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_HE_ENABLE_DYNAMIC_DURATION_BASED_RTS_THRESHOLD 0x0ABC

/*******************************************************************************
 * NAME          : UnifiSoftApMuedcaParamAc
 * PSID          : 2749 (0x0ABD)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 12
 * MAX           : 12
 * DEFAULT       : {0X02, 0X33, 0X06, 0X02, 0XA4, 0X06, 0X02, 0X44, 0X06, 0X02, 0X32, 0X06}
 * DESCRIPTION   :
 *  MU EDCA parameters for SoftAP to advertise in beacon, probe resp and
 *  asspc resp. octet 0 - BE AIFS octet 1 - [7:4] ECW MAX [3:0] ECW MIN octet
 *  2 - MU EDCA timer for BE in 8 TU units. octet 3 - BK AIFS octet 4 - [7:4]
 *  ECW MAX [3:0] ECW MIN octet 5 - MU EDCA timer for BK in 8 TU units. octet
 *  6 - VI AIFS octet 7 - [7:4] ECW MAX [3:0] ECW MIN octet 8 - MU EDCA timer
 *  for VI in 8 TU units. octet 9 - VO AIFS octet 10 - [7:4] ECW MAX [3:0]
 *  ECW MIN octet 11 - MU EDCA timer for VO in 8 TU units.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_SOFT_AP_MUEDCA_PARAM_AC 0x0ABD

/*******************************************************************************
 * NAME          : UnifiHePuncturedPreambleRx
 * PSID          : 2750 (0x0ABE)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Enable/Disable HE Punctured preamble rx
 *******************************************************************************/
#define SLSI_PSID_UNIFI_HE_PUNCTURED_PREAMBLE_RX 0x0ABE

/*******************************************************************************
 * NAME          : Unifi6GhzEnabled
 * PSID          : 2751 (0x0ABF)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Enables 6G band operations.
 *******************************************************************************/
#define SLSI_PSID_UNIFI6_GHZ_ENABLED 0x0ABF

/*******************************************************************************
 * NAME          : Unifi6GrnrMaxCacheCount
 * PSID          : 2752 (0x0AC0)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 15
 * DEFAULT       : 10
 * DESCRIPTION   :
 *  Maximum RNR entries per channel
 *******************************************************************************/
#define SLSI_PSID_UNIFI6_GRNR_MAX_CACHE_COUNT 0x0AC0

/*******************************************************************************
 * NAME          : Unifi6GScanAccTimeoutDur
 * PSID          : 2753 (0x0AC1)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 80
 * DEFAULT       : 30
 * DESCRIPTION   :
 *  Scan Accelerator Timeout duration in TUs
 *******************************************************************************/
#define SLSI_PSID_UNIFI6_GSCAN_ACC_TIMEOUT_DUR 0x0AC1

/*******************************************************************************
 * NAME          : Unifi6GPrescanConcurrentMacs
 * PSID          : 2754 (0x0AC2)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 2
 * DEFAULT       : 2
 * DESCRIPTION   :
 *  Indicates number of concurrent macs to be used for scan accelerator
 *******************************************************************************/
#define SLSI_PSID_UNIFI6_GPRESCAN_CONCURRENT_MACS 0x0AC2

/*******************************************************************************
 * NAME          : Unifi6GEnablePrescan
 * PSID          : 2755 (0x0AC3)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Enables prescan of 6 GHz channels using scan accelerator.
 *******************************************************************************/
#define SLSI_PSID_UNIFI6_GENABLE_PRESCAN 0x0AC3

/*******************************************************************************
 * NAME          : UnifiHe6GHzBandCapabilities
 * PSID          : 2756 (0x0AC4)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 2
 * MAX           : 2
 * DEFAULT       : { 0X7D, 0X06 }
 * DESCRIPTION   :
 *  HE 6 GHz band capabilities of chip.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_HE6_GHZ_BAND_CAPABILITIES 0x0AC4

/*******************************************************************************
 * NAME          : Unifi6GScanPolicy
 * PSID          : 2757 (0x0AC5)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 3
 * DEFAULT       : 3
 * DESCRIPTION   :
 *  0: Auto which scan all channels given to scan fsm regardless whether RNRs
 *  are found 1: PSC only channels 2: NON_PSC only channels (scans all
 *  non-psc regardless whether RNRs are found) 3: PSC + NON_PSC_RNR_ONLY
 *  (scans all PSC and only non-PSC on which RNRs are found)
 *******************************************************************************/
#define SLSI_PSID_UNIFI6_GSCAN_POLICY 0x0AC5

/*******************************************************************************
 * NAME          : Unifi6GEnableRoamingSplitScan
 * PSID          : 2758 (0x0AC6)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Enables 6GHz roaming split scan.
 *******************************************************************************/
#define SLSI_PSID_UNIFI6_GENABLE_ROAMING_SPLIT_SCAN 0x0AC6

/*******************************************************************************
 * NAME          : Unifi6GSafeMode
 * PSID          : 2759 (0x0AC7)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Enables 6GHz Safe mode which'll have OPEN security.
 *******************************************************************************/
#define SLSI_PSID_UNIFI6_GSAFE_MODE 0x0AC7

/*******************************************************************************
 * NAME          : Unifi6GDisableUnsoliProbeNonIdle
 * PSID          : 2760 (0x0AC8)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  in 6G MHS, Disable Unsolicited Probe response transmission once peer
 *  associate.
 *******************************************************************************/
#define SLSI_PSID_UNIFI6_GDISABLE_UNSOLI_PROBE_NON_IDLE 0x0AC8

/*******************************************************************************
 * NAME          : Unifi6GSoftApDisableFullWindowPipeline
 * PSID          : 2761 (0x0AC9)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  in 6G MHS, Disable full window pipeline, half window pipeline giving good
 *  throughput.
 *******************************************************************************/
#define SLSI_PSID_UNIFI6_GSOFT_AP_DISABLE_FULL_WINDOW_PIPELINE 0x0AC9

/*******************************************************************************
 * NAME          : Unifi6GDualClientCert
 * PSID          : 2762 (0x0ACA)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Enables 6GHz standard power.
 *******************************************************************************/
#define SLSI_PSID_UNIFI6_GDUAL_CLIENT_CERT 0x0ACA

/*******************************************************************************
 * NAME          : UnifiOceEnabled
 * PSID          : 2771 (0x0AD3)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Enables OCE operations.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_OCE_ENABLED 0x0AD3

/*******************************************************************************
 * NAME          : Dot11FilsProbeDelayTime
 * PSID          : 2772 (0x0AD4)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 15
 * MAX           : 20
 * DEFAULT       : 15
 * DESCRIPTION   :
 *  OCE probe deferral delay time
 *******************************************************************************/
#define SLSI_PSID_DOT11_FILS_PROBE_DELAY_TIME 0x0AD4

/*******************************************************************************
 * NAME          : UnifiOceFilsCertSupport
 * PSID          : 2773 (0x0AD5)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Includes Filsdiscovery BSSID's in supression attribute in probe request.
 *  Enabled only for certification as FD frames does not carry complete
 *  information for connection and sending directed probe request for each FD
 *  frame will increase scan time. Based on this flag skip exiting probe
 *  deferal based on bss load ie in beacon or probe response
 *******************************************************************************/
#define SLSI_PSID_UNIFI_OCE_FILS_CERT_SUPPORT 0x0AD5

/*******************************************************************************
 * NAME          : UnifiOceUseRnrForDiscovery
 * PSID          : 2774 (0x0AD6)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Enables RNR data base for OCE discovery.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_OCE_USE_RNR_FOR_DISCOVERY 0x0AD6

/*******************************************************************************
 * NAME          : UnifiOceProbeDeferralEnabled
 * PSID          : 2775 (0x0AD7)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Enables OCE probe deferral mechanism
 *******************************************************************************/
#define SLSI_PSID_UNIFI_OCE_PROBE_DEFERRAL_ENABLED 0x0AD7

/*******************************************************************************
 * NAME          : UnifiSpatialReusePerobssConfig
 * PSID          : 2776 (0x0AD8)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 1
 * MAX           : 51
 * DEFAULT       : {0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, }
 * DESCRIPTION   :
 *  Spatial Reuse Config to enable/disable Spatial Reuse support for per also
 *  to override Spatial Reuse parameters. octet 0 - 1 to enable spatial reuse
 *  0 to disable spatial reuse octet 1 to 6 - MAC Address octet 7 - OBSS-PD
 *  Offset
 *******************************************************************************/
#define SLSI_PSID_UNIFI_SPATIAL_REUSE_PEROBSS_CONFIG 0x0AD8

/*******************************************************************************
 * NAME          : UnifiSpatialReuseObssStatistics
 * PSID          : 2777 (0x0AD9)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 1
 * MAX           : 421
 * DEFAULT       :
 * DESCRIPTION   :
 *  Get or clear OBSS statistics also to override Spatial Reuse parameters.
 *  octet 0 - Set zero to clear statistics.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_SPATIAL_REUSE_OBSS_STATISTICS 0x0AD9

/*******************************************************************************
 * NAME          : UnifiTwtAvgTxPktNum
 * PSID          : 2781 (0x0ADD)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  It provides average num of tx frames for a particular session id.
 *  (indexed by unifiTWTSessTableIndex).
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TWT_AVG_TX_PKT_NUM 0x0ADD

/*******************************************************************************
 * NAME          : UnifiTwtAvgRxPktNum
 * PSID          : 2782 (0x0ADE)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  It provides average num of rx frames for a particular session id.
 *  (indexed by unifiTWTSessTableIndex).
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TWT_AVG_RX_PKT_NUM 0x0ADE

/*******************************************************************************
 * NAME          : UnifiTwtAvgTxPktSize
 * PSID          : 2783 (0x0ADF)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  It provides average packet size(bytes) for a particular session id.
 *  (indexed by unifiTWTSessTableIndex).
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TWT_AVG_TX_PKT_SIZE 0x0ADF

/*******************************************************************************
 * NAME          : UnifiTwtAvgRxPktSize
 * PSID          : 2784 (0x0AE0)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  It provides average packet size(bytes) for a particular session id.
 *  (indexed by unifiTWTSessTableIndex).
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TWT_AVG_RX_PKT_SIZE 0x0AE0

/*******************************************************************************
 * NAME          : UnifiTwtAvgEarlySpTerm
 * PSID          : 2785 (0x0AE1)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  It provides average SP early termination on particular session id.
 *  (indexed by unifiTWTSessTableIndex).
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TWT_AVG_EARLY_SP_TERM 0x0AE1

/*******************************************************************************
 * NAME          : UnifiTwtClearStats
 * PSID          : 2786 (0x0AE2)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 7
 * DEFAULT       :
 * DESCRIPTION   :
 *  Clear TWT stats of all or particular session id
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TWT_CLEAR_STATS 0x0AE2

/*******************************************************************************
 * NAME          : Unifi5GEnablePrescan
 * PSID          : 2791 (0x0AE7)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Enables prescan of 5 GHz channels using scan accelerator.
 *******************************************************************************/
#define SLSI_PSID_UNIFI5_GENABLE_PRESCAN 0x0AE7

/*******************************************************************************
 * NAME          : Unifi5GScanAccTimeoutDur
 * PSID          : 2792 (0x0AE8)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 220
 * DEFAULT       : 140
 * DESCRIPTION   :
 *  Scan Accelerator Timeout duration in TUs
 *******************************************************************************/
#define SLSI_PSID_UNIFI5_GSCAN_ACC_TIMEOUT_DUR 0x0AE8

/*******************************************************************************
 * NAME          : UnifiUserControlBaSession
 * PSID          : 2799 (0x0AEF)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 9
 * MAX           : 9
 * DEFAULT       : { 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00}
 * DESCRIPTION   :
 *  User control block ack session parameters octet 0 - action to be
 *  performed, tid and block ack session role bit [2:0] : action to be
 *  performed. if set to 0: add a block ack session if set to 1: delete a
 *  block ack session if set to 2: force reject incoming block ack session if
 *  set to 3: can initiate a block ack session if set to 4: can accept an
 *  incoming block ack session bit [7:3] : unused octet 1 - tid bit [2:0] :
 *  tid if set to 0: tid 0 if set to 1: tid 1 if set to 2: tid 2 if set to 3:
 *  tid 3 if set to 4: tid 4 if set to 5: tid 5 if set to 6: tid 6 if set to
 *  7: tid 7 bit [7:3] : unused octet 2 - block ack session role bit 0 : if
 *  set to 0: block ack initiator if set to 1: block ack responder bit [7:1]
 *  : unused octet 3-8 - peer MAC address
 *******************************************************************************/
#define SLSI_PSID_UNIFI_USER_CONTROL_BA_SESSION 0x0AEF

/*******************************************************************************
 * NAME          : UnifiMlmestaKeepAliveFailure
 * PSID          : 2842 (0x0B1A)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Link lost by keep alive failure. 0 = Disabled. This is required by MCD.
 *  This value is read only once when an interface is added.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MLMESTA_KEEP_ALIVE_FAILURE 0x0B1A

/*******************************************************************************
 * NAME          : UnifiMiscFeaturesActivated
 * PSID          : 2880 (0x0B40)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 18
 * MAX           : 18
 * DEFAULT       :
 * DESCRIPTION   :
 *  This MIB is for Query Supported Feature Set support. This is a read-only
 *  bitmap. This attribute is set from the platform/hw/code capabilities
 *  using HCF. Refer to SC-510742-ME for further details.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MISC_FEATURES_ACTIVATED 0x0B40

/*******************************************************************************
 * NAME          : UnifiAppendixVersions
 * PSID          : 2881 (0x0B41)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 4
 * MAX           : 4
 * DEFAULT       :
 * DESCRIPTION   :
 *  This MIB is for Query Supported Feature Set support. This is a read-only
 *  octetmap. This attribute is set from the platform/hw/code capabilities
 *  using HCF. Refer to SC-510742-ME for further details.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_APPENDIX_VERSIONS 0x0B41

/*******************************************************************************
 * NAME          : UnifiSoftApHostRequestedBandwidth
 * PSID          : 2882 (0x0B42)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Golden Certification MIB don't delete, for certification Tests APs
 *  bandwidth must not be increased to max supported BW, this mib takes care
 *  of this during certification
 *******************************************************************************/
#define SLSI_PSID_UNIFI_SOFT_AP_HOST_REQUESTED_BANDWIDTH 0x0B42

/*******************************************************************************
 * NAME          : UnifiContentionTime
 * PSID          : 2883 (0x0B43)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  Contention time
 *******************************************************************************/
#define SLSI_PSID_UNIFI_CONTENTION_TIME 0x0B43

/*******************************************************************************
 * NAME          : UnifiLocalPacketCaptureMode
 * PSID          : 2884 (0x0B44)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Local packet capture feature state 0 - Disabled 1 - Enabled
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LOCAL_PACKET_CAPTURE_MODE 0x0B44

/*******************************************************************************
 * NAME          : UnifiTpcUseAfterIpv4Set
 * PSID          : 2885 (0x0B45)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Use TPC only after IPV4 has been set by the Host to ensure DHCP exchange
 *  is complete. Otherwise, extra NULL frames will be transmitted during the
 *  initial connection that could delay dynamic IP address allocation and
 *  result in disconnection. This value is read only once when an interface
 *  is added.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TPC_USE_AFTER_IPV4_SET 0x0B45

/*******************************************************************************
 * NAME          : UnifiMheapReportMemoryUsageThreshold
 * PSID          : 2900 (0x0B54)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 16
 * DESCRIPTION   :
 *  For every change (specified by the threshold boundary) in the current
 *  heap usage, heap memory usage will get logged into UDI log. It's unit is
 *  KBytes. 0 value will disable the reporting.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MHEAP_REPORT_MEMORY_USAGE_THRESHOLD 0x0B54

/*******************************************************************************
 * NAME          : UnifiMheapFreeMemory
 * PSID          : 2901 (0x0B55)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  It returns the free heap memory space available.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MHEAP_FREE_MEMORY 0x0B55

/*******************************************************************************
 * NAME          : UnifiPeerRxRateStats
 * PSID          : 2905 (0x0B59)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  Peer Rx packet counter per data rate
 *******************************************************************************/
#define SLSI_PSID_UNIFI_PEER_RX_RATE_STATS 0x0B59

/*******************************************************************************
 * NAME          : UnifiPeerTxRateStats
 * PSID          : 2906 (0x0B5A)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  Peer Tx packet counter per data rate
 *******************************************************************************/
#define SLSI_PSID_UNIFI_PEER_TX_RATE_STATS 0x0B5A

/*******************************************************************************
 * NAME          : UnifiPeerInfoUpdateInterval
 * PSID          : 2907 (0x0B5B)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 500
 * MAX           : 10000
 * DEFAULT       : 1000
 * DESCRIPTION   :
 *  A peer update interval in milliseconds
 *******************************************************************************/
#define SLSI_PSID_UNIFI_PEER_INFO_UPDATE_INTERVAL 0x0B5B

/*******************************************************************************
 * NAME          : UnifiIgmpGuardInterval
 * PSID          : 2908 (0x0B5C)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 5
 * MAX           : 1000
 * DEFAULT       : 10
 * DESCRIPTION   :
 *  Guard interval against flooding of requests in milliseconds
 *******************************************************************************/
#define SLSI_PSID_UNIFI_IGMP_GUARD_INTERVAL 0x0B5C

/*******************************************************************************
 * NAME          : UnifiEhtActivated
 * PSID          : 2921 (0x0B69)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Enables EHT Caps and Ops IE inclusion.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_EHT_ACTIVATED 0x0B69

/*******************************************************************************
 * NAME          : UnifiEhtCapabilities
 * PSID          : 2922 (0x0B6A)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 14
 * MAX           : 32
 * DEFAULT       : {0X22, 0X00, 0XC0, 0X0D, 0X00, 0X00, 0X00, 0X30, 0X00, 0X00, 0X00, 0X22, 0X00, 0X00, 0X22, 0X00, 0X00}
 * DESCRIPTION   :
 *  EHT capabilities of chip. This includes EHT MAC capabilities information,
 *  EHT PHY capabilities information, Supported EHT-MCS and NSS set, PPE
 *  thresholds(optional) fields.see SC-XXXXXX-SP.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_EHT_CAPABILITIES 0x0B6A

/*******************************************************************************
 * NAME          : UnifiMultilinkNumberOfLinks
 * PSID          : 2923 (0x0B6B)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 1
 * MAX           : 3
 * DEFAULT       : 2
 * DESCRIPTION   :
 *  Number of Maximum amount of links, we can support for MLO operation
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MULTILINK_NUMBER_OF_LINKS 0x0B6B

/*******************************************************************************
 * NAME          : UnifiBasicMultiLinkCapabilities
 * PSID          : 2924 (0x0B6C)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 9
 * MAX           : 32
 * DEFAULT       :
 * DESCRIPTION   :
 *  Basic Multilink MLO IE (TID to mapping disabled, MAC address Bcast)
 *******************************************************************************/
#define SLSI_PSID_UNIFI_BASIC_MULTI_LINK_CAPABILITIES 0x0B6C

/*******************************************************************************
 * NAME          : UnifiTidToLinkMapping
 * PSID          : 2925 (0x0B6D)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 1
 * MAX           : 3
 * DEFAULT       : {0X00, 0X00, 0X00}
 * DESCRIPTION   :
 *  Each octet represent link ID and mapping for AC, Max 3 links. Bit 3 to
 *  Bit 7 -> link_id Bit 0 to Bit 3 -> AC bitmap. Bit 0 -> BE Bit 1 -> BK Bit
 *  2 -> Video Bit 3 -> Voice So if BK and BE are mapped to link_id1 then
 *  "link_id1 , Bitmap for link1" will be 0x13. Video and voice are mapped to
 *  link_id2 then "link_id2 , Bitmap for link2" will be 0x2C. BE and voice
 *  are mapped to link_id3 then "link_id3 , Bitmap for link3" bitmap for
 *  link3 will be 0x39. By default all ACs are mapped to all active links.
 *  Set 2925=[00] to reset/disable tid2link mapping.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TID_TO_LINK_MAPPING 0x0B6D

/*******************************************************************************
 * NAME          : UnifiEhtActivatedSoftAp
 * PSID          : 2926 (0x0B6E)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Enables EHT mode for soft AP.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_EHT_ACTIVATED_SOFT_AP 0x0B6E

/*******************************************************************************
 * NAME          : UnifiBasicMultiLinkCapabilitiesSoftAp
 * PSID          : 2927 (0x0B6F)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 10
 * MAX           : 32
 * DEFAULT       :
 * DESCRIPTION   :
 *  Basic Multilink MLO IE (TID to mapping disabled, MAC address Bcast) for
 *  softAP
 *******************************************************************************/
#define SLSI_PSID_UNIFI_BASIC_MULTI_LINK_CAPABILITIES_SOFT_AP 0x0B6F

/*******************************************************************************
 * NAME          : UnifiEhtCapabilitiesSoftAp
 * PSID          : 2928 (0x0B70)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 14
 * MAX           : 32
 * DEFAULT       : {0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X20, 0X00, 0X00, 0X00, 0X22, 0X22, 0X00, 0X22, 0X22, 0X00}
 * DESCRIPTION   :
 *  EHT capabilities of Soft AP. This includes EHT MAC capabilities
 *  information, EHT PHY capabilities information, Supported EHT-MCS and NSS
 *  set, PPE thresholds(optional) fields.see SC-XXXXXX-SP.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_EHT_CAPABILITIES_SOFT_AP 0x0B70

/*******************************************************************************
 * NAME          : UnifiMultiLinkCapsEnabledSoftAp
 * PSID          : 2929 (0x0B71)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 4
 * DEFAULT       :
 * DESCRIPTION   :
 *  ML capabilities present like EMLSE, STR etc 0 - disable ML IE 1 - MLO
 *  caps without EMLSR/STR 2 - EMLSR Capabilities Present 3 - STR
 *  Capabilities Present 4 - EMLSR + STR Present
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MULTI_LINK_CAPS_ENABLED_SOFT_AP 0x0B71

/*******************************************************************************
 * NAME          : MaxNumofSimultaneousLinks
 * PSID          : 2950 (0x0B86)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 2
 * DEFAULT       : 1
 * DESCRIPTION   :
 *  Maximum number of links to support for STR operation
 *******************************************************************************/
#define SLSI_PSID_MAX_NUMOF_SIMULTANEOUS_LINKS 0x0B86

/*******************************************************************************
 * NAME          : EmlCapaPresent
 * PSID          : 2951 (0x0B87)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Whether EML CAPS to be included in ASSOCIATION.
 *******************************************************************************/
#define SLSI_PSID_EML_CAPA_PRESENT 0x0B87

/*******************************************************************************
 * NAME          : UnifiEmlsrOperation
 * PSID          : 2952 (0x0B88)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 2
 * DEFAULT       : 2
 * DESCRIPTION   :
 *  Set to send EML OMN frame. Set to 1/0 to Initiate/Exit EMLSR. Please also
 *  refer to unifiEMLSROperationEnum to define this value
 *******************************************************************************/
#define SLSI_PSID_UNIFI_EMLSR_OPERATION 0x0B88

/*******************************************************************************
 * NAME          : UnifiOdOverride
 * PSID          : 3001 (0x0BB9)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  Overdrive (OD) override control (REDWOOD only). bit 0: Force/Automatic;
 *  if set, the value of bit #1 will be used to enable/disable overdrive bit
 *  1: Force overdrive or not; Valid only when bit #0 is set. 1 to force
 *  overdrive enabled, 0 to force disabled. Possible use cases: 0: Automatic
 *  1: Forced OFF 3: Forced ON
 *******************************************************************************/
#define SLSI_PSID_UNIFI_OD_OVERRIDE 0x0BB9

/*******************************************************************************
 * NAME          : UnifiDtimMultiplier
 * PSID          : 3002 (0x0BBA)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * UNITS         : DTIM multipliers
 * MIN           : 0
 * MAX           : 9
 * DEFAULT       :
 * DESCRIPTION   :
 *  Number of DTIM multiplier for DTIM beacon skipping, which is controlled
 *  by Android U framework onwards. DTIM multiplier effects only in suspend
 *  mode. '0' indicates DTIM multiplier is not set so uses the maximum beacon
 *  listen interval specified with 'unifiListenIntervalMaxTime' to calculate
 *  number of DTIM beacon skipping. If it sets in range of 1~5 then DTIM
 *  beacon skipping will be calculated accordingly but still it will be
 *  limited not to be beyond maximum beacon listen interval.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_DTIM_MULTIPLIER 0x0BBA

/*******************************************************************************
 * NAME          : UnifiMacrameLpEarlyBeaconLength
 * PSID          : 3003 (0x0BBB)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  Early beacon termination length in bytes.If 0xFFFF TIM offset from beacon
 *  used else specified size is used, if set to 0 feature is disabled.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MACRAME_LP_EARLY_BEACON_LENGTH 0x0BBB

/*******************************************************************************
 * NAME          : UnifiStalpControl
 * PSID          : 3004 (0x0BBC)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       : 0X00000001
 * DESCRIPTION   :
 *  Control Station Low Power behaviour of idle,mifless and diet
 *  modes.Controlled as a bit mask. Refer to unifiSTALPControlBits for the
 *  full set of bit masks. b'0: enable STA Idle mode b'1: Enable DUAL STA
 *  Idle mode b'2: Enable STA Idle mode with LCD ON b'3: Enable mifless STA
 *  v1(legacy mifless mode) b'4: Enable mifless STA keepalive TX b'5: Enable
 *  STA diet mode b'6: Disable STA diet mode if BT active
 *******************************************************************************/
#define SLSI_PSID_UNIFI_STALP_CONTROL 0x0BBC

/*******************************************************************************
 * NAME          : UnifiRadioOnTimeStation
 * PSID          : 3100 (0x0C1C)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  Get radio on time ms value for vif-specific
 *******************************************************************************/
#define SLSI_PSID_UNIFI_RADIO_ON_TIME_STATION 0x0C1C

/*******************************************************************************
 * NAME          : UnifiRadioTxTimeStation
 * PSID          : 3101 (0x0C1D)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  Get radio tx time ms value for vif-specific
 *******************************************************************************/
#define SLSI_PSID_UNIFI_RADIO_TX_TIME_STATION 0x0C1D

/*******************************************************************************
 * NAME          : UnifiRadioRxTimeStation
 * PSID          : 3102 (0x0C1E)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  Get radio rx time ms value for vif-specific
 *******************************************************************************/
#define SLSI_PSID_UNIFI_RADIO_RX_TIME_STATION 0x0C1E

/*******************************************************************************
 * NAME          : UnifiCcaBusyTimeStation
 * PSID          : 3103 (0x0C1F)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  Get CCA Busy Time for vif-specific
 *******************************************************************************/
#define SLSI_PSID_UNIFI_CCA_BUSY_TIME_STATION 0x0C1F

/*******************************************************************************
 * NAME          : UnifiAcSuccess
 * PSID          : 3104 (0x0C20)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  It represents the number of success frames under each ac priority
 *  (indexed by unifiAccessClassIndex). This number will wrap to zero after
 *  the range is exceeded.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_AC_SUCCESS 0x0C20

/*******************************************************************************
 * NAME          : UnifiCsrOnlyMibShield
 * PSID          : 4001 (0x0FA1)
 * PER INTERFACE?: NO
 * TYPE          : unifiCSROnlyMIBShield
 * MIN           : 0
 * MAX           : 255
 * DEFAULT       : 2
 * DESCRIPTION   :
 *  Each element of the MIB has a set of read/write access constraints that
 *  may be applied when the element is accessed by the host. For most
 *  elements the constants are derived from their MAX-ACCESS clauses.
 *  unifiCSROnlyMIBShield controls the access mechanism. If this entry is set
 *  to &apos;warn&apos;, when the host makes an inappropriate access to a MIB
 *  variable (e.g., writing to a &apos;read-only&apos; entry) then the
 *  firmware attempts to send a warning message to the host, but access is
 *  allowed to the MIB variable. If this entry is set to &apos;guard&apos;
 *  then inappropriate accesses from the host are prevented. If this entry is
 *  set to &apos;alarm&apos; then inappropriate accesses from the host are
 *  prevented and the firmware attempts to send warning messages to the host.
 *  If this entry is set to &apos;open&apos; then no access constraints are
 *  applied and now warnings issued. Note that certain MIB entries have
 *  further protection schemes. In particular, the MIB prevents the host from
 *  reading some security keys (WEP keys, etc.).
 *******************************************************************************/
#define SLSI_PSID_UNIFI_CSR_ONLY_MIB_SHIELD 0x0FA1

/*******************************************************************************
 * NAME          : UnifiPrivateBbbTxFilterConfig
 * PSID          : 4071 (0x0FE7)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 0X17
 * DESCRIPTION   :
 *  entry is written directly to the BBB_TX_FILTER_CONFIG register. Only the
 *  lower eight bits of this register are implemented . Bits 0-3 are the
 *  &apos;Tx Gain&apos;, bits 6-8 are the &apos;Tx Delay&apos;. This register
 *  should only be changed by an expert.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_PRIVATE_BBB_TX_FILTER_CONFIG 0x0FE7

/*******************************************************************************
 * NAME          : UnifiPrivateSwagcFrontEndGain
 * PSID          : 4075 (0x0FEB)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt16
 * MIN           : -128
 * MAX           : 127
 * DEFAULT       :
 * DESCRIPTION   :
 *  Gain of the path between chip and antenna when LNA is on.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_PRIVATE_SWAGC_FRONT_END_GAIN 0x0FEB

/*******************************************************************************
 * NAME          : UnifiPrivateSwagcFrontEndLoss
 * PSID          : 4076 (0x0FEC)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt16
 * MIN           : -128
 * MAX           : 127
 * DEFAULT       :
 * DESCRIPTION   :
 *  Loss of the path between chip and antenna when LNA is off.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_PRIVATE_SWAGC_FRONT_END_LOSS 0x0FEC

/*******************************************************************************
 * NAME          : UnifiPrivateSwagcExtThresh
 * PSID          : 4077 (0x0FED)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt16
 * MIN           : -128
 * MAX           : 127
 * DEFAULT       : -25
 * DESCRIPTION   :
 *  Signal level at which external LNA will be used for AGC purposes.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_PRIVATE_SWAGC_EXT_THRESH 0x0FED

/*******************************************************************************
 * NAME          : UnifiCsrOnlyPowerCalDelay
 * PSID          : 4078 (0x0FEE)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * UNITS         : microseconds
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  Delay applied at each step of the power calibration routine used with an
 *  external PA.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_CSR_ONLY_POWER_CAL_DELAY 0x0FEE

/*******************************************************************************
 * NAME          : UnifiRxAgcControl
 * PSID          : 4079 (0x0FEF)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 9
 * MAX           : 18
 * DEFAULT       :
 * DESCRIPTION   :
 *  Override the AGC by adjusting the Rx minimum and maximum gains of each
 *  stage. Set requests write the values to a static structure in firmware.
 *  The saved values are written to hardware, and also used to configure the
 *  AGC whenever halradio_agc_setup() is called. Get requests read the values
 *  from the static structure in firmware. AGC enables are not altered. Fixed
 *  gain may be tested by setting the minimums and maximums to the same
 *  value. Version 1 (for rfchip jar - not supported on this firmware branch)
 *  Version 2 (for rfchip hopper, lark) octet 0 - Version number = 2. Gain
 *  values. Default in brackets. octet 1 - 5G FE minimum gain (hopper 1, lark
 *  0). octet 2 - 5G FE maximum gain (8). octet 3 - 2G FE minimum gain (0).
 *  octet 4 - 2G FE maximum gain (8). octet 5 - ABB minimum gain (1). octet 6
 *  - ABB maximum gain (9). octet 7 - Digital minimum gain (depends on bb
 *  chip). octet 8 - Digital maximum gain (depends on bb chip). Version 3
 *  (for rfchip leopard with 6G compatible and ax capable BBIC) octet 0 -
 *  Version number = 3. octet 1 - AGC operation mode (0) octet 2 - 2G FE
 *  minimum gain (0). octet 3 - 2G FE maximum gain (8). octet 4 - 5G FE
 *  minimum gain (0). octet 5 - 5G FE maximum gain (8). octet 6 - 6G FE
 *  minimum gain (0). octet 7 - 6G FE maximum gain (8). octet 8 - ABB minimum
 *  gain (1). octet 9 - ABB maximum gain (9). octet 10 - Digital minimum gain
 *  (depends on bb chip). octet 11 - Digital maximum gain (depends on bb
 *  chip). octet 12 - FE maximum Nudge gain. octet 13 - free run extra gain.
 *  octet 14-15 - Free run high limit RSSI. octet 16-17 - Free run low limit
 *  RSSI.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_RX_AGC_CONTROL 0x0FEF

/*******************************************************************************
 * NAME          : UnifiDebugAdditionalOffChipMemory
 * PSID          : 4090 (0x0FFA)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Use some additional memory for logging debug messages CAUTION: May impact
 *  system performance.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_DEBUG_ADDITIONAL_OFF_CHIP_MEMORY 0x0FFA

/*******************************************************************************
 * NAME          : UnifiWmmStallEnable
 * PSID          : 4139 (0x102B)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 1
 * DESCRIPTION   :
 *  Golden Certification MIB don't delete, change PSID or name: Enable
 *  workaround stall WMM traffic if the admitted time has been used up, used
 *  for certtification.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_WMM_STALL_ENABLE 0x102B

/*******************************************************************************
 * NAME          : UnifiDeactivateInessentialDebug
 * PSID          : 4140 (0x102C)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Allow Deactivate Inessential debug in high TRx throughput condition -
 *  default value is false.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_DEACTIVATE_INESSENTIAL_DEBUG 0x102C

/*******************************************************************************
 * NAME          : UnifiRaaTxHostRate
 * PSID          : 4148 (0x1034)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  Fixed TX rate set by Host. Ideally this should be done by the driver. 0
 *  means "host did not specified any rate".
 *******************************************************************************/
#define SLSI_PSID_UNIFI_RAA_TX_HOST_RATE 0x1034

/*******************************************************************************
 * NAME          : UnifiFallbackShortFrameRetryDistribution
 * PSID          : 4149 (0x1035)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 6
 * MAX           : 5
 * DEFAULT       : {0X3, 0X2, 0X2, 0X2, 0X1, 0X0}
 * DESCRIPTION   :
 *  Configure the retry distribution for fallback for short frames octet 0 -
 *  Number of retries for starting rate. octet 1 - Number of retries for next
 *  rate. octet 2 - Number of retries for next rate. octet 3 - Number of
 *  retries for next rate. octet 4 - Number of retries for next rate. octet 5
 *  - Number of retries for last rate. If 0 is written to an entry then the
 *  retries for that rate will be the short retry limit minus the sum of the
 *  retries for each rate above that entry (e.g. 15 - 5). Therefore, this
 *  should always be the value for octet 4. Also, when the starting rate has
 *  short guard enabled, the number of retries in octet 1 will be used and
 *  for the next rate in the fallback table (same MCS value, but with sgi
 *  disabled) octet 0 number of retries will be used.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_FALLBACK_SHORT_FRAME_RETRY_DISTRIBUTION 0x1035

/*******************************************************************************
 * NAME          : UnifiThroughputLow
 * PSID          : 4150 (0x1036)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       : 37500000
 * DESCRIPTION   :
 *  Lower threshold for number of bytes received in a second - default value
 *  based on 300Mbps
 *******************************************************************************/
#define SLSI_PSID_UNIFI_THROUGHPUT_LOW 0x1036

/*******************************************************************************
 * NAME          : UnifiThroughputHigh
 * PSID          : 4151 (0x1037)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       : 50000000
 * DESCRIPTION   :
 *  Upper threshold for number of bytes received in a second - default value
 *  based on 400Mbps
 *******************************************************************************/
#define SLSI_PSID_UNIFI_THROUGHPUT_HIGH 0x1037

/*******************************************************************************
 * NAME          : UnifiSetFixedAmpduAggregationSize
 * PSID          : 4152 (0x1038)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  A non 0 value defines the max number of mpdus that a ampdu can have. A 0
 *  value tells FW to use half the current BA window
 *******************************************************************************/
#define SLSI_PSID_UNIFI_SET_FIXED_AMPDU_AGGREGATION_SIZE 0x1038

/*******************************************************************************
 * NAME          : UnifiThroughputDebugReportInterval
 * PSID          : 4153 (0x1039)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 1000
 * DESCRIPTION   :
 *  dataplane reports throughput diag report every this interval in msec. 0
 *  means to disable this report.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_THROUGHPUT_DEBUG_REPORT_INTERVAL 0x1039

/*******************************************************************************
 * NAME          : UnifiDplaneTest1
 * PSID          : 4154 (0x103A)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  Dplane test mib to read and write uint32
 *******************************************************************************/
#define SLSI_PSID_UNIFI_DPLANE_TEST1 0x103A

/*******************************************************************************
 * NAME          : UnifiDplaneTest2
 * PSID          : 4155 (0x103B)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  Dplane test mib to read and write uint32
 *******************************************************************************/
#define SLSI_PSID_UNIFI_DPLANE_TEST2 0x103B

/*******************************************************************************
 * NAME          : UnifiDplaneTest3
 * PSID          : 4156 (0x103C)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  Dplane test mib to read and write uint32
 *******************************************************************************/
#define SLSI_PSID_UNIFI_DPLANE_TEST3 0x103C

/*******************************************************************************
 * NAME          : UnifiDplaneTest4
 * PSID          : 4157 (0x103D)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  Dplane test mib to read and write uint32
 *******************************************************************************/
#define SLSI_PSID_UNIFI_DPLANE_TEST4 0x103D

/*******************************************************************************
 * NAME          : UnifiEnableCopyPhyLogToDram
 * PSID          : 4158 (0x103E)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  Enable or disable copying phy event logs in SRAM to DRAM
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ENABLE_COPY_PHY_LOG_TO_DRAM 0x103E

/*******************************************************************************
 * NAME          : UnifiLaaFixedMaxBandwidth
 * PSID          : 4159 (0x103F)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  Fix maximum allowed bandwidth for transmission. - 20: allowed up to 20MHz
 *  - 40: allowed up to 40MHz - 80: allowed up to 80MHz - others: maximum
 *  bandwidth is not fixed
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LAA_FIXED_MAX_BANDWIDTH 0x103F

/*******************************************************************************
 * NAME          : UnifiDplaneDeafDetection
 * PSID          : 4160 (0x1040)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  Configuration for Deaf detection algorithm. This feature only becomes
 *  enabled when it is FlexiMac and FM_DEAF_DETECTION flag is set. Usage:
 *  octet 0 - the number of consecutive cts without data frames to detect as
 *  deafness octet 1 - the number of consecutive rts without cts to detect as
 *  deafness octet 2 and 3 are not used 0 means disabling it.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_DPLANE_DEAF_DETECTION 0x1040

/*******************************************************************************
 * NAME          : UnifiLaaLocalBwControlEnable
 * PSID          : 4161 (0x1041)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  On the platform which supports secondary channel monitoring such as
 *  FlexiMAC, LAA can automatically adjust maximum allowed bandwidth to avoid
 *  long-delayed transmission due to busy secondary channels.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LAA_LOCAL_BW_CONTROL_ENABLE 0x1041

/*******************************************************************************
 * NAME          : UnifiLaaLocalBwBusyWeigh
 * PSID          : 4162 (0x1042)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  Weigh for busy secondary channel in LAA local BW control algorithm. 0
 *  means this value is not applied to firmware.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LAA_LOCAL_BW_BUSY_WEIGH 0x1042

/*******************************************************************************
 * NAME          : UnifiLaaLocalBwBusyLevelThres
 * PSID          : 4163 (0x1043)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  Busy level threshold by which LAA local BW control drops bandwidth. 0
 *  means this value is not applied to firmware.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LAA_LOCAL_BW_BUSY_LEVEL_THRES 0x1043

/*******************************************************************************
 * NAME          : UnifiLaaMinMcsForBwChange
 * PSID          : 4164 (0x1044)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 1
 * DESCRIPTION   :
 *  LAA considers to drop bandwidth when MCS drops and hits the configured
 *  value.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LAA_MIN_MCS_FOR_BW_CHANGE 0x1044

/*******************************************************************************
 * NAME          : UnifiDphpHwMonitorLogEnable
 * PSID          : 4165 (0x1045)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  Enable/Disable DPHP HW Monitor Log
 *******************************************************************************/
#define SLSI_PSID_UNIFI_DPHP_HW_MONITOR_LOG_ENABLE 0x1045

/*******************************************************************************
 * NAME          : UnifiPhyEventToUdiMinInterval
 * PSID          : 4166 (0x1046)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * UNITS         : milliseconds
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 5
 * DESCRIPTION   :
 *  Minimal gap in msec from the last PHY Event Logs to UDI. PHY logs are
 *  generated only when the gap from the last logging is larger than MIB
 *  value.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_PHY_EVENT_TO_UDI_MIN_INTERVAL 0x1046

/*******************************************************************************
 * NAME          : UnifiPhyEventToUdiMinBeaconMissInRow
 * PSID          : 4167 (0x1047)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 5
 * DESCRIPTION   :
 *  Minimal beacon misses in a row to start PHY logging on beacon misses.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_PHY_EVENT_TO_UDI_MIN_BEACON_MISS_IN_ROW 0x1047

/*******************************************************************************
 * NAME          : UnifiPhyEventToUdiConfig
 * PSID          : 4168 (0x1048)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 0X003C
 * DESCRIPTION   :
 *  Configure events that trigger sending PHY Event logs to UDI This feature
 *  is enabled when DPHP_DEBUG_PHY_ENABLE is defined. Bitmap for configuring
 *  the trigger condition: 0x0000 : disabled 0x0001 : periodically 0x0002 :
 *  tx_failure 0x0004 : beacon_missed 0x0008 : live_restart 0x0010 :
 *  basf_failure 0x0020 : active_dump
 *******************************************************************************/
#define SLSI_PSID_UNIFI_PHY_EVENT_TO_UDI_CONFIG 0x1048

/*******************************************************************************
 * NAME          : UnifiPhyEventToUdiInterval
 * PSID          : 4169 (0x1049)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * UNITS         : milliseconds
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 1000
 * DESCRIPTION   :
 *  Interval (milliseconds) to send PHY Event Logs to UDI Trigger condition
 *  is controlled by unifiPhyEventToUDIControl This feature is enabled when
 *  DPHP_DEBUG_PHY_ENABLE is defined.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_PHY_EVENT_TO_UDI_INTERVAL 0x1049

/*******************************************************************************
 * NAME          : UnifiMonitorStaAddress
 * PSID          : 4170 (0x104A)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 6
 * MAX           : 6
 * DEFAULT       : { 0X00, 0X00, 0X00, 0X00, 0X00, 0X00 }
 * DESCRIPTION   :
 *  MAC address of a target station to be captured in monitor (sniffer) mode.
 *  Monitor mode captures all received frames on a best-effort basis but
 *  there are certain frames which require pre-configuration before
 *  capturing. This feature will allow to capture HE-MU OFDMA frame by
 *  setting corresponding AID to the target station. Monitor vif uses this
 *  target station address to retrieve AID from AID-contained-frames
 *  automatically, e.g. assoc resp and NDPA.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MONITOR_STA_ADDRESS 0x104A

/*******************************************************************************
 * NAME          : UnifiPreEbrtWindow
 * PSID          : 4171 (0x104B)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * UNITS         : microseconds
 * MIN           : 0
 * MAX           : 2147483647
 * DEFAULT       : 100
 * DESCRIPTION   :
 *  Latest time before the expected beacon reception time that UniFi will
 *  turn on its radio in order to receive the beacon. Reducing this value can
 *  reduce UniFi power consumption when using low power modes, however a
 *  value which is too small may cause beacons to be missed, requiring the
 *  radio to remain on for longer periods to ensure reception of the
 *  subsequent beacon.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_PRE_EBRT_WINDOW 0x104B

/*******************************************************************************
 * NAME          : UnifiMonitorFrequencyConfig
 * PSID          : 4172 (0x104C)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 1
 * MAX           : 16
 * DEFAULT       :
 * DESCRIPTION   :
 *  Configure the target frequency information to capture in monitor
 *  (sniffer) mode. This feature enables to configure the target frequency
 *  directly to WLAN firmware w/o iw command. Usage: Octet 0 : Option type
 *  and the number of target multi links. - 1st 4 bits - Option type 0: Add
 *  monitor vif 1: Delete monitor vif 2: Set frequency config of monitor vifs
 *  - 2nd 4 bitsIt - The number of target multi links. It supports maximum 3
 *  multi links. Octet 1 ~ 5 : (Optional) The frequency information of the
 *  1st target link. Octet 1 ~ 2 - The doubled target center frequency of
 *  bonding channel. Octet 3 - The position of primary channel frequency (0 ~
 *  15). Octet 4 - The target bonding bandwidth in MHz. Octet 5 - The
 *  addtitional frequency segment information. Octet 6 ~ 10 : (Optional) The
 *  frequency information of the 2nd target link. Octet 11 ~ 15 : (Optional)
 *  The frequency information of the 3rd target link.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MONITOR_FREQUENCY_CONFIG 0x104C

/*******************************************************************************
 * NAME          : UnifiPostEbrtWindow
 * PSID          : 4173 (0x104D)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * UNITS         : microseconds
 * MIN           : 0
 * MAX           : 2147483647
 * DEFAULT       : 5000
 * DESCRIPTION   :
 *  Minimum time after the expected normal mode beacon reception time that
 *  UniFi will continue to listen for the beacon in an infrastructure BSS
 *  before timing out. Reducing this value can reduce UniFi power consumption
 *  when using low power modes, however a value which is too small may cause
 *  beacons to be missed, requiring the radio to remain on for longer periods
 *  to ensure reception of the subsequent beacon.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_POST_EBRT_WINDOW 0x104D

/*******************************************************************************
 * NAME          : UnifiLiveRestartRssiThreshold
 * PSID          : 4174 (0x104E)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt16
 * MIN           : -32768
 * MAX           : 32767
 * DEFAULT       :
 * DESCRIPTION   :
 *  Minimum rssi threshold (dBm) to trigger live-restart If the RSSI is
 *  lower(poor) than this value, even if live-restart is requested by
 *  unifiDplaneDeafDetection, do not execute live-restart to avoid false
 *  alarm from environmental cause. This feature is disabled if set to 0 and
 *  only valid for STA VIFs.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LIVE_RESTART_RSSI_THRESHOLD 0x104E

/*******************************************************************************
 * NAME          : UnifiLiveRestartMinInterval
 * PSID          : 4175 (0x104F)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * UNITS         : ms
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  Minimum duration(ms) to execute live-restart from the previous restart.
 *  If elapsed time from previous live-restart is smaller than this value,
 *  even if live-restart is requested by unifiDplaneDeafDetection, do not
 *  execute live-restart to avoid redundant chatty alarm. This feature is
 *  disabled if set to 0.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LIVE_RESTART_MIN_INTERVAL 0x104F

/*******************************************************************************
 * NAME          : UnifiMonitorAid
 * PSID          : 4176 (0x1050)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 6
 * MAX           : 6
 * DEFAULT       :
 * DESCRIPTION   :
 *  AID of the target station to be captured in monitor (sniffer) mode.
 *  Monitor mode basically captures all received frames on a best-effort
 *  basis. However, some OFDMA frames, namely HE TB and EHT, require to have
 *  AID of the target station pre-configured. This mib indicates AID of the
 *  target station to be captured for this purpose. Usage: octet 0 ~ 1 - AID
 *  of the number of 1st monitor link at vif1 octet 2 ~ 3 - AID of the number
 *  of 2nd monitor link at vif2 octet 4 ~ 5 - AID of the number of 3rd
 *  monitor link at vif3 The number of monitor link is limited by radio
 *  capability of the product within maximum 3 links
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MONITOR_AID 0x1050

/*******************************************************************************
 * NAME          : UnifiPsPollThreshold
 * PSID          : 4179 (0x1053)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 30
 * DESCRIPTION   :
 *  PS Poll threshold. When Unifi chip is configured for normal power save
 *  mode and when access point does not respond to PS-Poll requests, then a
 *  fault will be generated on non-zero PS Poll threshold indicating mode has
 *  been switched from power save to fast power save. Ignored PS Poll count
 *  is given as the fault argument.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_PS_POLL_THRESHOLD 0x1053

/*******************************************************************************
 * NAME          : UnifiPostEbrtWindowIdle
 * PSID          : 4180 (0x1054)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * UNITS         : microseconds
 * MIN           : 0
 * MAX           : 2147483647
 * DEFAULT       : 2000
 * DESCRIPTION   :
 *  Minimum time after the expected idle mode beacon reception time that
 *  UniFi will continue to listen for the beacon in an infrastructure BSS
 *  before timing out. Reducing this value can reduce UniFi power consumption
 *  when using low power modes, however a value which is too small may cause
 *  beacons to be missed, requiring the radio to remain on for longer periods
 *  to ensure reception of the subsequent beacon.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_POST_EBRT_WINDOW_IDLE 0x1054

/*******************************************************************************
 * NAME          : UnifiPreEbrtWindowIdle
 * PSID          : 4181 (0x1055)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * UNITS         : microseconds
 * MIN           : 0
 * MAX           : 2147483647
 * DEFAULT       : 100
 * DESCRIPTION   :
 *  Latest time before the expected idle mode beacon reception time that
 *  UniFi will turn on its radio in order to receive the beacon. Reducing
 *  this value can reduce UniFi power consumption when using low power modes,
 *  however a value which is too small may cause beacons to be missed,
 *  requiring the radio to remain on for longer periods to ensure reception
 *  of the subsequent beacon.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_PRE_EBRT_WINDOW_IDLE 0x1055

/*******************************************************************************
 * NAME          : UnifiRecycleAggregratedMpdUs
 * PSID          : 4182 (0x1056)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Dataplane debug mode where MPDU metadata is recycled and frames aren't
 *  returned to the host
 *******************************************************************************/
#define SLSI_PSID_UNIFI_RECYCLE_AGGREGRATED_MPD_US 0x1056

/*******************************************************************************
 * NAME          : UnifiSableContainerSizeConfiguration
 * PSID          : 5000 (0x1388)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 100
 * DEFAULT       :
 * DESCRIPTION   :
 *  Sable Container Size Configuration Sable WLAN reserved memory size is
 *  determined by the host. Sable TLV containers are allocated from this WLAN
 *  reserved area. Each container has different requirement on its size. For
 *  example, frame logging or IQ capture would be very greedy, requesting
 *  most of available memroy. But some just need fixed size, but not large.
 *  To cope with such requirements, each container size is configured with
 *  the following rules: 1. To allocate a certain percentage of the whole
 *  wlan reserved area, put the percentage in hex format. For example,
 *  0x28(=40) means 40% of reserved area will be assigned. The number
 *  0x64(=100) is specially treated that all remaining space will be assigned
 *  after all the other containers are first served. 2. To request (n * 2048)
 *  bytes, put (100 + n) value in hex format. For example, 0x96 (= 150) means
 *  50 * 2048 = 102400 bytes. 3. If this regions is shared with MBULK POOL
 *  DRAM, the index should be at the last. Here are the list of containers: -
 *  Index 1 : WTLV_CONTAINER_ID_DPLANE_FRAME_LOG - Index 2 :
 *  WTLV_CONTAINER_ID_RX_BUFFER_POOL_FOR_MBULK
 *******************************************************************************/
#define SLSI_PSID_UNIFI_SABLE_CONTAINER_SIZE_CONFIGURATION 0x1388

/*******************************************************************************
 * NAME          : UnifiSableFrameLogMode
 * PSID          : 5001 (0x1389)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 2
 * DEFAULT       : 2
 * DESCRIPTION   :
 *  Sable Frame Logging mode - 0: disable frame logging - 1: enable frame
 *  logging(LPC) always, regardless of CPU resource state - 2: dynamically
 *  enable frame logging base on CPU resource. If CPU too busy, frame logging
 *  is disabled. Logging is enabled when CPU resource gets recovered.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_SABLE_FRAME_LOG_MODE 0x1389

/*******************************************************************************
 * NAME          : UnifiSableFrameLogCpuThresPercent
 * PSID          : 5002 (0x138A)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 100
 * DEFAULT       : 95
 * DESCRIPTION   :
 *  CPU target in percent. When CPU usage is higher than this target, frame
 *  logging will be disabled by firmware. Firmware will check if CPU resource
 *  is recovered every 1 second. If CPU resource recovered, then frame
 *  logging is re-enabled.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_SABLE_FRAME_LOG_CPU_THRES_PERCENT 0x138A

/*******************************************************************************
 * NAME          : UnifiSableFrameLogCpuOverheadPercent
 * PSID          : 5003 (0x138B)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 100
 * DEFAULT       : 3
 * DESCRIPTION   :
 *  Expected CPU overhead introduced by frame logging.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_SABLE_FRAME_LOG_CPU_OVERHEAD_PERCENT 0x138B

/*******************************************************************************
 * NAME          : UnifiDplaneSableTxDelayThresMs
 * PSID          : 5004 (0x138C)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  TX Delay threshold in msec to take action.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_DPLANE_SABLE_TX_DELAY_THRES_MS 0x138C

/*******************************************************************************
 * NAME          : UnifiDplaneSableTxDelayAction
 * PSID          : 5005 (0x138D)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  Action to take when TX delay is larger than
 *  unifiDplaneSableTxDelayThresMs. 0: do nothing 1: print delay in mxlog 2:
 *  trigger a panic
 *******************************************************************************/
#define SLSI_PSID_UNIFI_DPLANE_SABLE_TX_DELAY_ACTION 0x138D

/*******************************************************************************
 * NAME          : UnifiDebugD12DiscardBulk
 * PSID          : 5009 (0x1391)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  Control whether to discard the mbulk that's associated with debug
 *  signals. - 0: do not discard mbulk - 1: discard onchip mbulk only - 2:
 *  discard offchip mbulk only - 3: discard all (onchip and offchip) mbulk
 *******************************************************************************/
#define SLSI_PSID_UNIFI_DEBUG_D12_DISCARD_BULK 0x1391

/*******************************************************************************
 * NAME          : UnifiDebugSvcModeStackHighWaterMark
 * PSID          : 5010 (0x1392)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  Read the SVC mode stack high water mark in bytes
 *******************************************************************************/
#define SLSI_PSID_UNIFI_DEBUG_SVC_MODE_STACK_HIGH_WATER_MARK 0x1392

/*******************************************************************************
 * NAME          : UnifiPanicSubSystemControl
 * PSID          : 5026 (0x13A2)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 255
 * DEFAULT       :
 * DESCRIPTION   :
 *  PANIC levels for WLAN SubSystems. Panic level is used to filter Panic
 *  sent to the host. 4 levels of Panic per subsystem are available
 *  (FAILURE_LEVEL_T): a. 0 FATAL - Always reported to host b. 1 ERROR c. 2
 *  WARNING d. 3 DEBUG NOTE: If Panic level of a subsystem is configured to
 *  FATAL, all the Panics within that subsystem configured to FATAL will be
 *  effective, panics with ERROR, WARNING and Debug level will be converted
 *  to faults. If Panic level of a subsystem is configured to WARNING, all
 *  the panics within that subsystem configured to FATAL, ERROR and WARNING
 *  will be issued to host, panics with Debug level will be converted to
 *  faults.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_PANIC_SUB_SYSTEM_CONTROL 0x13A2

/*******************************************************************************
 * NAME          : UnifiFaultEnable
 * PSID          : 5027 (0x13A3)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Send Fault to host state.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_FAULT_ENABLE 0x13A3

/*******************************************************************************
 * NAME          : UnifiFaultSubSystemControl
 * PSID          : 5028 (0x13A4)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 255
 * DEFAULT       :
 * DESCRIPTION   :
 *  Fault levels for WLAN SubSystems. Fault level is used to filter faults
 *  sent to the host. 4 levels of faults per subsystem are available
 *  (FAILURE_LEVEL_T): a. 0 ERROR b. 1 WARNING c. 2 INFO_1 d. 3 INFO_2
 *  Modifying Fault Levels at run time: 1. Set the fault level for the
 *  subsystems in unifiFaultConfigTable 2. Set unifiFaultEnable NOTE: If
 *  fault level of a subsystem is configured to ERROR, all the faults within
 *  that subsystem configured to ERROR will only be issued to host, faults
 *  with WARNING, INFO_1 and INFO_2 level will be converted to debug message
 *  If fault level of a subsystem is configured to WARNING, all the faults
 *  within that subsystem configured to ERROR and WARNING will be issued to
 *  host, faults with INFO_1 and INFO_2 level will be converted to debug
 *  message
 *******************************************************************************/
#define SLSI_PSID_UNIFI_FAULT_SUB_SYSTEM_CONTROL 0x13A4

/*******************************************************************************
 * NAME          : UnifiDebugModuleControl
 * PSID          : 5029 (0x13A5)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  Debug Module levels for all modules. Module debug level is used to filter
 *  debug messages sent to the host. Only 6 levels of debug per module are
 *  available: a. -1 No debug created. b. 0 Debug if compiled in. Should not
 *  cause Buffer Full in normal testing. c. 1 - 3 Levels to allow sensible
 *  setting of the .hcf file while running specific tests or debugging d. 4
 *  Debug will harm normal execution due to excessive levels or processing
 *  time required. Only used in emergency debugging. Additional control for
 *  FSM transition and FSM signals logging is provided. Debug module level
 *  and 2 boolean flags are encoded within a uint16: Function | Is sending
 *  FSM signals | Is sending FSM transitions | Is sending FSM Timers |
 *  Reserved | Module level (signed int)
 *  -_-_-_-_-_+-_-_-_-_-_-_-_-_-_-_-_-_-_+-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_+-_-_-_-_-_-_-_-_-_-_-_-_-_+-_-_-_-_-_-_+-_-_-_-_-_-_-_-_-_-_-_-_-_- Bits | 15 | 14 | 13 | 12 - 8 | 7 - 0 Note: 0x00FF disables any debug for a module 0xE004 enables all debug for a module
 *******************************************************************************/
#define SLSI_PSID_UNIFI_DEBUG_MODULE_CONTROL 0x13A5

/*******************************************************************************
 * NAME          : UnifiTxUsingLdpcActivated
 * PSID          : 5030 (0x13A6)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  LDPC will be used to code packets, for transmit only. If disabled, chip
 *  will not send LDPC coded packets even if peer supports it. To advertise
 *  reception of LDPC coded packets,enable bit 0 of unifiHtCapabilities, and
 *  bit 4 of unifiVhtCapabilities.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TX_USING_LDPC_ACTIVATED 0x13A6

/*******************************************************************************
 * NAME          : UnifiTxSettings
 * PSID          : 5031 (0x13A7)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 255
 * DEFAULT       :
 * DESCRIPTION   :
 *  Hardware specific transmitter settings
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TX_SETTINGS 0x13A7

/*******************************************************************************
 * NAME          : UnifiTxGainSettings
 * PSID          : 5032 (0x13A8)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 255
 * DEFAULT       :
 * DESCRIPTION   :
 *  Hardware specific transmitter gain settings
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TX_GAIN_SETTINGS 0x13A8

/*******************************************************************************
 * NAME          : UnifiTxSgI20Activated
 * PSID          : 5040 (0x13B0)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  SGI 20MHz will be used to code packets for transmit only. If disabled,
 *  chip will not send SGI 20MHz packets even if peer supports it. To
 *  advertise reception of SGI 20MHz packets, enable bit 5 of
 *  unifiHtCapabilities.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TX_SG_I20_ACTIVATED 0x13B0

/*******************************************************************************
 * NAME          : UnifiTxSgI40Activated
 * PSID          : 5041 (0x13B1)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  SGI 40MHz will be used to code packets, for transmit only. If disabled,
 *  chip will not send SGI 40MHz packets even if peer supports it. To
 *  advertise reception of SGI 40MHz packets, enable bit 6 of
 *  unifiHtCapabilities.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TX_SG_I40_ACTIVATED 0x13B1

/*******************************************************************************
 * NAME          : UnifiTxSgI80Activated
 * PSID          : 5042 (0x13B2)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  SGI 80MHz will be used to code packets, for transmit only. If disabled,
 *  chip will not send SGI 80MHz packets even if peer supports it. To
 *  advertise reception of SGI 80MHz packets, enable bit 5 of
 *  unifiVhtCapabilities.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TX_SG_I80_ACTIVATED 0x13B2

/*******************************************************************************
 * NAME          : UnifiTxSgI160Activated
 * PSID          : 5043 (0x13B3)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  SGI 160/80+80MHz will be used to code packets, for transmit only. If
 *  disabled, chip will not send SGI 160/80+80MHz packets even if peer
 *  supports it. To advertise reception of SGI 160/80+80MHz packets, enable
 *  bit 6 of unifiVhtCapabilities.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TX_SG_I160_ACTIVATED 0x13B3

/*******************************************************************************
 * NAME          : UnifiMacAddressRandomisation
 * PSID          : 5044 (0x13B4)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Golden Certification MIB don't delete, change PSID or name: Enabling Mac
 *  Address Randomisation to be applied for Probe Requests when scanning.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MAC_ADDRESS_RANDOMISATION 0x13B4

/*******************************************************************************
 * NAME          : UnifiMacAddressRandomisationMask
 * PSID          : 5047 (0x13B7)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 6
 * MAX           : 6
 * DEFAULT       : { 0X00, 0X00, 0X00, 0X00, 0X00, 0X00 }
 * DESCRIPTION   :
 *  FW randomises MAC Address bits that have a corresponding bit set to 0 in
 *  the MAC Mask for Probe Requests. This excludes U/L and I/G bits which
 *  will be set to Local and Individual respectively.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MAC_ADDRESS_RANDOMISATION_MASK 0x13B7

/*******************************************************************************
 * NAME          : UnifiWipsActivated
 * PSID          : 5050 (0x13BA)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Activate Wips.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_WIPS_ACTIVATED 0x13BA

/*******************************************************************************
 * NAME          : UnifiPsidSubversions
 * PSID          : 5051 (0x13BB)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 8
 * MAX           : 248
 * DEFAULT       :
 * DESCRIPTION   :
 *  8-byte hash followed by PSID and PSID Subversion pairs (4 bytes per pair)
 *  for customer reference. Min of 8 allows for hash + 0 versioned mib pairs.
 *  Max of 248 allows for hash + 60 versioned mib pairs.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_PSID_SUBVERSIONS 0x13BB

/*******************************************************************************
 * NAME          : UnifiObfuscateUserInfo
 * PSID          : 5052 (0x13BC)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Obfuscate the User information in the Firmware if this MIB is set. Host
 *  sets this MIB when the USER mode build is getting used
 *******************************************************************************/
#define SLSI_PSID_UNIFI_OBFUSCATE_USER_INFO 0x13BC

/*******************************************************************************
 * NAME          : UnifiRfTestModeActivated
 * PSID          : 5054 (0x13BE)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Test only: Set to true when running in RF Test mode. Setting this MIB key
 *  to true prevents setting mandatory HT MCS Rates.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_RF_TEST_MODE_ACTIVATED 0x13BE

/*******************************************************************************
 * NAME          : UnifiTxPowerDetectorResponse
 * PSID          : 5055 (0x13BF)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 255
 * DEFAULT       :
 * DESCRIPTION   :
 *  Hardware specific transmitter detector response settings. 2G settings
 *  before 5G. Increasing order within band.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TX_POWER_DETECTOR_RESPONSE 0x13BF

/*******************************************************************************
 * NAME          : UnifiTxDetectorTemperatureCompensation
 * PSID          : 5056 (0x13C0)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 255
 * DEFAULT       :
 * DESCRIPTION   :
 *  Deprecated
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TX_DETECTOR_TEMPERATURE_COMPENSATION 0x13C0

/*******************************************************************************
 * NAME          : UnifiTxDetectorFrequencyCompensation
 * PSID          : 5057 (0x13C1)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 255
 * DEFAULT       :
 * DESCRIPTION   :
 *  Deprecated
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TX_DETECTOR_FREQUENCY_COMPENSATION 0x13C1

/*******************************************************************************
 * NAME          : UnifiTxOpenLoopTemperatureCompensation
 * PSID          : 5058 (0x13C2)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 255
 * DEFAULT       :
 * DESCRIPTION   :
 *  Hardware specific transmitter open-loop temperature compensation
 *  settings. 2G settings before 5G. Increasing order within band.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TX_OPEN_LOOP_TEMPERATURE_COMPENSATION 0x13C2

/*******************************************************************************
 * NAME          : UnifiTxOpenLoopFrequencyCompensation
 * PSID          : 5059 (0x13C3)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 255
 * DEFAULT       :
 * DESCRIPTION   :
 *  Hardware specific transmitter open-loop frequency compensation settings.
 *  2G settings before 5G. Increasing order within band.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TX_OPEN_LOOP_FREQUENCY_COMPENSATION 0x13C3

/*******************************************************************************
 * NAME          : UnifiTxOfdmSelect
 * PSID          : 5060 (0x13C4)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 4
 * MAX           : 12
 * DEFAULT       :
 * DESCRIPTION   :
 *  Hardware specific transmitter OFDM selection settings
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TX_OFDM_SELECT 0x13C4

/*******************************************************************************
 * NAME          : UnifiTxDigGain
 * PSID          : 5061 (0x13C5)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 8
 * MAX           : 46
 * DEFAULT       :
 * DESCRIPTION   :
 *  Specify gain specific modulation power optimisation.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TX_DIG_GAIN 0x13C5

/*******************************************************************************
 * NAME          : UnifiChipTemperature
 * PSID          : 5062 (0x13C6)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt16
 * UNITS         : celsius
 * MIN           : -32768
 * MAX           : 32767
 * DEFAULT       :
 * DESCRIPTION   :
 *  Read the chip temperature as seen by WLAN radio firmware.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_CHIP_TEMPERATURE 0x13C6

/*******************************************************************************
 * NAME          : UnifiBatteryVoltage
 * PSID          : 5063 (0x13C7)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * UNITS         : millivolt
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  Battery voltage
 *******************************************************************************/
#define SLSI_PSID_UNIFI_BATTERY_VOLTAGE 0x13C7

/*******************************************************************************
 * NAME          : UnifiTxOpenLoopFrequencyPreGain
 * PSID          : 5064 (0x13C8)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 255
 * DEFAULT       :
 * DESCRIPTION   :
 *  Hardware specific transmitter open-loop frequency pre-gain compensation
 *  settings. 2G settings before 5G. Increasing order within band.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TX_OPEN_LOOP_FREQUENCY_PRE_GAIN 0x13C8

/*******************************************************************************
 * NAME          : UnifiTxPowerTrimConfig
 * PSID          : 5072 (0x13D0)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 7
 * MAX           : 26
 * DEFAULT       :
 * DESCRIPTION   :
 *  Hardware specific transmitter power trim settings
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TX_POWER_TRIM_CONFIG 0x13D0

/*******************************************************************************
 * NAME          : UnifiNannyTrimDisable
 * PSID          : 5073 (0x13D1)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  Bitmap to selectively disable nanny retrim of per channel trims. For
 *  bitmap, please refer to unifiEnabledTrims.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NANNY_TRIM_DISABLE 0x13D1

/*******************************************************************************
 * NAME          : UnifiForceShortSlotTime
 * PSID          : 5080 (0x13D8)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  If set to true, forces FW to use short slot times for all VIFs.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_FORCE_SHORT_SLOT_TIME 0x13D8

/*******************************************************************************
 * NAME          : UnifiTxGainStepSettings
 * PSID          : 5081 (0x13D9)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 255
 * DEFAULT       :
 * DESCRIPTION   :
 *  Hardware specific transmitter gain step settings. 2G settings before 5G.
 *  Increasing order within band.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TX_GAIN_STEP_SETTINGS 0x13D9

/*******************************************************************************
 * NAME          : UnifiDebugDisableRadioNannyActions
 * PSID          : 5082 (0x13DA)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  Bitmap to disable the radio nanny actions. Each bit corresponds to a
 *  radio according to RICE_RADIO_BM_T.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_DEBUG_DISABLE_RADIO_NANNY_ACTIONS 0x13DA

/*******************************************************************************
 * NAME          : UnifiRxCckModemSensitivity
 * PSID          : 5083 (0x13DB)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 6
 * MAX           : 6
 * DEFAULT       :
 * DESCRIPTION   :
 *  Specify values of CCK modem sensitivity for scan, normal and low
 *  sensitivity.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_RX_CCK_MODEM_SENSITIVITY 0x13DB

/*******************************************************************************
 * NAME          : UnifiDpdPerBandwidth
 * PSID          : 5084 (0x13DC)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 0XFFFF
 * DESCRIPTION   :
 *  Bitmask to enable Digital Pre-Distortion per bandwidth
 *******************************************************************************/
#define SLSI_PSID_UNIFI_DPD_PER_BANDWIDTH 0x13DC

/*******************************************************************************
 * NAME          : UnifiBbVersion
 * PSID          : 5085 (0x13DD)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  Baseband chip version number determined by reading BBIC version
 *******************************************************************************/
#define SLSI_PSID_UNIFI_BB_VERSION 0x13DD

/*******************************************************************************
 * NAME          : UnifiRfVersion
 * PSID          : 5086 (0x13DE)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  RF chip version number determined by reading RFIC version
 *******************************************************************************/
#define SLSI_PSID_UNIFI_RF_VERSION 0x13DE

/*******************************************************************************
 * NAME          : UnifiReadHardwareCounter
 * PSID          : 5087 (0x13DF)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  Read a value from a hardware packet counter for a specific radio_id and
 *  return it. The firmware will convert the radio_id to the associated
 *  mac_instance.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_READ_HARDWARE_COUNTER 0x13DF

/*******************************************************************************
 * NAME          : UnifiClearRadioTrimCache
 * PSID          : 5088 (0x13E0)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  Clears the radio trim cache. The parameter is ignored.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_CLEAR_RADIO_TRIM_CACHE 0x13E0

/*******************************************************************************
 * NAME          : UnifiRadioTxSettingsRead
 * PSID          : 5089 (0x13E1)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  Read value from Tx settings, rfchip up to and including S620.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_RADIO_TX_SETTINGS_READ 0x13E1

/*******************************************************************************
 * NAME          : UnifiModemSgiOffset
 * PSID          : 5090 (0x13E2)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  Overwrite SGI sampling offset. Indexed by Band and Bandwidth. Defaults
 *  currently defined in fw.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MODEM_SGI_OFFSET 0x13E2

/*******************************************************************************
 * NAME          : UnifiRadioTxPowerOverride
 * PSID          : 5091 (0x13E3)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt16
 * MIN           : -128
 * MAX           : 127
 * DEFAULT       :
 * DESCRIPTION   :
 *  Option in radio code to override the power requested by the upper layer
 *******************************************************************************/
#define SLSI_PSID_UNIFI_RADIO_TX_POWER_OVERRIDE 0x13E3

/*******************************************************************************
 * NAME          : UnifiAgcThresholds
 * PSID          : 5095 (0x13E7)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 255
 * DEFAULT       :
 * DESCRIPTION   :
 *  AGC Thresholds settings
 *******************************************************************************/
#define SLSI_PSID_UNIFI_AGC_THRESHOLDS 0x13E7

/*******************************************************************************
 * NAME          : UnifiRadioRxSettingsRead
 * PSID          : 5096 (0x13E8)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  Read value from Rx settings.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_RADIO_RX_SETTINGS_READ 0x13E8

/*******************************************************************************
 * NAME          : UnifiIqBufferSize
 * PSID          : 5098 (0x13EA)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  Buffer Size for IQ capture to allow CATs to read it.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_IQ_BUFFER_SIZE 0x13EA

/*******************************************************************************
 * NAME          : UnifiRadioCcaDebug
 * PSID          : 5100 (0x13EC)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  Read values from Radio CCA settings.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_RADIO_CCA_DEBUG 0x13EC

/*******************************************************************************
 * NAME          : UnifiCcaMasterSwitch
 * PSID          : 5102 (0x13EE)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       : 0X00540050
 * DESCRIPTION   :
 *  Enables CCA
 *******************************************************************************/
#define SLSI_PSID_UNIFI_CCA_MASTER_SWITCH 0x13EE

/*******************************************************************************
 * NAME          : UnifiRxSyncCcaCfg
 * PSID          : 5103 (0x13EF)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  Configures CCA per 20 MHz sub-band.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_RX_SYNC_CCA_CFG 0x13EF

/*******************************************************************************
 * NAME          : UnifiMacCcaBusyTime
 * PSID          : 5104 (0x13F0)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  Counts the time CCA indicates busy
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MAC_CCA_BUSY_TIME 0x13F0

/*******************************************************************************
 * NAME          : UnifiDpdDebug
 * PSID          : 5106 (0x13F2)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  Debug MIBs for DPD
 *******************************************************************************/
#define SLSI_PSID_UNIFI_DPD_DEBUG 0x13F2

/*******************************************************************************
 * NAME          : UnifiNarrowbandCcaDebug
 * PSID          : 5107 (0x13F3)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  Read values from Radio CCA settings.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NARROWBAND_CCA_DEBUG 0x13F3

/*******************************************************************************
 * NAME          : UnifiDpdEmphFiltCoeffs
 * PSID          : 5108 (0x13F4)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 255
 * DEFAULT       :
 * DESCRIPTION   :
 *  This mib holds the FIR filter coefficients used on the post-emphasis
 *  block of memory dpd. Each coefficient is an int16 but it will be stored
 *  in the mib as a string of octets, where each row contains the coeffs for
 *  a given radio. Here is a description of the syntax: row 1: 17
 *  pre-emphasis coeffs for RADIO_0_A | 17 post-emphasis coeffs for RADIO_0_A
 *  row 2: 17 pre-emphasis coeffs for RADIO_1_A | 17 post-emphasis coeffs for
 *  RADIO_1_A row 3: 17 pre-emphasis coeffs for RADIO_0_B | 17 post-emphasis
 *  coeffs for RADIO_0_B row 4: 17 pre-emphasis coeffs for RADIO_1_B | 17
 *  post-emphasis coeffs for RADIO_1_B The conversion from string of octets
 *  to int16 will be done by FW.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_DPD_EMPH_FILT_COEFFS 0x13F4

/*******************************************************************************
 * NAME          : UnifiNannyTemperatureReportDelta
 * PSID          : 5109 (0x13F5)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 4
 * DESCRIPTION   :
 *  A temperature difference, in degrees Celsius, above which the nanny
 *  process will generate a temperature update debug word
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NANNY_TEMPERATURE_REPORT_DELTA 0x13F5

/*******************************************************************************
 * NAME          : UnifiNannyTemperatureReportInterval
 * PSID          : 5110 (0x13F6)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 250
 * DESCRIPTION   :
 *  A report interval in milliseconds where temperature is checked
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NANNY_TEMPERATURE_REPORT_INTERVAL 0x13F6

/*******************************************************************************
 * NAME          : UnifiRadioRxDcocDebugIqValue
 * PSID          : 5111 (0x13F7)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  RX DCOC debug testing. Allows user to override LUT index IQ values in
 *  combination with unifiRadioRxDcocDebug. This MIB sets IQ value that all
 *  LUT index Is and Qs get set to.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_RADIO_RX_DCOC_DEBUG_IQ_VALUE 0x13F7

/*******************************************************************************
 * NAME          : UnifiRadioRxDcocDebug
 * PSID          : 5112 (0x13F8)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  RX DCOC debug testing. Allows user to override LUT index IQ values in
 *  combination with unifiRadioRxDcocDebugIqValue. This MIB enables the
 *  feature.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_RADIO_RX_DCOC_DEBUG 0x13F8

/*******************************************************************************
 * NAME          : UnifiNannyRetrimDpdMod
 * PSID          : 5113 (0x13F9)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 2
 * DESCRIPTION   :
 *  Bitmap to selectively enable nanny retrim of DPD per modulation.
 *  B0==OFDM0, B1==OFDM1, B2==CCK
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NANNY_RETRIM_DPD_MOD 0x13F9

/*******************************************************************************
 * NAME          : UnifiDisableDpdSubIteration
 * PSID          : 5114 (0x13FA)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  For Engineering debug use only.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_DISABLE_DPD_SUB_ITERATION 0x13FA

/*******************************************************************************
 * NAME          : UnifiRxRssiAdjustments
 * PSID          : 5115 (0x13FB)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 4
 * MAX           : 4
 * DEFAULT       :
 * DESCRIPTION   :
 *  Provides platform dependent rssi adjustments. Octet string (length 4),
 *  each octet represents a signed 8 bit value in units of quarter dB.
 *  octet[0] = always_adjust (applied unconditionally in all cases) octet[1]
 *  = low_power_adjust (applied in low_power mode only) octet[2] =
 *  ext_lna_on_adjust (applied only if we have a FEM and the external LNA is
 *  enabled) octet[3] = ext_lna_off_adjust (applied only if we have a FEM and
 *  the external LNA is disabled)
 *******************************************************************************/
#define SLSI_PSID_UNIFI_RX_RSSI_ADJUSTMENTS 0x13FB

/*******************************************************************************
 * NAME          : UnifiFleximacCcaEdEnable
 * PSID          : 5116 (0x13FC)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  Enable/disable CCA-ED in Fleximac.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_FLEXIMAC_CCA_ED_ENABLE 0x13FC

/*******************************************************************************
 * NAME          : UnifiRadioTxIqDelay
 * PSID          : 5117 (0x13FD)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 255
 * DEFAULT       :
 * DESCRIPTION   :
 *  The differential delay applied between I and Q paths in Tx.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_RADIO_TX_IQ_DELAY 0x13FD

/*******************************************************************************
 * NAME          : UnifiDisableLnaBypass
 * PSID          : 5118 (0x13FE)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  Prevents the use of the LNA bypass. Can be set at any time, but takes
 *  effect the next time the radio is turned on from off. Set a bit to 1 to
 *  disable the LNA bypass in that configuration. B0 2.4G Radio 0 B1 2.4G
 *  Radio 1 B2 2.4G Radio 2 B3 2.4G Radio 3 B4 5G Radio 0 B5 5G Radio 1 B6 5G
 *  Radio 2 B7 5G Radio 3
 *******************************************************************************/
#define SLSI_PSID_UNIFI_DISABLE_LNA_BYPASS 0x13FE

/*******************************************************************************
 * NAME          : UnifiInitialFrequencyOffset
 * PSID          : 5119 (0x13FF)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt16
 * MIN           : -32768
 * MAX           : 32767
 * DEFAULT       :
 * DESCRIPTION   :
 *  Allows carrier frequency and sample clock offset initial value in trim to
 *  be non-zero. Use to test Tx/Rx with a freq offset with significant freq
 *  offset or test convergence of freq tracking loop. Relevant to chips with
 *  11ax capability only. Provided for test and characterisation only. Should
 *  normally be zero.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_INITIAL_FREQUENCY_OFFSET 0x13FF

/*******************************************************************************
 * NAME          : UnifiRxRssiNbHwGain
 * PSID          : 5120 (0x1400)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 30
 * MAX           : 30
 * DEFAULT       :
 * DESCRIPTION   :
 *  Narrowband RSSI gain values for some combination of radio, band and
 *  bandwidth.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_RX_RSSI_NB_HW_GAIN 0x1400

/*******************************************************************************
 * NAME          : UnifiRadioTxSettingsReadV2
 * PSID          : 5122 (0x1402)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  Read value from Tx settings, rfchip S621 onwards.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_RADIO_TX_SETTINGS_READ_V2 0x1402

/*******************************************************************************
 * NAME          : UnifiRxCckModemDesense
 * PSID          : 5123 (0x1403)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 6
 * MAX           : 6
 * DEFAULT       :
 * DESCRIPTION   :
 *  Specify values of CCK modem desense level, wlanlite only.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_RX_CCK_MODEM_DESENSE 0x1403

/*******************************************************************************
 * NAME          : UnifiRxOfdmModemDesense
 * PSID          : 5125 (0x1405)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 6
 * MAX           : 6
 * DEFAULT       :
 * DESCRIPTION   :
 *  Specify values of OFDM modem desense level, wlanlite only.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_RX_OFDM_MODEM_DESENSE 0x1405

/*******************************************************************************
 * NAME          : UnifiRadioFecConfig
 * PSID          : 5127 (0x1407)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 7
 * MAX           : 7
 * DEFAULT       :
 * DESCRIPTION   :
 *  Configuration for Front End Control
 *******************************************************************************/
#define SLSI_PSID_UNIFI_RADIO_FEC_CONFIG 0x1407

/*******************************************************************************
 * NAME          : UnifiTxPowerDetectorResponseFrequency
 * PSID          : 5128 (0x1408)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 255
 * DEFAULT       :
 * DESCRIPTION   :
 *  Hardware specific transmitter detector response settings per frequency.
 *  2G settings before 5G. Increasing order within band.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TX_POWER_DETECTOR_RESPONSE_FREQUENCY 0x1408

/*******************************************************************************
 * NAME          : UnifiRadioMiscPerBand
 * PSID          : 5129 (0x1409)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 16
 * MAX           : 16
 * DEFAULT       :
 * DESCRIPTION   :
 *  Miscellaneous per band radio configuration values octet[0] = band
 *  octet[1] = mix_lo_bias octet[2] = vco_ictrl octet[3] = vco_ictrl_lp
 *  octet[4] = vco_ictrl_dpd octet[5] = vco_ictrl_dpd_lp octets[15:6] =
 *  reserved
 *******************************************************************************/
#define SLSI_PSID_UNIFI_RADIO_MISC_PER_BAND 0x1409

/*******************************************************************************
 * NAME          : UnifiNannyTrimNow
 * PSID          : 5130 (0x140A)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  Trigger Nanny retrim with next timer tick
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NANNY_TRIM_NOW 0x140A

/*******************************************************************************
 * NAME          : UnifiMapRadioIdToMacId
 * PSID          : 5131 (0x140B)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  Convert radio_id to mac_id
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MAP_RADIO_ID_TO_MAC_ID 0x140B

/*******************************************************************************
 * NAME          : UnifiReportNarrowbandRssi
 * PSID          : 5132 (0x140C)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Enable reporting of Fleximac narrowband RSSI to higher layers.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_REPORT_NARROWBAND_RSSI 0x140C

/*******************************************************************************
 * NAME          : UnifiEnableFemRfLoopback
 * PSID          : 5133 (0x140D)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  Enable RF loopback from a suitably equipped external PA.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ENABLE_FEM_RF_LOOPBACK 0x140D

/*******************************************************************************
 * NAME          : UnifiRadioTrimCount
 * PSID          : 5134 (0x140E)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  Radio trims since service start
 *******************************************************************************/
#define SLSI_PSID_UNIFI_RADIO_TRIM_COUNT 0x140E

/*******************************************************************************
 * NAME          : UnifiRxRssiBandCompensation
 * PSID          : 5135 (0x140F)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 24
 * MAX           : 81
 * DEFAULT       :
 * DESCRIPTION   :
 *  RSSI band flatness compensation for combinations of radio, band and
 *  bandwidth.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_RX_RSSI_BAND_COMPENSATION 0x140F

/*******************************************************************************
 * NAME          : UnifiEnableFrequencyTracking
 * PSID          : 5136 (0x1410)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Allow RadioFW to tune the PLL frequency to that of the BSS.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ENABLE_FREQUENCY_TRACKING 0x1410

/*******************************************************************************
 * NAME          : UnifiMediumPowerModeEnabled
 * PSID          : 5137 (0x1411)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Enable Medium Power mode to set DIV3/256QAM reduced power LO modes
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MEDIUM_POWER_MODE_ENABLED 0x1411

/*******************************************************************************
 * NAME          : UnifiChipTemperatureDirect
 * PSID          : 5138 (0x1412)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt16
 * UNITS         : celsius
 * MIN           : -32768
 * MAX           : 32767
 * DEFAULT       :
 * DESCRIPTION   :
 *  Read the chip current temperature by sending a reading request to the RF
 *  chip and waits for value to return.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_CHIP_TEMPERATURE_DIRECT 0x1412

/*******************************************************************************
 * NAME          : UnifiRxRssiFrequencyCompensation
 * PSID          : 5139 (0x1413)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 255
 * DEFAULT       :
 * DESCRIPTION   :
 *  Hardware specific receiver RSSI frequency compensation settings. 2G
 *  settings before 5G. Increasing order within band.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_RX_RSSI_FREQUENCY_COMPENSATION 0x1413

/*******************************************************************************
 * NAME          : UnifiDebugDisableRadioNannyDuringWlanliteRx
 * PSID          : 5140 (0x1414)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Flag to disable the radio nanny actions during Wlanlite RX to prevent
 *  triggering calibrations during bathtubs. This is an additional gate to
 *  unifiDebugDisableRadioNannyActions.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_DEBUG_DISABLE_RADIO_NANNY_DURING_WLANLITE_RX 0x1414

/*******************************************************************************
 * NAME          : UnifiLoadMemDpdPolyCoeffs
 * PSID          : 5141 (0x1415)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 1
 * MAX           : 350
 * DEFAULT       :
 * DESCRIPTION   :
 *  Write DPD coeffients into the FW, read DPD LUT from FW for devices that
 *  support Memory DPD
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LOAD_MEM_DPD_POLY_COEFFS 0x1415

/*******************************************************************************
 * NAME          : UnifiRadioLongTrimIterations
 * PSID          : 5142 (0x1416)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * UNITS         : iteration
 * MIN           : 1
 * MAX           : 100
 * DEFAULT       : 4
 * DESCRIPTION   :
 *  How many MP-DPD host iterations are required.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_RADIO_LONG_TRIM_ITERATIONS 0x1416

/*******************************************************************************
 * NAME          : UnifiForceRpllMode
 * PSID          : 5143 (0x1417)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 3
 * DEFAULT       :
 * DESCRIPTION   :
 *  Force RPLL mode (PAEAN only). 0: Do not force. Clock configuration
 *  depends on circumstance. 1: WPLL only. 2: RPLL mode, normal. 3: RPLL
 *  mode, overdrive (turbo).
 *******************************************************************************/
#define SLSI_PSID_UNIFI_FORCE_RPLL_MODE 0x1417

/*******************************************************************************
 * NAME          : UnifiDpdSwed
 * PSID          : 5144 (0x1418)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 4096
 * DEFAULT       :
 * DESCRIPTION   :
 *  This MIB sets the threshold on the DPD SWED block, which is applied after
 *  the DPD LUT is applied to the signal. This feature is for MP-DPD only.
 *  The threshold is a value relative to the max dynamic range of the DAC.
 *  For instance a threshold of 0.5 dB means that it's applied for signals
 *  closer than 0.5 dB of the max dynamic range. The MIB value is 1024 *
 *  power(10, threshold / 20), where threshold is the number of dB below the
 *  max dynamic range. Default is 850 (-1.6 dB) which is the reset value. A
 *  value of 0 will disable the DPD SWED.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_DPD_SWED 0x1418

/*******************************************************************************
 * NAME          : UnifiBbbTxDiversityConfig1
 * PSID          : 5145 (0x1419)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       : 0X4000
 * DESCRIPTION   :
 *  Configures BBB_TX_DIVERSITY_CONFIG1
 *******************************************************************************/
#define SLSI_PSID_UNIFI_BBB_TX_DIVERSITY_CONFIG1 0x1419

/*******************************************************************************
 * NAME          : UnifiOverrideTxiq
 * PSID          : 5146 (0x141A)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  Write TX IQ compensation values to HW
 *******************************************************************************/
#define SLSI_PSID_UNIFI_OVERRIDE_TXIQ 0x141A

/*******************************************************************************
 * NAME          : UnifiRxAgcPhaseCompensation
 * PSID          : 5147 (0x141B)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 255
 * DEFAULT       :
 * DESCRIPTION   :
 *  AGC phase compensation settings. This MIB defines the gain thresholds and
 *  the phase rotation to apply for each threshold. The phase rotation
 *  resolution is 11.25 degrees (360/32) and two corrections can be applied.
 *  Correction can be applied for each band and AGC mode, and enabled
 *  independently. Version 1 (for RF chips Bow, Lark2, Leopard, Shiba): octet
 *  0 - Version number = 1. octet 1 - Band bitmap. octet 2 - AGC mode. octet
 *  3 - Enable bit. octet 4 - Gain threshold 1. octet 5 - Gain threshold 2.
 *  octet 6 - Phase threshold 1. octet 7 - Phase threshold 2.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_RX_AGC_PHASE_COMPENSATION 0x141B

/*******************************************************************************
 * NAME          : UnifiDsmEnable
 * PSID          : 5148 (0x141C)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  Dynamic SISO MIMO switching Enable, power saving feature for MIMO
 *  operational modes. Each bit can be used to change the configuration for
 *  DSM. A '1' defines Enabled, '0' defines Disabled. Bit 0: Enable / Disable
 *  DSM
 *******************************************************************************/
#define SLSI_PSID_UNIFI_DSM_ENABLE 0x141C

/*******************************************************************************
 * NAME          : UnifiTxOobSettings
 * PSID          : 5149 (0x141D)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 255
 * DEFAULT       :
 * DESCRIPTION   :
 *  OOB settings table. | octects | description |
 * |-_-_-_-_-+-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-| | 0 | DPD applicability bitmask: bit0 = no DPD, bit1 = dynamic DPD, bit2 = static DPD, bit3 = hardcoded DPD | | 1-2 | Bitmask indicating which regulatory domains this rule applies to FCC=bit0, ETSI=bit1, JAPAN=bit2 | | 3-6 | Bitmask indicating which band edges this rule applies to RICE_BAND_EDGE_ISM_24G_LOWER = bit 0, RICE_BAND_EDGE_ISM_24G_UPPER = bit 1, RICE_BAND_EDGE_U_NII_1_LOWER = bit 2, RICE_BAND_EDGE_U_NII_1_UPPER = bit 3, RICE_BAND_EDGE_U_NII_2_LOWER = bit 4, RICE_BAND_EDGE_U_NII_2_UPPER = bit 5, RICE_BAND_EDGE_U_NII_2E_LOWER = bit 6, RICE_BAND_EDGE_U_NII_2E_UPPER = bit 7, RICE_BAND_EDGE_U_NII_3_LOWER = bit 8, RICE_BAND_EDGE_U_NII_3_UPPER = bit 9 | | 7 | Bitmask indicating which modulation types this rule applies to (LSB/b0=DSSS/CCK, b1= OFDM0 modulation group, b2= OFDM1 modulation group) | | 8 | Bitmask indicating which channel bandwidths this rule applies to (LSB/b0=20MHz, b1=40MHz, b2=80MHz) | | 9-10 | Minimum distance to nearest band edge in 500 kHz units for which this constraint becomes is applicable. | | 11 | Maximum power (EIRP) for this particular constraint - specified in units of quarter dBm. | | 12 | Spectral shaping configuration index to be used for this particular constraint. The value is specific to the radio hardware and should only be altered under advice from the IC supplier. | |
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TX_OOB_SETTINGS 0x141D

/*******************************************************************************
 * NAME          : UnifiTxSsfConfig
 * PSID          : 5150 (0x141E)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 255
 * DEFAULT       :
 * DESCRIPTION   :
 *  SSF config table
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TX_SSF_CONFIG 0x141E

/*******************************************************************************
 * NAME          : UnifiDpdWpalDelayReturn
 * PSID          : 5151 (0x141F)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 255
 * DEFAULT       :
 * DESCRIPTION   :
 *  This MIB sets the number of seconds to hold wpal before returning from
 *  mpdpd_algo_run_algorithm. In reduced performance, this will not wait for
 *  DPD to complete before turning radio on.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_DPD_WPAL_DELAY_RETURN 0x141F

/*******************************************************************************
 * NAME          : UnifiRadioTimerDelay
 * PSID          : 5152 (0x1420)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 255
 * DEFAULT       :
 * DESCRIPTION   :
 *  Radio Timer Delay
 *******************************************************************************/
#define SLSI_PSID_UNIFI_RADIO_TIMER_DELAY 0x1420

/*******************************************************************************
 * NAME          : UnifiRttCapabilities
 * PSID          : 5300 (0x14B4)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 8
 * MAX           : 8
 * DEFAULT       : { 0X00, 0X01, 0X01, 0X01, 0X00, 0X07, 0X1C, 0X32 }
 * DESCRIPTION   :
 *  RTT capabilities of the chip. see SC-506960-SW.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_RTT_CAPABILITIES 0x14B4

/*******************************************************************************
 * NAME          : UnifiFtmMinDeltaFrames
 * PSID          : 5301 (0x14B5)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 255
 * DEFAULT       : 20
 * DESCRIPTION   :
 *  Default minimum time between consecutive FTM frames in units of 100 us.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_FTM_MIN_DELTA_FRAMES 0x14B5

/*******************************************************************************
 * NAME          : UnifiFtmPerBurst
 * PSID          : 5302 (0x14B6)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 1
 * MAX           : 31
 * DEFAULT       : 10
 * DESCRIPTION   :
 *  Requested FTM frames per burst.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_FTM_PER_BURST 0x14B6

/*******************************************************************************
 * NAME          : UnifiFtmBurstDuration
 * PSID          : 5303 (0x14B7)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 2
 * MAX           : 11
 * DEFAULT       : 10
 * DESCRIPTION   :
 *  indicates the duration of a burst instance, values 0, 1, 12-14 are
 *  reserved, [2..11], the burst duration is defined as (250 x 2)^(N-2), and
 *  15 means "no preference".
 *******************************************************************************/
#define SLSI_PSID_UNIFI_FTM_BURST_DURATION 0x14B7

/*******************************************************************************
 * NAME          : UnifiFtmNumOfBurstsExponent
 * PSID          : 5304 (0x14B8)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 14
 * DEFAULT       :
 * DESCRIPTION   :
 *  The number of burst instances is 2^(Number of Bursts Exponent), value 15
 *  means "no preference".
 *******************************************************************************/
#define SLSI_PSID_UNIFI_FTM_NUM_OF_BURSTS_EXPONENT 0x14B8

/*******************************************************************************
 * NAME          : UnifiFtmAsapModeActivated
 * PSID          : 5305 (0x14B9)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Activate support for ASAP mode in FTM
 *******************************************************************************/
#define SLSI_PSID_UNIFI_FTM_ASAP_MODE_ACTIVATED 0x14B9

/*******************************************************************************
 * NAME          : UnifiFtmResponderActivated
 * PSID          : 5306 (0x14BA)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Activate support for FTM Responder
 *******************************************************************************/
#define SLSI_PSID_UNIFI_FTM_RESPONDER_ACTIVATED 0x14BA

/*******************************************************************************
 * NAME          : UnifiFtmDefaultSessionEstablishmentTimeout
 * PSID          : 5307 (0x14BB)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 10
 * MAX           : 100
 * DEFAULT       : 50
 * DESCRIPTION   :
 *  Default timeout for session estabishmant in units of ms.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_FTM_DEFAULT_SESSION_ESTABLISHMENT_TIMEOUT 0x14BB

/*******************************************************************************
 * NAME          : UnifiFtmDefaultGapBetweenBursts
 * PSID          : 5309 (0x14BD)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 5
 * MAX           : 50
 * DEFAULT       : 10
 * DESCRIPTION   :
 *  Interval between consecutive Bursts. In units of ms.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_FTM_DEFAULT_GAP_BETWEEN_BURSTS 0x14BD

/*******************************************************************************
 * NAME          : UnifiFtmDefaultTriggerDelay
 * PSID          : 5310 (0x14BE)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 100
 * DEFAULT       : 1
 * DESCRIPTION   :
 *  Delay to account for differences in time between Initiator and Responder
 *  at start of the Burst. In units of ms.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_FTM_DEFAULT_TRIGGER_DELAY 0x14BE

/*******************************************************************************
 * NAME          : UnifiFtmDefaultEndBurstDelay
 * PSID          : 5311 (0x14BF)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 100
 * DEFAULT       : 10
 * DESCRIPTION   :
 *  Delay to account for differences in time between Initiator and Responder
 *  at the end of the Burst. In units of ms.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_FTM_DEFAULT_END_BURST_DELAY 0x14BF

/*******************************************************************************
 * NAME          : UnifiFtmRequestValidationEnabled
 * PSID          : 5312 (0x14C0)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Enable Validation for FTM Add Range request RTT_Configs
 *******************************************************************************/
#define SLSI_PSID_UNIFI_FTM_REQUEST_VALIDATION_ENABLED 0x14C0

/*******************************************************************************
 * NAME          : UnifiFtmResponseValidationEnabled
 * PSID          : 5313 (0x14C1)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Enable Validation for FTM Response
 *******************************************************************************/
#define SLSI_PSID_UNIFI_FTM_RESPONSE_VALIDATION_ENABLED 0x14C1

/*******************************************************************************
 * NAME          : UnifiFtmUseResponseParameters
 * PSID          : 5314 (0x14C2)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Use Response burst parameters for burst
 *******************************************************************************/
#define SLSI_PSID_UNIFI_FTM_USE_RESPONSE_PARAMETERS 0x14C2

/*******************************************************************************
 * NAME          : UnifiFtmInitialResponseTimeout
 * PSID          : 5315 (0x14C3)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 10
 * MAX           : 100
 * DEFAULT       : 50
 * DESCRIPTION   :
 *  Default timeout for FtmInitialResponse in units of ms.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_FTM_INITIAL_RESPONSE_TIMEOUT 0x14C3

/*******************************************************************************
 * NAME          : UnifiFtmMacAddressRandomisation
 * PSID          : 5316 (0x14C4)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Enable mac address randomisation for FTM initiator
 *******************************************************************************/
#define SLSI_PSID_UNIFI_FTM_MAC_ADDRESS_RANDOMISATION 0x14C4

/*******************************************************************************
 * NAME          : UnifiFtmForceHeBandwidth
 * PSID          : 5317 (0x14C5)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  Test only: Force HE FTM packet bandwidth value in units of MHz. This can
 *  be used to send IFTMR frame as a HE PPDU and at a specific bandwidth
 *  regardless of the range request value sent by the host. Setting it to 0
 *  uses the default phy type and bandwidth as selected by firmware.
 *  Acceptable values are 80 (i.e. channel_bw_80_mhz) and 160 (i.e.
 *  channel_bw_160_mhz) for now
 *******************************************************************************/
#define SLSI_PSID_UNIFI_FTM_FORCE_HE_BANDWIDTH 0x14C5

/*******************************************************************************
 * NAME          : UnifiFtmUseRequestParameters
 * PSID          : 5321 (0x14C9)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Use certain burst params from user while sending FTM req
 *******************************************************************************/
#define SLSI_PSID_UNIFI_FTM_USE_REQUEST_PARAMETERS 0x14C9

/*******************************************************************************
 * NAME          : UnifiFtmMeanAroundCluster
 * PSID          : 5322 (0x14CA)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Whether to get simple mean or mean around cluster
 *******************************************************************************/
#define SLSI_PSID_UNIFI_FTM_MEAN_AROUND_CLUSTER 0x14CA

/*******************************************************************************
 * NAME          : UnifiFtm11azI2RLmrFeedback
 * PSID          : 5323 (0x14CB)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  I2R LMR Feedback ranging parameter in 11az IFTM request frame
 *******************************************************************************/
#define SLSI_PSID_UNIFI_FTM11AZ_I2_RLMR_FEEDBACK 0x14CB

/*******************************************************************************
 * NAME          : UnifiFtm11azSupport
 * PSID          : 5324 (0x14CC)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Whether IEEE 802.11az FTM is supported
 *******************************************************************************/
#define SLSI_PSID_UNIFI_FTM11AZ_SUPPORT 0x14CC

/*******************************************************************************
 * NAME          : UnifiFtm11azMinTimeBetweenMeasurements
 * PSID          : 5325 (0x14CD)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       : 3000
 * DESCRIPTION   :
 *  This is a field in IEEE 802.11az FTM Ranging Non-TB specific sub element.
 *  In units of 100usec
 *******************************************************************************/
#define SLSI_PSID_UNIFI_FTM11AZ_MIN_TIME_BETWEEN_MEASUREMENTS 0x14CD

/*******************************************************************************
 * NAME          : UnifiFtm11azMaxTimeBetweenMeasurements
 * PSID          : 5326 (0x14CE)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       : 500
 * DESCRIPTION   :
 *  This is a field in IEEE 802.11az FTM Ranging Non-TB specific sub element.
 *  In units of 10msec
 *******************************************************************************/
#define SLSI_PSID_UNIFI_FTM11AZ_MAX_TIME_BETWEEN_MEASUREMENTS 0x14CE

/*******************************************************************************
 * NAME          : UnifiFtm11azTerminateSession
 * PSID          : 5327 (0x14CF)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Whether IEEE 802.11az FTM Session has to be terminated or not
 *******************************************************************************/
#define SLSI_PSID_UNIFI_FTM11AZ_TERMINATE_SESSION 0x14CF

/*******************************************************************************
 * NAME          : UnifiFtm11azFormatBwRanging
 * PSID          : 5328 (0x14D0)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 1
 * DESCRIPTION   :
 *  802.11az format bw and ranging parameter
 *******************************************************************************/
#define SLSI_PSID_UNIFI_FTM11AZ_FORMAT_BW_RANGING 0x14D0

/*******************************************************************************
 * NAME          : UnifiFtm11azImmediateR2i
 * PSID          : 5329 (0x14D1)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  802.11az Immediate R2I parameter
 *******************************************************************************/
#define SLSI_PSID_UNIFI_FTM11AZ_IMMEDIATE_R2I 0x14D1

/*******************************************************************************
 * NAME          : UnifiFtmLegacyCalibration
 * PSID          : 5333 (0x14D5)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  Table for legacy mode FTM calibration values.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_FTM_LEGACY_CALIBRATION 0x14D5

/*******************************************************************************
 * NAME          : UnifiFtmHtVhtCalibration
 * PSID          : 5334 (0x14D6)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  Table for HT/VHT mode FTM calibration values.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_FTM_HT_VHT_CALIBRATION 0x14D6

/*******************************************************************************
 * NAME          : UnifiFtmChannelEstimate
 * PSID          : 5335 (0x14D7)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  True if hardware channel estimate based FTM is enabled. False if software
 *  IQ capture based FTM is enabled
 *******************************************************************************/
#define SLSI_PSID_UNIFI_FTM_CHANNEL_ESTIMATE 0x14D7

/*******************************************************************************
 * NAME          : UnifiScanMaxCacheCount
 * PSID          : 5395 (0x1513)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 200
 * DEFAULT       : 150
 * DESCRIPTION   :
 *  Maximum count the Scan will maintain the scan result in cache. Zero to
 *  disable the caching.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_SCAN_MAX_CACHE_COUNT 0x1513

/*******************************************************************************
 * NAME          : UnifiScanMaxCacheTime
 * PSID          : 5396 (0x1514)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * UNITS         : seconds
 * MIN           : 1
 * MAX           : 120
 * DEFAULT       : 30
 * DESCRIPTION   :
 *  Maximum period, in seconds, the Scan will keep the scan result in cache.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_SCAN_MAX_CACHE_TIME 0x1514

/*******************************************************************************
 * NAME          : UnifiRmMaxMeasurementDuration
 * PSID          : 5400 (0x1518)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 100
 * MAX           : 200
 * DEFAULT       : 150
 * DESCRIPTION   :
 *  Maximum measurement duration in milliseconds for operating channel
 *  measurements, i.e. maximum channel time duration for measurement scan
 *******************************************************************************/
#define SLSI_PSID_UNIFI_RM_MAX_MEASUREMENT_DURATION 0x1518

/*******************************************************************************
 * NAME          : UnifiRmMaxScanCacheTime
 * PSID          : 5401 (0x1519)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 120
 * DEFAULT       : 60
 * DESCRIPTION   :
 *  Maximum period, in seconds, the Mlme Measurement will keep the scan
 *  result for beacon table mode. When set to zero, result never expires
 *  acting as a Golden Certification MIB for WFA VE Certification.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_RM_MAX_SCAN_CACHE_TIME 0x1519

/*******************************************************************************
 * NAME          : UnifiRmMaxScanCacheCount
 * PSID          : 5402 (0x151A)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 10
 * MAX           : 150
 * DEFAULT       : 70
 * DESCRIPTION   :
 *  Maximum count the Mlme Measurement will maintain the scan result for
 *  beacon table mode.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_RM_MAX_SCAN_CACHE_COUNT 0x151A

/*******************************************************************************
 * NAME          : UnifiRmIgnoreIdleStaMode
 * PSID          : 5403 (0x151B)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  When set to true, ignore idle sta mode acting as a Golden Certification
 *  MIB for WFA VE Certification.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_RM_IGNORE_IDLE_STA_MODE 0x151B

/*******************************************************************************
 * NAME          : UnifiObssScanStartWait
 * PSID          : 5405 (0x151D)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 200
 * DEFAULT       : 150
 * DESCRIPTION   :
 *  Time in seconds to wait before starting a OBSS scan after channel switch.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_OBSS_SCAN_START_WAIT 0x151D

/*******************************************************************************
 * NAME          : UnifiMlmeScanDefaultShortFullScanTime
 * PSID          : 5406 (0x151E)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * UNITS         : TU
 * MIN           : 20
 * MAX           : 30
 * DEFAULT       : 25
 * DESCRIPTION   :
 *  Scan channel duration when in short full scan mode.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MLME_SCAN_DEFAULT_SHORT_FULL_SCAN_TIME 0x151E

/*******************************************************************************
 * NAME          : UnifiMlmeScanShortFullScanNumOfProbes
 * PSID          : 5407 (0x151F)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 1
 * MAX           : 3
 * DEFAULT       : 2
 * DESCRIPTION   :
 *  Number of probes for short full scan.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MLME_SCAN_SHORT_FULL_SCAN_NUM_OF_PROBES 0x151F

/*******************************************************************************
 * NAME          : UnifiMlmeScanContinueIfMoreThanXAps
 * PSID          : 5410 (0x1522)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 10
 * DESCRIPTION   :
 *  Part of Scan Algorithm: Keep scanning on a channel with lots of APs.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MLME_SCAN_CONTINUE_IF_MORE_THAN_XAPS 0x1522

/*******************************************************************************
 * NAME          : UnifiMlmeScanStopIfLessThanXNewAps
 * PSID          : 5411 (0x1523)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 4
 * DESCRIPTION   :
 *  Part of Scan Algorithm: Stop scanning on a channel if less than X NEW APs
 *  are seen.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_MLME_SCAN_STOP_IF_LESS_THAN_XNEW_APS 0x1523

/*******************************************************************************
 * NAME          : UnifiScanMultiVifActivated
 * PSID          : 5412 (0x1524)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : 1
 * DESCRIPTION   :
 *  Part of Scan Algorithm: Activate support for Multi Vif channel times.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_SCAN_MULTI_VIF_ACTIVATED 0x1524

/*******************************************************************************
 * NAME          : UnifiScanNewAlgorithmActivated
 * PSID          : 5413 (0x1525)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : 1
 * DESCRIPTION   :
 *  Part of Scan Algorithm: Activate support for the new algorithm.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_SCAN_NEW_ALGORITHM_ACTIVATED 0x1525

/*******************************************************************************
 * NAME          : UnifiScanRegisteredVifInfrastructureSta
 * PSID          : 5414 (0x1526)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Part of Scan Algorithm: Consider only Infrastructure STA vif as
 *  registered vif to choose VIF >= 1 scan parameters.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_SCAN_REGISTERED_VIF_INFRASTRUCTURE_STA 0x1526

/*******************************************************************************
 * NAME          : UnifiLowLatencyScanSplitActive
 * PSID          : 5415 (0x1527)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Split low latency scan into smaller chuncks.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LOW_LATENCY_SCAN_SPLIT_ACTIVE 0x1527

/*******************************************************************************
 * NAME          : UnifiLowLatencyScanReducePassive
 * PSID          : 5416 (0x1528)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  Cut the passive scan time for low latency scan. In TUs
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LOW_LATENCY_SCAN_REDUCE_PASSIVE 0x1528

/*******************************************************************************
 * NAME          : UnifiLnaControlEvaluationInterval
 * PSID          : 6008 (0x1778)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * UNITS         : milliseconds
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 200
 * DESCRIPTION   :
 *  Interval in milliseconds, for evaluating LNA control.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LNA_CONTROL_EVALUATION_INTERVAL 0x1778

/*******************************************************************************
 * NAME          : UnifiLnaControlRssiThresholds
 * PSID          : 6009 (0x1779)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * UNITS         : dBm
 * MIN           : 2
 * MAX           : 2
 * DEFAULT       :
 * DESCRIPTION   :
 *  RSSI threshold table for LNA dynamic control. The thresholds are
 *  band(index 1, 2.4GHz is 1, 5GHz is 2, 6GHz is 3) and association type and
 *  bandwidth(index 2) specific. Association and bandwidth specific values
 *  are used only for STA VIFs, for other VIFs only default per-band values
 *  are used. Some combinations can make little sense as 11AX_160MHZ 2G, but
 *  for the simplicity the table organised as it is. For Function LNA control
 *  API(used before Paean), these thresholds are thresholds to turn LNA
 *  OFF/ON. For Mode LNA control API, these thresholds are thresholds to
 *  select between smart mode(eLNA is off between frames and if necessary
 *  automatically turned on by HW during preamble reception) and maximum
 *  sensitivity mode(eLNA is always ON). Because of the difference between 2
 *  APIs the thresholds are very different for these 2 APIs. The default
 *  values defined here (-30,-40) are values defined for Function LNA control
 *  API, so must be changed in HTF files for Mode LNA control API. The
 *  thresholds define the desired state of eLNA per VIF but the actual state
 *  of eLNA is defined by all scheduled VIFs(including Scan VIFs).
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LNA_CONTROL_RSSI_THRESHOLDS 0x1779

/*******************************************************************************
 * NAME          : UnifiUnsyncVifLnaEnabled
 * PSID          : 6010 (0x177A)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Enable or disable use of the LNA for unsynchronised VIFs.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_UNSYNC_VIF_LNA_ENABLED 0x177A

/*******************************************************************************
 * NAME          : UnifiTpcMaxPower2Gmimo
 * PSID          : 6011 (0x177B)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt16
 * MIN           : -32768
 * MAX           : 32767
 * DEFAULT       : 52
 * DESCRIPTION   :
 *  Maximum power for 2.4GHz MIMO interface when RSSI is above
 *  unifiTPCMinPowerRSSIThreshold (quarter dbm). Should be greater than
 *  dot11PowerCapabilityMinImplemented. This value is read only once when an
 *  interface is added.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TPC_MAX_POWER2_GMIMO 0x177B

/*******************************************************************************
 * NAME          : UnifiTpcMaxPower5Gmimo
 * PSID          : 6012 (0x177C)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt16
 * MIN           : -32768
 * MAX           : 32767
 * DEFAULT       : 52
 * DESCRIPTION   :
 *  Maximum power for 5 GHz MIMO interface when RSSI is above
 *  unifiTPCMinPowerRSSIThreshold (quarter dbm). Should be greater than
 *  dot11PowerCapabilityMinImplemented. This value is read only once when an
 *  interface is added.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TPC_MAX_POWER5_GMIMO 0x177C

/*******************************************************************************
 * NAME          : UnifiLnaControlEnabled
 * PSID          : 6013 (0x177D)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 3
 * DEFAULT       : 1
 * DESCRIPTION   :
 *  Original pre-Paean description: Enable dynamic switching of the LNA based
 *  on RSSI for synchronised VIFs. This MIB has been extended to support Host
 *  selected LNA mode(look at C-510619-SP - MCD Appendix 49: eLNA control for
 *  details). Values 0(LNA_CONTROL_DISABLED, look at unifiLnaModeSelection
 *  enum) and 1(LNA_AUTOMATIC_CONTROL_ENABLED) still work in the same way for
 *  functional API, but 2(LNA_FORCE_ENABLED) and 3(LNA_FORCE_DISABLED) are
 *  supported only for platforms that support new eLNA mode API. For
 *  platforms which support new eLNA mode API LNA_AUTOMATIC_CONTROL_DISABLED
 *  (value 0) means that there are no LNAs that can be controlled and
 *  ELNA_MODE_T_off will be provided as lna_mode parameter to rice.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LNA_CONTROL_ENABLED 0x177D

/*******************************************************************************
 * NAME          : UnifiPowerIsGrip
 * PSID          : 6016 (0x1780)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Is using Grip power cap instead of SAR cap.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_POWER_IS_GRIP 0x1780

/*******************************************************************************
 * NAME          : UnifiLowPowerRxConfig
 * PSID          : 6018 (0x1782)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 3
 * DESCRIPTION   :
 *  Enables low power radio RX for idle STA and AP VIFs respectively.
 *  Setting/clearing bit 0 enables/disabled LP RX for (all) STA/Cli VIFs.
 *  Setting/clearing bit 1 enables/disabled LP RX for AP/GO VIFs.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LOW_POWER_RX_CONFIG 0x1782

/*******************************************************************************
 * NAME          : UnifiTpcEnabled
 * PSID          : 6019 (0x1783)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       :
 * DESCRIPTION   :
 *  Deprecated. Golden Certification MIB don't delete, change PSID or name
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TPC_ENABLED 0x1783

/*******************************************************************************
 * NAME          : UnifiCurrentTxpowerLevel
 * PSID          : 6020 (0x1784)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt16
 * UNITS         : qdBm
 * MIN           : -32768
 * MAX           : 32767
 * DEFAULT       :
 * DESCRIPTION   :
 *  Maximum air power for the VIF. Values are expressed in 0.25 dBm units.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_CURRENT_TXPOWER_LEVEL 0x1784

/*******************************************************************************
 * NAME          : UnifiUserSetTxpowerLevel
 * PSID          : 6021 (0x1785)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt16
 * MIN           : -32768
 * MAX           : 32767
 * DEFAULT       : 127
 * DESCRIPTION   :
 *  Test only: Maximum User Set Tx Power (quarter dBm). Enable it in
 *  unifiTestTxPowerEnable.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_USER_SET_TXPOWER_LEVEL 0x1785

/*******************************************************************************
 * NAME          : UnifiTpcMaxPowerRssiThreshold
 * PSID          : 6022 (0x1786)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt16
 * MIN           : -32768
 * MAX           : 32767
 * DEFAULT       : -55
 * DESCRIPTION   :
 *  Below the (dBm) threshold, switch to the max power allowed by regulatory,
 *  if it has been previously reduced due to unifiTPCMinPowerRSSIThreshold.
 *  This value is read only once when an interface is added.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TPC_MAX_POWER_RSSI_THRESHOLD 0x1786

/*******************************************************************************
 * NAME          : UnifiTpcMinPowerRssiThreshold
 * PSID          : 6023 (0x1787)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt16
 * MIN           : -32768
 * MAX           : 32767
 * DEFAULT       : -45
 * DESCRIPTION   :
 *  Above the (dBm) threshold, limit the transmit power by
 *  unifiTPCMaxPower2G/unifiTPCMaxPower5G. A Zero value reverts the power to
 *  a default state. This value is read only once when an interface is added.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TPC_MIN_POWER_RSSI_THRESHOLD 0x1787

/*******************************************************************************
 * NAME          : UnifiTpcMaxPower2g
 * PSID          : 6024 (0x1788)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt16
 * MIN           : -32768
 * MAX           : 32767
 * DEFAULT       : 52
 * DESCRIPTION   :
 *  Maximum power for 2.4GHz SISO interface when RSSI is above
 *  unifiTPCMinPowerRSSIThreshold (quarter dbm). Should be greater than
 *  dot11PowerCapabilityMinImplemented. This value is read only once when an
 *  interface is added.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TPC_MAX_POWER2G 0x1788

/*******************************************************************************
 * NAME          : UnifiTpcMaxPower5g
 * PSID          : 6025 (0x1789)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt16
 * MIN           : -32768
 * MAX           : 32767
 * DEFAULT       : 40
 * DESCRIPTION   :
 *  Maximum power for 5 GHz SISO interface when RSSI is above
 *  unifiTPCMinPowerRSSIThreshold (quarter dbm). Should be greater than
 *  dot11PowerCapabilityMinImplemented. This value is read only once when an
 *  interface is added.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TPC_MAX_POWER5G 0x1789

/*******************************************************************************
 * NAME          : UnifiSarBackoff
 * PSID          : 6026 (0x178A)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt8
 * MIN           : -128
 * MAX           : 127
 * DEFAULT       :
 * DESCRIPTION   :
 *  Max power values per band per index(quarter dBm).
 *******************************************************************************/
#define SLSI_PSID_UNIFI_SAR_BACKOFF 0x178A

/*******************************************************************************
 * NAME          : UnifiRadioLpRxRssiThresholdLower
 * PSID          : 6028 (0x178C)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt16
 * UNITS         : dBm
 * MIN           : -128
 * MAX           : 127
 * DEFAULT       : -75
 * DESCRIPTION   :
 *  The lower RSSI threshold for switching between low power rx and normal
 *  rx. If the RSSI avg of received frames is lower than this value for a
 *  VIF, then that VIF will vote against using low-power radio RX. Low power
 *  rx could negatively influence the receiver sensitivity.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_RADIO_LP_RX_RSSI_THRESHOLD_LOWER 0x178C

/*******************************************************************************
 * NAME          : UnifiRadioLpRxRssiThresholdUpper
 * PSID          : 6029 (0x178D)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt16
 * UNITS         : dBm
 * MIN           : -128
 * MAX           : 127
 * DEFAULT       : -65
 * DESCRIPTION   :
 *  The upper RSSI threshold for switching between low power rx and normal
 *  rx. If the RSSI avg of received frames is higher than this value for a
 *  VIF, then that VIF will vote in favour of using low-power radio RX. Low
 *  power RX could negatively influence the receiver sensitivity.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_RADIO_LP_RX_RSSI_THRESHOLD_UPPER 0x178D

/*******************************************************************************
 * NAME          : UnifiTestTxPowerEnable
 * PSID          : 6032 (0x1790)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 0X03DD
 * DESCRIPTION   :
 *  Test only: Bitfield to enable Control Plane Tx Power processing.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TEST_TX_POWER_ENABLE 0x1790

/*******************************************************************************
 * NAME          : UnifiLteCoexMaxPowerRssiThreshold
 * PSID          : 6033 (0x1791)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt16
 * MIN           : -32768
 * MAX           : 32767
 * DEFAULT       : -55
 * DESCRIPTION   :
 *  Below this (dBm) threshold, switch to max power allowed by regulatory, if
 *  it has been previously reduced due to unifiTPCMinPowerRSSIThreshold. This
 *  value is read only once when an interface is added.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LTE_COEX_MAX_POWER_RSSI_THRESHOLD 0x1791

/*******************************************************************************
 * NAME          : UnifiLteCoexMinPowerRssiThreshold
 * PSID          : 6034 (0x1792)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt16
 * MIN           : -32768
 * MAX           : 32767
 * DEFAULT       : -45
 * DESCRIPTION   :
 *  Above this(dBm) threshold, switch to minimum hardware supported - capped
 *  by unifiTPCMaxPower2G/unifiTPCMaxPower5G. Zero reverts the power to its
 *  default state. This value is read only once when an interface is added.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LTE_COEX_MIN_POWER_RSSI_THRESHOLD 0x1792

/*******************************************************************************
 * NAME          : UnifiLteCoexPowerReduction
 * PSID          : 6035 (0x1793)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 127
 * DEFAULT       : 24
 * DESCRIPTION   :
 *  When LTE Coex Power Reduction provisions are met, impose a power cap of
 *  the regulatory domain less the amount specified by this MIB (quarter dB).
 *  This value is read only once when an interface is added.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LTE_COEX_POWER_REDUCTION 0x1793

/*******************************************************************************
 * NAME          : UnifiPmfAssociationComebackTimeDelta
 * PSID          : 6050 (0x17A2)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       : 1100
 * DESCRIPTION   :
 *  Timeout interval, in TU, for the TimeOut IE in the SA Query request
 *  frame.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_PMF_ASSOCIATION_COMEBACK_TIME_DELTA 0x17A2

/*******************************************************************************
 * NAME          : UnifiTestTspecHack
 * PSID          : 6060 (0x17AC)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Test only: Hack to allow in-house tspec testing
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TEST_TSPEC_HACK 0x17AC

/*******************************************************************************
 * NAME          : UnifiTestTspecHackValue
 * PSID          : 6061 (0x17AD)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  Test only: Saved dialog number of tspec request action frame from the
 *  Host
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TEST_TSPEC_HACK_VALUE 0x17AD

/*******************************************************************************
 * NAME          : UnifiDebugInstantDelivery
 * PSID          : 6069 (0x17B5)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Instant delivery control of the debug messages when set to true. Note:
 *  will not allow the host to suspend when set to True.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_DEBUG_INSTANT_DELIVERY 0x17B5

/*******************************************************************************
 * NAME          : UnifiDebugEnable
 * PSID          : 6071 (0x17B7)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Debug to host state. Debug is either is sent to the host or it isn't.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_DEBUG_ENABLE 0x17B7

/*******************************************************************************
 * NAME          : UnifiDebugConnectionEnable
 * PSID          : 6072 (0x17B8)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Enable or disable debug logging of connection related info.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_DEBUG_CONNECTION_ENABLE 0x17B8

/*******************************************************************************
 * NAME          : UnifiDPlaneDebug
 * PSID          : 6073 (0x17B9)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       : 0X203
 * DESCRIPTION   :
 *  Bit mask for turning on individual debug entities in the data_plane that
 *  if enabled effect throughput. See DPLP_DEBUG_ENTITIES_T in
 *  dplane_dplp_debug.h for bits. Default of 0x203 means dplp, ampdu and
 *  metadata logs are enabled.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_DPLANE_DEBUG 0x17B9

/*******************************************************************************
 * NAME          : UnifiNanHeActivated
 * PSID          : 6074 (0x17BA)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Enable HE mode for NAN.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_HE_ACTIVATED 0x17BA

/*******************************************************************************
 * NAME          : UnifiNanDefaultEdcaParam
 * PSID          : 6075 (0x17BB)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 255
 * DEFAULT       :
 * DESCRIPTION   :
 *  EDCA Parameters to be used as the default in NAN, as there is no AP to
 *  specify them, indexed by unifiAccessClassIndex octet 0 - AIFSN octet 1 -
 *  [7:4] ECW MAX [3:0] ECW MIN octet 2 ~ 3 - TXOP[7:0] TXOP[15:8] in 32 usec
 *  units for both non-HT and HT connections.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_DEFAULT_EDCA_PARAM 0x17BB

/*******************************************************************************
 * NAME          : UnifiNanAllowCipherSuiteDowngrade
 * PSID          : 6076 (0x17BC)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  PRI-12398: Workaround for Framework not selecting correct Cipher Suite
 *  ID. FW would use Cipher Suite ID '1' if peer is using it even if
 *  Framework requested ID '2'.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_ALLOW_CIPHER_SUITE_DOWNGRADE 0x17BC

/*******************************************************************************
 * NAME          : UnifiNanMaxSdfAllowedInDw
 * PSID          : 6077 (0x17BD)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 1
 * MAX           : 16
 * DEFAULT       : 8
 * DESCRIPTION   :
 *  Maximum number of SDFs allowed within DW.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_MAX_SDF_ALLOWED_IN_DW 0x17BD

/*******************************************************************************
 * NAME          : UnifiNanMaxSdaTxRetryCount
 * PSID          : 6078 (0x17BE)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 1
 * MAX           : 65535
 * DEFAULT       : 8
 * DESCRIPTION   :
 *  Maximum number of reties to send SDA to a peer.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_MAX_SDA_TX_RETRY_COUNT 0x17BE

/*******************************************************************************
 * NAME          : UnifiNanSupportedCipherSuites
 * PSID          : 6079 (0x17BF)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 0X41
 * DESCRIPTION   :
 *  Set of supported cipher suites. NanCipherSuiteType bit field format. 0x1
 *  - NCS_SK_CCM_128 0x40 - NCS_PK_PASN_128
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_SUPPORTED_CIPHER_SUITES 0x17BF

/*******************************************************************************
 * NAME          : UnifiNanActivated
 * PSID          : 6080 (0x17C0)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Activate Neighbour Aware Networking (NAN)
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_ACTIVATED 0x17C0

/*******************************************************************************
 * NAME          : UnifiNanBeaconCapabilities
 * PSID          : 6081 (0x17C1)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 0X0620
 * DESCRIPTION   :
 *  The 16-bit field follows the coding of IEEE 802.11 Capability
 *  Information.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_BEACON_CAPABILITIES 0x17C1

/*******************************************************************************
 * NAME          : UnifiNanMaxConcurrentClusters
 * PSID          : 6082 (0x17C2)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 1
 * MAX           : 1
 * DEFAULT       : 1
 * DESCRIPTION   :
 *  Maximum number of concurrent NAN clusters supported.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_MAX_CONCURRENT_CLUSTERS 0x17C2

/*******************************************************************************
 * NAME          : UnifiNanMaxConcurrentPublishes
 * PSID          : 6083 (0x17C3)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 64
 * DEFAULT       : 8
 * DESCRIPTION   :
 *  Maximum number of concurrent NAN Publish instances supported.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_MAX_CONCURRENT_PUBLISHES 0x17C3

/*******************************************************************************
 * NAME          : UnifiNanMaxConcurrentSubscribes
 * PSID          : 6084 (0x17C4)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 64
 * DEFAULT       : 64
 * DESCRIPTION   :
 *  Maximum number of concurrent NAN Subscribe instances supported.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_MAX_CONCURRENT_SUBSCRIBES 0x17C4

/*******************************************************************************
 * NAME          : UnifiNanMaxServiceNameLength
 * PSID          : 6085 (0x17C5)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 255
 * DEFAULT       : 255
 * DESCRIPTION   :
 *  Not used by FW. Maximum Service Name Length.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_MAX_SERVICE_NAME_LENGTH 0x17C5

/*******************************************************************************
 * NAME          : UnifiNanMaxMatchFilterLength
 * PSID          : 6086 (0x17C6)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 255
 * DEFAULT       : 255
 * DESCRIPTION   :
 *  Not used by FW. Maximum Match Filter Length.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_MAX_MATCH_FILTER_LENGTH 0x17C6

/*******************************************************************************
 * NAME          : UnifiNanMaxTotalMatchFilterLength
 * PSID          : 6087 (0x17C7)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 256
 * DEFAULT       : 256
 * DESCRIPTION   :
 *  Not used by FW. Maximum Total Match Filter Length.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_MAX_TOTAL_MATCH_FILTER_LENGTH 0x17C7

/*******************************************************************************
 * NAME          : UnifiNanMaxServiceSpecificInfoLength
 * PSID          : 6088 (0x17C8)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 255
 * DEFAULT       : 255
 * DESCRIPTION   :
 *  Not used by FW. Maximum Service Specific Info Length.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_MAX_SERVICE_SPECIFIC_INFO_LENGTH 0x17C8

/*******************************************************************************
 * NAME          : UnifiNanMaxExtendedServiceSpecificInfoLen
 * PSID          : 6089 (0x17C9)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 255
 * DEFAULT       : 255
 * DESCRIPTION   :
 *  Not used by FW. Maximum Extended Service Specific Info Length.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_MAX_EXTENDED_SERVICE_SPECIFIC_INFO_LEN 0x17C9

/*******************************************************************************
 * NAME          : UnifiNanMaxSubscribeInterfaceAddresses
 * PSID          : 6090 (0x17CA)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 256
 * DEFAULT       : 8
 * DESCRIPTION   :
 *  Not used by FW. Maximum number interface addresses in subscribe filter
 *  list.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_MAX_SUBSCRIBE_INTERFACE_ADDRESSES 0x17CA

/*******************************************************************************
 * NAME          : UnifiNanMaxNdiInterfaces
 * PSID          : 6091 (0x17CB)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 8
 * DEFAULT       : 1
 * DESCRIPTION   :
 *  Not used by FW. Maximum NDI Interfaces. Note: This does not affect number
 *  of NDL Vifs supported by FW as they are hard coded.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_MAX_NDI_INTERFACES 0x17CB

/*******************************************************************************
 * NAME          : UnifiNanMaxNdpSessions
 * PSID          : 6092 (0x17CC)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 16
 * DEFAULT       : 5
 * DESCRIPTION   :
 *  Not used by FW. Maximum NDP Sessions. Note: This does not affect number
 *  of NDP sessions supported by FW as they are hard coded.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_MAX_NDP_SESSIONS 0x17CC

/*******************************************************************************
 * NAME          : UnifiNanMaxAppInfoLength
 * PSID          : 6093 (0x17CD)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 2048
 * DEFAULT       : 2048
 * DESCRIPTION   :
 *  Not used by FW. Maximum App Info Length.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_MAX_APP_INFO_LENGTH 0x17CD

/*******************************************************************************
 * NAME          : UnifiNanMatchExpirationTime
 * PSID          : 6094 (0x17CE)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * UNITS         : seconds
 * MIN           : 0
 * MAX           : 2147
 * DEFAULT       : 8
 * DESCRIPTION   :
 *  Time limit in which Mlme will expire a match for discovered service. DW0
 *  interval is 8s and all NAN devices shall be awake in DW0.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_MATCH_EXPIRATION_TIME 0x17CE

/*******************************************************************************
 * NAME          : UnifiNanPermittedChannels
 * PSID          : 6095 (0x17CF)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 8
 * MAX           : 8
 * DEFAULT       :
 * DESCRIPTION   :
 *  Applicable Primary Channels mask. Defined in a uint64 represented by the
 *  octet string. Mapping defined in ChannelisationRules; i.e. Bit 14 in the
 *  first list maps to channel 36.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_PERMITTED_CHANNELS 0x17CF

/*******************************************************************************
 * NAME          : UnifiNanDefaultScanPeriod
 * PSID          : 6096 (0x17D0)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * UNITS         : seconds
 * MIN           : 0
 * MAX           : 2147
 * DEFAULT       : 20
 * DESCRIPTION   :
 *  The default value of scan period in seconds.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_DEFAULT_SCAN_PERIOD 0x17D0

/*******************************************************************************
 * NAME          : UnifiNanMaxChannelSwitchTime
 * PSID          : 6097 (0x17D1)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * UNITS         : microseconds
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 7000
 * DESCRIPTION   :
 *  Maximum Channel Switch Time.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_MAX_CHANNEL_SWITCH_TIME 0x17D1

/*******************************************************************************
 * NAME          : UnifiNanMacRandomisationActivated
 * PSID          : 6098 (0x17D2)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Enabling Mac Address Randomisation for NMI address.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_MAC_RANDOMISATION_ACTIVATED 0x17D2

/*******************************************************************************
 * NAME          : UnifiNanDefaultSchedule
 * PSID          : 6099 (0x17D3)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 12
 * MAX           : 12
 * DEFAULT       :
 * DESCRIPTION   :
 *  Default Schedule to be proposed for NDL. FW will remove impossible /
 *  overlapping slots from proposal. Values must match with NAN Spec Time
 *  BitMap Control field values. Octet 0 : Band, expressed as INTERFACE
 *  number, applicable to the schedule proposal. Octet 1 ~ 2 : Ordered
 *  ChannelList[2] from most to least prefered channel number. Zero means not
 *  in used. Octet 3 : Bit Duration in TUs. Octet 4 ~ 5 : Period in TUs.
 *  Octet 6 ~ 7 : Start Offset in TUs. Octet 8 ~ 11 : TimeBitMap[4]
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_DEFAULT_SCHEDULE 0x17D3

/*******************************************************************************
 * NAME          : hutsReadWriteDataElementInt32
 * PSID          : 6100 (0x17D4)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       : 1000
 * DESCRIPTION   :
 *  Reserved for HUTS tests - Data element read/write entry of int32 type.
 *******************************************************************************/
#define SLSI_PSID_HUTS_READ_WRITE_DATA_ELEMENT_INT32 0x17D4

/*******************************************************************************
 * NAME          : hutsReadWriteDataElementBoolean
 * PSID          : 6101 (0x17D5)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Reserved for HUTS tests - Data element read/write entry of boolean type.
 *******************************************************************************/
#define SLSI_PSID_HUTS_READ_WRITE_DATA_ELEMENT_BOOLEAN 0x17D5

/*******************************************************************************
 * NAME          : hutsReadWriteDataElementOctetString
 * PSID          : 6102 (0x17D6)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 9
 * MAX           : 9
 * DEFAULT       : { 0X00, 0X01, 0X00, 0X00, 0X00, 0XFF, 0X00, 0X00, 0X00 }
 * DESCRIPTION   :
 *  Reserved for HUTS tests - Data element read/write entry of octet string
 *  type.
 *******************************************************************************/
#define SLSI_PSID_HUTS_READ_WRITE_DATA_ELEMENT_OCTET_STRING 0x17D6

/*******************************************************************************
 * NAME          : hutsReadWriteTableInt16Row
 * PSID          : 6103 (0x17D7)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt16
 * MIN           : -32768
 * MAX           : 32767
 * DEFAULT       :
 * DESCRIPTION   :
 *  Reserved for HUTS tests - Data element read/write entry table of int16
 *  type.
 *******************************************************************************/
#define SLSI_PSID_HUTS_READ_WRITE_TABLE_INT16_ROW 0x17D7

/*******************************************************************************
 * NAME          : hutsReadWriteTableOctetStringRow
 * PSID          : 6104 (0x17D8)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 6
 * MAX           : 73
 * DEFAULT       :
 * DESCRIPTION   :
 *  Reserved for HUTS tests - Data element read/write entry table of octet
 *  string type.
 *******************************************************************************/
#define SLSI_PSID_HUTS_READ_WRITE_TABLE_OCTET_STRING_ROW 0x17D8

/*******************************************************************************
 * NAME          : hutsReadWriteRemoteProcedureCallInt32
 * PSID          : 6105 (0x17D9)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       : 0X000A0001
 * DESCRIPTION   :
 *  Reserved for HUTS tests - Remote Procedure call read/write entry of int32
 *  type.
 *******************************************************************************/
#define SLSI_PSID_HUTS_READ_WRITE_REMOTE_PROCEDURE_CALL_INT32 0x17D9

/*******************************************************************************
 * NAME          : hutsReadWriteRemoteProcedureCallOctetString
 * PSID          : 6107 (0x17DB)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 144
 * MAX           : 144
 * DEFAULT       :
 * DESCRIPTION   :
 *  Reserved for HUTS tests - Remote Procedure call read/write entry of octet
 *  string type.
 *******************************************************************************/
#define SLSI_PSID_HUTS_READ_WRITE_REMOTE_PROCEDURE_CALL_OCTET_STRING 0x17DB

/*******************************************************************************
 * NAME          : hutsReadWriteInternalApiInt16
 * PSID          : 6108 (0x17DC)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt16
 * MIN           : -32768
 * MAX           : 32767
 * DEFAULT       : -55
 * DESCRIPTION   :
 *  Reserved for HUTS tests - Data element read/write entry of int16 type via
 *  internal API.
 *******************************************************************************/
#define SLSI_PSID_HUTS_READ_WRITE_INTERNAL_API_INT16 0x17DC

/*******************************************************************************
 * NAME          : hutsReadWriteInternalApiUint16
 * PSID          : 6109 (0x17DD)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 0X0730
 * DESCRIPTION   :
 *  Reserved for HUTS tests - Data element read/write entry of unsigned int16
 *  type via internal API.
 *******************************************************************************/
#define SLSI_PSID_HUTS_READ_WRITE_INTERNAL_API_UINT16 0x17DD

/*******************************************************************************
 * NAME          : hutsReadWriteInternalApiUint32
 * PSID          : 6110 (0x17DE)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * UNITS         : microseconds
 * MIN           : 0
 * MAX           : 2147483647
 * DEFAULT       : 30000
 * DESCRIPTION   :
 *  Reserved for HUTS tests - Data element read/write entry of unsigned int32
 *  type via internal API.
 *******************************************************************************/
#define SLSI_PSID_HUTS_READ_WRITE_INTERNAL_API_UINT32 0x17DE

/*******************************************************************************
 * NAME          : hutsReadWriteInternalApiInt64
 * PSID          : 6111 (0x17DF)
 * PER INTERFACE?: NO
 * TYPE          : UINT64
 * MIN           : 0
 * MAX           : 18446744073709551615
 * DEFAULT       :
 * DESCRIPTION   :
 *  Reserved for HUTS tests - Data element read/write entry of uint64 type
 *  via internal API.
 *******************************************************************************/
#define SLSI_PSID_HUTS_READ_WRITE_INTERNAL_API_INT64 0x17DF

/*******************************************************************************
 * NAME          : hutsReadWriteInternalApiBoolean
 * PSID          : 6112 (0x17E0)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Reserved for HUTS tests - Data element read/write entry of boolean type
 *  via internal API.
 *******************************************************************************/
#define SLSI_PSID_HUTS_READ_WRITE_INTERNAL_API_BOOLEAN 0x17E0

/*******************************************************************************
 * NAME          : hutsReadWriteInternalApiOctetString
 * PSID          : 6113 (0x17E1)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 8
 * MAX           : 8
 * DEFAULT       : { 0X00, 0X18, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00 }
 * DESCRIPTION   :
 *  Reserved for HUTS tests - Data element read/write entry of octet string
 *  type via internal API.
 *******************************************************************************/
#define SLSI_PSID_HUTS_READ_WRITE_INTERNAL_API_OCTET_STRING 0x17E1

/*******************************************************************************
 * NAME          : hutsReadWriteInternalApiFixedSizeTableRow
 * PSID          : 6114 (0x17E2)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt16
 * MIN           : 0
 * MAX           : 100
 * DEFAULT       :
 * DESCRIPTION   :
 *  Reserved for HUTS tests - Fixed size table rows of int16 type via
 *  internal API
 *******************************************************************************/
#define SLSI_PSID_HUTS_READ_WRITE_INTERNAL_API_FIXED_SIZE_TABLE_ROW 0x17E2

/*******************************************************************************
 * NAME          : hutsReadWriteInternalApiVarSizeTableRow
 * PSID          : 6115 (0x17E3)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 6
 * MAX           : 73
 * DEFAULT       :
 * DESCRIPTION   :
 *  Reserved for HUTS tests - Variable size table rows of octet string type
 *  via internal API
 *******************************************************************************/
#define SLSI_PSID_HUTS_READ_WRITE_INTERNAL_API_VAR_SIZE_TABLE_ROW 0x17E3

/*******************************************************************************
 * NAME          : hutsReadWriteInternalApiFixSizeTableKey1Row
 * PSID          : 6116 (0x17E4)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt16
 * MIN           : -32768
 * MAX           : 32767
 * DEFAULT       :
 * DESCRIPTION   :
 *  Reserved for HUTS tests - Fixed size table rows of int16 type via
 *  internal API
 *******************************************************************************/
#define SLSI_PSID_HUTS_READ_WRITE_INTERNAL_API_FIX_SIZE_TABLE_KEY1_ROW 0x17E4

/*******************************************************************************
 * NAME          : hutsReadWriteInternalApiFixSizeTableKey2Row
 * PSID          : 6117 (0x17E5)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt16
 * MIN           : -32768
 * MAX           : 32767
 * DEFAULT       :
 * DESCRIPTION   :
 *  Reserved for HUTS tests - Fixed size table rows of int16 type via
 *  internal API
 *******************************************************************************/
#define SLSI_PSID_HUTS_READ_WRITE_INTERNAL_API_FIX_SIZE_TABLE_KEY2_ROW 0x17E5

/*******************************************************************************
 * NAME          : hutsReadWriteInternalApiFixVarSizeTableKey1Row
 * PSID          : 6118 (0x17E6)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  The values stored in hutsReadWriteInternalAPIFixVarSizeTableKeys
 *******************************************************************************/
#define SLSI_PSID_HUTS_READ_WRITE_INTERNAL_API_FIX_VAR_SIZE_TABLE_KEY1_ROW 0x17E6

/*******************************************************************************
 * NAME          : hutsReadWriteInternalApiFixVarSizeTableKey2Row
 * PSID          : 6119 (0x17E7)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  The values stored in hutsReadWriteInternalAPIFixVarSizeTableKeys
 *******************************************************************************/
#define SLSI_PSID_HUTS_READ_WRITE_INTERNAL_API_FIX_VAR_SIZE_TABLE_KEY2_ROW 0x17E7

/*******************************************************************************
 * NAME          : hutsReadWriteInternalApiFixSizeTableKeyRow
 * PSID          : 6120 (0x17E8)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  The number of received MPDUs discarded by the CCMP decryption algorithm.
 *******************************************************************************/
#define SLSI_PSID_HUTS_READ_WRITE_INTERNAL_API_FIX_SIZE_TABLE_KEY_ROW 0x17E8

/*******************************************************************************
 * NAME          : hutsReadWriteInternalApiVarSizeTableKeyRow
 * PSID          : 6121 (0x17E9)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 144
 * MAX           : 144
 * DEFAULT       :
 * DESCRIPTION   :
 *  Write a DPD LUT entry
 *******************************************************************************/
#define SLSI_PSID_HUTS_READ_WRITE_INTERNAL_API_VAR_SIZE_TABLE_KEY_ROW 0x17E9

/*******************************************************************************
 * NAME          : UnifiTestScanNoMedium
 * PSID          : 6122 (0x17EA)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Test only: Stop Scan from using the Medium to allow thruput testing.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TEST_SCAN_NO_MEDIUM 0x17EA

/*******************************************************************************
 * NAME          : UnifiDualBandConcurrency
 * PSID          : 6123 (0x17EB)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Golden Certification MIB don't delete, change PSID or name. Identify
 *  whether the chip supports dualband concurrency or not (RSDB vs. VSDB).
 *  Set in the respective platform htf file.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_DUAL_BAND_CONCURRENCY 0x17EB

/*******************************************************************************
 * NAME          : UnifiLoggerMaxDelayedEvents
 * PSID          : 6124 (0x17EC)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 10
 * DESCRIPTION   :
 *  Maximum number of events to keep when host is suspended.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LOGGER_MAX_DELAYED_EVENTS 0x17EC

/*******************************************************************************
 * NAME          : UnifiNanSkipNanChannelsIfNdlExists
 * PSID          : 6125 (0x17ED)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Disable Nan Scan if an NDL exists.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_SKIP_NAN_CHANNELS_IF_NDL_EXISTS 0x17ED

/*******************************************************************************
 * NAME          : UnifiNanKeepAliveTimeout
 * PSID          : 6140 (0x17FC)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * UNITS         : seconds
 * MIN           : 0
 * MAX           : 2147
 * DEFAULT       : 14
 * DESCRIPTION   :
 *  Golden Certification MIB don't delete, change PSID or name. Timeout
 *  before terminating in seconds. 0 = Disabled. Capped to greater than 6
 *  seconds. This value is read only once when an interface is added.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_KEEP_ALIVE_TIMEOUT 0x17FC

/*******************************************************************************
 * NAME          : UnifiNanKeepAliveTimeoutCheck
 * PSID          : 6141 (0x17FD)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * UNITS         : seconds
 * MIN           : 1
 * MAX           : 100
 * DEFAULT       : 5
 * DESCRIPTION   :
 *  DO NOT SET TO A VALUE HIGHER THAN THE TIMEOUT. How long before keepalive
 *  timeout to start polling, in seconds. This value is read only once when
 *  an interface is added.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_KEEP_ALIVE_TIMEOUT_CHECK 0x17FD

/*******************************************************************************
 * NAME          : UnifiNanDataPathTrafficTimeout
 * PSID          : 6142 (0x17FE)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * UNITS         : seconds
 * MIN           : 0
 * MAX           : 2147
 * DEFAULT       : 60
 * DESCRIPTION   :
 *  Golden Certification MIB don't delete, change PSID or name. Timeout
 *  before terminating NDP for no data traffic in seconds. 0 = Disabled.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_DATA_PATH_TRAFFIC_TIMEOUT 0x17FE

/*******************************************************************************
 * NAME          : UnifiNanDataPathSetupTimeout
 * PSID          : 6143 (0x17FF)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * UNITS         : seconds
 * MIN           : 0
 * MAX           : 20
 * DEFAULT       : 8
 * DESCRIPTION   :
 *  Timeout for a NDP setup.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_DATA_PATH_SETUP_TIMEOUT 0x17FF

/*******************************************************************************
 * NAME          : UnifiNanWarmupTimeout
 * PSID          : 6144 (0x1800)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * UNITS         : seconds
 * MIN           : 0
 * MAX           : 2147
 * DEFAULT       : 120
 * DESCRIPTION   :
 *  Test MIB Only. Warmup time which FW doesn't set it's master preference.
 *  It's defined as 120s in spec.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_WARMUP_TIMEOUT 0x1800

/*******************************************************************************
 * NAME          : UnifiNanMultiVifSharedSlots
 * PSID          : 6146 (0x1802)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 9
 * MAX           : 9
 * DEFAULT       :
 * DESCRIPTION   :
 *  In multi vif scenarios, e.g. NAN + STA, NAN will share these slots with
 *  other VIFs. DW and NDC slots are not affected and are not removed due to
 *  MultiViF scenarios. Octet 0 : Bit Duration in TUs. Octet 1 ~ 2 : Period
 *  in TUs. Octet 3 ~ 4 : Start Offset in TUs. Octet 5 ~ 8 : TimeBitMap[4].
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_MULTI_VIF_SHARED_SLOTS 0x1802

/*******************************************************************************
 * NAME          : UnifiRoamingInNdlActivated
 * PSID          : 6147 (0x1803)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  When Activated, allow Roaming scan whilst NDL is active.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_ROAMING_IN_NDL_ACTIVATED 0x1803

/*******************************************************************************
 * NAME          : UnifiNanMultiBandNdlPermitted
 * PSID          : 6148 (0x1804)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Golden Certification MIB don't delete, change PSID or name. When
 *  permitted, NDLs can use multi-band schedules.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_MULTI_BAND_NDL_PERMITTED 0x1804

/*******************************************************************************
 * NAME          : UnifiLoggerNanTrafficReportPeriod
 * PSID          : 6149 (0x1805)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * UNITS         : ms
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       : 1000
 * DESCRIPTION   :
 *  Period in ms for sending NAN traffic updates to the host.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_LOGGER_NAN_TRAFFIC_REPORT_PERIOD 0x1805

/*******************************************************************************
 * NAME          : UnifiNanDiscoveryBeaconConfig
 * PSID          : 6151 (0x1807)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 7
 * DESCRIPTION   :
 *  Enables NAN discovery beacon transmission for NMI vif. Setting/clearing
 *  bit 0 enables/disabled discovery beacons on 2GHz band. Setting/clearing
 *  bit 1 enables/disabled discovery beacons on 5GHz band. Setting/clearing
 *  bit 3 enables/disabled discovery beacons when theres an NDL vif on same
 *  band.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_DISCOVERY_BEACON_CONFIG 0x1807

/*******************************************************************************
 * NAME          : UnifiNanForceWidestBandwidth
 * PSID          : 6152 (0x1808)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  When Activated, schedules on NDL VIF with same primary channel will have
 *  same bandwidth equal to widest one.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_FORCE_WIDEST_BANDWIDTH 0x1808

/*******************************************************************************
 * NAME          : UnifiNanScanBoRepeatCount
 * PSID          : 6153 (0x1809)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       : 3
 * DESCRIPTION   :
 *  The repeat count value to be used for blackouts installed for scan on NAN
 *  VIFs
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_SCAN_BO_REPEAT_COUNT 0x1809

/*******************************************************************************
 * NAME          : UnifiNanBlackoutFawReductionActivated
 * PSID          : 6154 (0x180A)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  When Activated, Blackouts aligned to FAWs with count == 255 are converted
 *  to FAW Reduction.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_BLACKOUT_FAW_REDUCTION_ACTIVATED 0x180A

/*******************************************************************************
 * NAME          : UnifiNanRangingActivated
 * PSID          : 6155 (0x180B)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Activate Ranging Support.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_RANGING_ACTIVATED 0x180B

/*******************************************************************************
 * NAME          : UnifiNanMaxConcurrentRangingAsInitiator
 * PSID          : 6156 (0x180C)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 8
 * DEFAULT       : 4
 * DESCRIPTION   :
 *  Maximum number of concurrent NAN Ranging Sessions supported as initiator.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_MAX_CONCURRENT_RANGING_AS_INITIATOR 0x180C

/*******************************************************************************
 * NAME          : UnifiNanMaxConcurrentRangingAsResponder
 * PSID          : 6157 (0x180D)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 8
 * DEFAULT       : 4
 * DESCRIPTION   :
 *  Maximum number of concurrent NAN Ranging Sessions supported as responder.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_MAX_CONCURRENT_RANGING_AS_RESPONDER 0x180D

/*******************************************************************************
 * NAME          : UnifiNanRangingSetupTimeout
 * PSID          : 6158 (0x180E)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * UNITS         : seconds
 * MIN           : 0
 * MAX           : 20
 * DEFAULT       : 8
 * DESCRIPTION   :
 *  Timeout for a Ranging setup.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_RANGING_SETUP_TIMEOUT 0x180E

/*******************************************************************************
 * NAME          : UnifiNanMaxRangingRetryCount
 * PSID          : 6159 (0x180F)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 1
 * MAX           : 65535
 * DEFAULT       : 4
 * DESCRIPTION   :
 *  Maximum number of retries FW does to perform a ranging (FTM burst
 *  sessions) and fails before terminating the ranging session. i.e. value of
 *  4 will result in termination of ranging session after 5 consecutive
 *  ranging failure.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_MAX_RANGING_RETRY_COUNT 0x180F

/*******************************************************************************
 * NAME          : UnifiNanTestRangingDistance
 * PSID          : 6160 (0x1810)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * UNITS         : millimeters
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  Test MIB for Distance which Ranging reports. When this MIB is set, FW
 *  doesn't use FTM for ranging anymore.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_TEST_RANGING_DISTANCE 0x1810

/*******************************************************************************
 * NAME          : UnifiNansdfTransmissionOutsideDwPermitted
 * PSID          : 6161 (0x1811)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Golden Certification MIB don't delete, change PSID or name. When this MIB
 *  is set FW is allowed to send SDF during remote CRB otherwise FW only
 *  sends SDF during DW.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NANSDF_TRANSMISSION_OUTSIDE_DW_PERMITTED 0x1811

/*******************************************************************************
 * NAME          : UnifiNanDeviceLinkSetupTimeout
 * PSID          : 6162 (0x1812)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * UNITS         : seconds
 * MIN           : 0
 * MAX           : 20
 * DEFAULT       : 8
 * DESCRIPTION   :
 *  Timeout for a NDL setup (re-negotiation).
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_DEVICE_LINK_SETUP_TIMEOUT 0x1812

/*******************************************************************************
 * NAME          : UnifiNanDeviceLinkScheduleUpdateRetryInterval
 * PSID          : 6163 (0x1813)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * UNITS         : seconds
 * MIN           : 0
 * MAX           : 127
 * DEFAULT       :
 * DESCRIPTION   :
 *  Retry interval of NDL renegotiation to fill empty slots. Set to zero for
 *  diabling retry.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_DEVICE_LINK_SCHEDULE_UPDATE_RETRY_INTERVAL 0x1813

/*******************************************************************************
 * NAME          : UnifiNanMultiVifDualBandConcurrencyPermitted
 * PSID          : 6164 (0x1814)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  In multi vif scenarios, e.g. NAN + STA, NAN use RSDB, when possible,
 *  instead of time sharing with other VIFs.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_MULTI_VIF_DUAL_BAND_CONCURRENCY_PERMITTED 0x1814

/*******************************************************************************
 * NAME          : UnifiNanMaxNafTxRetryCount
 * PSID          : 6165 (0x1815)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 254
 * DEFAULT       : 10
 * DESCRIPTION   :
 *  Maximum number of retries to send NAF(For NDP Responder and NDP Terminate
 *  at the moment.) to a peer. Currently set to 10 to account for peer being
 *  away due to calibration as seen in FIRM-100116
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_MAX_NAF_TX_RETRY_COUNT 0x1815

/*******************************************************************************
 * NAME          : UnifiNanFastConnectSlots
 * PSID          : 6166 (0x1816)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 9
 * MAX           : 9
 * DEFAULT       :
 * DESCRIPTION   :
 *  Define the slots that will additionally be added to the NAN NMI VIF for
 *  the Fast Connect feature. Octet 0 : Bit Duration in TUs. Octet 1 ~ 2 :
 *  Period in TUs. Octet 3 ~ 4 : Start Offset in TUs. Octet 5 ~ 8 :
 *  TimeBitMap[4].
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_FAST_CONNECT_SLOTS 0x1816

/*******************************************************************************
 * NAME          : UnifiNanFastConnectEnabled
 * PSID          : 6167 (0x1817)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Enable or disable the NAN Fast Connect feature.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_FAST_CONNECT_ENABLED 0x1817

/*******************************************************************************
 * NAME          : UnifiNanAcceleratedDiscoveryTimeout
 * PSID          : 6168 (0x1818)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * UNITS         : milliseconds
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       : 1000
 * DESCRIPTION   :
 *  The timeout duration for NAN Instant Comm. channel and Fast Connect
 *  feature
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_ACCELERATED_DISCOVERY_TIMEOUT 0x1818

/*******************************************************************************
 * NAME          : UnifiNanTestMasterPreference
 * PSID          : 6170 (0x181A)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 255
 * DEFAULT       :
 * DESCRIPTION   :
 *  Test MIB for Master Preference value. This overwrites host value, and to
 *  take effect this MIB should be set before NAN start(FW reads this MIB on
 *  start and config request signals.).
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_TEST_MASTER_PREFERENCE 0x181A

/*******************************************************************************
 * NAME          : UnifiNanMaxNdlCount
 * PSID          : 6171 (0x181B)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 8
 * DEFAULT       : 5
 * DESCRIPTION   :
 *  Maximum number of NAN NDL.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_MAX_NDL_COUNT 0x181B

/*******************************************************************************
 * NAME          : UnifiNanDelayDpResumeAfterSyncTx
 * PSID          : 6172 (0x181C)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  The delay in time (expressed in ms) between Sync beacon transmission and
 *  other frame transmission attempted on behalf of the NMI VIF.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_DELAY_DP_RESUME_AFTER_SYNC_TX 0x181C

/*******************************************************************************
 * NAME          : UnifiNanMaxConcurrentSubscribeAndPublish
 * PSID          : 6173 (0x181D)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 72
 * DEFAULT       : 72
 * DESCRIPTION   :
 *  Maximum number of concurrent NAN Publish + Subscribe instances supported.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_MAX_CONCURRENT_SUBSCRIBE_AND_PUBLISH 0x181D

/*******************************************************************************
 * NAME          : UnifiNanInstantCommSlots
 * PSID          : 6174 (0x181E)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 9
 * MAX           : 9
 * DEFAULT       :
 * DESCRIPTION   :
 *  Define the slots that will additionally be added to the NAN NMI VIF for
 *  the Instant Communication Channel feature. Octet 0 : Bit Duration in TUs.
 *  Octet 1 ~ 2 : Period in TUs. Octet 3 ~ 4 : Start Offset in TUs. Octet 5 ~
 *  8 : TimeBitMap[4].
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_INSTANT_COMM_SLOTS 0x181E

/*******************************************************************************
 * NAME          : UnifiNanInstantCommSupported
 * PSID          : 6176 (0x1820)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Enable or disable the NAN Fast Connect feature.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_INSTANT_COMM_SUPPORTED 0x1820

/*******************************************************************************
 * NAME          : UnifiNanRetrySdfTxTimeout
 * PSID          : 6177 (0x1821)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * UNITS         : milliseconds
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       : 200
 * DESCRIPTION   :
 *  The timeout duration for retrying sending SDFs when Instant Comm or Fast
 *  connect is active
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_RETRY_SDF_TX_TIMEOUT 0x1821

/*******************************************************************************
 * NAME          : UnifiNanUsdTimeout
 * PSID          : 6178 (0x1822)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 60
 * DESCRIPTION   :
 *  This is a FW internal MIB for FW to use a timer to end USD. This timer
 *  shall be always greater than 2 seconds (pause state timeout is 60 seconds
 + time to remain in SCM/MCM).
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_USD_TIMEOUT 0x1822

/*******************************************************************************
 * NAME          : UnifiNanGroupSecurityActivated
 * PSID          : 6179 (0x1823)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 0X00
 * DESCRIPTION   :
 *  Activates the usage of group security associations in the NAN engine Bit
 *  0: If set then IGTK is activated, else IGTK inactive by default (may be
 *  activated based on local and peer support for GTK). Bit 1: If set then
 *  BIGTK is activated, else BIGTK inactive.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_GROUP_SECURITY_ACTIVATED 0x1823

/*******************************************************************************
 * NAME          : UnifiNanAcceleratedDiscoveryConfig
 * PSID          : 6180 (0x1824)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 0X0003
 * DESCRIPTION   :
 *  Bitmap to configure NAN Fast connect behavior Bit 0: Set to 1, Retain the
 *  Fast connect slots even after service discovery. Bit 1: Set to 1,
 *  Advertise the Fast connect slots in the availability. Currently Bit 0 and
 *  Bit 1 are either both 1 or both 0, this can be changed in the future
 *  based on use case scenarios.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_ACCELERATED_DISCOVERY_CONFIG 0x1824

/*******************************************************************************
 * NAME          : UnifiNanUsdMcmModeActivated
 * PSID          : 6181 (0x1825)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Host can control activation of USD multi channel mode through this MIB.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_USD_MCM_MODE_ACTIVATED 0x1825

/*******************************************************************************
 * NAME          : UnifiNanUsdActivated
 * PSID          : 6182 (0x1826)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Enable or disable the NAN USD feature.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_USD_ACTIVATED 0x1826

/*******************************************************************************
 * NAME          : UnifiNanlpControl
 * PSID          : 6183 (0x1827)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       : 0X00000000
 * DESCRIPTION   :
 *  Control NAN Low Power behaviour. Refer to unifiNANLPControlBits for the
 *  full set of bit masks. b'0: Supress Deep Sleep if NAN vif exists
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NANLP_CONTROL 0x1827

/*******************************************************************************
 * NAME          : UnifiNanPairingActivated
 * PSID          : 6185 (0x1829)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : TRUE
 * DESCRIPTION   :
 *  Activates the usage of NAN Pairing to setup NMTKSA with a peer before NDP
 *  setup.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_PAIRING_ACTIVATED 0x1829

/*******************************************************************************
 * NAME          : UnifiNanSetupConfigFlags
 * PSID          : 6186 (0x182A)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 0X0000
 * DESCRIPTION   :
 *  Bitmap containing flags to control various aspects of NAN Configuration
 *  See unifiNANSetupConfigFlagsType for bit definitions bit 0: Set to 1 to
 *  disallow NAN 2G4 Operation while BT is enabled.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_SETUP_CONFIG_FLAGS 0x182A

/*******************************************************************************
 * NAME          : UnifiNanStdPlusVersion
 * PSID          : 6187 (0x182B)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 0X0801
 * DESCRIPTION   :
 *  FW implemented STD+ Version. 0x{minor}{major}
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_STD_PLUS_VERSION 0x182B

/*******************************************************************************
 * NAME          : UnifiNanPairingNumSessions
 * PSID          : 6188 (0x182C)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 1
 * MAX           : 64
 * DEFAULT       : 8
 * DESCRIPTION   :
 *  Maximum NAN pairing sessions active in FW.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_PAIRING_NUM_SESSIONS 0x182C

/*******************************************************************************
 * NAME          : UnifiNanPairingInactivityTimeout
 * PSID          : 6189 (0x182D)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * UNITS         : seconds
 * MIN           : 1
 * MAX           : 2147
 * DEFAULT       : 60
 * DESCRIPTION   :
 *  Timeout in seconds after which an inactive pairing session (no unicast
 *  management frames exchanged) will be marked as inactive.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_PAIRING_INACTIVITY_TIMEOUT 0x182D

/*******************************************************************************
 * NAME          : UnifiNanCertificationMib
 * PSID          : 6190 (0x182E)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Golden Certification MIB don't delete, change PSID or name: - Blocks
 *  counter proposal when DUT is responder.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_CERTIFICATION_MIB 0x182E

/*******************************************************************************
 * NAME          : UnifiNanehtActivated
 * PSID          : 6191 (0x182F)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Enable EHT mode for NAN.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NANEHT_ACTIVATED 0x182F

/*******************************************************************************
 * NAME          : UnifiNanStdPlusFrMaxDevices
 * PSID          : 6192 (0x1830)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 8
 * DEFAULT       : 5
 * DESCRIPTION   :
 *  Maximum number of Fast recovery entries/devices FW can have. 0 means not
 *  supported
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_STD_PLUS_FR_MAX_DEVICES 0x1830

/*******************************************************************************
 * NAME          : UnifiNanCtsPairingWorkaround
 * PSID          : 6193 (0x1831)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 3
 * DEFAULT       : 0X00
 * DESCRIPTION   :
 *  Refer FIRM-117100. Add a workaround for incomplete information from CTS
 *  test framework. Bit 0: If set, add a dummy SDA if not provided by
 *  framework to the bootstrapping follow-up SDFs Bit 1: If set, add a
 *  default cipher type if not provided by framework to the bootstrapping
 *  follow-up SDFs
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NAN_CTS_PAIRING_WORKAROUND 0x1831

/*******************************************************************************
 * NAME          : UnifiSarAsfCoeffs
 * PSID          : 6215 (0x1847)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt8
 * MIN           : -128
 * MAX           : 127
 * DEFAULT       :
 * DESCRIPTION   :
 *  ASF coefficients to convert power in dBm to SAR in dBm/kg. Units are
 *  0.0625 dB. These coefficients are per-band (index1: 1 for 2.4GHz, 2 for
 *  5GHz, 3 for 6GHz) per-antenna(index2) per-position(index3, look at
 *  unifiSarPositionTableIndexEnum). Default values are not provided as they
 *  are mobile phone specific as depend on antenna location, etc.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_SAR_ASF_COEFFS 0x1847

/*******************************************************************************
 * NAME          : UnifiSarNegligibleThreshold
 * PSID          : 6216 (0x1848)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 255
 * DEFAULT       : 5
 * DESCRIPTION   :
 *  Negligible SAR threshold in mW/kg. The value is used to check whether
 *  MLME-SAR.indication needs to be sent.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_SAR_NEGLIGIBLE_THRESHOLD 0x1848

/*******************************************************************************
 * NAME          : UnifiSarAlgorithm
 * PSID          : 6217 (0x1849)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 255
 * DEFAULT       :
 * DESCRIPTION   :
 *  Defines what SAR algorithm will be used. Options are defined in
 *  unifiSarAlgorithmEnum.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_SAR_ALGORITHM 0x1849

/*******************************************************************************
 * NAME          : UnifiStaticWlanSar
 * PSID          : 6218 (0x184A)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 255
 * DEFAULT       : 5
 * DESCRIPTION   :
 *  Static allocation of SAR for WLAN in mW/kg. This value is used in 2 ways:
 *  - it is added to SARlimit provided by HOST, - first SAR indication within
 *  short window is sent only if SAR exceeded unifiStaticWlanSar +
 *  unifiSarNegligibleThreshold, and unifiStaticWlanSar will be excluded from
 *  this indication. Rare further indications within the same window are sent
 *  when SAR accumulated exceeds unifiSarNegligibleThreshold.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_STATIC_WLAN_SAR 0x184A

/*******************************************************************************
 * NAME          : UnifiSarLimitUpperPerBand
 * PSID          : 6219 (0x184B)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       :
 * DESCRIPTION   :
 *  SAR corresponding to P_max(maximum power limit that can be used for
 *  WLAN). The unit is mW/kg. Let's say P_max for 2.4GHz band is 20dBm, and
 *  worst(greatest) ASF coefficient is 10dB/kg. That gives us
 *  SARlimit,upper,2G4 to be 30dBm/kg or 1000mW/kg. Index1: 1 for 2.4GHz, 2
 *  for 5GHz, 3 for 6GHz.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_SAR_LIMIT_UPPER_PER_BAND 0x184B

/*******************************************************************************
 * NAME          : UnifiSarMaxSarSplit
 * PSID          : 6220 (0x184C)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Select how SAR budget is split between two bands
 *******************************************************************************/
#define SLSI_PSID_UNIFI_SAR_MAX_SAR_SPLIT 0x184C

/*******************************************************************************
 * NAME          : UnifiPmax
 * PSID          : 6221 (0x184D)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt8
 * MIN           : -128
 * MAX           : 127
 * DEFAULT       :
 * DESCRIPTION   :
 *  Pmax (Maximum TX power) as a function of band(index 1) and
 *  antenna(index2) in quarter dBm. Default values are not provided as they
 *  are mobile phone specific.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_PMAX 0x184D

/*******************************************************************************
 * NAME          : UnifiSarAllowance
 * PSID          : 6222 (0x184E)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint16
 * MIN           : 0
 * MAX           : 65535
 * DEFAULT       : 700
 * DESCRIPTION   :
 *  SAR assigned to WLBT(WLAN + BT).
 *******************************************************************************/
#define SLSI_PSID_UNIFI_SAR_ALLOWANCE 0x184E

/*******************************************************************************
 * NAME          : UnifiTasPlimit
 * PSID          : 6223 (0x184F)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt8
 * MIN           : -128
 * MAX           : 127
 * DEFAULT       :
 * DESCRIPTION   :
 *  Plimits (power limits) are TX power which if being used(as
 *  nominal/average power) will lead to SAR equal to SAR allowance
 *  (unifiSarAllowance) being produced. These Plimits are per-band (index1: 1
 *  for 2.4GHz, 2 for 5GHz, 3 for 6GHz) per-antenna(index2) per DSI_ID(look
 *  at unifiDsiIdStandaloneTasTableIndexEnum). The units are quarter dBm.
 *  Default values are not provided as they are mobile phone specific as
 *  depend on antenna location, etc.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TAS_PLIMIT 0x184F

/*******************************************************************************
 * NAME          : UnifiTasRegulatory
 * PSID          : 6224 (0x1850)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 255
 * DEFAULT       : 0X7
 * DESCRIPTION   :
 *  Determines whether to use TAS or legacy backoff according to the
 *  regulatory domain. Options are defined in
 *  unifiTasEnableRegulatoryBitfield.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_TAS_REGULATORY 0x1850

/*******************************************************************************
 * NAME          : UnifiSupportedChannels
 * PSID          : 8012 (0x1F4C)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 0
 * MAX           : 20
 * DEFAULT       :
 * DESCRIPTION   :
 *  Supported 20MHz channel primary frequency grouped in sub-bands. For each
 *  sub-band: starting channel number, followed by number of channels. If
 *  range max is changed, please update MLME_IE_SUPPORTED_CHANNELS_MAX_TUPLES
 *  as well.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_SUPPORTED_CHANNELS 0x1F4C

/*******************************************************************************
 * NAME          : UnifiOperatingClassParamters
 * PSID          : 8015 (0x1F4F)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 1
 * MAX           : 89
 * DEFAULT       :
 * DESCRIPTION   :
 *  Supported Operating Class parameters. If range max is changed, please
 *  update MLME_IE_SUPPORTED_OPERATING_CLASSES_MAX_DATA as well.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_OPERATING_CLASS_PARAMTERS 0x1F4F

/*******************************************************************************
 * NAME          : UnifiNoCellMaxPower
 * PSID          : 8017 (0x1F51)
 * PER INTERFACE?: NO
 * TYPE          : SlsiInt16
 * MIN           : -32768
 * MAX           : 32767
 * DEFAULT       :
 * DESCRIPTION   :
 *  Max power values for included channels (quarter dBm).
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NO_CELL_MAX_POWER 0x1F51

/*******************************************************************************
 * NAME          : UnifiNoCellIncludedChannels
 * PSID          : 8018 (0x1F52)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint8
 * MIN           : 8
 * MAX           : 8
 * DEFAULT       : { 0X00, 0X18, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00 }
 * DESCRIPTION   :
 *  Channels applicable. Defined in a uint64 represented by the octet string.
 *  First byte of the octet string maps to LSB. Bit 0 maps to channel 1.
 *  Mapping defined in ChannelisationRules.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_NO_CELL_INCLUDED_CHANNELS 0x1F52

/*******************************************************************************
 * NAME          : UnifiDefaultCountryWithoutCH12CH13
 * PSID          : 8020 (0x1F54)
 * PER INTERFACE?: NO
 * TYPE          : SlsiBool
 * MIN           : 0
 * MAX           : 1
 * DEFAULT       : FALSE
 * DESCRIPTION   :
 *  Update the default country code to ensure CH12 and CH13 are not used.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_DEFAULT_COUNTRY_WITHOUT_CH12_CH13 0x1F54

/*******************************************************************************
 * NAME          : UnifiReadReg
 * PSID          : 8051 (0x1F73)
 * PER INTERFACE?: NO
 * TYPE          : SlsiUint32
 * MIN           : 0
 * MAX           : 4294967295
 * DEFAULT       :
 * DESCRIPTION   :
 *  Read value from a register and return it.
 *******************************************************************************/
#define SLSI_PSID_UNIFI_READ_REG 0x1F73

/* Need to check: Duplicated psid(6077) */
#define SLSI_PSID_UNIFI_NAN_MAX_QUEUED_FOLLOWUPS 0x17BD

/* After adding this PSID via MIB autogen, it should be removed */
#define SLSI_PSID_UNIFI_LAST_STA_CONNECTED_CHANNEL 0x0A3F

/* TODO: Deprecated mibs to be removed */
#define SLSI_PSID_UNIFI_WI_FI_SHARING5_GHZ_CHANNEL 0x0A16
#define SLSI_PSID_UNIFI_GOOGLE_MAX_NUMBER_OF_PERIODIC_SCANS 0x08D4
#define SLSI_PSID_UNIFI_GOOGLE_MAX_RSSI_SAMPLE_SIZE 0x08D5
#define SLSI_PSID_UNIFI_GOOGLE_MAX_HOTLIST_APS 0x08D6
#define SLSI_PSID_UNIFI_GOOGLE_MAX_SIGNIFICANT_WIFI_CHANGE_APS 0x08D7
#define SLSI_PSID_UNIFI_GOOGLE_MAX_BSSID_HISTORY_ENTRIES 0x08D8
#define SLSI_PSID_UNIFI_ROAM_SOFT_ROAMING_ENABLED 0x0806

#ifdef __cplusplus
}
#endif
#endif /* SLSI_MIB_H__ */
