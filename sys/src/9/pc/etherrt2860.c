/* 
 * Ralink RT2860 driver
 *
 * Written without any documentation but Damien Bergaminis
 * OpenBSD ral(4) driver sources. Requires ralink firmware
 * to be present in /lib/firmware/ral-rt2860 on attach.
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/error.h"
#include "../port/netif.h"

#include "etherif.h"
#include "wifi.h"

/* for consistency */
typedef signed char s8int;

enum {
	/* PCI registers */
	PciCfg = 0x0000,
		PciCfgUsb = (1 << 17),
		PciCfgPci = (1 << 16),
	PciEectrl = 0x0004,
		EectrlC = (1 << 0),
		EectrlS = (1 << 1),
		EectrlD = (1 << 2),
		EectrlShiftD = 2,
		EectrlQ = (1 << 3),
		EectrlShiftQ = 3,
	PciMcuctrl = 0x0008,
	PciSysctrl = 0x000c,
	PcieJtag = 0x0010,

	Rt3090AuxCtrl = 0x010c,

	Rt3070Opt14 = 0x0114,
};

enum {
	/* SCH/DMA registers */
	IntStatus = 0x0200,
		/* flags for registers IntStatus/IntMask */
		TxCoherent = (1 << 17),
		RxCoherent = (1 << 16),
		MacInt4 = (1 << 15),
		MacInt3 = (1 << 14),
		MacInt2 = (1 << 13),
		MacInt1 = (1 << 12),
		MacInt0 = (1 << 11),
		TxRxCoherent = (1 << 10),
		McuCmdInt = (1 << 9),
		TxDoneInt5 = (1 << 8),
		TxDoneInt4 = (1 << 7),
		TxDoneInt3 = (1 << 6),
		TxDoneInt2 = (1 << 5),
		TxDoneInt1 = (1 << 4),
		TxDoneInt0 = (1 << 3),
		RxDoneInt = (1 << 2),
		TxDlyInt = (1 << 1),
		RxDlyInt = (1 << 0),
	IntMask = 0x0204,
	WpdmaGloCfg = 0x0208,
		HdrSegLenShift = 8,
		BigEndian = (1 << 7),
		TxWbDdone = (1 << 6),
		WpdmaBtSizeShift = 4,
		WpdmaBtSize16 = 0,
		WpdmaBtSize32 = 1,
		WpdmaBtSize64 = 2,
		WpdmaBtSize128 = 3,
		RxDmaBusy = (1 << 3),
		RxDmaEn = (1 << 2),
		TxDmaBusy = (1 << 1),
		TxDmaEn = (1 << 0),
	WpdmaRstIdx = 0x020c,
	DelayIntCfg = 0x0210,
		TxdlyIntEn = (1 << 31),
		TxmaxPintShift = 24,
		TxmaxPtimeShift = 16,
		RxdlyIntEn = (1 << 15),
		RxmaxPintShift = 8,
		RxmaxPtimeShift = 0,
	WmmAifsnCfg = 0x0214,
	WmmCwminCfg = 0x0218,
	WmmCwmaxCfg = 0x021c,
	WmmTxop0Cfg = 0x0220,
	WmmTxop1Cfg = 0x0224,
	GpioCtrl = 0x0228,
		GpioDShift = 8,
		GpioOShift = 0,
	McuCmdReg = 0x022c,
#define TxBasePtr(qid) (0x0230 + (qid) * 16)
#define TxMaxCnt(qid) (0x0234 + (qid) * 16)
#define TxCtxIdx(qid) (0x0238 + (qid) * 16)
#define TxDtxIdx(qid) (0x023c + (qid) * 16)
	RxBasePtr = 0x0290,
	RxMaxCnt = 0x0294,
	RxCalcIdx = 0x0298,
	FsDrxIdx = 0x029c,
	UsbDmaCfg = 0x02a0 /* RT2870 only */,
		UsbTxBusy = (1 << 31),
		UsbRxBusy = (1 << 30),
		UsbEpoutVldShift = 24,
		UsbTxEn = (1 << 23),
		UsbRxEn = (1 << 22),
		UsbRxAggEn = (1 << 21),
		UsbTxopHalt = (1 << 20),
		UsbTxClear = (1 << 19),
		UsbPhyWdEn = (1 << 16),
		UsbPhyManRst = (1 << 15),
#define UsbRxAggLmt(x) ((x) << 8) /* in unit of 1KB */
#define UsbRxAggTo(x) ((x) & 0xff) /* in unit of 33ns */
	UsCycCnt = 0x02a4,
		TestEn = (1 << 24),
		TestSelShift = 16,
		BtModeEn = (1 << 8),
		UsCycCntShift = 0,
};

enum {
	/* PBF registers */
	SysCtrl = 0x0400,
		HstPmSel = (1 << 16),
		CapMode = (1 << 14),
		PmeOen = (1 << 13),
		Clkselect = (1 << 12),
		PbfClkEn = (1 << 11),
		MacClkEn = (1 << 10),
		DmaClkEn = (1 << 9),
		McuReady = (1 << 7),
		AsyReset = (1 << 4),
		PbfReset = (1 << 3),
		MacReset = (1 << 2),
		DmaReset = (1 << 1),
		McuReset = (1 << 0),
	HostCmd = 0x0404,
		McuCmdSleep = 0x30,
		McuCmdWakeup = 0x31,
		McuCmdLeds = 0x50,
			LedRadio = (1 << 13),
			LedLink2ghz = (1 << 14),
			LedLink5ghz = (1 << 15),
		McuCmdLedRssi = 0x51,
		McuCmdLed1 = 0x52,
		McuCmdLed2 = 0x53,
		McuCmdLed3 = 0x54,
		McuCmdRfreset = 0x72,
		McuCmdAntsel = 0x73,
		McuCmdBbp = 0x80,
		McuCmdPslevel = 0x83,
	PbfCfg = 0x0408,
		Tx1qNumShift = 21,
		Tx2qNumShift = 16,
		Null0Mode = (1 << 15),
		Null1Mode = (1 << 14),
		RxDropMode = (1 << 13),
		Tx0qManual = (1 << 12),
		Tx1qManual = (1 << 11),
		Tx2qManual = (1 << 10),
		Rx0qManual = (1 << 9),
		HccaEn = (1 << 8),
		Tx0qEn = (1 << 4),
		Tx1qEn = (1 << 3),
		Tx2qEn = (1 << 2),
		Rx0qEn = (1 << 1),
	MaxPcnt = 0x040c,
	BufCtrl = 0x0410,
#define WriteTxq(qid) (1 << (11 - (qid)))
		Null0Kick = (1 << 7),
		Null1Kick = (1 << 6),
		BufReset = (1 << 5),
#define ReadTxq(qid) = (1 << (3 - (qid))
		ReadRx0q = (1 << 0),
	McuIntSta = 0x0414,
		/* flags for registers McuIntSta/McuIntEna */
		McuMacInt8 = (1 << 24),
		McuMacInt7 = (1 << 23),
		McuMacInt6 = (1 << 22),
		McuMacInt4 = (1 << 20),
		McuMacInt3 = (1 << 19),
		McuMacInt2 = (1 << 18),
		McuMacInt1 = (1 << 17),
		McuMacInt0 = (1 << 16),
		Dtx0Int = (1 << 11),
		Dtx1Int = (1 << 10),
		Dtx2Int = (1 << 9),
		Drx0Int = (1 << 8),
		HcmdInt = (1 << 7),
		N0txInt = (1 << 6),
		N1txInt = (1 << 5),
		BcntxInt = (1 << 4),
		Mtx0Int = (1 << 3),
		Mtx1Int = (1 << 2),
		Mtx2Int = (1 << 1),
		Mrx0Int = (1 << 0),
	McuIntEna = 0x0418,
#define TxqIo(qid) (0x041c + (qid) * 4)
	Rx0qIo = 0x0424,
	BcnOffset0 = 0x042c,
	BcnOffset1 = 0x0430,
	TxrxqSta = 0x0434,
	TxrxqPcnt = 0x0438,
		Rx0qPcntMask = 0xff000000,
		Tx2qPcntMask = 0x00ff0000,
		Tx1qPcntMask = 0x0000ff00,
		Tx0qPcntMask = 0x000000ff,
	PbfDbg = 0x043c,
	CapCtrl = 0x0440,
		CapAdcFeq = (1 << 31),
		CapStart = (1 << 30),
		ManTrig = (1 << 29),
		TrigOffsetShift = 16,
		StartAddrShift = 0,
};

enum {
	/* RT3070 registers */
	Rt3070RfCsrCfg = 0x0500,
		Rt3070RfKick = (1 << 17),
		Rt3070RfWrite = (1 << 16),
	Rt3070EfuseCtrl = 0x0580,
		Rt3070SelEfuse = (1 << 31),
		Rt3070EfsromKick = (1 << 30),
		Rt3070EfsromAinMask = 0x03ff0000,
		Rt3070EfsromAinShift = 16,
		Rt3070EfsromModeMask = 0x000000c0,
		Rt3070EfuseAoutMask = 0x0000003f,
	Rt3070EfuseData0 = 0x0590,
	Rt3070EfuseData1 = 0x0594,
	Rt3070EfuseData2 = 0x0598,
	Rt3070EfuseData3 = 0x059c,
	Rt3090OscCtrl = 0x05a4,
	Rt3070LdoCfg0 = 0x05d4,
	Rt3070GpioSwitch = 0x05dc,
};

enum {
	/* MAC registers */
	AsicVerId = 0x1000,
	MacSysCtrl = 0x1004,
		RxTsEn = (1 << 7),
		WlanHaltEn = (1 << 6),
		PbfLoopEn = (1 << 5),
		ContTxTest = (1 << 4),
		MacRxEn = (1 << 3),
		MacTxEn = (1 << 2),
		BbpHrst = (1 << 1),
		MacSrst = (1 << 0),
	MacAddrDw0 = 0x1008,
	MacAddrDw1 = 0x100c,
	MacBssidDw0 = 0x1010,
	MacBssidDw1 = 0x1014,
		MultiBcnNumShift = 18,
		MultiBssidModeShift = 16,
	MaxLenCfg = 0x1018,
		MinMpduLenShift = 16,
		MaxPsduLenShift = 12,
		MaxPsduLen8k = 0,
		MaxPsduLen16k = 1,
		MaxPsduLen32k = 2,
		MaxPsduLen64k = 3,
		MaxMpduLenShift = 0,
	BbpCsrCfg = 0x101c,
		BbpRwParallel = (1 << 19),
		BbpParDur1125 = (1 << 18),
		BbpCsrKick = (1 << 17),
		BbpCsrRead = (1 << 16),
		BbpAddrShift = 8,
		BbpDataShift = 0,
	RfCsrCfg0 = 0x1020,
		RfRegCtrl = (1 << 31),
		RfLeSel1 = (1 << 30),
		RfLeStby = (1 << 29),
		RfRegWidthShift = 24,
		RfReg0Shift = 0,
	RfCsrCfg1 = 0x1024,
		RfDur5 = (1 << 24),
		RfReg1Shift = 0,
	RfCsrCfg2 = 0x1028,
	LedCfg = 0x102c,
		LedPol = (1 << 30),
		YLedModeShift = 28,
		GLedModeShift = 26,
		RLedModeShift = 24,
		LedModeOff = 0,
		LedModeBlinkTx = 1,
		LedModeSlowBlink = 2,
		LedModeOn = 3,
		SlowBlkTimeShift = 16,
		LedOffTimeShift = 8,
		LedOnTimeShift = 0,
};

enum {
	/* undocumented registers */
	Debug = 0x10f4,
};

enum {
	/* MAC Timing control registers */
	XifsTimeCfg = 0x1100,
		BbRxendEn = (1 << 29),
		EifsTimeShift = 20,
		OfdmXifsTimeShift = 16,
		OfdmSifsTimeShift = 8,
		CckSifsTimeShift = 0,
	BkoffSlotCfg = 0x1104,
		CcDelayTimeShift = 8,
		SlotTime = 0,
	NavTimeCfg = 0x1108,
		NavUpd = (1 << 31),
		NavUpdValShift = 16,
		NavClrEn = (1 << 15),
		NavTimerShift = 0,
	ChTimeCfg = 0x110c,
		EifsAsChBusy = (1 << 4),
		NavAsChBusy = (1 << 3),
		RxAsChBusy = (1 << 2),
		TxAsChBusy = (1 << 1),
		ChStaTimerEn = (1 << 0),
	PbfLifeTimer = 0x1110,
	BcnTimeCfg = 0x1114,
		TsfInsCompShift = 24,
		BcnTxEn = (1 << 20),
		TbttTimerEn = (1 << 19),
		TsfSyncModeShift = 17,
		TsfSyncModeDis = 0,
		TsfSyncModeSta = 1,
		TsfSyncModeIbss = 2,
		TsfSyncModeHostap = 3,
		TsfTimerEn = (1 << 16),
		BcnIntvalShift = 0,
	TbttSyncCfg = 0x1118,
		BcnCwminShift = 20,
		BcnAifsnShift = 16,
		BcnExpWinShift = 8,
		TbttAdjustShift = 0,
	TsfTimerDw0 = 0x111c,
	TsfTimerDw1 = 0x1120,
	TbttTimer = 0x1124,
	IntTimerCfg = 0x1128,
		GpTimerShift = 16,
		PreTbttTimerShift = 0,
	IntTimerEn = 0x112c,
		GpTimerEn = (1 << 1),
		PreTbttIntEn = (1 << 0),
	ChIdleTime = 0x1130,
};

enum {
	/* MAC Power Save configuration registers */
	MacStatusReg = 0x1200,
		RxStatusBusy = (1 << 1),
		TxStatusBusy = (1 << 0),
	PwrPinCfg = 0x1204,
		IoAddaPd = (1 << 3),
		IoPllPd = (1 << 2),
		IoRaPe = (1 << 1),
		IoRfPe = (1 << 0),
	AutoWakeupCfg = 0x1208,
		AutoWakeupEn = (1 << 15),
		SleepTbttNumShift = 8,
		WakeupLeadTimeShift = 0,
};

enum {
	/* MAC TX configuration registers */
#define EdcaAcCfg(aci) (0x1300 + (aci) * 4)
	EdcaTidAcMap = 0x1310,
#define TxPwrCfg(ridx) (0x1314 + (ridx) * 4)
	TxPinCfg = 0x1328,
		Rt3593LnaPeG2Pol = (1 << 31),
		Rt3593LnaPeA2Pol = (1 << 30),
		Rt3593LnaPeG2En = (1 << 29),
		Rt3593LnaPeA2En = (1 << 28),
		Rt3593LnaPe2En = (Rt3593LnaPeA2En | Rt3593LnaPeG2En),
		Rt3593PaPeG2Pol = (1 << 27),
		Rt3593PaPeA2Pol = (1 << 26),
		Rt3593PaPeG2En = (1 << 25),
		Rt3593PaPeA2En = (1 << 24),
		TrswPol = (1 << 19),
		TrswEn = (1 << 18),
		RftrPol = (1 << 17),
		RftrEn = (1 << 16),
		LnaPeG1Pol = (1 << 15),
		LnaPeA1Pol = (1 << 14),
		LnaPeG0Pol = (1 << 13),
		LnaPeA0Pol = (1 << 12),
		LnaPeG1En = (1 << 11),
		LnaPeA1En = (1 << 10),
		LnaPe1En = (LnaPeA1En | LnaPeG1En),
		LnaPeG0En = (1 << 9),
		LnaPeA0En = (1 << 8),
		LnaPe0En = (LnaPeA0En | LnaPeG0En),
		PaPeG1Pol = (1 << 7),
		PaPeA1Pol = (1 << 6),
		PaPeG0Pol = (1 << 5),
		PaPeA0Pol = (1 << 4),
		PaPeG1En = (1 << 3),
		PaPeA1En = (1 << 2),
		PaPeG0En = (1 << 1),
		PaPeA0En = (1 << 0),
	TxBandCfg = 0x132c,
		Tx5gBandSelN = (1 << 2),
		Tx5gBandSelP = (1 << 1),
		TxBandSel = (1 << 0),
	TxSwCfg0 = 0x1330,
		DlyRftrEnShift = 24,
		DlyTrswEnShift = 16,
		DlyPapeEnShift = 8,
		DlyTxpeEnShift = 0,
	TxSwCfg1 = 0x1334,
		DlyRftrDisShift = 16,
		DlyTrswDisShift = 8,
		DlyPapeDisShift = 0,
	TxSwCfg2 = 0x1338,
		DlyLnaEnShift = 24,
		DlyLnaDisShift = 16,
		DlyDacEnShift = 8,
		DlyDacDisShift = 0,
	TxopThresCfg = 0x133c,
		TxopRemThresShift = 24,
		CfEndThresShift = 16,
		RdgInThres = 8,
		RdgOutThres = 0,
	TxopCtrlCfg = 0x1340,
		ExtCwMinShift = 16,
		ExtCcaDlyShift = 8,
		ExtCcaEn = (1 << 7),
		LsigTxopEn = (1 << 6),
		TxopTrunEnMimops = (1 << 4),
		TxopTrunEnTxop = (1 << 3),
		TxopTrunEnRate = (1 << 2),
		TxopTrunEnAc = (1 << 1),
		TxopTrunEnTimeout = (1 << 0),
	TxRtsCfg = 0x1344,
		RtsFbkEn = (1 << 24),
		RtsThresShift = 8,
		RtsRtyLimitShift = 0,
	TxTimeoutCfg = 0x1348,
		TxopTimeoutShift = 16,
		RxAckTimeoutShift = 8,
		MpduLifeTimeShift = 4,
	TxRtyCfg = 0x134c,
		TxAutofbEn = (1 << 30),
		AggRtyModeTimer = (1 << 29),
		NagRtyModeTimer = (1 << 28),
		LongRtyThresShift = 16,
		LongRtyLimitShift = 8,
		ShortRtyLimitShift = 0,
	TxLinkCfg = 0x1350,
		RemoteMfsShift = 24,
		RemoteMfbShift = 16,
		TxCfackEn = (1 << 12),
		TxRdgEn = (1 << 11),
		TxMrqEn = (1 << 10),
		RemoteUmfsEn = (1 << 9),
		TxMfbEn = (1 << 8),
		RemoteMfbLtShift = 0,
	HtFbkCfg0 = 0x1354,
	HtFbkCfg1 = 0x1358,
	LgFbkCfg0 = 0x135c,
	LgFbkCfg1 = 0x1360,
	CckProtCfg = 0x1364,
		/* possible flags for registers *ProtCfg */
		RtsthEn = (1 << 26),
		TxopAllowGf40 = (1 << 25),
		TxopAllowGf20 = (1 << 24),
		TxopAllowMm40 = (1 << 23),
		TxopAllowMm20 = (1 << 22),
		TxopAllowOfdm = (1 << 21),
		TxopAllowCck = (1 << 20),
		TxopAllowAll = (0x3f << 20),
		ProtNavShort = (1 << 18),
		ProtNavLong = (2 << 18),
		ProtCtrlRtsCts = (1 << 16),
		ProtCtrlCts = (2 << 16),
	OfdmProtCfg = 0x1368,
	Mm20ProtCfg = 0x136c,
	Mm40ProtCfg = 0x1370,
	Gf20ProtCfg = 0x1374,
	Gf40ProtCfg = 0x1378,
	ExpCtsTime = 0x137c,
		/* possible flags for registers EXP_{CTS,ACK}_TIME */
		ExpOfdmTimeShift = 16,
		ExpCckTimeShift = 0,
	ExpAckTime = 0x1380,
};

enum {
	/* MAC RX configuration registers */
	RxFiltrCfg = 0x1400,
		DropCtrlRsv = (1 << 16),
		DropBar = (1 << 15),
		DropBa = (1 << 14),
		DropPspoll = (1 << 13),
		DropRts = (1 << 12),
		DropCts = (1 << 11),
		DropAck = (1 << 10),
		DropCfend = (1 << 9),
		DropCfack = (1 << 8),
		DropDupl = (1 << 7),
		DropBc = (1 << 6),
		DropMc = (1 << 5),
		DropVerErr = (1 << 4),
		DropNotMybss = (1 << 3),
		DropUcNome = (1 << 2),
		DropPhyErr = (1 << 1),
		DropCrcErr = (1 << 0),
	AutoRspCfg = 0x1404,
		CtrlPwrBit = (1 << 7),
		BacAckPolicy = (1 << 6),
		CckShortEn = (1 << 4),
		Cts40mRefEn = (1 << 3),
		Cts40mModeEn = (1 << 2),
		BacAckpolicyEn = (1 << 1),
		AutoRspEn = (1 << 0),
	LegacyBasicRate = 0x1408,
	HtBasicRate = 0x140c,
	HtCtrlCfg = 0x1410,
	SifsCostCfg = 0x1414,
		OfdmSifsCostShift = 8,
		CckSifsCostShift = 0,
	RxParserCfg = 0x1418,
};

enum {
	/* MAC Security configuration registers */
	TxSecCnt0 = 0x1500,
	RxSecCnt0 = 0x1504,
	CcmpFcMute = 0x1508,
};

enum {
	/* MAC HCCA/PSMP configuration registers */
	TxopHldrAddr0 = 0x1600,
	TxopHldrAddr1 = 0x1604,
	TxopHldrEt = 0x1608,
		TxopEtm1En = (1 << 25),
		TxopEtm0En = (1 << 24),
		TxopEtmThresShift = 16,
		TxopEtoEn = (1 << 8),
		TxopEtoThresShift = 1,
		PerRxRstEn = (1 << 0),
	QosCfpollRaDw0 = 0x160c,
	QosCfpollA1Dw1 = 0x1610,
	QosCfpollQc = 0x1614,
};

enum {
	/* MAC Statistics Counters */
	RxStaCnt0 = 0x1700,
	RxStaCnt1 = 0x1704,
	RxStaCnt2 = 0x1708,
	TxStaCnt0 = 0x170c,
	TxStaCnt1 = 0x1710,
	TxStaCnt2 = 0x1714,
	TxStatFifo = 0x1718,
		TxqMcsShift = 16,
		TxqWcidShift = 8,
		TxqAckreq = (1 << 7),
		TxqAgg = (1 << 6),
		TxqOk = (1 << 5),
		TxqPidShift = 1,
		TxqVld = (1 << 0),
};

/* RX WCID search table */
#define WcidEntry(wcid) (0x1800 + (wcid) * 8)

enum {
	FwBase = 0x2000,
	Rt2870FwBase = 0x3000,
};

/* Pair-wise key table */
#define Pkey(wcid) (0x4000 + (wcid) * 32)

/* IV/EIV table */
#define Iveiv(wcid) (0x6000 + (wcid) * 8)

/* WCID attribute table */
#define WcidAttr(wcid) (0x6800 + (wcid) * 4)

/* possible flags for register WCID_ATTR */
enum {
	ModeNosec = 0,
	ModeWep40 = 1,
	ModeWep104 = 2,
	ModeTkip = 3,
	ModeAesCcmp = 4,
	ModeCkip40 = 5,
	ModeCkip104 = 6,
	ModeCkip128 = 7,
	RxPkeyEn = (1 << 0),
};

/* Shared Key Table */
#define Skey(vap, kidx) (0x6c00 + (vap) * 128 + (kidx) * 32)

/* Shared Key Mode */
enum {
	SkeyMode07 = 0x7000,
	SkeyMode815 = 0x7004,
	SkeyMode1623 = 0x7008,
	SkeyMode2431 = 0x700c,
};

enum {
	/* Shared Memory between MCU and host */
	H2mMailbox = 0x7010,
		H2mBusy = (1 << 24),
		TokenNoIntr = 0xff,
	H2mMailboxCid = 0x7014,
	H2mMailboxStatus = 0x701c,
	H2mBbpagent = 0x7028,
#define BcnBase(vap) (0x7800 + (vap) * 512)
};

/* 
 *	RT2860 TX descriptor
 *	--------------------
 *	u32int	sdp0 		Segment Data Pointer 0
 *	u16int	sdl1 		Segment Data Length 1
 *	u16int	sdl0		Segment Data Length 0
 *	u32int	sdp1		Segment Data Pointer 1
 *	u8int 	reserved[3]
 *	u8int 	flags
 */

enum {
	/* sdl1 flags */
	TxBurst = (1 << 15),
	TxLs1 = (1 << 14) /* SDP1 is the last segment */,
	/* sdl0 flags */
	TxDdone = (1 << 15),
	TxLs0 = (1 << 14) /* SDP0 is the last segment */,
	/* flags */
	TxQselShift = 1,
	TxQselMgmt = (0 << 1),
	TxQselHcca = (1 << 1),
	TxQselEdca = (2 << 1),
	TxWiv = (1 << 0),
};

/* 
 *	TX Wireless Information
 *	-----------------------
 *	u8int	flags
 *	u8int	txop
 *	u16int	phy
 *	u8int	xflags
 *	u8int	wcid 	Wireless Client ID
 *	u16int	len
 *	u32int	iv
 *	u32int	eiv
 */

enum {
	/* flags */
	TxMpduDsityShift = 5,
	TxAmpdu = (1 << 4),
	TxTs = (1 << 3),
	TxCfack = (1 << 2),
	TxMmps = (1 << 1),
	TxFrag = (1 << 0),
	/* txop */
	TxTxopHt = 0,
	TxTxopPifs = 1,
	TxTxopSifs = 2,
	TxTxopBackoff = 3,
	/* phy */
	PhyMode = 0xc000,
	PhyCck = (0 << 14),
	PhyOfdm = (1 << 14),
	PhyHt = (2 << 14),
	PhyHtGf = (3 << 14),
	PhySgi = (1 << 8),
	PhyBw40 = (1 << 7),
	PhyMcs = 0x7f,
	PhyShpre = (1 << 3),
	/* xflags */
	TxBawinsizeShift = 2,
	TxNseq = (1 << 1),
	TxAck = (1 << 0),
	/* len */
	TxPidShift = 12,
};

/* 
 *	RT2860 RX descriptor
 *	--------------------
 *	u32int	sdp0
 *	u16int	sdl1 	unused
 *	u16int  sdl0
 *	u32int	sdp1	unused
 *	u32int	flags
 */

enum {
	/* sdl flags */
	RxDdone = (1 << 15),
	RxLs0 = (1 << 14),
	/* flags */
	RxDec = (1 << 16),
	RxAmpdu = (1 << 15),
	RxL2pad = (1 << 14),
	RxRssi = (1 << 13),
	RxHtc = (1 << 12),
	RxAmsdu = (1 << 11),
	RxMicerr = (1 << 10),
	RxIcverr = (1 << 9),
	RxCrcerr = (1 << 8),
	RxMybss = (1 << 7),
	RxBc = (1 << 6),
	RxMc = (1 << 5),
	RxUc2me = (1 << 4),
	RxFrag = (1 << 3),
	RxNull = (1 << 2),
	RxData = (1 << 1),
	RxBa = (1 << 0),
};

/* 
 *	RX Wireless Information
 *	-----------------------
 *	u8int	wcid
 *	u8int	keyidx
 *	u16int	len
 *	u16int	seq
 *	u16int	phy
 *	u8int	rssi[3]
 *	u8int	reserved1
 *	u8int	snr[2]
 *	u16int	reserved2
 */

enum {
	/* keyidx flags */
	RxUdfShift = 5,
	RxBssIdxShift = 2,
	/* len flags */
	RxTidShift = 12,
};

enum {
	WIFIHDRSIZE = 2+2+3*6+2,
	Rdscsize = 16,
	Tdscsize = 16,
	Rbufsize = 4096,
	Tbufsize = 4096,
	Rxwisize = 16,
	Txwisize = 16,
	/* first DMA segment contains TXWI + 802.11 header + 32-bit padding */
	TxwiDmaSz = Txwisize + WIFIHDRSIZE + 2
};

/* RF registers */
enum {
	Rf1 = 0,
	Rf2 = 2,
	Rf3 = 1,
	Rf4 = 3,
};

enum {
	Rf2820 = 1 /* 2T3R */,
	Rf2850 = 2 /* dual-band 2T3R */,
	Rf2720 = 3 /* 1T2R */,
	Rf2750 = 4 /* dual-band 1T2R */,
	Rf3020 = 5 /* 1T1R */,
	Rf2020 = 6 /* b/g */,
	Rf3021 = 7 /* 1T2R */,
	Rf3022 = 8 /* 2T2R */,
 	Rf3052 = 9 /* dual-band 2T2R */,
	Rf3320 = 11 /* 1T1R */,
	Rf3053 = 13 /* dual-band 3T3R */,
};

enum {	
	Rt3070RfBlock = (1 << 0),
	Rt3070Rx0Pd = (1 << 2),
	Rt3070Tx0Pd = (1 << 3),
	Rt3070Rx1Pd = (1 << 4),
	Rt3070Tx1Pd = (1 << 5),
	Rt3070Rx2Pd = (1 << 6),
	Rt3070Tx2Pd = (1 << 7),
	Rt3070Tune = (1 << 0),
	Rt3070TxLo2 = (1 << 3),
	Rt3070TxLo1 = (1 << 3),
	Rt3070RxLo1 = (1 << 3),
	Rt3070RxLo2 = (1 << 3),
	Rt3070RxCtb = (1 << 7),
	Rt3070BbLoopback = (1 << 0),

	Rt3593Vco = (1 << 0),
	Rt3593Rescal = (1 << 7),
	Rt3593Vcocal = (1 << 7),
	Rt3593VcoIc = (1 << 6),
	Rt3593LdoPllVcMask = 0x0e,
	Rt3593LdoRfVcMask = 0xe0,
	Rt3593CpIcMask = 0xe0,
	Rt3593CpIcShift = 5,
	Rt3593RxCtb = (1 << 5)
};

static const char* rfnames[] = {
	[Rf2820] "RT2820",
	[Rf2850] "RT2850",
	[Rf2720] "RT2720",
	[Rf2750] "RT2750",
	[Rf3020] "RT3020",
	[Rf2020] "RT2020",
	[Rf3021] "RT3021",
	[Rf3022] "RT3022",
	[Rf3052] "RT3052",
	[Rf3320] "RT3320",
	[Rf3053] "RT3053",
};

enum {
	/* USB commands, RT2870 only */
	Rt2870Reset = 1,
	Rt2870Write2 = 2,
	Rt2870WriteRegion1 = 6,
	Rt2870ReadRegion1 = 7,
	Rt2870EepromRead = 9,
};

enum {
	EepromDelay = 1 /* minimum hold time (microsecond) */,

	EepromVersion = 0x01,
	EepromMac01 = 0x02,
	EepromMac23 = 0x03,
	EepromMac45 = 0x04,
	EepromPciePslevel = 0x11,
	EepromRev = 0x12,
	EepromAntenna = 0x1a,
	EepromConfig = 0x1b,
	EepromCountry = 0x1c,
	EepromFreqLeds = 0x1d,
	EepromLed1 = 0x1e,
	EepromLed2 = 0x1f,
	EepromLed3 = 0x20,
	EepromLna = 0x22,
	EepromRssi12ghz = 0x23,
	EepromRssi22ghz = 0x24,
	EepromRssi15ghz = 0x25,
	EepromRssi25ghz = 0x26,
	EepromDeltapwr = 0x28,
	EepromPwr2ghzBase1 = 0x29,
	EepromPwr2ghzBase2 = 0x30,
	EepromTssi12ghz = 0x37,
	EepromTssi22ghz = 0x38,
	EepromTssi32ghz = 0x39,
	EepromTssi42ghz = 0x3a,
	EepromTssi52ghz = 0x3b,
	EepromPwr5ghzBase1 = 0x3c,
	EepromPwr5ghzBase2 = 0x53,
	EepromTssi15ghz = 0x6a,
	EepromTssi25ghz = 0x6b,
	EepromTssi35ghz = 0x6c,
	EepromTssi45ghz = 0x6d,
	EepromTssi55ghz = 0x6e,
	EepromRpwr = 0x6f,
	EepromBbpBase = 0x78,
	Rt3071EepromRfBase = 0x82,
};

enum {
	RidxCck1 = 0,
	RidxCck11 = 3,
	RidxOfdm6 = 4,
	RidxMax = 11,
};

/* ring and pool count */
enum {
	Nrx = 128,
	Ntx = 64,
	Ntxpool = Ntx * 2
};

typedef struct FWImage FWImage;
typedef struct TXQ TXQ;
typedef struct RXQ RXQ;
typedef struct Pool Pool;

typedef struct Ctlr Ctlr;

struct FWImage {
	uint  size;
	uchar *data;
};

struct TXQ
{
	uint	n; /* next */
	uint	i; /* current */
	Block	**b;
	u32int	*d; /* descriptors */

	Rendez;
	QLock;
};

struct RXQ
{
	uint	i;
	Block	**b;
	u32int	*p;
};

struct Pool
{
	uint i; /* current */
	uchar *p; /* txwi */
};

struct Ctlr {
	Lock;
	QLock;

	Ctlr *link;
	Pcidev *pdev;
	Wifi *wifi;

	u16int mac_ver;
	u16int mac_rev;
	u8int rf_rev;
	u8int freq;
	u8int ntxchains;
	u8int nrxchains;
	u8int pslevel;
	s8int txpow1[54];
	s8int txpow2[54];
	s8int rssi_2ghz[3];
	s8int rssi_5ghz[3];
	u8int lna[4];
	u8int rf24_20mhz;
	u8int rf24_40mhz;
	u8int patch_dac;
	u8int rfswitch;
	u8int ext_2ghz_lna;
	u8int ext_5ghz_lna;
	u8int calib_2ghz;
	u8int calib_5ghz;
	u8int txmixgain_2ghz;
	u8int txmixgain_5ghz;
	u8int tssi_2ghz[9];
	u8int tssi_5ghz[9];
	u8int step_2ghz;
	u8int step_5ghz;
	uint mgtqid;

	struct {
		u8int	reg;
		u8int	val;
	} bbp[8], rf[10];
	u8int leds;
	u16int led[3];
	u32int txpow20mhz[5];
	u32int txpow40mhz_2ghz[5];
	u32int txpow40mhz_5ghz[5];

	int flags;

	int port;
	int power;
	int active;
	int broken;
	int attached;

	u32int *nic;

	/* assigned node ids in hardware node table or -1 if unassigned */
	int bcastnodeid;
	int bssnodeid;
	u8int wcid;
	/* current receiver settings */
	uchar bssid[Eaddrlen];
	int channel;
	int prom;
	int aid;

	RXQ rx;
	TXQ tx[6];
	Pool pool;

	FWImage *fw;
};

/* controller flags */
enum {
	AdvancedPs = 1 << 0,
	ConnPciE   = 1 << 1,
};

static const struct rt2860_rate {
	u8int		rate;
	u8int		mcs;
	/*enum		ieee80211_phytype phy;*/
	u8int		ctl_ridx;
	u16int		sp_ack_dur;
	u16int		lp_ack_dur;
} rt2860_rates[] = {
	{   2, 0,/* IEEE80211_T_DS,*/   0, 314, 314 },
	{   4, 1,/* IEEE80211_T_DS,*/   1, 258, 162 },
	{  11, 2,/* IEEE80211_T_DS,*/   2, 223, 127 },
	{  22, 3,/* IEEE80211_T_DS,*/   3, 213, 117 },
	{  12, 0,/* IEEE80211_T_OFDM,*/ 4,  60,  60 },
	{  18, 1,/* IEEE80211_T_OFDM,*/ 4,  52,  52 },
	{  24, 2,/* IEEE80211_T_OFDM,*/ 6,  48,  48 },
	{  36, 3,/* IEEE80211_T_OFDM,*/ 6,  44,  44 },
	{  48, 4,/* IEEE80211_T_OFDM,*/ 8,  44,  44 },
	{  72, 5,/* IEEE80211_T_OFDM,*/ 8,  40,  40 },
	{  96, 6,/* IEEE80211_T_OFDM,*/ 8,  40,  40 },
	{ 108, 7,/* IEEE80211_T_OFDM,*/ 8,  40,  40 }
};

/*
 * Default values for MAC registers; values taken from the reference driver.
 */
static const struct {
	u32int	reg;
	u32int	val;
} rt2860_def_mac[] = {
	{ BcnOffset0,	 	 0xf8f0e8e0 }, 
	{ LegacyBasicRate,	 0x0000013f }, 
	{ HtBasicRate,		 0x00008003 }, 
	{ MacSysCtrl,	 	 0x00000000 }, 
	{ BkoffSlotCfg,		 0x00000209 }, 
	{ TxSwCfg0,			 0x00000000 }, 
	{ TxSwCfg1,			 0x00080606 }, 
	{ TxLinkCfg,		 0x00001020 }, 
	{ TxTimeoutCfg,		 0x000a2090 }, 
	{ LedCfg,			 0x7f031e46 }, 
	{ WmmAifsnCfg,		 0x00002273 }, 
	{ WmmCwminCfg,		 0x00002344 }, 
	{ WmmCwmaxCfg,		 0x000034aa }, 
	{ MaxPcnt,			 0x1f3fbf9f }, 
	{ TxRtyCfg,			 0x47d01f0f }, 
	{ AutoRspCfg,		 0x00000013 }, 
	{ CckProtCfg,		 0x05740003 }, 
	{ OfdmProtCfg,		 0x05740003 }, 
	{ Gf20ProtCfg,		 0x01744004 }, 
	{ Gf40ProtCfg,		 0x03f44084 }, 
	{ Mm20ProtCfg,		 0x01744004 }, 
	{ Mm40ProtCfg,		 0x03f54084 }, 
	{ TxopCtrlCfg,		 0x0000583f }, 
	{ TxopHldrEt,		 0x00000002 }, 
	{ TxRtsCfg,			 0x00092b20 }, 
	{ ExpAckTime,		 0x002400ca }, 
	{ XifsTimeCfg,		 0x33a41010 }, 
	{ PwrPinCfg,		 0x00000003 },
};

/*
 * Default values for BBP registers; values taken from the reference driver.
 */
static const struct {
	u8int	reg;
	u8int	val;
} rt2860_def_bbp[] = {
	{  65, 0x2c },	
	{  66, 0x38 },	
	{  69, 0x12 },	
	{  70, 0x0a },	
	{  73, 0x10 },	
	{  81, 0x37 },	
	{  82, 0x62 },	
	{  83, 0x6a },	
	{  84, 0x99 },	
	{  86, 0x00 },	
	{  91, 0x04 },	
	{  92, 0x00 },	
	{ 103, 0x00 },	
	{ 105, 0x05 },	
	{ 106, 0x35 },
};

/*
 * Default settings for RF registers; values derived from the reference driver.
 */
static const struct rfprog {
	u8int		chan;
	u32int		r1, r2, r3, r4;
} rt2860_rf2850[] = {
	{   1, 0x100bb3, 0x1301e1, 0x05a014, 0x001402 },	
	{   2, 0x100bb3, 0x1301e1, 0x05a014, 0x001407 },	
	{   3, 0x100bb3, 0x1301e2, 0x05a014, 0x001402 },	
	{   4, 0x100bb3, 0x1301e2, 0x05a014, 0x001407 },	
	{   5, 0x100bb3, 0x1301e3, 0x05a014, 0x001402 },	
	{   6, 0x100bb3, 0x1301e3, 0x05a014, 0x001407 },	
	{   7, 0x100bb3, 0x1301e4, 0x05a014, 0x001402 },	
	{   8, 0x100bb3, 0x1301e4, 0x05a014, 0x001407 },	
	{   9, 0x100bb3, 0x1301e5, 0x05a014, 0x001402 },	
	{  10, 0x100bb3, 0x1301e5, 0x05a014, 0x001407 },	
	{  11, 0x100bb3, 0x1301e6, 0x05a014, 0x001402 },	
	{  12, 0x100bb3, 0x1301e6, 0x05a014, 0x001407 },	
	{  13, 0x100bb3, 0x1301e7, 0x05a014, 0x001402 },	
	{  14, 0x100bb3, 0x1301e8, 0x05a014, 0x001404 },	
	{  36, 0x100bb3, 0x130266, 0x056014, 0x001408 },	
	{  38, 0x100bb3, 0x130267, 0x056014, 0x001404 },	
	{  40, 0x100bb2, 0x1301a0, 0x056014, 0x001400 },	
	{  44, 0x100bb2, 0x1301a0, 0x056014, 0x001408 },	
	{  46, 0x100bb2, 0x1301a1, 0x056014, 0x001402 },	
	{  48, 0x100bb2, 0x1301a1, 0x056014, 0x001406 },	
	{  52, 0x100bb2, 0x1301a2, 0x056014, 0x001404 },	
	{  54, 0x100bb2, 0x1301a2, 0x056014, 0x001408 },	
	{  56, 0x100bb2, 0x1301a3, 0x056014, 0x001402 },	
	{  60, 0x100bb2, 0x1301a4, 0x056014, 0x001400 },	
	{  62, 0x100bb2, 0x1301a4, 0x056014, 0x001404 },	
	{  64, 0x100bb2, 0x1301a4, 0x056014, 0x001408 },	
	{ 100, 0x100bb2, 0x1301ac, 0x05e014, 0x001400 },	
	{ 102, 0x100bb2, 0x1701ac, 0x15e014, 0x001404 },	
	{ 104, 0x100bb2, 0x1701ac, 0x15e014, 0x001408 },	
	{ 108, 0x100bb3, 0x17028c, 0x15e014, 0x001404 },	
	{ 110, 0x100bb3, 0x13028d, 0x05e014, 0x001400 },	
	{ 112, 0x100bb3, 0x13028d, 0x05e014, 0x001406 },	
	{ 116, 0x100bb3, 0x13028e, 0x05e014, 0x001408 },	
	{ 118, 0x100bb3, 0x13028f, 0x05e014, 0x001404 },	
	{ 120, 0x100bb1, 0x1300e0, 0x05e014, 0x001400 },	
	{ 124, 0x100bb1, 0x1300e0, 0x05e014, 0x001404 },	
	{ 126, 0x100bb1, 0x1300e0, 0x05e014, 0x001406 },	
	{ 128, 0x100bb1, 0x1300e0, 0x05e014, 0x001408 },	
	{ 132, 0x100bb1, 0x1300e1, 0x05e014, 0x001402 },	
	{ 134, 0x100bb1, 0x1300e1, 0x05e014, 0x001404 },	
	{ 136, 0x100bb1, 0x1300e1, 0x05e014, 0x001406 },	
	{ 140, 0x100bb1, 0x1300e2, 0x05e014, 0x001400 },	
	{ 149, 0x100bb1, 0x1300e2, 0x05e014, 0x001409 },	
	{ 151, 0x100bb1, 0x1300e3, 0x05e014, 0x001401 },	
	{ 153, 0x100bb1, 0x1300e3, 0x05e014, 0x001403 },	
	{ 157, 0x100bb1, 0x1300e3, 0x05e014, 0x001407 },	
	{ 159, 0x100bb1, 0x1300e3, 0x05e014, 0x001409 },	
	{ 161, 0x100bb1, 0x1300e4, 0x05e014, 0x001401 },	
	{ 165, 0x100bb1, 0x1300e4, 0x05e014, 0x001405 },	
	{ 167, 0x100bb1, 0x1300f4, 0x05e014, 0x001407 },	
	{ 169, 0x100bb1, 0x1300f4, 0x05e014, 0x001409 },	
	{ 171, 0x100bb1, 0x1300f5, 0x05e014, 0x001401 },	
	{ 173, 0x100bb1, 0x1300f5, 0x05e014, 0x001403 },
};

struct {
	u8int n;
	u8int r;
	u8int k;
} rt3090_freqs[] = {
	{ 0xf1, 2,  2 },
	{ 0xf1, 2,  7 },
	{ 0xf2, 2,  2 },
	{ 0xf2, 2,  7 },
	{ 0xf3, 2,  2 },
	{ 0xf3, 2,  7 },
	{ 0xf4, 2,  2 },
	{ 0xf4, 2,  7 },
	{ 0xf5, 2,  2 },
	{ 0xf5, 2,  7 },
	{ 0xf6, 2,  2 },
	{ 0xf6, 2,  7 },
	{ 0xf7, 2,  2 },
	{ 0xf8, 2,  4 },
	{ 0x56, 0,  4 },
	{ 0x56, 0,  6 },
	{ 0x56, 0,  8 },
	{ 0x57, 0,  0 },
	{ 0x57, 0,  2 },
	{ 0x57, 0,  4 },
	{ 0x57, 0,  8 },
	{ 0x57, 0, 10 },
	{ 0x58, 0,  0 },
	{ 0x58, 0,  4 },
	{ 0x58, 0,  6 },
	{ 0x58, 0,  8 },
	{ 0x5b, 0,  8 },
	{ 0x5b, 0, 10 },
	{ 0x5c, 0,  0 },
	{ 0x5c, 0,  4 },
	{ 0x5c, 0,  6 },
	{ 0x5c, 0,  8 },
	{ 0x5d, 0,  0 },
	{ 0x5d, 0,  2 },
	{ 0x5d, 0,  4 },
	{ 0x5d, 0,  8 },
	{ 0x5d, 0, 10 },
	{ 0x5e, 0,  0 },
	{ 0x5e, 0,  4 },
	{ 0x5e, 0,  6 },
	{ 0x5e, 0,  8 },
	{ 0x5f, 0,  0 },
	{ 0x5f, 0,  9 },
	{ 0x5f, 0, 11 },
	{ 0x60, 0,  1 },
	{ 0x60, 0,  5 },
	{ 0x60, 0,  7 },
	{ 0x60, 0,  9 },
	{ 0x61, 0,  1 },
	{ 0x61, 0,  3 },
	{ 0x61, 0,  5 },
	{ 0x61, 0,  7 },
	{ 0x61, 0,  9 }
};

static const struct {
	u8int reg;
	u8int val;
} rt3090_def_rf[] = {
	{  4, 0x40 },
	{  5, 0x03 },
	{  6, 0x02 },
	{  7, 0x70 },
	{  9, 0x0f },
	{ 10, 0x41 },
	{ 11, 0x21 },
	{ 12, 0x7b },
	{ 14, 0x90 },
	{ 15, 0x58 },
	{ 16, 0xb3 },
	{ 17, 0x92 },
	{ 18, 0x2c },
	{ 19, 0x02 },
	{ 20, 0xba },
	{ 21, 0xdb },
	{ 24, 0x16 },
	{ 25, 0x01 },
	{ 29, 0x1f }
};

/* vendors */
enum {
	Ralink = 0x1814,
	Awt = 0x1a3b,
};
/* products */
enum {
	RalinkRT2890 = 0x0681,
	RalinkRT2790 = 0x0781,
	RalinkRT3090 = 0x3090,
	AwtRT2890 = 0x1059,
};

#define csr32r(c, r)	(*((c)->nic+((r)/4)))
#define csr32w(c, r, v)	(*((c)->nic+((r)/4)) = (v))

static int rbplant(Ctlr*, int);
static void setchan(Ctlr*, uint);
static void rt3090setchan(Ctlr*, uint);
static void selchangroup(Ctlr*, int);
static void setleds(Ctlr*, u16int);

static uint
get16(uchar *p){
	return *((u16int*)p);
}
static uint
get32(uchar *p){
	return *((u32int*)p);
}
static void
put32(uchar *p, uint v){
	*((u32int*)p) = v;
}
static void
put16(uchar *p, uint v){
	*((u16int*)p) = v;
};
static void
memwrite(Ctlr *ctlr, u32int off, uchar *data, uint size){
	memmove((uchar*)ctlr->nic + off, data, size);
}
static void
setregion(Ctlr *ctlr, u32int off, uint val, uint size){
	memset((uchar*)ctlr->nic + off, val, size);
}

static long
rt2860ctl(Ether *edev, void *buf, long n)
{
	Ctlr *ctlr;

	ctlr = edev->ctlr;
	if(ctlr->wifi)
		return wifictl(ctlr->wifi, buf, n);
	return 0;
}

static long
rt2860ifstat(Ether *edev, void *buf, long n, ulong off)
{
	Ctlr *ctlr;

	ctlr = edev->ctlr;
	if(ctlr->wifi)
		return wifistat(ctlr->wifi, buf, n, off);
	return 0;
}

static void
setoptions(Ether *edev)
{
	Ctlr *ctlr;
	int i;

	ctlr = edev->ctlr;
	for(i = 0; i < edev->nopt; i++)
		wificfg(ctlr->wifi, edev->opt[i]);
}

static void
rxon(Ether *edev, Wnode *bss)
{
	u32int tmp;
	Ctlr *ctlr;
	int cap;

	ctlr = edev->ctlr;

	if(bss != nil){
		cap = bss->cap;
		ctlr->channel = bss->channel;
		memmove(ctlr->bssid, bss->bssid, Eaddrlen);
		ctlr->aid = bss->aid;
		if(ctlr->aid != 0){
			if(ctlr->wifi->debug)
				print("new assoc!");
			ctlr->bssnodeid = -1;
		}else
			ctlr->bcastnodeid = -1;
	}else{
		cap = 0;
		memmove(ctlr->bssid, edev->bcast, Eaddrlen);
		ctlr->aid = 0;
		ctlr->bcastnodeid = -1;
		ctlr->bssnodeid = -1;
	}
	if(ctlr->aid != 0)
		setleds(ctlr, LedRadio | LedLink2ghz);
	else
		setleds(ctlr, LedRadio);

	if(ctlr->wifi->debug)
		print("#l%d: rxon: bssid %E, aid %x, channel %d wcid %d\n",
			edev->ctlrno, ctlr->bssid, ctlr->aid, ctlr->channel, ctlr->wcid);

	/* Set channel */
	if(ctlr->mac_ver >= 0x3071)
		rt3090setchan(ctlr, ctlr->channel);
	else
		setchan(ctlr, ctlr->channel);
	selchangroup(ctlr, 0);
	microdelay(1000);

	/* enable mrr(?) */
#define CCK(mcs)	(mcs)
#define OFDM(mcs)	(1 << 3 | (mcs))
	csr32w(ctlr, LgFbkCfg0,
	    OFDM(6) << 28 |	/* 54->48 */
	    OFDM(5) << 24 |	/* 48->36 */
	    OFDM(4) << 20 |	/* 36->24 */
	    OFDM(3) << 16 |	/* 24->18 */
	    OFDM(2) << 12 |	/* 18->12 */
	    OFDM(1) <<  8 |	/* 12-> 9 */
	    OFDM(0) <<  4 |	/*  9-> 6 */
	    OFDM(0));		/*  6-> 6 */

	csr32w(ctlr, LgFbkCfg1,
	    CCK(2) << 12 |	/* 11->5.5 */
	    CCK(1) <<  8 |	/* 5.5-> 2 */
	    CCK(0) <<  4 |	/*   2-> 1 */
	    CCK(0));		/*   1-> 1 */
#undef OFDM
#undef CCK
	/* update slot */
	tmp = csr32r(ctlr, BkoffSlotCfg);
	tmp &= ~0xff;
	tmp |= (cap & (1<<10)) ? 9 : 20;
	csr32w(ctlr, BkoffSlotCfg, tmp);
	
	/* set TX preamble */
	tmp = csr32r(ctlr, AutoRspCfg);
	tmp &= ~CckShortEn;
	if(cap & (1<<5)) tmp |= CckShortEn;	
	csr32w(ctlr, AutoRspCfg, tmp);

	/* set basic rates */
	csr32w(ctlr, LegacyBasicRate, 0x003); /* 11B */

	/* Set BSSID */
	csr32w(ctlr, MacBssidDw0,
	    ctlr->bssid[0] | ctlr->bssid[1] << 8 | ctlr->bssid[2] << 16 | ctlr->bssid[3] << 24);
	csr32w(ctlr, MacBssidDw1,
	    ctlr->bssid[4] | ctlr->bssid[5] << 8);

	if(ctlr->bcastnodeid == -1){
		ctlr->bcastnodeid = 0xff;
		memwrite(ctlr, WcidEntry(ctlr->bcastnodeid), edev->bcast, Eaddrlen);
	}
	if(ctlr->bssnodeid == -1 && bss != nil && ctlr->aid != 0){
		ctlr->bssnodeid = 0;
		memwrite(ctlr, WcidEntry(ctlr->bssnodeid), ctlr->bssid, Eaddrlen);
	}
}

static void
rt2860promiscuous(void *arg, int on)
{
	Ether *edev;
	Ctlr *ctlr;

	edev = arg;
	ctlr = edev->ctlr;
	if(ctlr->attached == 0)
		return;
	qlock(ctlr);
	ctlr->prom = on;
	rxon(edev, ctlr->wifi->bss);
	qunlock(ctlr);
}

static void
rt2860multicast(void *, uchar*, int)
{
}

static FWImage*
readfirmware(void){
	static char name[] = "ral-rt2860";
	uchar dirbuf[sizeof(Dir)+100], *data;
	char buf[128];
	FWImage *fw;
	int n, r;
	Chan *c;
	Dir d;

	if(!iseve())
		error(Eperm);
	if(!waserror()){
		snprint(buf, sizeof buf, "/boot/%s", name);
		c = namec(buf, Aopen, OREAD, 0);
		poperror();
	}else{
		snprint(buf, sizeof buf, "/lib/firmware/%s", name);
		c = namec(buf, Aopen, OREAD, 0);
	}
	if(waserror()){
		cclose(c);
		nexterror();
	}
	n = devtab[c->type]->stat(c, dirbuf, sizeof dirbuf);
	if(n <= 0)
		error("can't stat firmware");
	convM2D(dirbuf, n, &d, nil);
	fw = malloc(sizeof(*fw));
	fw->size = d.length;
	data = fw->data = smalloc(d.length);
	if(waserror()){
		free(fw);
		nexterror();
	}
	r = 0;
	while(r < d.length){
		n = devtab[c->type]->read(c, data+r, d.length-r, (vlong)r);
		if(n <= 0)
			break;
		r += n;
	}
	poperror();
	poperror();
	cclose(c);
	return fw;
}

static char*
boot(Ctlr *ctlr)
{
	int ntries;

	/* set "host program ram write selection" bit */
	csr32w(ctlr, SysCtrl, HstPmSel);
	/* write microcode image */
	memwrite(ctlr, FwBase, ctlr->fw->data, ctlr->fw->size);
	/* kick microcontroller unit */
	csr32w(ctlr, SysCtrl, 0);
	coherence();
	csr32w(ctlr, SysCtrl, McuReset);

	csr32w(ctlr, H2mBbpagent, 0);
	csr32w(ctlr, H2mMailbox, 0);

	/* wait until microcontroller is ready */
	coherence();
	for(ntries = 0; ntries < 1000; ntries++){
		if(csr32r(ctlr, SysCtrl) & McuReady)
			break;
		microdelay(1000);
	}
	if(ntries == 1000)
		return "timeout waiting for MCU to initialize";
	return 0;
}

/*
 * Send a command to the 8051 microcontroller unit.
 */
static int
mcucmd(Ctlr *ctlr, u8int cmd, u16int arg, int wait)
{
	int slot, ntries;
	u32int tmp;
	u8int cid;

	SET(slot);
	for(ntries = 0; ntries < 100; ntries++){
		if(!(csr32r(ctlr, H2mMailbox) & H2mBusy))
			break;
		microdelay(2);
	}
	if(ntries == 100)
		return -1;

	cid = wait ? cmd : TokenNoIntr;
	csr32w(ctlr, H2mMailbox, H2mBusy | cid << 16 | arg);
	coherence();
	csr32w(ctlr, HostCmd, cmd);

	if(!wait)
		return 0;
	/* wait for the command to complete */
	for(ntries = 0; ntries < 200; ntries++){
		tmp = csr32r(ctlr, H2mMailboxCid);
		/* find the command slot */
		for(slot = 0; slot < 4; slot++, tmp >>= 8)
			if((tmp & 0xff) == cid)
				break;
		if(slot < 4)
			break;
		microdelay(100);
	}
	if(ntries == 200){
		/* clear command and status */
		csr32w(ctlr, H2mMailboxStatus, 0xffffffff);
		csr32w(ctlr, H2mMailboxCid, 0xffffffff);
		return -1;
	}
	/* get command status (1 means success) */
	tmp = csr32r(ctlr, H2mMailboxStatus);
	tmp = (tmp >> (slot * 8)) & 0xff;
	/* clear command and status */
	csr32w(ctlr, H2mMailboxStatus, 0xffffffff);
	csr32w(ctlr, H2mMailboxCid, 0xffffffff);
	return (tmp == 1) ? 0 : -1;
}


/*
 * Reading and writing from/to the BBP is different from RT2560 and RT2661.
 * We access the BBP through the 8051 microcontroller unit which means that
 * the microcode must be loaded first.
 */
static void
bbpwrite(Ctlr *ctlr, u8int reg, u8int val)
{
	int ntries;

	for(ntries = 0; ntries < 100; ntries++){
		if(!(csr32r(ctlr, H2mBbpagent) & BbpCsrKick))
			break;
		microdelay(1);
	}
	if(ntries == 100){
		print("could not write to BBP through MCU\n");
		return;
	}

	csr32w(ctlr, H2mBbpagent, BbpRwParallel |
	    BbpCsrKick | reg << 8 | val);
	coherence();

	mcucmd(ctlr, McuCmdBbp, 0, 0);
	microdelay(1000);
}

static u8int
bbpread(Ctlr *ctlr, u8int reg)
{
	u32int val;
	int ntries;

	for(ntries = 0; ntries < 100; ntries++){
		if(!(csr32r(ctlr, H2mBbpagent) & BbpCsrKick))
			break;
		microdelay(1);
	}
	if(ntries == 100){
		print("could not read from BBP through MCU");
		return 0;
	}

	csr32w(ctlr, H2mBbpagent, BbpRwParallel |
	    BbpCsrKick | BbpCsrRead | reg << 8);
	coherence();

	mcucmd(ctlr, McuCmdBbp, 0, 0);
	microdelay(1000);

	for(ntries = 0; ntries < 100; ntries++){
		val = csr32r(ctlr, H2mBbpagent);
		if(!(val & BbpCsrKick))
			return val & 0xff;
		microdelay(1);
	}
	print("could not read from BBP through MCU\n");

	return 0;
}

static char*
bbpinit(Ctlr *ctlr)
{
	int i, ntries;
	char *err;

	/* wait for BBP to wake up */
	for(ntries = 0; ntries < 20; ntries++){
		u8int bbp0 = bbpread(ctlr, 0);
		if(bbp0 != 0 && bbp0 != 0xff)
			break;
	}
	if(ntries == 20){
		err = "timeout waiting for BBP to wake up";
		return err;
	}

	/* initialize BBP registers to default values */
	for(i = 0; i < nelem(rt2860_def_bbp); i++){
		bbpwrite(ctlr, rt2860_def_bbp[i].reg,
		    rt2860_def_bbp[i].val);
	}

	/* fix BBP84 for RT2860E */
	if(ctlr->mac_ver == 0x2860 && ctlr->mac_rev != 0x0101)
		bbpwrite(ctlr, 84, 0x19);

	if(ctlr->mac_ver >= 0x3071){
		bbpwrite(ctlr, 79, 0x13);
		bbpwrite(ctlr, 80, 0x05);
		bbpwrite(ctlr, 81, 0x33);
	}else if(ctlr->mac_ver == 0x2860 && ctlr->mac_rev == 0x0100){
		bbpwrite(ctlr, 69, 0x16);
		bbpwrite(ctlr, 73, 0x12);
	}

	return nil;

}

static void
setleds(Ctlr *ctlr, u16int which)
{
	mcucmd(ctlr, McuCmdLeds,
	    which | (ctlr->leds & 0x7f), 0);
}

static char*
txrxon(Ctlr *ctlr)
{
	u32int tmp;
	int ntries;
	char *err;

	SET(tmp);
	/* enable Tx/Rx DMA engine */
	csr32w(ctlr, MacSysCtrl, MacTxEn);
	coherence();
	for(ntries = 0; ntries < 200; ntries++){
		tmp = csr32r(ctlr, WpdmaGloCfg);
		if((tmp & (TxDmaBusy | RxDmaBusy)) == 0)
			break;
		microdelay(1000);
	}
	if(ntries == 200){
		err = "timeout waiting for DMA engine";
		return err;
	}

	microdelay(50);

	tmp |= RxDmaEn | TxDmaEn |
	    WpdmaBtSize64 << WpdmaBtSizeShift;
	csr32w(ctlr, WpdmaGloCfg, tmp);

	/* set Rx filter */
	tmp = DropCrcErr | DropPhyErr;
	if(!ctlr->prom){
		tmp |= DropUcNome | DropDupl |
		    DropCts | DropBa | DropAck |
		    DropVerErr | DropCtrlRsv |
		    DropCfack | DropCfend;
		tmp |= DropRts | DropPspoll;
	} 
	csr32w(ctlr, RxFiltrCfg, tmp);

	csr32w(ctlr, MacSysCtrl, MacRxEn | MacTxEn);

	return 0;
}

/*
 * Write to one of the 4 programmable 24-bit RF registers.
 */
static void
rfwrite(Ctlr *ctlr, u8int reg, u32int val)
{
	u32int tmp;
	int ntries;

	for(ntries = 0; ntries < 100; ntries++){
		if(!(csr32r(ctlr, RfCsrCfg0) & RfRegCtrl))
			break;
		microdelay(1);
	}
	if(ntries == 100){
		print("could not write to RF\n");
		return;
	}

	/* RF registers are 24-bit on the RT2860 */
	tmp = RfRegCtrl | 24 << RfRegWidthShift |
	    (val & 0x3fffff) << 2 | (reg & 3);
	csr32w(ctlr, RfCsrCfg0, tmp);
}

u8int
rt3090rfread(Ctlr *ctlr, u8int reg)
{
	u32int tmp;
	int ntries;

	for(ntries = 0; ntries < 100; ntries++){
		if(!(csr32r(ctlr, Rt3070RfCsrCfg) & Rt3070RfKick))
			break;
		microdelay(1);
	}
	if(ntries == 100){
		print("could not read RF register\n");
		return 0xff;
	}
	tmp = Rt3070RfKick | reg << 8;
	csr32w(ctlr, Rt3070RfCsrCfg, tmp);

	for(ntries = 0; ntries < 100; ntries++){
		tmp = csr32r(ctlr, Rt3070RfCsrCfg);
		if(!(tmp & Rt3070RfKick))
			break;
		microdelay(1);
	}
	if(ntries == 100){
		print("could not read RF register\n");
		return 0xff;
	}
	return tmp & 0xff;
}

static void
rt3090rfwrite(Ctlr *ctlr, u8int reg, u8int val)
{
	u32int tmp;
	int ntries;

	for(ntries = 0; ntries < 10; ntries++){
		if(!(csr32r(ctlr, Rt3070RfCsrCfg) & Rt3070RfKick))
			break;
		microdelay(10);
	}
	if(ntries == 10){
		print("could not write to RF\n");
		return;
	}

	tmp = Rt3070RfWrite | Rt3070RfKick | reg << 8 | val;
	csr32w(ctlr, Rt3070RfCsrCfg, tmp);
}

static void
selchangroup(Ctlr *ctlr, int group)
{
	u32int tmp;
	u8int agc;

	bbpwrite(ctlr, 62, 0x37 - ctlr->lna[group]);
	bbpwrite(ctlr, 63, 0x37 - ctlr->lna[group]);
	bbpwrite(ctlr, 64, 0x37 - ctlr->lna[group]);
	bbpwrite(ctlr, 86, 0x00);

	if(group == 0){
		if(ctlr->ext_2ghz_lna){
			bbpwrite(ctlr, 82, 0x62);
			bbpwrite(ctlr, 75, 0x46);
		}else{
			bbpwrite(ctlr, 82, 0x84);
			bbpwrite(ctlr, 75, 0x50);
		}
	}else{
		if(ctlr->ext_5ghz_lna){
			bbpwrite(ctlr, 82, 0xf2);
			bbpwrite(ctlr, 75, 0x46);
		}else{
			bbpwrite(ctlr, 82, 0xf2);
			bbpwrite(ctlr, 75, 0x50);
		}
	}

	tmp = csr32r(ctlr, TxBandCfg);
	tmp &= ~(Tx5gBandSelN | Tx5gBandSelP);
	tmp |= (group == 0) ? Tx5gBandSelN : Tx5gBandSelP;
	csr32w(ctlr, TxBandCfg, tmp);

	/* enable appropriate Power Amplifiers and Low Noise Amplifiers */
	tmp = RftrEn | TrswEn | LnaPe0En;
	if(ctlr->nrxchains > 1)
		tmp |= LnaPe1En;
	if(ctlr->mac_ver == 0x3593 && ctlr->nrxchains > 2)
		tmp |= Rt3593LnaPe2En;
	if(group == 0){	/* 2GHz */
		tmp |= PaPeG0En;
		if(ctlr->ntxchains > 1)
			tmp |= PaPeG1En;
		if(ctlr->mac_ver == 0x3593 && ctlr->ntxchains > 2)
			tmp |= Rt3593PaPeG2En;
	}else{		/* 5GHz */
		tmp |= PaPeA0En;
		if(ctlr->ntxchains > 1)
			tmp |= PaPeA1En;
		if(ctlr->mac_ver == 0x3593 && ctlr->ntxchains > 2)
			tmp |= Rt3593PaPeA2En;
	}
	csr32w(ctlr, TxPinCfg, tmp);

	if(ctlr->mac_ver == 0x3593){
		tmp = csr32r(ctlr, GpioCtrl);
		if(ctlr->flags & ConnPciE){
			tmp &= ~0x01010000;
			if(group == 0)
				tmp |= 0x00010000;
		}else{
			tmp &= ~0x00008080;
			if(group == 0)
				tmp |= 0x00000080;
		}
		tmp = (tmp & ~0x00001000) | 0x00000010;
		csr32w(ctlr, GpioCtrl, tmp);
	}

	/* set initial AGC value */
	if(group == 0){	/* 2GHz band */
		if(ctlr->mac_ver >= 0x3071)
			agc = 0x1c + ctlr->lna[0] * 2;
		else
			agc = 0x2e + ctlr->lna[0];
	}else{		/* 5GHz band */
		agc = 0x32 + (ctlr->lna[group] * 5) / 3;
	}
	bbpwrite(ctlr, 66, agc);

	microdelay(1000);

}

static void
setchan(Ctlr *ctlr, uint chan)
{
	const struct rfprog *rfprog = rt2860_rf2850;
	u32int r2, r3, r4;
	s8int txpow1, txpow2;
	uint i;

	/* find the settings for this channel (we know it exists) */
	for(i = 0; rfprog[i].chan != chan; i++);

	r2 = rfprog[i].r2;
	if(ctlr->ntxchains == 1)
		r2 |= 1 << 12;		/* 1T: disable Tx chain 2 */
	if(ctlr->nrxchains == 1)
		r2 |= 1 << 15 | 1 << 4;	/* 1R: disable Rx chains 2 & 3 */
	else if(ctlr->nrxchains == 2)
		r2 |= 1 << 4;		/* 2R: disable Rx chain 3 */

	/* use Tx power values from EEPROM */
	txpow1 = ctlr->txpow1[i];
	txpow2 = ctlr->txpow2[i];
	if(chan > 14){
		if(txpow1 >= 0)
			txpow1 = txpow1 << 1 | 1;
		else
			txpow1 = (7 + txpow1) << 1;
		if(txpow2 >= 0)
			txpow2 = txpow2 << 1 | 1;
		else
			txpow2 = (7 + txpow2) << 1;
	}
	r3 = rfprog[i].r3 | txpow1 << 7;
	r4 = rfprog[i].r4 | ctlr->freq << 13 | txpow2 << 4;

	rfwrite(ctlr, Rf1, rfprog[i].r1);
	rfwrite(ctlr, Rf2, r2);
	rfwrite(ctlr, Rf3, r3);
	rfwrite(ctlr, Rf4, r4);

	microdelay(200);

	rfwrite(ctlr, Rf1, rfprog[i].r1);
	rfwrite(ctlr, Rf2, r2);
	rfwrite(ctlr, Rf3, r3 | 1);
	rfwrite(ctlr, Rf4, r4);

	microdelay(200);

	rfwrite(ctlr, Rf1, rfprog[i].r1);
	rfwrite(ctlr, Rf2, r2);
	rfwrite(ctlr, Rf3, r3);
	rfwrite(ctlr, Rf4, r4);
}

static void
rt3090setchan(Ctlr *ctlr, uint chan)
{
	s8int txpow1, txpow2;
	u8int rf;
	int i;

	assert(chan >= 1 && chan <= 14);	/* RT3090 is 2GHz only */

	/* find the settings for this channel (we know it exists) */
	for(i = 0; rt2860_rf2850[i].chan != chan; i++);

	/* use Tx power values from EEPROM */
	txpow1 = ctlr->txpow1[i];
	txpow2 = ctlr->txpow2[i];

	rt3090rfwrite(ctlr, 2, rt3090_freqs[i].n);
	rf = rt3090rfread(ctlr, 3);
	rf = (rf & ~0x0f) | rt3090_freqs[i].k;
	rt3090rfwrite(ctlr, 3, rf);
	rf = rt3090rfread(ctlr, 6);
	rf = (rf & ~0x03) | rt3090_freqs[i].r;
	rt3090rfwrite(ctlr, 6, rf);

	/* set Tx0 power */
	rf = rt3090rfread(ctlr, 12);
	rf = (rf & ~0x1f) | txpow1;
	rt3090rfwrite(ctlr, 12, rf);

	/* set Tx1 power */
	rf = rt3090rfread(ctlr, 13);
	rf = (rf & ~0x1f) | txpow2;
	rt3090rfwrite(ctlr, 13, rf);

	rf = rt3090rfread(ctlr, 1);
	rf &= ~0xfc;
	if(ctlr->ntxchains == 1)
		rf |= Rt3070Tx1Pd | Rt3070Tx2Pd;
	else if(ctlr->ntxchains == 2)
		rf |= Rt3070Tx2Pd;
	if(ctlr->nrxchains == 1)
		rf |= Rt3070Rx1Pd | Rt3070Rx2Pd;
	else if(ctlr->nrxchains == 2)
		rf |= Rt3070Rx2Pd;
	rt3090rfwrite(ctlr, 1, rf);

	/* set RF offset */
	rf = rt3090rfread(ctlr, 23);
	rf = (rf & ~0x7f) | ctlr->freq;
	rt3090rfwrite(ctlr, 23, rf);

	/* program RF filter */
	rf = rt3090rfread(ctlr, 24);	/* Tx */
	rf = (rf & ~0x3f) | ctlr->rf24_20mhz;
	rt3090rfwrite(ctlr, 24, rf);
	rf = rt3090rfread(ctlr, 31);	/* Rx */
	rf = (rf & ~0x3f) | ctlr->rf24_20mhz;
	rt3090rfwrite(ctlr, 31, rf);

	/* enable RF tuning */
	rf = rt3090rfread(ctlr, 7);
	rt3090rfwrite(ctlr, 7, rf | Rt3070Tune);
}

static int
rt3090filtercalib(Ctlr *ctlr, u8int init, u8int target, u8int *val)
{
	u8int rf22, rf24;
	u8int bbp55_pb, bbp55_sb, delta;
	int ntries;

	/* program filter */
	rf24 = rt3090rfread(ctlr, 24);
	rf24 = (rf24 & 0xc0) | init;	/* initial filter value */
	rt3090rfwrite(ctlr, 24, rf24);

	/* enable baseband loopback mode */
	rf22 = rt3090rfread(ctlr, 22);
	rt3090rfwrite(ctlr, 22, rf22 | Rt3070BbLoopback);

	/* set power and frequency of passband test tone */
	bbpwrite(ctlr, 24, 0x00);
	for(ntries = 0, bbp55_pb = 0; ntries < 100; ntries++){
		/* transmit test tone */
		bbpwrite(ctlr, 25, 0x90);
		microdelay(1000);
		/* read received power */
		bbp55_pb = bbpread(ctlr, 55);
		if(bbp55_pb != 0)
			break;
	}
	if(ntries == 100)
		return -1;

	/* set power and frequency of stopband test tone */
	bbpwrite(ctlr, 24, 0x06);
	for(ntries = 0; ntries < 100; ntries++){
		/* transmit test tone */
		bbpwrite(ctlr, 25, 0x90);
		microdelay(1000);
		/* read received power */
		bbp55_sb = bbpread(ctlr, 55);

		delta = bbp55_pb - bbp55_sb;
		if(delta > target)
			break;

		/* reprogram filter */
		rf24++;
		rt3090rfwrite(ctlr, 24, rf24);
	}
	if(ntries < 100){
		if(rf24 != init)
			rf24--;	/* backtrack */
		*val = rf24;
		rt3090rfwrite(ctlr, 24, rf24);
	}

	/* restore initial state */
	bbpwrite(ctlr, 24, 0x00);

	/* disable baseband loopback mode */
	rf22 = rt3090rfread(ctlr, 22);
	rt3090rfwrite(ctlr, 22, rf22 & ~Rt3070BbLoopback);

	return 0;
}

static void
rt3090setrxantenna(Ctlr *ctlr, int aux)
{
	u32int tmp;

	if(aux){
		tmp = csr32r(ctlr, PciEectrl);
		csr32w(ctlr, PciEectrl, tmp & ~EectrlC);
		tmp = csr32r(ctlr, GpioCtrl);
		csr32w(ctlr, GpioCtrl, (tmp & ~0x0808) | 0x08);
	}else{
		tmp = csr32r(ctlr, PciEectrl);
		csr32w(ctlr, PciEectrl, tmp | EectrlC);
		tmp = csr32r(ctlr, GpioCtrl);
		csr32w(ctlr, GpioCtrl, tmp & ~0x0808);
	}
}

static int
rt3090rfinit(Ctlr *ctlr)
{
	u32int tmp;
	u8int rf, bbp;
	int i;

	rf = rt3090rfread(ctlr, 30);
	/* toggle RF R30 bit 7 */
	rt3090rfwrite(ctlr, 30, rf | 0x80);
	microdelay(1000);
	rt3090rfwrite(ctlr, 30, rf & ~0x80);

	tmp = csr32r(ctlr, Rt3070LdoCfg0);
	tmp &= ~0x1f000000;
	if(ctlr->patch_dac && ctlr->mac_rev < 0x0211)
		tmp |= 0x0d000000;	/* 1.35V */
	else
		tmp |= 0x01000000;	/* 1.2V */
	csr32w(ctlr, Rt3070LdoCfg0, tmp);

	/* patch LNA_PE_G1 */
	tmp = csr32r(ctlr, Rt3070GpioSwitch);
	csr32w(ctlr, Rt3070GpioSwitch, tmp & ~0x20);

	/* initialize RF registers to default value */
	for(i = 0; i < nelem(rt3090_def_rf); i++){
		rt3090rfwrite(ctlr, rt3090_def_rf[i].reg,
		    rt3090_def_rf[i].val);
	}

	/* select 20MHz bandwidth */
	rt3090rfwrite(ctlr, 31, 0x14);

	rf = rt3090rfread(ctlr, 6);
	rt3090rfwrite(ctlr, 6, rf | 0x40);

	if(ctlr->mac_ver != 0x3593){
		/* calibrate filter for 20MHz bandwidth */
		ctlr->rf24_20mhz = 0x1f;	/* default value */
		rt3090filtercalib(ctlr, 0x07, 0x16, &ctlr->rf24_20mhz);

		/* select 40MHz bandwidth */
		bbp = bbpread(ctlr, 4);
		bbpwrite(ctlr, 4, (bbp & ~0x08) | 0x10);
		rf = rt3090rfread(ctlr, 31);
		rt3090rfwrite(ctlr, 31, rf | 0x20);

		/* calibrate filter for 40MHz bandwidth */
		ctlr->rf24_40mhz = 0x2f;	/* default value */
		rt3090filtercalib(ctlr, 0x27, 0x19, &ctlr->rf24_40mhz);

		/* go back to 20MHz bandwidth */
		bbp = bbpread(ctlr, 4);
		bbpwrite(ctlr, 4, bbp & ~0x18);
	}
	if(ctlr->mac_rev < 0x0211)
		rt3090rfwrite(ctlr, 27, 0x03);

	tmp = csr32r(ctlr, Rt3070Opt14);
	csr32w(ctlr, Rt3070Opt14, tmp | 1);

	if(ctlr->rf_rev == Rf3020)
		rt3090setrxantenna(ctlr, 0);

	bbp = bbpread(ctlr, 138);
	if(ctlr->mac_ver == 0x3593){
		if(ctlr->ntxchains == 1)
			bbp |= 0x60;	/* turn off DAC1 and DAC2 */
		else if(ctlr->ntxchains == 2)
			bbp |= 0x40;	/* turn off DAC2 */
		if(ctlr->nrxchains == 1)
			bbp &= ~0x06;	/* turn off ADC1 and ADC2 */
		else if(ctlr->nrxchains == 2)
			bbp &= ~0x04;	/* turn off ADC2 */
	}else{
		if(ctlr->ntxchains == 1)
			bbp |= 0x20;	/* turn off DAC1 */
		if(ctlr->nrxchains == 1)
			bbp &= ~0x02;	/* turn off ADC1 */
	}
	bbpwrite(ctlr, 138, bbp);

	rf = rt3090rfread(ctlr, 1);
	rf &= ~(Rt3070Rx0Pd | Rt3070Tx0Pd);
	rf |= Rt3070RfBlock | Rt3070Rx1Pd | Rt3070Tx1Pd;
	rt3090rfwrite(ctlr, 1, rf);

	rf = rt3090rfread(ctlr, 15);
	rt3090rfwrite(ctlr, 15, rf & ~Rt3070TxLo2);

	rf = rt3090rfread(ctlr, 17);
	rf &= ~Rt3070TxLo1;
	if(ctlr->mac_rev >= 0x0211 && !ctlr->ext_2ghz_lna)
		rf |= 0x20;	/* fix for long range Rx issue */
	if(ctlr->txmixgain_2ghz >= 2)
		rf = (rf & ~0x7) | ctlr->txmixgain_2ghz;
	rt3090rfwrite(ctlr, 17, rf);

	rf = rt3090rfread(ctlr, 20);
	rt3090rfwrite(ctlr, 20, rf & ~Rt3070RxLo1);

	rf = rt3090rfread(ctlr, 21);
	rt3090rfwrite(ctlr, 21, rf & ~Rt3070RxLo2);

	return 0;
}

static void
rt3090rfwakeup(Ctlr *ctlr)
{
	u32int tmp;
	u8int rf;

	if(ctlr->mac_ver == 0x3593){
		/* enable VCO */
		rf = rt3090rfread(ctlr, 1);
		rt3090rfwrite(ctlr, 1, rf | Rt3593Vco);

		/* initiate VCO calibration */
		rf = rt3090rfread(ctlr, 3);
		rt3090rfwrite(ctlr, 3, rf | Rt3593Vcocal);

		/* enable VCO bias current control */
		rf = rt3090rfread(ctlr, 6);
		rt3090rfwrite(ctlr, 6, rf | Rt3593VcoIc);

		/* initiate res calibration */
		rf = rt3090rfread(ctlr, 2);
		rt3090rfwrite(ctlr, 2, rf | Rt3593Rescal);

		/* set reference current control to 0.33 mA */
		rf = rt3090rfread(ctlr, 22);
		rf &= ~Rt3593CpIcMask;
		rf |= 1 << Rt3593CpIcShift;
		rt3090rfwrite(ctlr, 22, rf);

		/* enable RX CTB */
		rf = rt3090rfread(ctlr, 46);
		rt3090rfwrite(ctlr, 46, rf | Rt3593RxCtb);

		rf = rt3090rfread(ctlr, 20);
		rf &= ~(Rt3593LdoRfVcMask | Rt3593LdoPllVcMask);
		rt3090rfwrite(ctlr, 20, rf);
	}else{
		/* enable RF block */
		rf = rt3090rfread(ctlr, 1);
		rt3090rfwrite(ctlr, 1, rf | Rt3070RfBlock);

		/* enable VCO bias current control */
		rf = rt3090rfread(ctlr, 7);
		rt3090rfwrite(ctlr, 7, rf | 0x30);

		rf = rt3090rfread(ctlr, 9);
		rt3090rfwrite(ctlr, 9, rf | 0x0e);

		/* enable RX CTB */
		rf = rt3090rfread(ctlr, 21);
		rt3090rfwrite(ctlr, 21, rf | Rt3070RxCtb);

		/* fix Tx to Rx IQ glitch by raising RF voltage */
		rf = rt3090rfread(ctlr, 27);
		rf &= ~0x77;
		if(ctlr->mac_rev < 0x0211)
			rf |= 0x03;
		rt3090rfwrite(ctlr, 27, rf);
	}
	if(ctlr->patch_dac && ctlr->mac_rev < 0x0211){
		tmp = csr32r(ctlr, Rt3070LdoCfg0);
		tmp = (tmp & ~0x1f000000) | 0x0d000000;
		csr32w(ctlr, Rt3070LdoCfg0, tmp);
	}
}

static void
rt3090rfsetup(Ctlr *ctlr)
{
	u8int bbp;
	int i;

	if(ctlr->mac_rev >= 0x0211){
		/* enable DC filter */
		bbpwrite(ctlr, 103, 0xc0);

		/* improve power consumption */
		bbp = bbpread(ctlr, 31);
		bbpwrite(ctlr, 31, bbp & ~0x03);
	}

	csr32w(ctlr, TxSwCfg1, 0);
	if(ctlr->mac_rev < 0x0211){
		csr32w(ctlr, TxSwCfg2,
		    ctlr->patch_dac ? 0x2c : 0x0f);
	}else
		csr32w(ctlr, TxSwCfg2, 0);

	/* initialize RF registers from ROM */
	for(i = 0; i < 10; i++){
		if(ctlr->rf[i].reg == 0 || ctlr->rf[i].reg == 0xff)
			continue;
		rt3090rfwrite(ctlr, ctlr->rf[i].reg, ctlr->rf[i].val);
	}
}

static void
updateprot(Ctlr *ctlr)
{
	u32int tmp;

	tmp = RtsthEn | ProtNavShort | TxopAllowAll;
	/* setup protection frame rate (MCS code) */
	tmp |= /*(ic->ic_curmode == IEEE80211_MODE_11A) ?
	    rt2860_rates[RT2860_RIDX_OFDM6].mcs :*/
	    rt2860_rates[RidxCck11].mcs;

	/* CCK frames don't require protection */
	csr32w(ctlr, CckProtCfg, tmp);
/* XXX
	if(ic->ic_flags & IEEE80211_F_USEPROT){
		if(ic->ic_protmode == IEEE80211_PROT_RTSCTS)
			tmp |= ProtCtrlRtsCts;
		else if(ic->ic_protmode == IEEE80211_PROT_CTSONLY)
			tmp |= ProtCtrlCts;
	}
	csr32w(ctlr, OfdmProtCfg, tmp); */
}

static char*
rt2860start(Ether *edev)
{
	u32int tmp;
	u8int bbp1, bbp3;
	int i, qid, ridx, ntries;
	char *err;
	Ctlr *ctlr;

	ctlr = edev->ctlr;
	csr32w(ctlr, PwrPinCfg, IoRaPe);

	/* disable DMA */
	tmp = csr32r(ctlr, WpdmaGloCfg);
	tmp &= 0xff0;
	csr32w(ctlr, WpdmaGloCfg, tmp);

	/* PBF hardware reset */
	csr32w(ctlr, SysCtrl, 0xe1f);
	coherence();
	csr32w(ctlr, SysCtrl, 0xe00);

	if((err = boot(ctlr)) != nil){
		/*XXX: rt2860stop(ifp, 1);*/
		return err;
	}
	/* set MAC address */
	csr32w(ctlr, MacAddrDw0,
	    edev->ea[0] | edev->ea[1] << 8 | edev->ea[2] << 16 | edev->ea[3] << 24);
	csr32w(ctlr, MacAddrDw1,
	    edev->ea[4] | edev->ea[5] << 8 | 0xff << 16);

	/* init Tx power for all Tx rates (from EEPROM) */
	for(ridx = 0; ridx < 5; ridx++){
		if(ctlr->txpow20mhz[ridx] == 0xffffffff)
			continue;
		csr32w(ctlr, TxPwrCfg(ridx), ctlr->txpow20mhz[ridx]);
	}

	for(ntries = 0; ntries < 100; ntries++){
		tmp = csr32r(ctlr, WpdmaGloCfg);
		if((tmp & (TxDmaBusy | RxDmaBusy)) == 0)
			break;
		microdelay(1000);
	}
	if(ntries == 100){
		err = "timeout waiting for DMA engine";
		/*rt2860_stop(ifp, 1);*/
		return err;
	}
	tmp &= 0xff0;
	csr32w(ctlr, WpdmaGloCfg, tmp);

	/* reset Rx ring and all 6 Tx rings */
	csr32w(ctlr, WpdmaRstIdx, 0x1003f);

	/* PBF hardware reset */
	csr32w(ctlr, SysCtrl, 0xe1f);
	coherence();
	csr32w(ctlr, SysCtrl, 0xe00);

	csr32w(ctlr, PwrPinCfg, IoRaPe | IoRfPe);

	csr32w(ctlr, MacSysCtrl, BbpHrst | MacSrst);
	coherence();
	csr32w(ctlr, MacSysCtrl, 0);

	for(i = 0; i < nelem(rt2860_def_mac); i++)
		csr32w(ctlr, rt2860_def_mac[i].reg, rt2860_def_mac[i].val);
	if(ctlr->mac_ver >= 0x3071){
		/* set delay of PA_PE assertion to 1us (unit of 0.25us) */
		csr32w(ctlr, TxSwCfg0,
		    4 << DlyPapeEnShift);
	}

	if(!(csr32r(ctlr, PciCfg) & PciCfgPci)){
		ctlr->flags |= ConnPciE;
		/* PCIe has different clock cycle count than PCI */
		tmp = csr32r(ctlr, UsCycCnt);
		tmp = (tmp & ~0xff) | 0x7d;
		csr32w(ctlr, UsCycCnt, tmp);
	}

	/* wait while MAC is busy */
	for(ntries = 0; ntries < 100; ntries++){
		if(!(csr32r(ctlr, MacStatusReg) &
		    (RxStatusBusy | TxStatusBusy)))
			break;
		microdelay(1000);
	}
	if(ntries == 100){
		err = "timeout waiting for MAC";
		/*rt2860_stop(ifp, 1);*/
		return err;
	}

	/* clear Host to MCU mailbox */
	csr32w(ctlr, H2mBbpagent, 0);
	csr32w(ctlr, H2mMailbox, 0);

	mcucmd(ctlr, McuCmdRfreset, 0, 0);
	microdelay(1000);

	if((err = bbpinit(ctlr)) != nil){
		/*rt2860_stop(ifp, 1);*/
		return err;
	}
	/* clear RX WCID search table */
	setregion(ctlr, WcidEntry(0), 0, 512);
	/* clear pairwise key table */
	setregion(ctlr, Pkey(0), 0, 2048);
	/* clear IV/EIV table */
	setregion(ctlr, Iveiv(0), 0, 512);
	/* clear WCID attribute table */
	setregion(ctlr, WcidAttr(0), 0, 256);
	/* clear shared key table */
	setregion(ctlr, Skey(0, 0), 0, 8 * 32);
	/* clear shared key mode */
	setregion(ctlr, SkeyMode07, 0, 4);

	/* init Tx rings (4 EDCAs + HCCA + Mgt) */
	for(qid = 0; qid < 6; qid++){
		csr32w(ctlr, TxBasePtr(qid), PCIWADDR(ctlr->tx[qid].d));
		csr32w(ctlr, TxMaxCnt(qid), Ntx);
		csr32w(ctlr, TxCtxIdx(qid), 0);
	}

	/* init Rx ring */
	csr32w(ctlr, RxBasePtr, PCIWADDR(ctlr->rx.p));
	csr32w(ctlr, RxMaxCnt, Nrx);
	csr32w(ctlr, RxCalcIdx, Nrx - 1);

	/* setup maximum buffer sizes */
	csr32w(ctlr, MaxLenCfg, 1 << 12 |
	    (Rbufsize - Rxwisize - 2));

	for(ntries = 0; ntries < 100; ntries++){
		tmp = csr32r(ctlr, WpdmaGloCfg);
		if((tmp & (TxDmaBusy | RxDmaBusy)) == 0)
			break;
		microdelay(1000);
	}
	if(ntries == 100){
		err = "timeout waiting for DMA engine";
		/*rt2860_stop(ifp, 1);*/
		return err;
	}
	tmp &= 0xff0;
	csr32w(ctlr, WpdmaGloCfg, tmp);

	/* disable interrupts mitigation */
	csr32w(ctlr, DelayIntCfg, 0);

	/* write vendor-specific BBP values (from EEPROM) */
	for(i = 0; i < 8; i++){
		if(ctlr->bbp[i].reg == 0 || ctlr->bbp[i].reg == 0xff)
			continue;
		bbpwrite(ctlr, ctlr->bbp[i].reg, ctlr->bbp[i].val);
	}

	/* select Main antenna for 1T1R devices */
	if(ctlr->rf_rev == Rf2020 ||
	    ctlr->rf_rev == Rf3020 ||
	    ctlr->rf_rev == Rf3320)
		rt3090setrxantenna(ctlr, 0);

	/* send LEDs operating mode to microcontroller */
	mcucmd(ctlr, McuCmdLed1, ctlr->led[0], 0);
	mcucmd(ctlr, McuCmdLed2, ctlr->led[1], 0);
	mcucmd(ctlr, McuCmdLed3, ctlr->led[2], 0);

	if(ctlr->mac_ver >= 0x3071)
		rt3090rfinit(ctlr);

	mcucmd(ctlr, McuCmdSleep, 0x02ff, 1);
	mcucmd(ctlr, McuCmdWakeup, 0, 1);

	if(ctlr->mac_ver >= 0x3071)
		rt3090rfwakeup(ctlr);

	/* disable non-existing Rx chains */
	bbp3 = bbpread(ctlr, 3);
	bbp3 &= ~(1 << 3 | 1 << 4);
	if(ctlr->nrxchains == 2)
		bbp3 |= 1 << 3;
	else if(ctlr->nrxchains == 3)
		bbp3 |= 1 << 4;
	bbpwrite(ctlr, 3, bbp3);

	/* disable non-existing Tx chains */
	bbp1 = bbpread(ctlr, 1);
	if(ctlr->ntxchains == 1)
		bbp1 = (bbp1 & ~(1 << 3 | 1 << 4));
	else if(ctlr->mac_ver == 0x3593 && ctlr->ntxchains == 2)
		bbp1 = (bbp1 & ~(1 << 4)) | 1 << 3;
	else if(ctlr->mac_ver == 0x3593 && ctlr->ntxchains == 3)
		bbp1 = (bbp1 & ~(1 << 3)) | 1 << 4;
	bbpwrite(ctlr, 1, bbp1);

	if(ctlr->mac_ver >= 0x3071)
		rt3090rfsetup(ctlr);

	/* select default channel */
	if(ctlr->mac_ver >= 0x3071)
		rt3090setchan(ctlr, 3);
	else
		setchan(ctlr, 3);

	/* reset RF from MCU */
	mcucmd(ctlr, McuCmdRfreset, 0, 0);

	/* set RTS threshold */
	tmp = csr32r(ctlr, TxRtsCfg);
	tmp &= ~0xffff00;
	tmp |= 1 /* ic->ic_rtsthreshold */ << 8;
	csr32w(ctlr, TxRtsCfg, tmp);

	/* setup initial protection mode */
	updateprot(ctlr);

	/* turn radio LED on */
	setleds(ctlr, LedRadio);

	/* enable Tx/Rx DMA engine */
	if((err = txrxon(ctlr)) != 0){
		/*rt2860_stop(ifp, 1);*/
		return err;
	}

	/* clear pending interrupts */
	csr32w(ctlr, IntStatus, 0xffffffff);
	/* enable interrupts */
	csr32w(ctlr, IntMask, 0x3fffc);

	if(ctlr->flags & AdvancedPs)
		mcucmd(ctlr, McuCmdPslevel, ctlr->pslevel, 0);
	return nil;
}

/*
 * Add `delta' (signed) to each 4-bit sub-word of a 32-bit word.
 * Used to adjust per-rate Tx power registers.
 */
static u32int
b4inc(u32int b32, s8int delta)
{
	s8int i, b4;

	for(i = 0; i < 8; i++){
		b4 = b32 & 0xf;
		b4 += delta;
		if(b4 < 0)
			b4 = 0;
		else if(b4 > 0xf)
			b4 = 0xf;
		b32 = b32 >> 4 | b4 << 28;
	}
	return b32;
}

static void
transmit(Wifi *wifi, Wnode *wn, Block *b)
{
	Ether *edev;
	Ctlr *ctlr;
	Wifipkt *w;
	u8int mcs, qid;
	int ridx, /*ctl_ridx,*/ hdrlen;
	uchar *p;
	int nodeid;
	Block *outb;
	TXQ *tx;
	Pool *pool;

	edev = wifi->ether;
	ctlr = edev->ctlr;

	qlock(ctlr);
	if(ctlr->attached == 0 || ctlr->broken){
		qunlock(ctlr);
		freeb(b);
		return;
	}
	if((wn->channel != ctlr->channel)
	|| (!ctlr->prom && (wn->aid != ctlr->aid || memcmp(wn->bssid, ctlr->bssid, Eaddrlen) != 0)))
		rxon(edev, wn);

	if(b == nil){
		/* association note has no data to transmit */
		qunlock(ctlr);
		return;
	}

	pool = &ctlr->pool;
	qid = 0; /* for now */
	ridx = 0; 
	tx = &ctlr->tx[qid];

	nodeid = ctlr->bcastnodeid;
	w = (Wifipkt*)b->rp;
	hdrlen = wifihdrlen(w);

	p = pool->p + pool->i * TxwiDmaSz;
	if((w->a1[0] & 1) == 0){
		*(p+4) = TxAck; /* xflags */

		if(BLEN(b) > 512-4)
			*(p+1) = TxTxopBackoff; /* txop */ 

		if((w->fc[0] & 0x0c) == 0x08 &&	ctlr->bssnodeid != -1){
			nodeid = ctlr->bssnodeid;
			ridx = 2; /* BUG: hardcode 11Mbit */
		}
	}

	/*ctl_ridx = rt2860_rates[ridx].ctl_ridx;*/
	mcs = rt2860_rates[ridx].mcs;

	/* setup TX Wireless Information */
	*p = 0; /* flags */
	*(p+2) = PhyCck | mcs; /* phy */
	/* let HW generate seq numbers */
	*(p+4) |= TxNseq; /* xflags */
	put16(p + 6, BLEN(b) | (((mcs+1) & 0xf) << TxPidShift) ); /* length */

/*	put16((uchar*)&w->dur[0], rt2860_rates[ctl_ridx].lp_ack_dur); */

	*(p+5) = nodeid; /* wcid */

 	/* copy packet header */
	memmove(p + Txwisize, b->rp, hdrlen);
	
	/* setup tx descriptor */
	/* first segment is TXWI + 802.11 header */
	p = (uchar*)tx->d + Tdscsize * tx->i;
	put32(p, PCIWADDR(pool->p + pool->i * TxwiDmaSz)); /* sdp0 */
	put16(p + 6, Txwisize + hdrlen); /* sdl0 */
	*(p + 15) = TxQselEdca; /* flags */

	/* allocate output buffer */
	b->rp += hdrlen;
	tx->b[tx->i] = outb = iallocb(BLEN(b) + 256);
	if(outb == nil){
		print("outb = nil\n");
		return;
	}
	outb->rp = (uchar*)ROUND((uintptr)outb->base, 256);
	memset(outb->rp, 0, BLEN(b));
	memmove(outb->rp, b->rp, BLEN(b));
	outb->wp = outb->rp + BLEN(b);
	freeb(b);

	/* setup payload segments */
	put32(p + 8, PCIWADDR(outb->rp)); /* sdp1 */
	put16(p + 4, BLEN(outb) | TxLs1); /* sdl1 */

	p = pool->p + pool->i * TxwiDmaSz;
	w = (Wifipkt*)(p + Txwisize);
	if(ctlr->wifi->debug){
		print("transmit: %E->%E,%E nodeid=%x txq[%d]=%d size=%ld\n", w->a2, w->a1, w->a3, nodeid, qid, ctlr->tx[qid].i, BLEN(outb));
	}

	tx->i = (tx->i + 1) % Ntx;
	pool->i = (pool->i + 1) % Ntxpool;

	coherence();

	/* kick Tx */
	csr32w(ctlr, TxCtxIdx(qid), ctlr->tx[qid].i);

	qunlock(ctlr);
	return;
}

static void
rt2860attach(Ether *edev)
{
	FWImage *fw;
	Ctlr *ctlr;
	char *err;

	ctlr = edev->ctlr;
	qlock(ctlr);
	if(waserror()){
		print("#l%d: %s\n", edev->ctlrno, up->errstr);
		/*if(ctlr->power)
			poweroff(ctlr);*/
		qunlock(ctlr);
		nexterror();
	}
	if(ctlr->attached == 0){
		if(ctlr->wifi == nil)
			ctlr->wifi = wifiattach(edev, transmit);

		if(ctlr->fw == nil){
			fw = readfirmware();
			ctlr->fw = fw;
		}
		if((err = rt2860start(edev)) != nil){
			error(err);
		} 

		ctlr->bcastnodeid = -1;
		ctlr->bssnodeid = -1;
		ctlr->channel = 1;
		ctlr->aid = 0;

		setoptions(edev);

		ctlr->attached = 1;
	}
	qunlock(ctlr);
	poperror();
}

static void
receive(Ctlr *ctlr)
{
	u32int hw;
	RXQ *rx;
	Block *b;
	uchar *d;

	rx = &ctlr->rx;
	if(rx->b == nil){
		print("rx->b == nil!");
		return;
	}
	hw = csr32r(ctlr, FsDrxIdx) & 0xfff;
	while(rx->i != hw){
		u16int sdl0, len;
		u32int flags;
		uchar *p;
		Wifipkt *w;
		int hdrlen;

		p = (uchar*)rx->p + Rdscsize * rx->i;
		sdl0 = get16(p + 4 /* sdp0 */ + 2 /* sdl1 */);
		if(!(sdl0 & RxDdone)){
			print("rxd ddone bit not set\n");
			break; /* should not happen */
		}
		flags = get32(p + 12);
		if(flags & (RxCrcerr | RxIcverr)){
		/*	print("crc | icv err\n"); */
			goto skip; 
		}

		b = rx->b[rx->i];
		if(b == nil){
			print("no buf\n");
			goto skip;
		}	
		d = b->rp;
		if(ctlr->wifi == nil)
			goto skip;
		if(rbplant(ctlr, rx->i) < 0){
			print("can't plant");
			goto skip;
		}
		ctlr->wcid = *b->rp;
		len = get16(b->rp + 2 /* wcid, keyidx */) & 0xfff;
		b->rp = d + Rxwisize;
		b->wp = b->rp + len;
		w = (Wifipkt*)b->rp;
		hdrlen = wifihdrlen(w);
		/* HW may insert 2 padding bytes after 802.11 header */
		if(flags & RxL2pad){
			memmove(b->rp + 2, b->rp, hdrlen);
			b->rp += 2;
		}
		w = (Wifipkt*)b->rp;
		if(ctlr->wifi->debug)
			print("receive: %E->%E,%E wcid 0x%x \n", w->a2, w->a1, w->a3, ctlr->wcid);
		wifiiq(ctlr->wifi, b);
skip:
		put16(p + 4 /* sdp0 */ + 2 /* sdl1 */, sdl0 & ~RxDdone);
		rx->i = (rx->i + 1) % Nrx;
	}
	coherence();
	/* tell HW what we have processed */
	csr32w(ctlr, RxCalcIdx, (rx->i - 1) % Nrx);
}

static void
stats(Ctlr *ctlr)
{
	u32int stat;
	u8int wcid;

	while((stat = csr32r(ctlr, TxStatFifo)) & TxqVld){
		wcid = (stat >> TxqWcidShift) & 0xff;
		/* if no ACK was requested, no feedback is available */
		if(!(stat & TxqAckreq) || wcid == 0xff){
			continue;
		}
	}
}

static void
rt2860tx(Ctlr *ctlr, u8int q)
{
	u32int hw;
	TXQ *tx;

	stats(ctlr);
	tx = &ctlr->tx[q];
	hw = csr32r(ctlr, TxDtxIdx(q));
	while(tx->n != hw){
		uchar *p = (uchar*)tx->d + Rdscsize * tx->n;
		u16int sdl0;

		if(tx->b[tx->n]){
			freeb(tx->b[tx->n]);
			tx->b[tx->n] = nil;
		}
		sdl0 = get16(p + 4 /* sdp0 */ + 2 /* sdl1 */);
		if(!(sdl0 & TxDdone)){
			print("txd ddone bit not set\n");
			break; /* should not happen */
		}
		memset((uchar*)ctlr->pool.p + TxwiDmaSz * tx->n, 0, TxwiDmaSz);
		memset((uchar*)tx->d + Tdscsize * tx->n, 0, Tdscsize);
		// put16(p + 4 /* sdp0 */ + 2 /* sdl1 */, sdl0 & ~TxDdone);
		tx->n = (tx->n + 1) % Ntx;
	}
	coherence();
}

static void
rt2860interrupt(Ureg*, void *arg)
{
	u32int r;
	Ether *edev;
	Ctlr *ctlr;
	int debug;

	edev = arg;
	ctlr = edev->ctlr;
	ilock(ctlr);

	debug = ctlr->wifi->debug;

	r = csr32r(ctlr, IntStatus);
	if(r == 0xffffffff){
		iunlock(ctlr);
		return;
	}
	if(r == 0){
		iunlock(ctlr);
		return;
	}

	/* acknowledge interrupts */
	csr32w(ctlr, IntStatus, r);

	if(r & TxRxCoherent){
		u32int tmp;
		/* DMA finds data coherent event when checking the DDONE bit */
		if(debug)
			print("txrx coherent intr\n");

		/* restart DMA engine */
		tmp = csr32r(ctlr, WpdmaGloCfg);
		tmp &= ~(TxWbDdone | RxDmaEn | TxDmaEn);
		csr32w(ctlr, WpdmaGloCfg, tmp);

		txrxon(ctlr);
	}
	if(r & MacInt2)
		stats(ctlr);
	
	if(r & TxDoneInt5)
		rt2860tx(ctlr, 5);

	if(r & RxDoneInt)
		receive(ctlr);

	if(r & TxDoneInt4)
		rt2860tx(ctlr, 4);

	if(r & TxDoneInt3)
		rt2860tx(ctlr, 3);

	if(r & TxDoneInt2)
		rt2860tx(ctlr, 2);

	if(r & TxDoneInt1)
		rt2860tx(ctlr, 1);

	if(r & TxDoneInt0)
		rt2860tx(ctlr, 0);

	iunlock(ctlr);
}

static void
eepromctl(Ctlr *ctlr, u32int val)
{
	csr32w(ctlr, PciEectrl, val);
	coherence();
	microdelay(EepromDelay);	
}

/*
 * Read 16 bits at address 'addr' from the serial EEPROM (either 93C46,
 * 93C66 or 93C86).
 */
static u16int
eeread2(Ctlr *ctlr, u16int addr)
{
	u32int tmp;
	u16int val;
	int n;

	/* clock C once before the first command */
	eepromctl(ctlr, 0);

	eepromctl(ctlr, EectrlS);
	eepromctl(ctlr, EectrlS | EectrlC);
	eepromctl(ctlr, EectrlS);

	/* write start bit (1) */
	eepromctl(ctlr, EectrlS | EectrlD);
	eepromctl(ctlr, EectrlS | EectrlD | EectrlC);

	/* write READ opcode (10) */
	eepromctl(ctlr, EectrlS | EectrlD);
	eepromctl(ctlr, EectrlS | EectrlD | EectrlC);
	eepromctl(ctlr, EectrlS);
	eepromctl(ctlr, EectrlS | EectrlC);

	/* write address (A5-A0 or A7-A0) */
	n = ((csr32r(ctlr, PciEectrl) & 0x30) == 0) ? 5 : 7;
	for(; n >= 0; n--){
		eepromctl(ctlr, EectrlS |
		    (((addr >> n) & 1) << EectrlShiftD));
		eepromctl(ctlr, EectrlS |
		    (((addr >> n) & 1) << EectrlShiftD) | EectrlC);
	}

	eepromctl(ctlr, EectrlS);

	/* read data Q15-Q0 */
	val = 0;
	for(n = 15; n >= 0; n--){
		eepromctl(ctlr, EectrlS | EectrlC);
		tmp = csr32r(ctlr, PciEectrl);
		val |= ((tmp & EectrlQ) >> EectrlShiftQ) << n;
		eepromctl(ctlr, EectrlS);
	}

	eepromctl(ctlr, 0);

	/* clear Chip Select and clock C */
	eepromctl(ctlr, EectrlS);
	eepromctl(ctlr, 0);
	eepromctl(ctlr, EectrlC);

	return val;
}

/* Read 16-bit from eFUSE ROM (>=RT3071 only.) */
static u16int
efuseread2(Ctlr *ctlr, u16int addr)
{
	u32int tmp;
	u16int reg;
	int ntries;

	addr *= 2;
	/*-
	 * Read one 16-byte block into registers EFUSE_DATA[0-3]:
	 * DATA0: F E D C
	 * DATA1: B A 9 8
	 * DATA2: 7 6 5 4
	 * DATA3: 3 2 1 0
	 */
	tmp = csr32r(ctlr, Rt3070EfuseCtrl);
	tmp &= ~(Rt3070EfsromModeMask | Rt3070EfsromAinMask);
	tmp |= (addr & ~0xf) << Rt3070EfsromAinShift | Rt3070EfsromKick;
	csr32w(ctlr, Rt3070EfuseCtrl, tmp);
	for(ntries = 0; ntries < 500; ntries++){
		tmp = csr32r(ctlr, Rt3070EfuseCtrl);
		if(!(tmp & Rt3070EfsromKick))
			break;
		microdelay(2);
	}
	if(ntries == 500)
		return 0xffff;

	if((tmp & Rt3070EfuseAoutMask) == Rt3070EfuseAoutMask)
		return 0xffff;	/* address not found */

	/* determine to which 32-bit register our 16-bit word belongs */
	reg = Rt3070EfuseData3 - (addr & 0xc);
	tmp = csr32r(ctlr, reg);

	return (addr & 2) ? tmp >> 16 : tmp & 0xffff;
}

static char*
eepromread(Ether *edev)
{
	s8int delta_2ghz, delta_5ghz;
	u32int tmp;
	u16int val;
	int ridx, ant, i;
	u16int (*rom_read)(Ctlr*, u16int);
	Ctlr *ctlr;

	enum { DefLna =	10 };

	ctlr = edev->ctlr;
	/* check whether the ROM is eFUSE ROM or EEPROM */
	rom_read = eeread2;
	if(ctlr->mac_ver >= 0x3071){
		tmp = csr32r(ctlr, Rt3070EfuseCtrl);
		if(tmp & Rt3070SelEfuse)
			rom_read = efuseread2;
	}

	/* read MAC address */
	val = rom_read(ctlr, EepromMac01);
	edev->ea[0] = val & 0xff;
	edev->ea[1] = val >> 8;
	val = rom_read(ctlr, EepromMac23);
	edev->ea[2] = val & 0xff;
	edev->ea[3] = val >> 8;
	val = rom_read(ctlr, EepromMac45);
	edev->ea[4] = val & 0xff;
	edev->ea[5] = val >> 8;

	/* read vendor BBP settings */
	for(i = 0; i < 8; i++){
		val = rom_read(ctlr, EepromBbpBase + i);
		ctlr->bbp[i].val = val & 0xff;
		ctlr->bbp[i].reg = val >> 8;
	}

	if(ctlr->mac_ver >= 0x3071){
		/* read vendor RF settings */
		for(i = 0; i < 10; i++){
			val = rom_read(ctlr, Rt3071EepromRfBase + i);
			ctlr->rf[i].val = val & 0xff;
			ctlr->rf[i].reg = val >> 8;
		}
	}

	/* read RF frequency offset from EEPROM */
	val = rom_read(ctlr, EepromFreqLeds);
	ctlr->freq = ((val & 0xff) != 0xff) ? val & 0xff : 0;
	if((val >> 8) != 0xff){
		/* read LEDs operating mode */
		ctlr->leds = val >> 8;
		ctlr->led[0] = rom_read(ctlr, EepromLed1);
		ctlr->led[1] = rom_read(ctlr, EepromLed2);
		ctlr->led[2] = rom_read(ctlr, EepromLed3);
	}else{
		/* broken EEPROM, use default settings */
		ctlr->leds = 0x01;
		ctlr->led[0] = 0x5555;
		ctlr->led[1] = 0x2221;
		ctlr->led[2] = 0xa9f8;
	}
	/* read RF information */
	val = rom_read(ctlr, EepromAntenna);
	if(val == 0xffff){
		if(ctlr->mac_ver == 0x3593){
			/* default to RF3053 3T3R */
			ctlr->rf_rev = Rf3053;
			ctlr->ntxchains = 3;
			ctlr->nrxchains = 3;
		}else if(ctlr->mac_ver >= 0x3071){
			/* default to RF3020 1T1R */
			ctlr->rf_rev = Rf3020;
			ctlr->ntxchains = 1;
			ctlr->nrxchains = 1;
		}else{
			/* default to RF2820 1T2R */
			ctlr->rf_rev = Rf2820;
			ctlr->ntxchains = 1;
			ctlr->nrxchains = 2;
		}
	}else{
		ctlr->rf_rev = (val >> 8) & 0xf;
		ctlr->ntxchains = (val >> 4) & 0xf;
		ctlr->nrxchains = val & 0xf;
	}

	/* check if RF supports automatic Tx access gain control */
	val = rom_read(ctlr, EepromConfig);
	/* check if driver should patch the DAC issue */
	if((val >> 8) != 0xff)
		ctlr->patch_dac = (val >> 15) & 1;
	if((val & 0xff) != 0xff){
		ctlr->ext_5ghz_lna = (val >> 3) & 1;
		ctlr->ext_2ghz_lna = (val >> 2) & 1;
		/* check if RF supports automatic Tx access gain control */
		ctlr->calib_2ghz = ctlr->calib_5ghz = 0; /* XXX (val >> 1) & 1 */;
		/* check if we have a hardware radio switch */
		ctlr->rfswitch = val & 1;
	}
	if(ctlr->flags & AdvancedPs){
		/* read PCIe power save level */
		val = rom_read(ctlr, EepromPciePslevel);
		if((val & 0xff) != 0xff){
			ctlr->pslevel = val & 0x3;
			val = rom_read(ctlr, EepromRev);
			if((val & 0xff80) != 0x9280)
				ctlr->pslevel = MIN(ctlr->pslevel, 1);
		}
	}

	/* read power settings for 2GHz channels */
	for(i = 0; i < 14; i += 2){
		val = rom_read(ctlr,
		    EepromPwr2ghzBase1 + i / 2);
		ctlr->txpow1[i + 0] = (s8int)(val & 0xff);
		ctlr->txpow1[i + 1] = (s8int)(val >> 8);

		val = rom_read(ctlr,
		    EepromPwr2ghzBase2 + i / 2);
		ctlr->txpow2[i + 0] = (s8int)(val & 0xff);
		ctlr->txpow2[i + 1] = (s8int)(val >> 8);
	}
	/* fix broken Tx power entries */

	for(i = 0; i < 14; i++){
		if(ctlr->txpow1[i] < 0 || ctlr->txpow1[i] > 31)
			ctlr->txpow1[i] = 5;
		if(ctlr->txpow2[i] < 0 || ctlr->txpow2[i] > 31)
			ctlr->txpow2[i] = 5;
	}
	/* read power settings for 5GHz channels */
	for(i = 0; i < 40; i += 2){
		val = rom_read(ctlr,
		    EepromPwr5ghzBase1 + i / 2);
		ctlr->txpow1[i + 14] = (s8int)(val & 0xff);
		ctlr->txpow1[i + 15] = (s8int)(val >> 8);

		val = rom_read(ctlr,
		    EepromPwr5ghzBase2 + i / 2);
		ctlr->txpow2[i + 14] = (s8int)(val & 0xff);
		ctlr->txpow2[i + 15] = (s8int)(val >> 8);
	}

	/* fix broken Tx power entries */
	for(i = 0; i < 40; i++){
		if(ctlr->txpow1[14 + i] < -7 || ctlr->txpow1[14 + i] > 15)
			ctlr->txpow1[14 + i] = 5;
		if(ctlr->txpow2[14 + i] < -7 || ctlr->txpow2[14 + i] > 15)
			ctlr->txpow2[14 + i] = 5;
	}

	/* read Tx power compensation for each Tx rate */
	val = rom_read(ctlr, EepromDeltapwr);
	delta_2ghz = delta_5ghz = 0;
	if((val & 0xff) != 0xff && (val & 0x80)){
		delta_2ghz = val & 0xf;
		if(!(val & 0x40))	/* negative number */
			delta_2ghz = -delta_2ghz;
	}
	val >>= 8;
	if((val & 0xff) != 0xff && (val & 0x80)){
		delta_5ghz = val & 0xf;
		if(!(val & 0x40))	/* negative number */
			delta_5ghz = -delta_5ghz;
	}

	for(ridx = 0; ridx < 5; ridx++){
		u32int reg;

		val = rom_read(ctlr, EepromRpwr + ridx * 2);
		reg = val;
		val = rom_read(ctlr, EepromRpwr + ridx * 2 + 1);
		reg |= (u32int)val << 16;

		ctlr->txpow20mhz[ridx] = reg;
		ctlr->txpow40mhz_2ghz[ridx] = b4inc(reg, delta_2ghz);
		ctlr->txpow40mhz_5ghz[ridx] = b4inc(reg, delta_5ghz);
	}

	/* read factory-calibrated samples for temperature compensation */
	val = rom_read(ctlr, EepromTssi12ghz);
	ctlr->tssi_2ghz[0] = val & 0xff;	/* [-4] */
	ctlr->tssi_2ghz[1] = val >> 8;	/* [-3] */
	val = rom_read(ctlr, EepromTssi22ghz);
	ctlr->tssi_2ghz[2] = val & 0xff;	/* [-2] */
	ctlr->tssi_2ghz[3] = val >> 8;	/* [-1] */
	val = rom_read(ctlr, EepromTssi32ghz);
	ctlr->tssi_2ghz[4] = val & 0xff;	/* [+0] */
	ctlr->tssi_2ghz[5] = val >> 8;	/* [+1] */
	val = rom_read(ctlr, EepromTssi42ghz);
	ctlr->tssi_2ghz[6] = val & 0xff;	/* [+2] */
	ctlr->tssi_2ghz[7] = val >> 8;	/* [+3] */
	val = rom_read(ctlr, EepromTssi52ghz);
	ctlr->tssi_2ghz[8] = val & 0xff;	/* [+4] */
	ctlr->step_2ghz = val >> 8;
	/* check that ref value is correct, otherwise disable calibration */
	if(ctlr->tssi_2ghz[4] == 0xff)
		ctlr->calib_2ghz = 0;

	val = rom_read(ctlr, EepromTssi15ghz);
	ctlr->tssi_5ghz[0] = val & 0xff;	/* [-4] */
	ctlr->tssi_5ghz[1] = val >> 8;	/* [-3] */
	val = rom_read(ctlr, EepromTssi25ghz);
	ctlr->tssi_5ghz[2] = val & 0xff;	/* [-2] */
	ctlr->tssi_5ghz[3] = val >> 8;	/* [-1] */
	val = rom_read(ctlr, EepromTssi35ghz);
	ctlr->tssi_5ghz[4] = val & 0xff;	/* [+0] */
	ctlr->tssi_5ghz[5] = val >> 8;	/* [+1] */
	val = rom_read(ctlr, EepromTssi45ghz);
	ctlr->tssi_5ghz[6] = val & 0xff;	/* [+2] */
	ctlr->tssi_5ghz[7] = val >> 8;	/* [+3] */
	val = rom_read(ctlr, EepromTssi55ghz);
	ctlr->tssi_5ghz[8] = val & 0xff;	/* [+4] */
	ctlr->step_5ghz = val >> 8;
	/* check that ref value is correct, otherwise disable calibration */
	if(ctlr->tssi_5ghz[4] == 0xff)
		ctlr->calib_5ghz = 0;

	/* read RSSI offsets and LNA gains from EEPROM */
	val = rom_read(ctlr, EepromRssi12ghz);
	ctlr->rssi_2ghz[0] = val & 0xff;	/* Ant A */
	ctlr->rssi_2ghz[1] = val >> 8;	/* Ant B */
	val = rom_read(ctlr, EepromRssi22ghz);
	if(ctlr->mac_ver >= 0x3071){
		/*
		 * On RT3090 chips (limited to 2 Rx chains), this ROM
		 * field contains the Tx mixer gain for the 2GHz band.
		 */
		if((val & 0xff) != 0xff)
			ctlr->txmixgain_2ghz = val & 0x7;
	}else
		ctlr->rssi_2ghz[2] = val & 0xff;	/* Ant C */
	ctlr->lna[2] = val >> 8;		/* channel group 2 */

	val = rom_read(ctlr, EepromRssi15ghz);
	ctlr->rssi_5ghz[0] = val & 0xff;	/* Ant A */
	ctlr->rssi_5ghz[1] = val >> 8;	/* Ant B */
	val = rom_read(ctlr, EepromRssi25ghz);
	ctlr->rssi_5ghz[2] = val & 0xff;	/* Ant C */
	ctlr->lna[3] = val >> 8;		/* channel group 3 */

	val = rom_read(ctlr, EepromLna);
	if(ctlr->mac_ver >= 0x3071)
		ctlr->lna[0] = DefLna;
	else				/* channel group 0 */
		ctlr->lna[0] = val & 0xff;
	ctlr->lna[1] = val >> 8;		/* channel group 1 */

	/* fix broken 5GHz LNA entries */
	if(ctlr->lna[2] == 0 || ctlr->lna[2] == 0xff){
		ctlr->lna[2] = ctlr->lna[1];
	}
	if(ctlr->lna[3] == 0 || ctlr->lna[3] == 0xff){
		ctlr->lna[3] = ctlr->lna[1];
	}

	/* fix broken RSSI offset entries */
	for(ant = 0; ant < 3; ant++){
		if(ctlr->rssi_2ghz[ant] < -10 || ctlr->rssi_2ghz[ant] > 10){
			ctlr->rssi_2ghz[ant] = 0;
		}
		if(ctlr->rssi_5ghz[ant] < -10 || ctlr->rssi_5ghz[ant] > 10){
			ctlr->rssi_5ghz[ant] = 0;
		}
	}

	return 0;
}

static const char *
getrfname(u8int rev)
{
	if((rev == 0) || (rev >= nelem(rfnames)))
		return "unknown";
	if(rfnames[rev][0] == '\0')
		return "unknown";
	return rfnames[rev];
}

static int
rbplant(Ctlr *ctlr, int i)
{
	Block *b;
	uchar *p;

	b = iallocb(Rbufsize + 256);
	if(b == nil)
		return -1;
	b->rp = b->wp = (uchar*)ROUND((uintptr)b->base, 256);
	memset(b->rp, 0, Rbufsize);
	ctlr->rx.b[i] = b;
	p = (uchar*)&ctlr->rx.p[i * 4]; /* sdp0 */
	memset(p, 0, Rdscsize);
	put32(p, PCIWADDR(b->rp)); 
	p = (uchar*)&ctlr->rx.p[i * 4 + 1]; /* sdl0 */
	p += 2; /* sdl1 */
	put16(p, Rbufsize);;
	
	return 0;
}

static char*
allocrx(Ctlr *ctlr, RXQ *rx)
{
	int i;

	if(rx->b == nil)
		rx->b = malloc(sizeof(Block*) * Nrx);
	if(rx->p == nil) /* Rx descriptors */
		rx->p = mallocalign(Nrx * Rdscsize, 16, 0, 0);
	if(rx->b == nil || rx->p == nil)
		return "no memory for rx ring";
	memset(rx->p, 0, Nrx * Rdscsize);
	for(i=0; i<Nrx; i++){
		if(rx->b[i] != nil){
			freeb(rx->b[i]);
			rx->b[i] = nil;
		}
		if(rbplant(ctlr, i) < 0)
			return "no memory for rx descriptors";
	}
	rx->i = 0;
	return nil;
}

static void 
freerx(Ctlr *, RXQ *rx)
{
	int i;

	for(i = 0; i < Nrx; i++){
		if(rx->b[i] != nil){
			freeb(rx->b[i]);
			rx->b[i] = nil;
		}
	}
	free(rx->b);
	free(rx->p);
	rx->p = nil;
	rx->b = nil;
	rx->i = 0;
}

static char*
alloctx(Ctlr *, TXQ *tx)
{
	if(tx->b == nil)
		tx->b = malloc(sizeof(Block*) * Ntx);
	if(tx->d == nil) /* Tx descriptors */
		tx->d = mallocalign(Ntx * Tdscsize, 16, 0, 0);
	if(tx->b == nil || tx->d == nil)
		return "no memory for tx ring";
	memset(tx->d, 0, Ntx * Tdscsize);
	memset(tx->b, 0, Ntx * sizeof(Block*));
	tx->i = 0;
	return nil;
}

static void
freetx(Ctlr *, TXQ *tx)
{
	free(tx->b);
	free(tx->d);
	tx->d = nil;
	tx->i = 0;
}

static char*
alloctxpool(Ctlr *ctlr)
{
	Pool *pool;

	pool = &ctlr->pool;
	if(pool->p == nil)
		pool->p = mallocalign(Ntxpool * TxwiDmaSz, 4096, 0, 0);
	if(pool->p == nil)
		return "no memory for pool";
	memset(pool->p, 0, Ntxpool * TxwiDmaSz);
	pool->i = 0;
	return 0;
}

static char*
initring(Ctlr *ctlr)
{
	int qid;
	char *err;

	/*
	 * Allocate Tx (4 EDCAs + HCCA + Mgt) and Rx rings.
	 */
	for(qid = 0; qid < 6; qid++){
		if((err = alloctx(ctlr, &ctlr->tx[qid])) != nil)
			goto fail1;
	}
	if((err = allocrx(ctlr, &ctlr->rx)) != nil)
		goto fail1;

	if((err = alloctxpool(ctlr)) != nil)
		goto fail2;
	/* mgmt ring is broken on RT2860C, use EDCA AC VO ring instead */

	ctlr->mgtqid = (ctlr->mac_ver == 0x2860 && ctlr->mac_rev == 0x0100) ?
	    3 : 5;

		return nil;
fail2:	freerx(ctlr, &ctlr->rx);
		return err;
fail1:	while(--qid >= 0)
			freetx(ctlr, &ctlr->tx[qid]);
		return err;
}

static int
rt2860init(Ether *edev)
{
	Ctlr *ctlr;
	int ntries;
	char *err;
	u32int tmp;

	SET(tmp);
	ctlr = edev->ctlr;
	/* wait for NIC to initialize */
	for(ntries = 0; ntries < 100; ntries++){
		tmp = csr32r(ctlr, AsicVerId);
		if(tmp != 0 && tmp != 0xffffffff)
			break;
		microdelay(10);
	}
	if(ntries == 100){
		print("timeout waiting for NIC to initialize");
		return -1;
	}
	ctlr->mac_ver = tmp >> 16;
	ctlr->mac_rev = tmp & 0xffff;

	if(ctlr->mac_ver != 0x2860){
		switch(ctlr->pdev->did){
			default:
					break;
			case RalinkRT2890:
			case RalinkRT2790:
			case RalinkRT3090:
			case AwtRT2890:
					ctlr->flags = AdvancedPs;
					break;
		}
	}
	/* retrieve RF rev. no and various other things from EEPROM */
	eepromread(edev);

	print("MAC/BBP RT%X (rev 0x%04X), RF %s (MIMO %dT%dR)\n",
	    ctlr->mac_ver, ctlr->mac_rev,
	    getrfname(ctlr->rf_rev), ctlr->ntxchains, ctlr->nrxchains);
	if((err = initring(ctlr)) != nil){
		print("error: %s", err);
		return -1;
	}

	return 0;
}

static Ctlr *rt2860head, *rt2860tail;

static void
rt2860pci(void)
{
	Pcidev *pdev;
	
	pdev = nil;
	while(pdev = pcimatch(pdev, 0, 0)){
		Ctlr *ctlr;
		void *mem;
		
		if(pdev->ccrb != 2 || pdev->ccru != 0x80)
			continue;
		if(pdev->vid != 0x1814) /* Ralink */
			continue;

		switch(pdev->did){
		default:
			continue;
		case RalinkRT2790:
		case RalinkRT3090:
			break;
		}

		pcisetbme(pdev);
		pcisetpms(pdev, 0);

		ctlr = malloc(sizeof(Ctlr));
		if(ctlr == nil){
			print("rt2860: unable to alloc Ctlr\n");
			continue;
		}
		ctlr->port = pdev->mem[0].bar & ~0x0F;
		mem = vmap(pdev->mem[0].bar & ~0x0F, pdev->mem[0].size);
		if(mem == nil){
			print("rt2860: can't map %8.8luX\n", pdev->mem[0].bar);
			free(ctlr);
			continue;
		}
		ctlr->nic = mem;
		ctlr->pdev = pdev;

		if(rt2860head != nil)
			rt2860tail->link = ctlr;
		else
			rt2860head = ctlr;
		rt2860tail = ctlr;
	}
}

static int
rt2860pnp(Ether* edev)
{
	Ctlr *ctlr;
	
	if(rt2860head == nil)
		rt2860pci();
again:
	for(ctlr = rt2860head; ctlr != nil; ctlr = ctlr->link){
		if(ctlr->active)
			continue;
		if(edev->port == 0 || edev->port == ctlr->port){
			ctlr->active = 1;
			break;
		}
	}

	if(ctlr == nil)
		return -1;

	edev->ctlr = ctlr;
	edev->port = ctlr->port;
	edev->irq = ctlr->pdev->intl;
	edev->tbdf = ctlr->pdev->tbdf;
	edev->arg = edev;
	edev->interrupt = rt2860interrupt;
	edev->attach = rt2860attach;
	edev->ifstat = rt2860ifstat;
	edev->ctl = rt2860ctl;
	edev->promiscuous = rt2860promiscuous;
	edev->multicast = rt2860multicast;
	edev->mbps = 10;

	if(rt2860init(edev) < 0){
		edev->ctlr = nil;
		goto again;
	}
	return 0;
}

void
etherrt2860link(void)
{
	addethercard("rt2860", rt2860pnp);
}
