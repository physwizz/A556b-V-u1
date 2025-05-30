#ifndef _UFS_VS_REGS_H_
#define _UFS_VS_REGS_H_

/* HCI */
#define HCI_TXPRDT_ENTRY_SIZE		0x1100
#define HCI_RXPRDT_ENTRY_SIZE		0x1104
#define HCI_TO_CNT_DIV_VAL              0x1108
#define HCI_1US_TO_CNT_VAL		0x110C
 #define CNT_VAL_1US_MASK	0x3ff
#define HCI_INVALID_UPIU_CTRL		0x1110
#define HCI_INVALID_UPIU_BADDR		0x1114
#define HCI_INVALID_UPIU_UBADDR		0x1118
#define HCI_INVALID_UTMR_OFFSET_ADDR	0x111C
#define HCI_INVALID_UTR_OFFSET_ADDR	0x1120
#define HCI_INVALID_DIN_OFFSET_ADDR	0x1124

#if defined(UFS_SFR_V1)
#define HCI_VENDOR_SPECIFIC_IS		0x1138
#define AH8_H8_ENTER		(1 << 13)

#define HCI_VENDOR_SPECIFIC_IE		0x113C
#define AH8_ERR_UECPA_EN		BIT(10)
#define AH8_ERR_UECDL_EN		BIT(9)
#define AH8_ERR_UECN_EN			BIT(8)
#define AH8_ERR_UECT_EN			BIT(7)
#define AH8_ERR_UECDME_EN		BIT(6)
/* When the controller is in auto-hibernation sequence and UIC error happens,
 * report as UIC error with IS.UE register.
 */
#define AH8_ERR_REPORT_UE		(AH8_ERR_UECPA_EN | AH8_ERR_UECDL_EN |\
					AH8_ERR_UECN_EN | AH8_ERR_UECT_EN |\
					AH8_ERR_UECDME_EN)

 #define AH8_H8_ENTER_EN	(1 << 13)
#endif

#define HCI_UTRL_NEXUS_TYPE		0x1140
#define HCI_UTMRL_NEXUS_TYPE		0x1144
#define HCI_E2EFC_CTRL			0x1148
#define HCI_SW_RST			0x1150
 #define UFS_LINK_SW_RST	(1 << 0)
 #define UFS_UNIPRO_SW_RST	(1 << 1)
 #define UFS_SW_RST_MASK	(UFS_UNIPRO_SW_RST | UFS_LINK_SW_RST)
#define HCI_LINK_VERSION		0x1154
#define HCI_IDLE_TIMER_CONFIG		0x1158
#define HCI_RX_UPIU_MATCH_ERROR_CODE	0x115C
#define HCI_DATA_REORDER		0x1160
#define HCI_MAX_DOUT_DATA_SIZE		0x1164
#define HCI_UNIPRO_APB_CLK_CTRL		0x1168
#define HCI_AXIDMA_RWDATA_BURST_LEN	0x116C
 #define BURST_LEN(x)			((x) << 27 | (x))
 #define WLU_EN				(1 << 31)
 #define AXIDMA_RWDATA_BURST_LEN	(0xF)
#define HCI_GPIO_OUT			0x1170
#define HCI_WRITE_DMA_CTRL		0x1174
#define HCI_ERROR_EN_PA_LAYER		0x1178
#define HCI_ERROR_EN_DL_LAYER		0x117C
#define HCI_ERROR_EN_N_LAYER		0x1180
#define HCI_ERROR_EN_T_LAYER		0x1184
#define HCI_ERROR_EN_DME_LAYER		0x1188
#define HCI_UFSHCI_V2P1_CTRL		0x118C
 #define AH8_RUN_BY_SW_H8			BIT(27)
 #define IA_TICK_SEL				BIT(16)
 #define UIC_CMD_COMPLETE_SEL			BIT(8)
 #define NEXUS_TYPE_SELECT_ENABLE		BIT(1)
#define HCI_REQ_HOLD_EN			0x11AC
 #define HCI_CONTROLLER_DP		BIT(1)

#define HCI_CLKSTOP_CTRL		0x11B0
 #define REFCLKOUT_STOP			BIT(4)
 #define MPHY_APBCLK_STOP		BIT(3)
 #define REFCLK_STOP			BIT(2)
 #define UNIPRO_MCLK_STOP		BIT(1)
 #define UNIPRO_PCLK_STOP		BIT(0)
 #define CLK_STOP_ALL		(REFCLKOUT_STOP |\
					REFCLK_STOP |\
					UNIPRO_MCLK_STOP |\
					UNIPRO_PCLK_STOP)

#define HCI_FORCE_HCS			0x11B4
 #define REFCLKOUT_STOP_EN	BIT(11)
 #define MPHY_APBCLK_STOP_EN	BIT(10)
 #define UFSP_DRCG_EN		BIT(8)   //FMP
 #define REFCLK_STOP_EN		BIT(7)
 #define UNIPRO_PCLK_STOP_EN	BIT(6)
 #define UNIPRO_MCLK_STOP_EN	BIT(5)
 #define HCI_CORECLK_STOP_EN	BIT(4)
 #define CLK_STOP_CTRL_EN_ALL	(UFSP_DRCG_EN |\
					MPHY_APBCLK_STOP_EN |\
					REFCLKOUT_STOP_EN |\
					REFCLK_STOP_EN |\
					UNIPRO_PCLK_STOP_EN |\
					UNIPRO_MCLK_STOP_EN)

#define HCI_FSM_MONITOR			0x11C0
#define HCI_PRDT_HIT_RATIO		0x11C4
#define HCI_DMA0_MONITOR_STATE		0x11C8
#define HCI_DMA0_MONITOR_CNT		0x11CC
#define HCI_DMA1_MONITOR_STATE		0x11D0
#define HCI_DMA1_MONITOR_CNT		0x11D4
#define HCI_DMA0_DOORBELL_DEBUG		0x11D8
#define HCI_DMA1_DOORBELL_DEBUG		0x11DC

#define HCI_UFS_AXI_DMA_IF_CTRL		0x11F8
#define HCI_UFS_ACG_DISABLE		0x11FC
 #define HCI_UFS_ACG_DISABLE_EN		BIT(0)
#define HCI_IOP_ACG_DISABLE		0x1200
 #define HCI_IOP_ACG_DISABLE_EN		BIT(0)
#define HCI_MPHY_REFCLK_SEL		0x1208
 #define MPHY_REFCLK_SEL		BIT(0)

/*
 * This type makes 1st DW and another DW be logged.
 * The second one is the head of CDB for COMMAND UPIU and
 * the head of data for DATA UPIU.
 */
#define HCI_PH_CPORT_LOG_CTRL		0x1210
#define CPORT_LOG_EN			BIT(0)
#define HCI_PH_CPORT_LOG_CFG		0x1214
#define TX_LOG_TYPE			BIT(1)
#define RX_LOG_TYPE			BIT(5)
#define CPORT_LOG_TYPE			(TX_LOG_TYPE | RX_LOG_TYPE)

#define HCI_SMU_RD_ABORT_MATCH_INFO		0x1218
#define HCI_SMU_WR_ABORT_MATCH_INFO		0x121C
#define HCI_DBR_DUPLICATION_INFO		0x1220
#define HCI_INVALID_PRDT_CTRL		0x1230

#define HCI_UTRL_DBR_3_0_TIMER_EXPIRED_VALUE		0x1260
#define HCI_UTRL_DBR_7_4_TIMER_EXPIRED_VALUE		0x1264
#define HCI_UTRL_DBR_11_8_TIMER_EXPIRED_VALUE		0x1268
#define HCI_UTRL_DBR_15_12_TIMER_EXPIRED_VALUE		0x126C
#define HCI_UTRL_DBR_19_16_TIMER_EXPIRED_VALUE		0x1270
#define HCI_UTRL_DBR_23_20_TIMER_EXPIRED_VALUE		0x1274
#define HCI_UTRL_DBR_27_24_TIMER_EXPIRED_VALUE		0x1278
#define HCI_UTRL_DBR_31_28_TIMER_EXPIRED_VALUE		0x127C

#define HCI_UTMRL_DBR_3_0_TIMER_EXPIRED_VALUE		0x1280

#define HCI_AH8_RESET			0x1600
 #define HCI_CLEAR_AH8_FSM		BIT(0)
 #define UFSHCD_AH8_CHECK_IS		0x199A
#define HCI_AH8_STATE			0x160C
 #define HCI_AH8_STATE_ERROR		BIT(16)
 #define HCI_AH8_HIBERNATION_STATE	BIT(8)
 #define HCI_AH8_IDLE_STATE		BIT(0)

#if defined(UFS_SFR_V1)
#else
#define HCI_VENDOR_SPECIFIC_IS_ERR_AH8		0x1360
 #define HCI_AH8_TIMEOUT		BIT(12)
#define HCI_VENDOR_SPECIFIC_IS_ERR_UTP		0x1370
 #define HCI_INVALID_PRDT		BIT(2)
 #define HCI_RX_UPIU_HIT_ERROR		BIT(1)
 #define HCI_RX_UPIU_LEN_ERROR		BIT(0)
#define HCI_UTP_ERR_REPORT		(HCI_INVALID_PRDT | HCI_RX_UPIU_HIT_ERROR |\
					HCI_RX_UPIU_LEN_ERROR)

#define HCI_AH8_UE_CONFIG		0x162C
 #define HCI_AH8_UECPA_ERROR		BIT(4)
 #define HCI_AH8_UECDL_ERROR		BIT(3)
 #define HCI_AH8_UECN_ERROR		BIT(2)
 #define HCI_AH8_UECT_ERROR		BIT(1)
 #define HCI_AH8_UECDME_ERROR		BIT(0)
#define HCI_AH8_ERR_REPORT		(HCI_AH8_UECPA_ERROR | HCI_AH8_UECDL_ERROR |\
					HCI_AH8_UECN_ERROR | HCI_AH8_UECT_ERROR |\
					HCI_AH8_UECDME_ERROR)
#endif

#define HCI_CLKMODE			0x1810
 #define REF_CLK_MODE			(2 << 12)
 #define PMA_CLKDIV_VAL			(2 << 8)

#define HCI_AXIDMA_FSM_TIMER		0x181C
 #define AXIDMA_FSM_TIMER(x)		(x << 16)
 #define AXIDMA_FSM_TIMER_ENABLE	BIT(0)

/* TIMEOUT VALUE (10ms) */
 #define AXIDMA_FSM_TIMER_VALUE		0x2710

/* MCQ registers */
#define REG_MCQIACR	0x8
#define REG_SQCFG	0x14
#define REG_CQCFG	0x34

#define CQE_UCD_BA GENMASK_ULL(63, 7)
#define MCQIACR_IAEN	BIT(31)
#define MCQIACR_IAPWEN	BIT(24)
#define MCQIACR_CTR	BIT(16)
#define MCQIACR_IACTH	BIT(8)
#define MCQIACR_OVAL_T	0xFF


/* Device fatal error */
#define DFES_ERR_EN	BIT(31)
#define DFES_DEF_DL_ERRS	(UIC_DATA_LINK_LAYER_ERROR_RX_BUF_OF |\
				 UIC_DATA_LINK_LAYER_ERROR_PA_INIT)
#define DFES_DEF_N_ERRS		(UIC_NETWORK_UNSUPPORTED_HEADER_TYPE |\
				 UIC_NETWORK_BAD_DEVICEID_ENC |\
				 UIC_NETWORK_LHDR_TRAP_PACKET_DROPPING)
#define DFES_DEF_T_ERRS		(UIC_TRANSPORT_UNSUPPORTED_HEADER_TYPE |\
				 UIC_TRANSPORT_UNKNOWN_CPORTID |\
				 UIC_TRANSPORT_NO_CONNECTION_RX |\
				 UIC_TRANSPORT_BAD_TC)

/* TXPRDT defines */
#define PRDT_PREFECT_EN		BIT(31)
#define PRDT_SET_SIZE(x)	((x) & 0x1F)

/*
 * UNIPRO registers
 */
#define UNIP_COMP_VERSION			0x000
#define UNIP_COMP_INFO				0x004
#define UNIP_COMP_RESET				0x010

#define UNIP_PA_AVAILTXDATALENS			0x3080	/* PA_AvailTxDataLanes */
#define UNIP_PA_AVAILRXDATALENS			0x3100	/* PA_AvailRxDataLanes */
#define UNIP_PA_ACTIVETXDATALENS		0x3180	/* PA_ActiveTxDataLanes */
#define UNIP_PA_CONNECTEDTXDATALENS		0x3184	/* PA_ConnectedTxDataLanes */
#define UNIP_PA_TXGEAR				0x31A0	/* PA_TxGear */
#define UNIP_PA_TXTERMINATION			0x31A4	/* PA_TxTermination */
#define UNIP_PA_HSSERIES			0x31A8	/* PA_HSSeries */
#define UNIP_PA_PWRMODE				0x31C4	/* PA_PWRMode */

#define UNIP_PA_ACTIVERXDATALENS		0x3200	/* PA_ActiveRxDataLanes */
#define UNIP_PA_CONNECTEDRXDATALENS		0x3204	/* PA_ConnectedRxDataLanes */
#define UNIP_PA_RXGEAR				0x320C	/* PA_RxGear */
#define UNIP_PA_RXTERMINATION			0x3210	/* PA_RXTERMINATION */
#define UNIP_PA_MAXRXHSGEAR			0x321C	/* PA_MaxRxHSGear */
#define UNIP_PA_HIBERN8TIME			0x329C	/* PA_Hibern8Time */
#define UNIP_PA_DBG_RESUME_HIBERNATE		0x3940	/* PA_DBG_RESUME_HIBERNATE */

#define RX_PWRMODE(x)		((x) << 0)
#define TX_PWRMODE(x)		((x) << 4)

#define UNIP_PA_DBG_OPTION_SUITE_1   0x39A8
#define UNIP_PA_DBG_OPTION_SUITE_2   0x39B4
#define DBG_SUITE1_ENABLE       0x90913C1C
#define DBG_SUITE2_ENABLE       0xE01C115F
#define DBG_SUITE1_DISABLE      0x98913C1C
#define DBG_SUITE2_DISABLE      0xE01C195F

#define UNIP_DBG_RX_INFO_CONTROL_DIRECT		0x4818	/* error injection */

#define UNIP_DME_POWERON_REQ			0x7800
#define UNIP_DME_POWERON_CNF_RESULT		0x7804
#define UNIP_DME_POWEROFF_REQ			0x7810
#define UNIP_DME_POWEROFF_CNF_RESULT		0x7814
#define UNIP_DME_RESET_REQ			0x7820
#define UNIP_DME_RESET_REQ_LEVEL		0x7824
#define UNIP_DME_ENABLE_REQ			0x7830
#define UNIP_DME_ENABLE_CNF_RESULT		0x7834
#define UNIP_DME_ENDPOINTRESET_REQ		0x7840
#define UNIP_DME_ENDPOINTRESET_CNF_RESULT	0x7844
#define UNIP_DME_LINKSTARTUP_REQ		0x7850
#define UNIP_DME_LINKSTARTUP_CNF_RESULT		0x7854
#define UNIP_DME_HIBERN8_ENTER_REQ		0x7860
#define UNIP_DME_HIBERN8_ENTER_CNF_RESULT	0x7864
#define UNIP_DME_HIBERN8_ENTER_IND_RESULT	0x7868
#define UNIP_DME_HIBERN8_EXIT_REQ		0x7870
#define UNIP_DME_HIBERN8_EXIT_CNF_RESULT	0x7874
#define UNIP_DME_HIBERN8_EXIT_IND_RESULT	0x7878
#define UNIP_DME_PWR_REQ			0x7880
#define UNIP_DME_PWR_REQ_POWERMODE		0x7884
#define UNIP_DME_PWR_REQ_LOCALL2TIMER0		0x7888
#define UNIP_DME_PWR_REQ_LOCALL2TIMER1		0x788C
#define UNIP_DME_PWR_REQ_LOCALL2TIMER2		0x7890
#define UNIP_DME_PWR_REQ_REMOTEL2TIMER0		0x78B8
#define UNIP_DME_PWR_REQ_REMOTEL2TIMER1		0x78BC
#define UNIP_DME_PWR_REQ_REMOTEL2TIMER2		0x78C0
#define UNIP_DME_PWR_CNF_RESULT			0x78E8
#define UNIP_DME_PWR_IND_RESULT			0x78EC
#define UNIP_DME_TEST_MODE_REQ			0x7900
#define UNIP_DME_TEST_MODE_CNF_RESULT		0x7904

#define UNIP_DME_ERROR_IND_LAYER		0x0C0
#define UNIP_DME_ERROR_IND_ERRCODE		0x0C4
#define UNIP_DME_PACP_CNFBIT			0x0C8
#define UNIP_DME_DL_FRAME_IND			0x0D0
#define UNIP_DME_INTR_STATUS			0x0E0
#define UNIP_DME_INTR_ENABLE			0x0E4

#define UNIP_DME_GETSET_CONTROL                0x7A00
#define UNIP_DME_GETSET_ADDR                   0x7A04
#define UNIP_DME_GETSET_WDATA                  0x7A08
#define UNIP_DME_GETSET_RDATA                  0x7A0C
#define UNIP_DME_GETSET_RESULT                 0x7A10
#define UNIP_DME_PEER_GETSET_CONTROL           0x7A20
#define UNIP_DME_PEER_GETSET_ADDR              0x7A24
#define UNIP_DME_PEER_GETSET_WDATA             0x7A28
#define UNIP_DME_PEER_GETSET_RDATA             0x7A2C
#define UNIP_DME_PEER_GETSET_RESULT            0x7A30

#define UNIP_DME_INTR_STATUS_LSB			   0x7B00
#define UNIP_DME_INTR_STATUS_MSB	           0x7B04
#define UNIP_DME_INTR_ERROR_CODE			   0x7B20
#define UNIP_DME_DISCARD_PORT_ID	           0x7B24
#define UNIP_DME_DBG_OPTION_SUITE			   0x7C00
#define UNIP_DME_DBG_CTRL_FSM		           0x7D00
#define UNIP_DME_DBG_FLAG_STATUS			   0x7D14
#define UNIP_DME_DBG_LINKCFG_FSM	           0x7D18

#define UNIP_DME_INTR_ERROR_CODE		0x7B20
#define UNIP_DME_DEEPSTALL_ENTER_REQ		0x7910
#define UNIP_DME_DISCARD_CPORT_ID		0x7B24

#define UNIP_DBG_FORCE_DME_CTRL_STATE		0x150
#define UNIP_DBG_AUTO_DME_LINKSTARTUP		0x158
#define UNIP_DBG_PA_CTRLSTATE			0x15C
#define UNIP_DBG_PA_TX_STATE			0x160
#define UNIP_DBG_BREAK_DME_CTRL_STATE		0x164
#define UNIP_DBG_STEP_DME_CTRL_STATE		0x168
#define UNIP_DBG_NEXT_DME_CTRL_STATE		0x16C

/*
 * PCS registers
 */
#define RX_EYEMON_CAPA				0x83C4
#define RX_EYEMON_T_MAX_STEP_CAPA		0x83C8
#define RX_EYEMON_T_MAX_OFFSET_CAPA		0x83CC
#define RX_EYEMON_V_MAX_STEP_CAPA		0x83D0
#define RX_EYEMON_V_MAX_OFFSET_CAPA		0x83D4
#define RX_EYEMON_ENABLE			0x83D8
#define RX_EYEMON_T_STEP			0x83DC
#define RX_EYEMON_V_STEP			0x83E0
#define RX_EYEMON_TARGET_TEST_CNT		0x83E4
#define RX_EYEMON_TARGET_TESTED_CNT		0x83E8
#define RX_EYEMON_ERROR_CNT			0x83EC
#define RX_EYEMON_START				0x83F0
/*
 * PCS register for device
 */
#define RX_EYEMON_DEVICE_CAPA				0xF1
#define RX_EYEMON_DEVICE_CT_MAX_STEP_CAPA		0xF2
#define RX_EYEMON_DEVICE_CT_MAX_OFFSET_CAPA		0xF3
#define RX_EYEMON_DEVICE_CV_MAX_STEP_CAPA		0xF4
#define RX_EYEMON_DEVICE_CV_MAX_OFFSET_CAPA		0xF5
#define RX_EYEMON_DEVICE_ENABLE				0xF6
#define RX_EYEMON_DEVICE_CT_STEP			0xF7
#define RX_EYEMON_DEVICE_CV_STEP			0xF8
#define RX_EYEMON_DEVICE_CTARGET_TEST_CNT		0xF9
#define RX_EYEMON_DEVICE_CTARGET_TESTED_CNT		0xFA
#define RX_EYEMON_DEVICE_CERROR_CNT			0xFB
#define RX_EYEMON_DEVICE_CSTART				0xFC
#define RX_EYEMON_DEVICE_LANE_BASE			0x4

#endif /* _UFS_VS_REGS_H_ */
