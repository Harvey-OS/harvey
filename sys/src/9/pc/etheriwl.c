/*
 * Intel WiFi Link driver.
 *
 * Written without any documentation but Damien Bergaminis
 * OpenBSD iwn(4) driver sources. Requires intel firmware
 * to be present in /lib/firmware/iwn-* on attach.
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

enum {
	Ntxlog		= 8,
	Ntx		= 1<<Ntxlog,
	Nrxlog		= 8,
	Nrx		= 1<<Nrxlog,

	Rstatsize	= 16,
	Rbufsize	= 4*1024,
	Rdscsize	= 8,

	Tbufsize	= 4*1024,
	Tdscsize	= 128,
	Tcmdsize	= 140,
};

/* registers */
enum {
	Cfg		= 0x000,	/* config register */
		MacSi		= 1<<8,
		RadioSi		= 1<<9,
		EepromLocked	= 1<<21,
		NicReady	= 1<<22,
		HapwakeL1A	= 1<<23,
		PrepareDone	= 1<<25,
		Prepare		= 1<<27,

	Isr		= 0x008,	/* interrupt status */
	Imr		= 0x00c,	/* interrupt mask */
		Ialive		= 1<<0,
		Iwakeup		= 1<<1,
		Iswrx		= 1<<3,
		Ictreached	= 1<<6,
		Irftoggled	= 1<<7,
		Iswerr		= 1<<25,
		Isched		= 1<<26,
		Ifhtx		= 1<<27,
		Irxperiodic	= 1<<28,
		Ihwerr		= 1<<29,
		Ifhrx		= 1<<31,

		Ierr		= Iswerr | Ihwerr,
		Idefmask	= Ierr | Ifhtx | Ifhrx | Ialive | Iwakeup | Iswrx | Ictreached | Irftoggled,

	FhIsr		= 0x010,	/* second interrupt status */

	Reset		= 0x020,
		
	Rev		= 0x028,	/* hardware revision */

	EepromIo	= 0x02c,	/* EEPROM i/o register */
	EepromGp	= 0x030,
		
	OtpromGp	= 0x034,
		DevSelOtp	= 1<<16,
		RelativeAccess	= 1<<17,
		EccCorrStts	= 1<<20,
		EccUncorrStts	= 1<<21,

	Gpc		= 0x024,	/* gp cntrl */
		MacAccessEna	= 1<<0,
		MacClockReady	= 1<<0,
		InitDone	= 1<<2,
		MacAccessReq	= 1<<3,
		NicSleep	= 1<<4,
		RfKill		= 1<<27,

	Gio		= 0x03c,
		EnaL0S		= 1<<1,

	GpDrv	= 0x050,
		GpDrvCalV6	= 1<<2,
		GpDrv1X2	= 1<<3,
		GpDrvRadioIqInvert	= 1<<7, 

	Led		= 0x094,
		LedBsmCtrl	= 1<<5,
		LedOn		= 0x38,
		LedOff		= 0x78,

	UcodeGp1Clr	= 0x05c,
		UcodeGp1RfKill		= 1<<1,
		UcodeGp1CmdBlocked	= 1<<2,
		UcodeGp1CtempStopRf	= 1<<3,

	ShadowRegCtrl	= 0x0a8,

	Giochicken	= 0x100,
		L1AnoL0Srx	= 1<<23,
		DisL0Stimer	= 1<<29,

	AnaPll		= 0x20c,

	Dbghpetmem	= 0x240,
	Dbglinkpwrmgmt	= 0x250,

	MemRaddr	= 0x40c,
	MemWaddr	= 0x410,
	MemWdata	= 0x418,
	MemRdata	= 0x41c,

	PrphWaddr	= 0x444,
	PrphRaddr	= 0x448,
	PrphWdata	= 0x44c,
	PrphRdata	= 0x450,

	HbusTargWptr	= 0x460,
};

/*
 * Flow-Handler registers.
 */
enum {
	FhTfbdCtrl0	= 0x1900,	// +q*8
	FhTfbdCtrl1	= 0x1904,	// +q*8

	FhKwAddr	= 0x197c,

	FhSramAddr	= 0x19a4,	// +q*4
	FhCbbcQueue	= 0x19d0,	// +q*4
	FhStatusWptr	= 0x1bc0,
	FhRxBase	= 0x1bc4,
	FhRxWptr	= 0x1bc8,
	FhRxConfig	= 0x1c00,
		FhRxConfigEna		= 1<<31,
		FhRxConfigRbSize8K	= 1<<16,
		FhRxConfigSingleFrame	= 1<<15,
		FhRxConfigIrqDstHost	= 1<<12,
		FhRxConfigIgnRxfEmpty	= 1<<2,

		FhRxConfigNrbdShift	= 20,
		FhRxConfigRbTimeoutShift= 4,

	FhRxStatus	= 0x1c44,

	FhTxConfig	= 0x1d00,	// +q*32
		FhTxConfigDmaCreditEna	= 1<<3,
		FhTxConfigDmaEna	= 1<<31,
		FhTxConfigCirqHostEndTfd= 1<<20,

	FhTxBufStatus	= 0x1d08,	// +q*32
		FhTxBufStatusTbNumShift	= 20,
		FhTxBufStatusTbIdxShift = 12,
		FhTxBufStatusTfbdValid	= 3,

	FhTxChicken	= 0x1e98,
	FhTxStatus	= 0x1eb0,
};

/*
 * NIC internal memory offsets.
 */
enum {
	ApmgClkCtrl	= 0x3000,
	ApmgClkEna	= 0x3004,
	ApmgClkDis	= 0x3008,
		DmaClkRqt	= 1<<9,
		BsmClkRqt	= 1<<11,

	ApmgPs		= 0x300c,
		EarlyPwroffDis	= 1<<22,
		PwrSrcVMain	= 0<<24,
		PwrSrcVAux	= 2<<24,
		PwrSrcMask	= 3<<24,
		ResetReq	= 1<<26,

	ApmgDigitalSvr	= 0x3058,
	ApmgAnalogSvr	= 0x306c,
	ApmgPciStt	= 0x3010,
	BsmWrCtrl	= 0x3400,
	BsmWrMemSrc	= 0x3404,
	BsmWrMemDst	= 0x3408,
	BsmWrDwCount	= 0x340c,
	BsmDramTextAddr	= 0x3490,
	BsmDramTextSize	= 0x3494,
	BsmDramDataAddr	= 0x3498,
	BsmDramDataSize	= 0x349c,
	BsmSramBase	= 0x3800,
};

/*
 * TX scheduler registers.
 */
enum {
	SchedBase		= 0xa02c00,
	SchedSramAddr		= SchedBase,

	SchedDramAddr4965	= SchedBase+0x010,
	SchedTxFact4965		= SchedBase+0x01c,
	SchedQueueRdptr4965	= SchedBase+0x064,	// +q*4
	SchedQChainSel4965	= SchedBase+0x0d0,
	SchedIntrMask4965	= SchedBase+0x0e4,
	SchedQueueStatus4965	= SchedBase+0x104,	// +q*4

	SchedDramAddr5000	= SchedBase+0x008,
	SchedTxFact5000		= SchedBase+0x010,
	SchedQueueRdptr5000	= SchedBase+0x068,	// +q*4
	SchedQChainSel5000	= SchedBase+0x0e8,
	SchedIntrMask5000	= SchedBase+0x108,
	SchedQueueStatus5000	= SchedBase+0x10c,	// +q*4
	SchedAggrSel5000	= SchedBase+0x248,
};

enum {
	SchedCtxOff4965		= 0x380,
	SchedCtxLen4965		= 416,

	SchedCtxOff5000		= 0x600,
	SchedCtxLen5000		= 512,
};

enum {
	FilterPromisc		= 1<<0,
	FilterCtl		= 1<<1,
	FilterMulticast		= 1<<2,
	FilterNoDecrypt		= 1<<3,
	FilterBSS		= 1<<5,
	FilterBeacon		= 1<<6,
};

enum {
	RFlag24Ghz		= 1<<0,
	RFlagCCK		= 1<<1,
	RFlagAuto		= 1<<2,
	RFlagShSlot		= 1<<4,
	RFlagShPreamble		= 1<<5,
	RFlagNoDiversity	= 1<<7,
	RFlagAntennaA		= 1<<8,
	RFlagAntennaB		= 1<<9,
	RFlagTSF		= 1<<15,
	RFlagCTSToSelf		= 1<<30,
};

typedef struct FWInfo FWInfo;
typedef struct FWImage FWImage;
typedef struct FWSect FWSect;

typedef struct TXQ TXQ;
typedef struct RXQ RXQ;

typedef struct Ctlr Ctlr;

struct FWSect
{
	uchar	*data;
	uint	size;
};

struct FWImage
{
	struct {
		FWSect	text;
		FWSect	data;
	} init, main, boot;

	uint	rev;
	uint	build;
	char	descr[64+1];
	uchar	data[];
};

struct FWInfo
{
	uchar	major;
	uchar	minjor;
	uchar	type;
	uchar	subtype;

	u32int	logptr;
	u32int	errptr;
	u32int	tstamp;
	u32int	valid;
};

struct TXQ
{
	uint	n;
	uint	i;
	Block	**b;
	uchar	*d;
	uchar	*c;

	uint	lastcmd;

	Rendez;
	QLock;
};

struct RXQ
{
	uint	i;
	Block	**b;
	u32int	*p;
	uchar	*s;
};

struct Ctlr {
	Lock;
	QLock;

	Ctlr *link;
	Pcidev *pdev;
	Wifi *wifi;

	int type;
	int port;
	int power;
	int active;
	int broken;
	int attached;

	u32int ie;

	u32int *nic;
	uchar *kwpage;

	/* assigned node ids in hardware node table or -1 if unassigned */
	int bcastnodeid;
	int bssnodeid;

	/* current receiver settings */
	uchar bssid[Eaddrlen];
	int channel;
	int prom;
	int aid;

	RXQ rx;
	TXQ tx[20];

	struct {
		Rendez;
		u32int	m;
		u32int	w;
	} wait;

	struct {
		uchar	type;
		uchar	step;
		uchar	dash;
		uchar	txantmask;
		uchar	rxantmask;
	} rfcfg;

	struct {
		int	otp;
		uint	off;

		uchar	version;
		uchar	type;
		u16int	volt;
		u16int	temp;
		u16int	rawtemp;

		char	regdom[4+1];

		u32int	crystal;
	} eeprom;

	struct {
		Block	*cmd[21];
		int	done;
	} calib;

	struct {
		u32int	base;
		uchar	*s;
	} sched;

	FWInfo fwinfo;
	FWImage *fw;
};

/* controller types */
enum {
	Type4965	= 0,
	Type5300	= 2,
	Type5350	= 3,
	Type5150	= 4,
	Type5100	= 5,
	Type1000	= 6,
	Type6000	= 7,
	Type6050	= 8,
	Type6005	= 11,	/* also Centrino Advanced-N 6030, 6235 */
	Type2030	= 12,
};

static char *fwname[32] = {
	[Type4965] "iwn-4965",
	[Type5300] "iwn-5000",
	[Type5350] "iwn-5000",
	[Type5150] "iwn-5150",
	[Type5100] "iwn-5000",
	[Type1000] "iwn-1000",
	[Type6000] "iwn-6000",
	[Type6050] "iwn-6050",
	[Type6005] "iwn-6005", /* see in iwlattach() below */
	[Type2030] "iwn-2030",
};

static char *qcmd(Ctlr *ctlr, uint qid, uint code, uchar *data, int size, Block *block);
static char *flushq(Ctlr *ctlr, uint qid);
static char *cmd(Ctlr *ctlr, uint code, uchar *data, int size);

#define csr32r(c, r)	(*((c)->nic+((r)/4)))
#define csr32w(c, r, v)	(*((c)->nic+((r)/4)) = (v))

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

static char*
niclock(Ctlr *ctlr)
{
	int i;

	csr32w(ctlr, Gpc, csr32r(ctlr, Gpc) | MacAccessReq);
	for(i=0; i<1000; i++){
		if((csr32r(ctlr, Gpc) & (NicSleep | MacAccessEna)) == MacAccessEna)
			return 0;
		delay(10);
	}
	return "niclock: timeout";
}

static void
nicunlock(Ctlr *ctlr)
{
	csr32w(ctlr, Gpc, csr32r(ctlr, Gpc) & ~MacAccessReq);
}

static u32int
prphread(Ctlr *ctlr, uint off)
{
	csr32w(ctlr, PrphRaddr, ((sizeof(u32int)-1)<<24) | off);
	coherence();
	return csr32r(ctlr, PrphRdata);
}
static void
prphwrite(Ctlr *ctlr, uint off, u32int data)
{
	csr32w(ctlr, PrphWaddr, ((sizeof(u32int)-1)<<24) | off);
	coherence();
	csr32w(ctlr, PrphWdata, data);
}

static u32int
memread(Ctlr *ctlr, uint off)
{
	csr32w(ctlr, MemRaddr, off);
	coherence();
	return csr32r(ctlr, MemRdata);
}
static void
memwrite(Ctlr *ctlr, uint off, u32int data)
{
	csr32w(ctlr, MemWaddr, off);
	coherence();
	csr32w(ctlr, MemWdata, data);
}

static void
setfwinfo(Ctlr *ctlr, uchar *d, int len)
{
	FWInfo *i;

	if(len < 32)
		return;
	i = &ctlr->fwinfo;
	i->minjor = *d++;
	i->major = *d++;
	d += 2+8;
	i->type = *d++;
	i->subtype = *d++;
	d += 2;
	i->logptr = get32(d); d += 4;
	i->errptr = get32(d); d += 4;
	i->tstamp = get32(d); d += 4;
	i->valid = get32(d);
};

static void
dumpctlr(Ctlr *ctlr)
{
	u32int dump[13];
	int i;

	print("lastcmd: %ud (0x%ux)\n", ctlr->tx[4].lastcmd,  ctlr->tx[4].lastcmd);
	if(ctlr->fwinfo.errptr == 0){
		print("no error pointer\n");
		return;
	}
	for(i=0; i<nelem(dump); i++)
		dump[i] = memread(ctlr, ctlr->fwinfo.errptr + i*4);
	print(	"error:\tid %ux, pc %ux,\n"
		"\tbranchlink %.8ux %.8ux, interruptlink %.8ux %.8ux,\n"
		"\terrordata %.8ux %.8ux, srcline %ud, tsf %ux, time %ux\n",
		dump[1], dump[2],
		dump[4], dump[3], dump[6], dump[5],
		dump[7], dump[8], dump[9], dump[10], dump[11]);
}

static char*
eepromlock(Ctlr *ctlr)
{
	int i, j;

	for(i=0; i<100; i++){
		csr32w(ctlr, Cfg, csr32r(ctlr, Cfg) | EepromLocked);
		for(j=0; j<100; j++){
			if(csr32r(ctlr, Cfg) & EepromLocked)
				return 0;
			delay(10);
		}
	}
	return "eepromlock: timeout";
}
static void
eepromunlock(Ctlr *ctlr)
{
	csr32w(ctlr, Cfg, csr32r(ctlr, Cfg) & ~EepromLocked);
}
static char*
eepromread(Ctlr *ctlr, void *data, int count, uint off)
{
	uchar *out = data;
	u32int w, s;
	int i;

	w = 0;
	off += ctlr->eeprom.off;
	for(; count > 0; count -= 2, off++){
		csr32w(ctlr, EepromIo, off << 2);
		for(i=0; i<10; i++){
			w = csr32r(ctlr, EepromIo);
			if(w & 1)
				break;
			delay(5);
		}
		if(i == 10)
			return "eepromread: timeout";
		if(ctlr->eeprom.otp){
			s = csr32r(ctlr, OtpromGp);
			if(s & EccUncorrStts)
				return "eepromread: otprom ecc error";
			if(s & EccCorrStts)
				csr32w(ctlr, OtpromGp, s);
		}
		*out++ = w >> 16;
		if(count > 1)
			*out++ = w >> 24;
	}
	return 0;
}

static char*
handover(Ctlr *ctlr)
{
	int i;

	csr32w(ctlr, Cfg, csr32r(ctlr, Cfg) | NicReady);
	for(i=0; i<5; i++){
		if(csr32r(ctlr, Cfg) & NicReady)
			return 0;
		delay(10);
	}
	csr32w(ctlr, Cfg, csr32r(ctlr, Cfg) | Prepare);
	for(i=0; i<15000; i++){
		if((csr32r(ctlr, Cfg) & PrepareDone) == 0)
			break;
		delay(10);
	}
	if(i >= 15000)
		return "handover: timeout";
	csr32w(ctlr, Cfg, csr32r(ctlr, Cfg) | NicReady);
	for(i=0; i<5; i++){
		if(csr32r(ctlr, Cfg) & NicReady)
			return 0;
		delay(10);
	}
	return "handover: timeout";
}

static char*
clockwait(Ctlr *ctlr)
{
	int i;

	/* Set "initialization complete" bit. */
	csr32w(ctlr, Gpc, csr32r(ctlr, Gpc) | InitDone);
	for(i=0; i<2500; i++){
		if(csr32r(ctlr, Gpc) & MacClockReady)
			return 0;
		delay(10);
	}
	return "clockwait: timeout";
}

static char*
poweron(Ctlr *ctlr)
{
	int capoff;
	char *err;

	/* Disable L0s exit timer (NMI bug workaround). */
	csr32w(ctlr, Giochicken, csr32r(ctlr, Giochicken) | DisL0Stimer);

	/* Don't wait for ICH L0s (ICH bug workaround). */
	csr32w(ctlr, Giochicken, csr32r(ctlr, Giochicken) | L1AnoL0Srx);

	/* Set FH wait threshold to max (HW bug under stress workaround). */
	csr32w(ctlr, Dbghpetmem, csr32r(ctlr, Dbghpetmem) | 0xffff0000);

	/* Enable HAP INTA to move adapter from L1a to L0s. */
	csr32w(ctlr, Cfg, csr32r(ctlr, Cfg) | HapwakeL1A);

	capoff = pcicap(ctlr->pdev, PciCapPCIe);
	if(capoff != -1){
		/* Workaround for HW instability in PCIe L0->L0s->L1 transition. */
		if(pcicfgr16(ctlr->pdev, capoff + 0x10) & 0x2)	/* LCSR -> L1 Entry enabled. */
			csr32w(ctlr, Gio, csr32r(ctlr, Gio) | EnaL0S);
		else
			csr32w(ctlr, Gio, csr32r(ctlr, Gio) & ~EnaL0S);
	}

	if(ctlr->type != Type4965 && ctlr->type <= Type1000)
		csr32w(ctlr, AnaPll, csr32r(ctlr, AnaPll) | 0x00880300);

	/* Wait for clock stabilization before accessing prph. */
	if((err = clockwait(ctlr)) != nil)
		return err;

	if((err = niclock(ctlr)) != nil)
		return err;

	/* Enable DMA and BSM (Bootstrap State Machine). */
	if(ctlr->type == Type4965)
		prphwrite(ctlr, ApmgClkEna, DmaClkRqt | BsmClkRqt);
	else
		prphwrite(ctlr, ApmgClkEna, DmaClkRqt);
	delay(20);

	/* Disable L1-Active. */
	prphwrite(ctlr, ApmgPciStt, prphread(ctlr, ApmgPciStt) | (1<<11));

	nicunlock(ctlr);

	ctlr->power = 1;

	return 0;
}

static void
poweroff(Ctlr *ctlr)
{
	int i, j;

	csr32w(ctlr, Reset, 1);

	/* Disable interrupts */
	ctlr->ie = 0;
	csr32w(ctlr, Imr, 0);
	csr32w(ctlr, Isr, ~0);
	csr32w(ctlr, FhIsr, ~0);

	/* Stop scheduler */
	if(ctlr->type != Type4965)
		prphwrite(ctlr, SchedTxFact5000, 0);
	else
		prphwrite(ctlr, SchedTxFact4965, 0);

	/* Stop TX ring */
	if(niclock(ctlr) == nil){
		for(i = (ctlr->type != Type4965) ? 7 : 6; i >= 0; i--){
			csr32w(ctlr, FhTxConfig + i*32, 0);
			for(j = 0; j < 200; j++){
				if(csr32r(ctlr, FhTxStatus) & (0x10000<<i))
					break;
				delay(10);
			}
		}
		nicunlock(ctlr);
	}

	/* Stop RX ring */
	if(niclock(ctlr) == nil){
		csr32w(ctlr, FhRxConfig, 0);
		for(j = 0; j < 200; j++){
			if(csr32r(ctlr, FhRxStatus) & 0x1000000)
				break;
			delay(10);
		}
		nicunlock(ctlr);
	}

	/* Disable DMA */
	if(niclock(ctlr) == nil){
		prphwrite(ctlr, ApmgClkDis, DmaClkRqt);
		nicunlock(ctlr);
	}
	delay(5);

	/* Stop busmaster DMA activity. */
	csr32w(ctlr, Reset, csr32r(ctlr, Reset) | (1<<9));
	for(j = 0; j < 100; j++){
		if(csr32r(ctlr, Reset) & (1<<8))
			break;
		delay(10);
	}

	/* Reset the entire device. */
	csr32w(ctlr, Reset, csr32r(ctlr, Reset) | (1<<7));
	delay(10);

	/* Clear "initialization complete" bit. */
	csr32w(ctlr, Gpc, csr32r(ctlr, Gpc) & ~InitDone);

	ctlr->power = 0;
}

static char*
rominit(Ctlr *ctlr)
{
	uint prev, last;
	uchar buf[2];
	char *err;
	int i;

	ctlr->eeprom.otp = 0;
	ctlr->eeprom.off = 0;
	if(ctlr->type < Type1000 || (csr32r(ctlr, OtpromGp) & DevSelOtp) == 0)
		return nil;

	/* Wait for clock stabilization before accessing prph. */
	if((err = clockwait(ctlr)) != nil)
		return err;

	if((err = niclock(ctlr)) != nil)
		return err;
	prphwrite(ctlr, ApmgPs, prphread(ctlr, ApmgPs) | ResetReq);
	delay(5);
	prphwrite(ctlr, ApmgPs, prphread(ctlr, ApmgPs) & ~ResetReq);
	nicunlock(ctlr);

	/* Set auto clock gate disable bit for HW with OTP shadow RAM. */
	if(ctlr->type != Type1000)
		csr32w(ctlr, Dbglinkpwrmgmt, csr32r(ctlr, Dbglinkpwrmgmt) | (1<<31));

	csr32w(ctlr, EepromGp, csr32r(ctlr, EepromGp) & ~0x00000180);

	/* Clear ECC status. */
	csr32w(ctlr, OtpromGp, csr32r(ctlr, OtpromGp) | (EccCorrStts | EccUncorrStts));

	ctlr->eeprom.otp = 1;
	if(ctlr->type != Type1000)
		return nil;

	/* Switch to absolute addressing mode. */
	csr32w(ctlr, OtpromGp, csr32r(ctlr, OtpromGp) & ~RelativeAccess);

	/*
	 * Find the block before last block (contains the EEPROM image)
	 * for HW without OTP shadow RAM.
	 */
	prev = last = 0;
	for(i=0; i<3; i++){
		if((err = eepromread(ctlr, buf, 2, last)) != nil)
			return err;
		if(get16(buf) == 0)
			break;
		prev = last;
		last = get16(buf);
	}
	if(i == 0 || i >= 3)
		return "rominit: missing eeprom image";

	ctlr->eeprom.off = prev+1;
	return nil;
}

static int
iwlinit(Ether *edev)
{
	Ctlr *ctlr;
	char *err;
	uchar b[4];
	uint u, caloff, regoff;

	ctlr = edev->ctlr;
	if((err = handover(ctlr)) != nil)
		goto Err;
	if((err = poweron(ctlr)) != nil)
		goto Err;
	if((csr32r(ctlr, EepromGp) & 0x7) == 0){
		err = "bad rom signature";
		goto Err;
	}
	if((err = eepromlock(ctlr)) != nil)
		goto Err;
	if((err = rominit(ctlr)) != nil)
		goto Err2;
	if((err = eepromread(ctlr, edev->ea, sizeof(edev->ea), 0x15)) != nil){
		eepromunlock(ctlr);
		goto Err;
	}
	if((err = eepromread(ctlr, b, 2, 0x048)) != nil){
	Err2:
		eepromunlock(ctlr);
		goto Err;
	}
	u = get16(b);
	ctlr->rfcfg.type = u & 3;	u >>= 2;
	ctlr->rfcfg.step = u & 3;	u >>= 2;
	ctlr->rfcfg.dash = u & 3;	u >>= 4;
	ctlr->rfcfg.txantmask = u & 15;	u >>= 4;
	ctlr->rfcfg.rxantmask = u & 15;
	if((err = eepromread(ctlr, b, 2, 0x66)) != nil)
		goto Err2;
	regoff = get16(b);
	if((err = eepromread(ctlr, b, 4, regoff+1)) != nil)
		goto Err2;
	strncpy(ctlr->eeprom.regdom, (char*)b, 4);
	ctlr->eeprom.regdom[4] = 0;
	if((err = eepromread(ctlr, b, 2, 0x67)) != nil)
		goto Err2;
	caloff = get16(b);
	if((err = eepromread(ctlr, b, 4, caloff)) != nil)
		goto Err2;
	ctlr->eeprom.version = b[0];
	ctlr->eeprom.type = b[1];
	ctlr->eeprom.volt = get16(b+2);

	ctlr->eeprom.temp = 0;
	ctlr->eeprom.rawtemp = 0;
	if(ctlr->type == Type2030){
		if((err = eepromread(ctlr, b, 2, caloff + 0x12a)) != nil)
			goto Err2;
		ctlr->eeprom.temp = get16(b);
		if((err = eepromread(ctlr, b, 2, caloff + 0x12b)) != nil)
			goto Err2;
		ctlr->eeprom.rawtemp = get16(b);
	}

	if(ctlr->type != Type4965 && ctlr->type != Type5150){
		if((err = eepromread(ctlr, b, 4, caloff + 0x128)) != nil)
			goto Err2;
		ctlr->eeprom.crystal = get32(b);
	}
	eepromunlock(ctlr);

	switch(ctlr->type){
	case Type4965:
		ctlr->rfcfg.txantmask = 3;
		ctlr->rfcfg.rxantmask = 7;
		break;
	case Type5100:
		ctlr->rfcfg.txantmask = 2;
		ctlr->rfcfg.rxantmask = 3;
		break;
	case Type6000:
		if(ctlr->pdev->did == 0x422c || ctlr->pdev->did == 0x4230){
			ctlr->rfcfg.txantmask = 6;
			ctlr->rfcfg.rxantmask = 6;
		}
		break;
	}
	poweroff(ctlr);
	return 0;
Err:
	print("iwlinit: %s\n", err);
	poweroff(ctlr);
	return -1;
}

static char*
crackfw(FWImage *i, uchar *data, uint size, int alt)
{
	uchar *p, *e;
	FWSect *s;

	memset(i, 0, sizeof(*i));
	if(size < 4){
Tooshort:
		return "firmware image too short";
	}
	p = data;
	e = p + size;
	i->rev = get32(p); p += 4;
	if(i->rev == 0){
		uvlong altmask;

		if(size < (4+64+4+4+8))
			goto Tooshort;
		if(memcmp(p, "IWL\n", 4) != 0)
			return "bad firmware signature";
		p += 4;
		strncpy(i->descr, (char*)p, 64);
		i->descr[64] = 0;
		p += 64;
		i->rev = get32(p); p += 4;
		i->build = get32(p); p += 4;
		altmask = get32(p); p += 4;
		altmask |= (uvlong)get32(p) << 32; p += 4;
		while(alt > 0 && (altmask & (1ULL<<alt)) == 0)
			alt--;
		while(p < e){
			FWSect dummy;

			if((p + 2+2+4) > e)
				goto Tooshort;
			switch(get16(p)){
			case 1:	s = &i->main.text; break;
			case 2: s = &i->main.data; break;
			case 3: s = &i->init.text; break;
			case 4: s = &i->init.data; break;
			case 5: s = &i->boot.text; break;
			default:s = &dummy;
			}
			p += 2;
			if(get16(p) != 0 && get16(p) != alt)
				s = &dummy;
			p += 2;
			s->size = get32(p); p += 4;
			s->data = p;
			if((p + s->size) > e)
				goto Tooshort;
			p += (s->size + 3) & ~3;
		}
	} else {
		if(((i->rev>>8) & 0xFF) < 2)
			return "need firmware api >= 2";
		if(((i->rev>>8) & 0xFF) >= 3){
			i->build = get32(p); p += 4;
		}
		if((p + 5*4) > e)
			goto Tooshort;
		i->main.text.size = get32(p); p += 4;
		i->main.data.size = get32(p); p += 4;
		i->init.text.size = get32(p); p += 4;
		i->init.data.size = get32(p); p += 4;
		i->boot.text.size = get32(p); p += 4;
		i->main.text.data = p; p += i->main.text.size;
		i->main.data.data = p; p += i->main.data.size;
		i->init.text.data = p; p += i->init.text.size;
		i->init.data.data = p; p += i->init.data.size;
		i->boot.text.data = p; p += i->boot.text.size;
		if(p > e)
			goto Tooshort;
	}
	return 0;
}

static FWImage*
readfirmware(char *name)
{
	uchar dirbuf[sizeof(Dir)+100], *data;
	char buf[128], *err;
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
	} else {
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
	fw = smalloc(sizeof(*fw) + 16 + d.length);
	data = (uchar*)(fw+1);
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
	if((err = crackfw(fw, data, r, 1)) != nil)
		error(err);
	poperror();
	poperror();
	cclose(c);
	return fw;
}


static int
gotirq(void *arg)
{
	Ctlr *ctlr = arg;
	return (ctlr->wait.m & ctlr->wait.w) != 0;
}

static u32int
irqwait(Ctlr *ctlr, u32int mask, int timeout)
{
	u32int r;

	ilock(ctlr);
	r = ctlr->wait.m & mask;
	if(r == 0){
		ctlr->wait.w = mask;
		iunlock(ctlr);
		if(!waserror()){
			tsleep(&ctlr->wait, gotirq, ctlr, timeout);
			poperror();
		}
		ilock(ctlr);
		ctlr->wait.w = 0;
		r = ctlr->wait.m & mask;
	}
	ctlr->wait.m &= ~r;
	iunlock(ctlr);
	return r;
}

static int
rbplant(Ctlr *ctlr, int i)
{
	Block *b;

	b = iallocb(Rbufsize + 256);
	if(b == nil)
		return -1;
	b->rp = b->wp = (uchar*)ROUND((uintptr)b->base, 256);
	memset(b->rp, 0, Rdscsize);
	ctlr->rx.b[i] = b;
	ctlr->rx.p[i] = PCIWADDR(b->rp) >> 8;
	return 0;
}

static char*
initring(Ctlr *ctlr)
{
	RXQ *rx;
	TXQ *tx;
	int i, q;

	rx = &ctlr->rx;
	if(rx->b == nil)
		rx->b = malloc(sizeof(Block*) * Nrx);
	if(rx->p == nil)
		rx->p = mallocalign(sizeof(u32int) * Nrx, 256, 0, 0);
	if(rx->s == nil)
		rx->s = mallocalign(Rstatsize, 16, 0, 0);
	if(rx->b == nil || rx->p == nil || rx->s == nil)
		return "no memory for rx ring";
	memset(ctlr->rx.s, 0, Rstatsize);
	for(i=0; i<Nrx; i++){
		rx->p[i] = 0;
		if(rx->b[i] != nil){
			freeb(rx->b[i]);
			rx->b[i] = nil;
		}
		if(rbplant(ctlr, i) < 0)
			return "no memory for rx descriptors";
	}
	rx->i = 0;

	if(ctlr->sched.s == nil)
		ctlr->sched.s = mallocalign(512 * nelem(ctlr->tx) * 2, 1024, 0, 0);
	if(ctlr->sched.s == nil)
		return "no memory for sched buffer";
	memset(ctlr->sched.s, 0, 512 * nelem(ctlr->tx));

	for(q=0; q<nelem(ctlr->tx); q++){
		tx = &ctlr->tx[q];
		if(tx->b == nil)
			tx->b = malloc(sizeof(Block*) * Ntx);
		if(tx->d == nil)
			tx->d = mallocalign(Tdscsize * Ntx, 256, 0, 0);
		if(tx->c == nil)
			tx->c = mallocalign(Tcmdsize * Ntx, 4, 0, 0);
		if(tx->b == nil || tx->d == nil || tx->c == nil)
			return "no memory for tx ring";
		memset(tx->d, 0, Tdscsize * Ntx);
		memset(tx->c, 0, Tcmdsize * Ntx);
		for(i=0; i<Ntx; i++){
			if(tx->b[i] != nil){
				freeb(tx->b[i]);
				tx->b[i] = nil;
			}
		}
		tx->i = 0;
		tx->n = 0;
		tx->lastcmd = 0;
	}

	if(ctlr->kwpage == nil)
		ctlr->kwpage = mallocalign(4096, 4096, 0, 0);
	if(ctlr->kwpage == nil)
		return "no memory for kwpage";		
	memset(ctlr->kwpage, 0, 4096);

	return nil;
}

static char*
reset(Ctlr *ctlr)
{
	char *err;
	int i, q;

	if(ctlr->power)
		poweroff(ctlr);
	if((err = initring(ctlr)) != nil)
		return err;
	if((err = poweron(ctlr)) != nil)
		return err;

	if((err = niclock(ctlr)) != nil)
		return err;
	prphwrite(ctlr, ApmgPs, (prphread(ctlr, ApmgPs) & ~PwrSrcMask) | PwrSrcVMain);
	nicunlock(ctlr);

	csr32w(ctlr, Cfg, csr32r(ctlr, Cfg) | RadioSi | MacSi);

	if((err = niclock(ctlr)) != nil)
		return err;
	if(ctlr->type != Type4965)
		prphwrite(ctlr, ApmgPs, prphread(ctlr, ApmgPs) | EarlyPwroffDis);
	if(ctlr->type == Type1000){
		/*
		 * Select first Switching Voltage Regulator (1.32V) to
		 * solve a stability issue related to noisy DC2DC line
		 * in the silicon of 1000 Series.
		 */
		prphwrite(ctlr, ApmgDigitalSvr, 
			(prphread(ctlr, ApmgDigitalSvr) & ~(0xf<<5)) | (3<<5));
	}
	nicunlock(ctlr);

	if((err = niclock(ctlr)) != nil)
		return err;
	if((ctlr->type == Type6005 || ctlr->type == Type6050) && ctlr->eeprom.version == 6)
		csr32w(ctlr, GpDrv, csr32r(ctlr, GpDrv) | GpDrvCalV6);
	if(ctlr->type == Type6005)
		csr32w(ctlr, GpDrv, csr32r(ctlr, GpDrv) | GpDrv1X2);
	if(ctlr->type == Type2030)
		csr32w(ctlr, GpDrv, csr32r(ctlr, GpDrv) | GpDrvRadioIqInvert);
	nicunlock(ctlr);

	if((err = niclock(ctlr)) != nil)
		return err;
	csr32w(ctlr, FhRxConfig, 0);
	csr32w(ctlr, FhRxWptr, 0);
	csr32w(ctlr, FhRxBase, PCIWADDR(ctlr->rx.p) >> 8);
	csr32w(ctlr, FhStatusWptr, PCIWADDR(ctlr->rx.s) >> 4);
	csr32w(ctlr, FhRxConfig,
		FhRxConfigEna | 
		FhRxConfigIgnRxfEmpty |
		FhRxConfigIrqDstHost | 
		FhRxConfigSingleFrame |
		(Nrxlog << FhRxConfigNrbdShift));
	csr32w(ctlr, FhRxWptr, (Nrx-1) & ~7);
	nicunlock(ctlr);

	if((err = niclock(ctlr)) != nil)
		return err;
	if(ctlr->type != Type4965)
		prphwrite(ctlr, SchedTxFact5000, 0);
	else
		prphwrite(ctlr, SchedTxFact4965, 0);
	csr32w(ctlr, FhKwAddr, PCIWADDR(ctlr->kwpage) >> 4);
	for(q = (ctlr->type != Type4965) ? 19 : 15; q >= 0; q--)
		csr32w(ctlr, FhCbbcQueue + q*4, PCIWADDR(ctlr->tx[q].d) >> 8);
	nicunlock(ctlr);

	for(i = (ctlr->type != Type4965) ? 7 : 6; i >= 0; i--)
		csr32w(ctlr, FhTxConfig + i*32, FhTxConfigDmaEna | FhTxConfigDmaCreditEna);

	csr32w(ctlr, UcodeGp1Clr, UcodeGp1RfKill);
	csr32w(ctlr, UcodeGp1Clr, UcodeGp1CmdBlocked);

	ctlr->broken = 0;
	ctlr->wait.m = 0;
	ctlr->wait.w = 0;

	ctlr->ie = Idefmask;
	csr32w(ctlr, Imr, ctlr->ie);
	csr32w(ctlr, Isr, ~0);

	if(ctlr->type >= Type6000)
		csr32w(ctlr, ShadowRegCtrl, csr32r(ctlr, ShadowRegCtrl) | 0x800fffff);

	return nil;
}

static char*
sendbtcoexadv(Ctlr *ctlr)
{
	static u32int btcoex3wire[12] = {
		0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa,
		0xcc00ff28,	0x0000aaaa, 0xcc00aaaa, 0x0000aaaa,
		0xc0004000, 0x00004000, 0xf0005000, 0xf0005000,
	};

	uchar c[Tcmdsize], *p;
	char *err;
	int i;

	/* set BT config */
	memset(c, 0, sizeof(c));
	p = c;

	if(ctlr->type == Type2030){
		*p++ = 145; /* flags */
		p++; /* lead time */
		*p++ = 5; /* max kill */
		*p++ = 1; /* bt3 t7 timer */
		put32(p, 0xffff0000); /* kill ack */
		p += 4;
		put32(p, 0xffff0000); /* kill cts */
		p += 4;
		*p++ = 2; /* sample time */
		*p++ = 0xc; /* bt3 t2 timer */
		p += 2; /* bt4 reaction */
		for (i = 0; i < nelem(btcoex3wire); i++){
			put32(p, btcoex3wire[i]);
			p += 4;
		}
		p += 2; /* bt4 decision */
		put16(p, 0xff); /* valid */
		p += 2;
		put32(p, 0xf0); /* prio boost */
		p += 4;
		p++; /* reserved */
		p++; /* tx prio boost */
		p += 2; /* rx prio boost */
	}
	if((err = cmd(ctlr, 155, c, p-c)) != nil)
		return err;

	/* set BT priority */
	memset(c, 0, sizeof(c));
	p = c;

	*p++ = 0x6; /* init1 */
	*p++ = 0x7; /* init2 */
	*p++ = 0x2; /* periodic low1 */
	*p++ = 0x3; /* periodic low2 */
	*p++ = 0x4; /* periodic high1 */
	*p++ = 0x5; /* periodic high2 */
	*p++ = 0x6; /* dtim */
	*p++ = 0x8; /* scan52 */
	*p++ = 0xa; /* scan24 */
	p += 7; /* reserved */
	if((err = cmd(ctlr, 204, c, p-c)) != nil)
		return err;

	/* force BT state machine change */
	memset(c, 0, sizeof(c));
	p = c;

	*p++ = 1; /* open */
	*p++ = 1; /* type */
	p += 2; /* reserved */
	if((err = cmd(ctlr, 205, c, p-c)) != nil)
		return err;

	c[0] = 0; /* open */
	return cmd(ctlr, 205, c, p-c);
}

static char*
postboot(Ctlr *ctlr)
{
	uint ctxoff, ctxlen, dramaddr;
	char *err;
	int i, q;

	if((err = niclock(ctlr)) != nil)
		return err;

	if(ctlr->type != Type4965){
		dramaddr = SchedDramAddr5000;
		ctxoff = SchedCtxOff5000;
		ctxlen = SchedCtxLen5000;
	} else {
		dramaddr = SchedDramAddr4965;
		ctxoff = SchedCtxOff4965;
		ctxlen = SchedCtxLen4965;
	}

	ctlr->sched.base = prphread(ctlr, SchedSramAddr);
	for(i=0; i < ctxlen; i += 4)
		memwrite(ctlr, ctlr->sched.base + ctxoff + i, 0);

	prphwrite(ctlr, dramaddr, PCIWADDR(ctlr->sched.s)>>10);

	csr32w(ctlr, FhTxChicken, csr32r(ctlr, FhTxChicken) | 2);

	if(ctlr->type != Type4965){
		/* Enable chain mode for all queues, except command queue 4. */
		prphwrite(ctlr, SchedQChainSel5000, 0xfffef);
		prphwrite(ctlr, SchedAggrSel5000, 0);

		for(q=0; q<20; q++){
			prphwrite(ctlr, SchedQueueRdptr5000 + q*4, 0);
			csr32w(ctlr, HbusTargWptr, q << 8);

			memwrite(ctlr, ctlr->sched.base + ctxoff + q*8, 0);
			/* Set scheduler window size and frame limit. */
			memwrite(ctlr, ctlr->sched.base + ctxoff + q*8 + 4, 64<<16 | 64);
		}
		/* Enable interrupts for all our 20 queues. */
		prphwrite(ctlr, SchedIntrMask5000, 0xfffff);

		/* Identify TX FIFO rings (0-7). */
		prphwrite(ctlr, SchedTxFact5000, 0xff);
	} else {
		/* Disable chain mode for all our 16 queues. */
		prphwrite(ctlr, SchedQChainSel4965, 0);

		for(q=0; q<16; q++) {
			prphwrite(ctlr, SchedQueueRdptr4965 + q*4, 0);
			csr32w(ctlr, HbusTargWptr, q << 8);

			/* Set scheduler window size. */
			memwrite(ctlr, ctlr->sched.base + ctxoff + q*8, 64);
			/* Set scheduler window size and frame limit. */
			memwrite(ctlr, ctlr->sched.base + ctxoff + q*8 + 4, 64<<16);
		}
		/* Enable interrupts for all our 16 queues. */
		prphwrite(ctlr, SchedIntrMask4965, 0xffff);

		/* Identify TX FIFO rings (0-7). */
		prphwrite(ctlr, SchedTxFact4965, 0xff);
	}

	/* Mark TX rings (4 EDCA + cmd + 2 HCCA) as active. */
	for(q=0; q<7; q++){
		if(ctlr->type != Type4965){
			static uchar qid2fifo[] = { 3, 2, 1, 0, 7, 5, 6 };
			prphwrite(ctlr, SchedQueueStatus5000 + q*4, 0x00ff0018 | qid2fifo[q]);
		} else {
			static uchar qid2fifo[] = { 3, 2, 1, 0, 4, 5, 6 };
			prphwrite(ctlr, SchedQueueStatus4965 + q*4, 0x0007fc01 | qid2fifo[q]<<1);
		}
	}
	nicunlock(ctlr);

	if(ctlr->type != Type4965){
		uchar c[Tcmdsize];

		/* disable wimax coexistance */
		memset(c, 0, sizeof(c));
		if((err = cmd(ctlr, 90, c, 4+4*16)) != nil)
			return err;

		if(ctlr->type != Type5150){
			/* calibrate crystal */
			memset(c, 0, sizeof(c));
			c[0] = 15;	/* code */
			c[1] = 0;	/* group */
			c[2] = 1;	/* ngroup */
			c[3] = 1;	/* isvalid */
			c[4] = ctlr->eeprom.crystal;
			c[5] = ctlr->eeprom.crystal>>16;
			/* for some reason 8086:4238 needs a second try */
			if(cmd(ctlr, 176, c, 8) != nil && (err = cmd(ctlr, 176, c, 8)) != nil)
				return err;
		}

		if(ctlr->calib.done == 0){
			/* query calibration (init firmware) */
			memset(c, 0, sizeof(c));
			put32(c + 0*(5*4) + 0, 0xffffffff);
			put32(c + 0*(5*4) + 4, 0xffffffff);
			put32(c + 0*(5*4) + 8, 0xffffffff);
			put32(c + 2*(5*4) + 0, 0xffffffff);
			if((err = cmd(ctlr, 101, c, (((2*(5*4))+4)*2)+4)) != nil)
				return err;

			/* wait to collect calibration records */
			if(irqwait(ctlr, Ierr, 2000))
				return "calibration failed";

			if(ctlr->calib.done == 0){
				print("iwl: no calibration results\n");
				ctlr->calib.done = 1;
			}
		} else {
			static uchar cmds[] = {8, 9, 11, 17, 16};

			/* send calibration records (runtime firmware) */
			for(q=0; q<nelem(cmds); q++){
				Block *b;

				i = cmds[q];
				if(i == 8 && ctlr->type != Type5150 && ctlr->type != Type2030)
					continue;
				if(i == 17 && (ctlr->type >= Type6000 || ctlr->type == Type5150) &&
					ctlr->type != Type2030)
					continue;

				if((b = ctlr->calib.cmd[i]) == nil)
					continue;
				b = copyblock(b, BLEN(b));
				if((err = qcmd(ctlr, 4, 176, nil, 0, b)) != nil){
					freeb(b);
					return err;
				}
				if((err = flushq(ctlr, 4)) != nil)
					return err;
			}

			/* temperature sensor offset */
			switch (ctlr->type){
			case Type6005:
				memset(c, 0, sizeof(c));
				c[0] = 18;
				c[1] = 0;
				c[2] = 1;
				c[3] = 1;
				put16(c + 4, 2700);
				if((err = cmd(ctlr, 176, c, 4+2+2)) != nil)
					return err;
				break;

			case Type2030:
				memset(c, 0, sizeof(c));
				c[0] = 18;
				c[1] = 0;
				c[2] = 1;
				c[3] = 1;
				if(ctlr->eeprom.rawtemp != 0){
					put16(c + 4, ctlr->eeprom.temp);
					put16(c + 6, ctlr->eeprom.rawtemp);
				} else{
					put16(c + 4, 2700);
					put16(c + 6, 2700);
				}
				put16(c + 8, ctlr->eeprom.volt);
				if((err = cmd(ctlr, 176, c, 4+2+2+2+2)) != nil)
					return err;
				break;
			}

			if(ctlr->type == Type6005 || ctlr->type == Type6050){
				/* runtime DC calibration */
				memset(c, 0, sizeof(c));
				put32(c + 0*(5*4) + 0, 0xffffffff);
				put32(c + 0*(5*4) + 4, 1<<1);
				if((err = cmd(ctlr, 101, c, (((2*(5*4))+4)*2)+4)) != nil)
					return err;
			}

			/* set tx antenna config */
			put32(c, ctlr->rfcfg.txantmask & 7);
			if((err = cmd(ctlr, 152, c, 4)) != nil)
				return err;

			if(ctlr->type == Type2030){
				if((err = sendbtcoexadv(ctlr)) != nil)
					return err;
			}
		}
	}

	return nil;
}

static char*
loadfirmware1(Ctlr *ctlr, u32int dst, uchar *data, int size)
{
	uchar *dma;
	char *err;

	dma = mallocalign(size, 16, 0, 0);
	if(dma == nil)
		return "no memory for dma";
	memmove(dma, data, size);
	coherence();
	if((err = niclock(ctlr)) != 0){
		free(dma);
		return err;
	}
	csr32w(ctlr, FhTxConfig + 9*32, 0);
	csr32w(ctlr, FhSramAddr + 9*4, dst);
	csr32w(ctlr, FhTfbdCtrl0 + 9*8, PCIWADDR(dma));
	csr32w(ctlr, FhTfbdCtrl1 + 9*8, size);
	csr32w(ctlr, FhTxBufStatus + 9*32,
		(1<<FhTxBufStatusTbNumShift) |
		(1<<FhTxBufStatusTbIdxShift) |
		FhTxBufStatusTfbdValid);
	csr32w(ctlr, FhTxConfig + 9*32, FhTxConfigDmaEna | FhTxConfigCirqHostEndTfd);
	nicunlock(ctlr);
	if(irqwait(ctlr, Ifhtx|Ierr, 5000) != Ifhtx){
		free(dma);
		return "dma error / timeout";
	}
	free(dma);
	return 0;
}

static char*
boot(Ctlr *ctlr)
{
	int i, n, size;
	uchar *p, *dma;
	FWImage *fw;
	char *err;

	fw = ctlr->fw;

	if(fw->boot.text.size == 0){
		if(ctlr->calib.done == 0){
			if((err = loadfirmware1(ctlr, 0x00000000, fw->init.text.data, fw->init.text.size)) != nil)
				return err;
			if((err = loadfirmware1(ctlr, 0x00800000, fw->init.data.data, fw->init.data.size)) != nil)
				return err;
			csr32w(ctlr, Reset, 0);
			if(irqwait(ctlr, Ierr|Ialive, 5000) != Ialive)
				return "init firmware boot failed";
			if((err = postboot(ctlr)) != nil)
				return err;
			if((err = reset(ctlr)) != nil)
				return err;
		}
		if((err = loadfirmware1(ctlr, 0x00000000, fw->main.text.data, fw->main.text.size)) != nil)
			return err;
		if((err = loadfirmware1(ctlr, 0x00800000, fw->main.data.data, fw->main.data.size)) != nil)
			return err;
		csr32w(ctlr, Reset, 0);
		if(irqwait(ctlr, Ierr|Ialive, 5000) != Ialive)
			return "main firmware boot failed";
		return postboot(ctlr);
	}

	size = ROUND(fw->init.data.size, 16) + ROUND(fw->init.text.size, 16);
	dma = mallocalign(size, 16, 0, 0);
	if(dma == nil)
		return "no memory for dma";

	if((err = niclock(ctlr)) != nil){
		free(dma);
		return err;
	}

	p = dma;
	memmove(p, fw->init.data.data, fw->init.data.size);
	coherence();
	prphwrite(ctlr, BsmDramDataAddr, PCIWADDR(p) >> 4);
	prphwrite(ctlr, BsmDramDataSize, fw->init.data.size);
	p += ROUND(fw->init.data.size, 16);
	memmove(p, fw->init.text.data, fw->init.text.size);
	coherence();
	prphwrite(ctlr, BsmDramTextAddr, PCIWADDR(p) >> 4);
	prphwrite(ctlr, BsmDramTextSize, fw->init.text.size);

	nicunlock(ctlr);
	if((err = niclock(ctlr)) != nil){
		free(dma);
		return err;
	}

	p = fw->boot.text.data;
	n = fw->boot.text.size/4;
	for(i=0; i<n; i++, p += 4)
		prphwrite(ctlr, BsmSramBase+i*4, get32(p));

	prphwrite(ctlr, BsmWrMemSrc, 0);
	prphwrite(ctlr, BsmWrMemDst, 0);
	prphwrite(ctlr, BsmWrDwCount, n);

	prphwrite(ctlr, BsmWrCtrl, 1<<31);

	for(i=0; i<1000; i++){
		if((prphread(ctlr, BsmWrCtrl) & (1<<31)) == 0)
			break;
		delay(10);
	}
	if(i == 1000){
		nicunlock(ctlr);
		free(dma);
		return "bootcode timeout";
	}

	prphwrite(ctlr, BsmWrCtrl, 1<<30);
	nicunlock(ctlr);

	csr32w(ctlr, Reset, 0);
	if(irqwait(ctlr, Ierr|Ialive, 5000) != Ialive){
		free(dma);
		return "init firmware boot failed";
	}
	free(dma);

	size = ROUND(fw->main.data.size, 16) + ROUND(fw->main.text.size, 16);
	dma = mallocalign(size, 16, 0, 0);
	if(dma == nil)
		return "no memory for dma";
	if((err = niclock(ctlr)) != nil){
		free(dma);
		return err;
	}
	p = dma;
	memmove(p, fw->main.data.data, fw->main.data.size);
	coherence();
	prphwrite(ctlr, BsmDramDataAddr, PCIWADDR(p) >> 4);
	prphwrite(ctlr, BsmDramDataSize, fw->main.data.size);
	p += ROUND(fw->main.data.size, 16);
	memmove(p, fw->main.text.data, fw->main.text.size);
	coherence();
	prphwrite(ctlr, BsmDramTextAddr, PCIWADDR(p) >> 4);
	prphwrite(ctlr, BsmDramTextSize, fw->main.text.size | (1<<31));
	nicunlock(ctlr);

	if(irqwait(ctlr, Ierr|Ialive, 5000) != Ialive){
		free(dma);
		return "main firmware boot failed";
	}
	free(dma);
	return postboot(ctlr);
}

static int
txqready(void *arg)
{
	TXQ *q = arg;
	return q->n < Ntx;
}

static char*
qcmd(Ctlr *ctlr, uint qid, uint code, uchar *data, int size, Block *block)
{
	uchar *d, *c;
	TXQ *q;

	assert(qid < nelem(ctlr->tx));
	assert(size <= Tcmdsize-4);

	ilock(ctlr);
	q = &ctlr->tx[qid];
	while(q->n >= Ntx && !ctlr->broken){
		iunlock(ctlr);
		qlock(q);
		if(!waserror()){
			tsleep(q, txqready, q, 10);
			poperror();
		}
		qunlock(q);
		ilock(ctlr);
	}
	if(ctlr->broken){
		iunlock(ctlr);
		return "qcmd: broken";
	}
	q->n++;

	q->lastcmd = code;
	q->b[q->i] = block;
	c = q->c + q->i * Tcmdsize;
	d = q->d + q->i * Tdscsize;

	/* build command */
	c[0] = code;
	c[1] = 0;	/* flags */
	c[2] = q->i;
	c[3] = qid;

	if(size > 0)
		memmove(c+4, data, size);

	size += 4;

	/* build descriptor */
	*d++ = 0;
	*d++ = 0;
	*d++ = 0;
	*d++ = 1 + (block != nil); /* nsegs */
	put32(d, PCIWADDR(c));	d += 4;
	put16(d, size << 4); d += 2;
	if(block != nil){
		size = BLEN(block);
		put32(d, PCIWADDR(block->rp)); d += 4;
		put16(d, size << 4);
	}

	coherence();

	q->i = (q->i+1) % Ntx;
	csr32w(ctlr, HbusTargWptr, (qid<<8) | q->i);

	iunlock(ctlr);

	return nil;
}

static int
txqempty(void *arg)
{
	TXQ *q = arg;
	return q->n == 0;
}

static char*
flushq(Ctlr *ctlr, uint qid)
{
	TXQ *q;
	int i;

	q = &ctlr->tx[qid];
	qlock(q);
	for(i = 0; i < 200 && !ctlr->broken; i++){
		if(txqempty(q)){
			qunlock(q);
			return nil;
		}
		if(!waserror()){
			tsleep(q, txqempty, q, 10);
			poperror();
		}
	}
	qunlock(q);
	if(ctlr->broken)
		return "flushq: broken";
	return "flushq: timeout";
}

static char*
cmd(Ctlr *ctlr, uint code, uchar *data, int size)
{
	char *err;

	if(0) print("cmd %ud\n", code);
	if((err = qcmd(ctlr, 4, code, data, size, nil)) != nil)
		return err;
	return flushq(ctlr, 4);
}

static void
setled(Ctlr *ctlr, int which, int on, int off)
{
	uchar c[8];

	csr32w(ctlr, Led, csr32r(ctlr, Led) & ~LedBsmCtrl);

	memset(c, 0, sizeof(c));
	put32(c, 10000);
	c[4] = which;
	c[5] = on;
	c[6] = off;
	cmd(ctlr, 72, c, sizeof(c));
}

static void
addnode(Ctlr *ctlr, uchar id, uchar *addr)
{
	uchar c[Tcmdsize], *p;

	memset(p = c, 0, sizeof(c));
	*p++ = 0;	/* control (1 = update) */
	p += 3;		/* reserved */
	memmove(p, addr, 6);
	p += 6;
	p += 2;		/* reserved */
	*p++ = id;	/* node id */
	p++;		/* flags */
	p += 2;		/* reserved */
	p += 2;		/* kflags */
	p++;		/* tcs2 */
	p++;		/* reserved */
	p += 5*2;	/* ttak */
	p++;		/* kid */
	p++;		/* reserved */
	p += 16;	/* key */
	if(ctlr->type != Type4965){
		p += 8;		/* tcs */
		p += 8;		/* rxmic */
		p += 8;		/* txmic */
	}
	p += 4;		/* htflags */
	p += 4;		/* mask */
	p += 2;		/* disable tid */
	p += 2;		/* reserved */
	p++;		/* add ba tid */
	p++;		/* del ba tid */
	p += 2;		/* add ba ssn */
	p += 4;		/* reserved */
	cmd(ctlr, 24, c, p - c);
}

static void
rxon(Ether *edev, Wnode *bss)
{
	uchar c[Tcmdsize], *p;
	int filter, flags;
	Ctlr *ctlr;
	char *err;

	ctlr = edev->ctlr;
	filter = FilterNoDecrypt | FilterMulticast | FilterBeacon;
	if(ctlr->prom){
		filter |= FilterPromisc;
		if(bss != nil)
			ctlr->channel = bss->channel;
		bss = nil;
	}
	flags = RFlagTSF | RFlagCTSToSelf | RFlag24Ghz | RFlagAuto;
	if(bss != nil){
		if(bss->cap & (1<<5))
			flags |= RFlagShPreamble;
		if(bss->cap & (1<<10))
			flags |= RFlagShSlot;
		ctlr->channel = bss->channel;
		memmove(ctlr->bssid, bss->bssid, Eaddrlen);
		ctlr->aid = bss->aid;
		if(ctlr->aid != 0){
			filter |= FilterBSS;
			filter &= ~FilterBeacon;
			ctlr->bssnodeid = -1;
		} else
			ctlr->bcastnodeid = -1;
	} else {
		memmove(ctlr->bssid, edev->bcast, Eaddrlen);
		ctlr->aid = 0;
		ctlr->bcastnodeid = -1;
		ctlr->bssnodeid = -1;
	}

	if(ctlr->aid != 0)
		setled(ctlr, 2, 0, 1);		/* on when associated */
	else if(memcmp(ctlr->bssid, edev->bcast, Eaddrlen) != 0)
		setled(ctlr, 2, 10, 10);	/* slow blink when connecting */
	else
		setled(ctlr, 2, 5, 5);		/* fast blink when scanning */

	if(ctlr->wifi->debug)
		print("#l%d: rxon: bssid %E, aid %x, channel %d, filter %x, flags %x\n",
			edev->ctlrno, ctlr->bssid, ctlr->aid, ctlr->channel, filter, flags);

	memset(p = c, 0, sizeof(c));
	memmove(p, edev->ea, 6); p += 8;	/* myaddr */
	memmove(p, ctlr->bssid, 6); p += 8;	/* bssid */
	memmove(p, edev->ea, 6); p += 8;	/* wlap */
	*p++ = 3;				/* mode (STA) */
	*p++ = 0;				/* air (?) */
	/* rxchain */
	put16(p, ((ctlr->rfcfg.rxantmask & 7)<<1) | (2<<10) | (2<<12));
	p += 2;
	*p++ = 0xff;				/* ofdm mask (not yet negotiated) */
	*p++ = 0x0f;				/* cck mask (not yet negotiated) */
	put16(p, ctlr->aid & 0x3fff);
	p += 2;					/* aid */
	put32(p, flags);
	p += 4;
	put32(p, filter);
	p += 4;
	*p++ = ctlr->channel;
	p++;					/* reserved */
	*p++ = 0xff;				/* ht single mask */
	*p++ = 0xff;				/* ht dual mask */
	if(ctlr->type != Type4965){
		*p++ = 0xff;			/* ht triple mask */
		p++;				/* reserved */
		put16(p, 0); p += 2;		/* acquisition */
		p += 2;				/* reserved */
	}
	if((err = cmd(ctlr, 16, c, p - c)) != nil){
		print("rxon: %s\n", err);
		return;
	}

	if(ctlr->bcastnodeid == -1){
		ctlr->bcastnodeid = (ctlr->type != Type4965) ? 15 : 31;
		addnode(ctlr, ctlr->bcastnodeid, edev->bcast);
	}
	if(ctlr->bssnodeid == -1 && bss != nil && ctlr->aid != 0){
		ctlr->bssnodeid = 0;
		addnode(ctlr, ctlr->bssnodeid, bss->bssid);
	}
}

static struct ratetab {
	uchar	rate;
	uchar	plcp;
	uchar	flags;
} ratetab[] = {
	{   2,  10, RFlagCCK },
	{   4,  20, RFlagCCK },
	{  11,  55, RFlagCCK },
	{  22, 110, RFlagCCK },
	{  12, 0xd, 0 },
	{  18, 0xf, 0 },
	{  24, 0x5, 0 },
	{  36, 0x7, 0 },
	{  48, 0x9, 0 },
	{  72, 0xb, 0 },
	{  96, 0x1, 0 },
	{ 108, 0x3, 0 },
	{ 120, 0x3, 0 }
};

static uchar iwlrates[] = {
	0x80 | 2,
	0x80 | 4,
	0x80 | 11,
	0x80 | 22,
	0x80 | 12,
	0x80 | 18,
	0x80 | 24,
	0x80 | 36,
	0x80 | 48,
	0x80 | 72,
	0x80 | 96,
	0x80 | 108,
	0x80 | 120,

	0
};

enum {
	TFlagNeedProtection	= 1<<0,
	TFlagNeedRTS		= 1<<1,
	TFlagNeedCTS		= 1<<2,
	TFlagNeedACK		= 1<<3,
	TFlagLinkq		= 1<<4,
	TFlagImmBa		= 1<<6,
	TFlagFullTxOp		= 1<<7,
	TFlagBtDis		= 1<<12,
	TFlagAutoSeq		= 1<<13,
	TFlagMoreFrag		= 1<<14,
	TFlagInsertTs		= 1<<16,
	TFlagNeedPadding	= 1<<20,
};

static void
transmit(Wifi *wifi, Wnode *wn, Block *b)
{
	int flags, nodeid, rate, ant;
	uchar c[Tcmdsize], *p;
	Ether *edev;
	Ctlr *ctlr;
	Wifipkt *w;
	char *err;

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

	flags = 0;
	nodeid = ctlr->bcastnodeid;
	p = wn->minrate;
	w = (Wifipkt*)b->rp;
	if((w->a1[0] & 1) == 0){
		flags |= TFlagNeedACK;

		if(BLEN(b) > 512-4)
			flags |= TFlagNeedRTS;

		if((w->fc[0] & 0x0c) == 0x08 &&	ctlr->bssnodeid != -1){
			nodeid = ctlr->bssnodeid;
			p = wn->actrate;
		}

		if(flags & (TFlagNeedRTS|TFlagNeedCTS)){
			if(ctlr->type != Type4965){
				flags &= ~(TFlagNeedRTS|TFlagNeedCTS);
				flags |= TFlagNeedProtection;
			} else
				flags |= TFlagFullTxOp;
		}
	}
	qunlock(ctlr);

	rate = 0;
	if(p >= iwlrates && p < &iwlrates[nelem(ratetab)])
		rate = p - iwlrates;

	/* select first available antenna */
	ant = ctlr->rfcfg.txantmask & 7;
	ant |= (ant == 0);
	ant = ((ant - 1) & ant) ^ ant;

	memset(p = c, 0, sizeof(c));
	put16(p, BLEN(b));
	p += 2;
	p += 2;		/* lnext */
	put32(p, flags);
	p += 4;
	put32(p, 0);
	p += 4;		/* scratch */

	*p++ = ratetab[rate].plcp;
	*p++ = ratetab[rate].flags | (ant<<6);

	p += 2;		/* xflags */
	*p++ = nodeid;
	*p++ = 0;	/* security */
	*p++ = 0;	/* linkq */
	p++;		/* reserved */
	p += 16;	/* key */
	p += 2;		/* fnext */
	p += 2;		/* reserved */
	put32(p, ~0);	/* lifetime */
	p += 4;

	/* BUG: scratch ptr? not clear what this is for */
	put32(p, PCIWADDR(ctlr->kwpage));
	p += 5;

	*p++ = 60;	/* rts ntries */
	*p++ = 15;	/* data ntries */
	*p++ = 0;	/* tid */
	put16(p, 0);	/* timeout */
	p += 2;
	p += 2;		/* txop */
	if((err = qcmd(ctlr, 0, 28, c, p - c, b)) != nil){
		print("transmit: %s\n", err);
		freeb(b);
	}
}

static long
iwlctl(Ether *edev, void *buf, long n)
{
	Ctlr *ctlr;

	ctlr = edev->ctlr;
	if(n >= 5 && memcmp(buf, "reset", 5) == 0){
		ctlr->broken = 1;
		return n;
	}
	if(ctlr->wifi)
		return wifictl(ctlr->wifi, buf, n);
	return 0;
}

static long
iwlifstat(Ether *edev, void *buf, long n, ulong off)
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
iwlpromiscuous(void *arg, int on)
{
	Ether *edev;
	Ctlr *ctlr;

	edev = arg;
	ctlr = edev->ctlr;
	qlock(ctlr);
	ctlr->prom = on;
	rxon(edev, ctlr->wifi->bss);
	qunlock(ctlr);
}

static void
iwlmulticast(void *, uchar*, int)
{
}

static void
iwlrecover(void *arg)
{
	Ether *edev;
	Ctlr *ctlr;

	edev = arg;
	ctlr = edev->ctlr;
	while(waserror())
		;
	for(;;){
		tsleep(&up->sleep, return0, 0, 4000);

		qlock(ctlr);
		for(;;){
			if(ctlr->broken == 0)
				break;

			if(ctlr->power)
				poweroff(ctlr);

			if((csr32r(ctlr, Gpc) & RfKill) == 0)
				break;

			if(reset(ctlr) != nil)
				break;
			if(boot(ctlr) != nil)
				break;

			ctlr->bcastnodeid = -1;
			ctlr->bssnodeid = -1;
			ctlr->aid = 0;
			rxon(edev, ctlr->wifi->bss);
			break;
		}
		qunlock(ctlr);
	}
}

static void
iwlattach(Ether *edev)
{
	FWImage *fw;
	Ctlr *ctlr;
	char *err;

	ctlr = edev->ctlr;
	qlock(ctlr);
	if(waserror()){
		print("#l%d: %s\n", edev->ctlrno, up->errstr);
		if(ctlr->power)
			poweroff(ctlr);
		qunlock(ctlr);
		nexterror();
	}
	if(ctlr->attached == 0){
		if((csr32r(ctlr, Gpc) & RfKill) == 0)
			error("wifi disabled by switch");

		if(ctlr->wifi == nil){
			ctlr->wifi = wifiattach(edev, transmit);
			/* tested with 2230, it has transmit issues using higher bit rates */
			if(ctlr->type != Type2030)
				ctlr->wifi->rates = iwlrates;
		}

		if(ctlr->fw == nil){
			char *fn = fwname[ctlr->type];
			if(ctlr->type == Type6005){
				switch(ctlr->pdev->did){
				case 0x0082:	/* Centrino Advanced-N 6205 */
				case 0x0085:	/* Centrino Advanced-N 6205 */
					break;
				default:	/* Centrino Advanced-N 6030, 6235 */
					fn = "iwn-6030";
				}
			}
			fw = readfirmware(fn);
			print("#l%d: firmware: %s, rev %ux, build %ud, size %ux+%ux+%ux+%ux+%ux\n",
				edev->ctlrno, fn,
				fw->rev, fw->build,
				fw->main.text.size, fw->main.data.size,
				fw->init.text.size, fw->init.data.size,
				fw->boot.text.size);
			ctlr->fw = fw;
		}

		if((err = reset(ctlr)) != nil)
			error(err);
		if((err = boot(ctlr)) != nil)
			error(err);

		ctlr->bcastnodeid = -1;
		ctlr->bssnodeid = -1;
		ctlr->channel = 1;
		ctlr->aid = 0;

		setoptions(edev);

		ctlr->attached = 1;

		kproc("iwlrecover", iwlrecover, edev);
	}
	qunlock(ctlr);
	poperror();
}

static void
receive(Ctlr *ctlr)
{
	Block *b, *bb;
	uchar *d;
	RXQ *rx;
	TXQ *tx;
	uint hw;

	rx = &ctlr->rx;
	if(ctlr->broken || rx->s == nil || rx->b == nil)
		return;

	bb = nil;
	for(hw = get16(rx->s) % Nrx; rx->i != hw; rx->i = (rx->i + 1) % Nrx){
		uchar type, flags, idx, qid;
		u32int len;

		b = rx->b[rx->i];
		if(b == nil)
			continue;

		d = b->rp;
		len = get32(d); d += 4;
		type = *d++;
		flags = *d++;
		USED(flags);
		idx = *d++;
		qid = *d++;

		if(bb != nil){
			freeb(bb);
			bb = nil;
		}
		if((qid & 0x80) == 0 && qid < nelem(ctlr->tx)){
			tx = &ctlr->tx[qid];
			if(tx->n > 0){
				bb = tx->b[idx];
				tx->b[idx] = nil;
				tx->n--;

				wakeup(tx);
			}
		}

		len &= 0x3fff;
		if(len < 4 || type == 0)
			continue;

		len -= 4;
		switch(type){
		case 1:		/* microcontroller ready */
			setfwinfo(ctlr, d, len);
			break;
		case 24:	/* add node done */
			break;
		case 28:	/* tx done */
			if(ctlr->type == Type4965){
				if(len <= 20 || d[20] == 1 || d[20] == 2)
					break;
			} else {
				if(len <= 32 || d[32] == 1 || d[32] == 2)
					break;
			}
			wifitxfail(ctlr->wifi, bb);
			break;
		case 102:	/* calibration result (Type5000 only) */
			if(len < 4)
				break;
			idx = d[0];
			if(idx >= nelem(ctlr->calib.cmd))
				break;
			if(rbplant(ctlr, rx->i) < 0)
				break;
			if(ctlr->calib.cmd[idx] != nil)
				freeb(ctlr->calib.cmd[idx]);
			b->rp = d;
			b->wp = d + len;
			ctlr->calib.cmd[idx] = b;
			continue;
		case 103:	/* calibration done (Type5000 only) */
			ctlr->calib.done = 1;
			break;
		case 130:	/* start scan */
			break;
		case 132:	/* stop scan */
			break;
		case 156:	/* rx statistics */
			break;
		case 157:	/* beacon statistics */
			break;
		case 161:	/* state changed */
			break;
		case 162:	/* beacon missed */
			break;
		case 192:	/* rx phy */
			break;
		case 195:	/* rx done */
			if(d + 2 > b->lim)
				break;
			d += d[1];
			d += 56;
		case 193:	/* mpdu rx done */
			if(d + 4 > b->lim)
				break;
			len = get16(d); d += 4;
			if(d + len + 4 > b->lim)
				break;
			if((get32(d + len) & 3) != 3)
				break;
			if(ctlr->wifi == nil)
				break;
			if(rbplant(ctlr, rx->i) < 0)
				break;
			b->rp = d;
			b->wp = d + len;
			wifiiq(ctlr->wifi, b);
			continue;
		case 197:	/* rx compressed ba */
			break;
		}
	}
	csr32w(ctlr, FhRxWptr, ((hw+Nrx-1) % Nrx) & ~7);
	if(bb != nil)
		freeb(bb);
}

static void
iwlinterrupt(Ureg*, void *arg)
{
	u32int isr, fhisr;
	Ether *edev;
	Ctlr *ctlr;

	edev = arg;
	ctlr = edev->ctlr;
	ilock(ctlr);
	csr32w(ctlr, Imr, 0);
	isr = csr32r(ctlr, Isr);
	fhisr = csr32r(ctlr, FhIsr);
	if(isr == 0xffffffff || (isr & 0xfffffff0) == 0xa5a5a5a0){
		iunlock(ctlr);
		return;
	}
	if(isr == 0 && fhisr == 0)
		goto done;
	csr32w(ctlr, Isr, isr);
	csr32w(ctlr, FhIsr, fhisr);
	if((isr & (Iswrx | Ifhrx | Irxperiodic)) || (fhisr & Ifhrx))
		receive(ctlr);
	if(isr & Ierr){
		ctlr->broken = 1;
		print("#l%d: fatal firmware error\n", edev->ctlrno);
		dumpctlr(ctlr);
	}
	ctlr->wait.m |= isr;
	if(ctlr->wait.m & ctlr->wait.w)
		wakeup(&ctlr->wait);
done:
	csr32w(ctlr, Imr, ctlr->ie);
	iunlock(ctlr);
}

static void
iwlshutdown(Ether *edev)
{
	Ctlr *ctlr;

	ctlr = edev->ctlr;
	if(ctlr->power)
		poweroff(ctlr);
	ctlr->broken = 0;
}

static Ctlr *iwlhead, *iwltail;

static void
iwlpci(void)
{
	Pcidev *pdev;
	
	pdev = nil;
	while(pdev = pcimatch(pdev, 0, 0)) {
		Ctlr *ctlr;
		void *mem;
		
		if(pdev->ccrb != 2 || pdev->ccru != 0x80)
			continue;
		if(pdev->vid != 0x8086)
			continue;

		switch(pdev->did){
		default:
			continue;
		case 0x0084:	/* WiFi Link 1000 */
		case 0x4229:	/* WiFi Link 4965 */
		case 0x4230:	/* WiFi Link 4965 */
		case 0x4232:	/* Wifi Link 5100 */
		case 0x4236:	/* WiFi Link 5300 AGN */
		case 0x4237:	/* Wifi Link 5100 AGN */
		case 0x423d:	/* Wifi Link 5150 */
		case 0x423b:	/* PRO/Wireless 5350 AGN */
		case 0x0082:	/* Centrino Advanced-N 6205 */
		case 0x0085:	/* Centrino Advanced-N 6205 */
		case 0x422b:	/* Centrino Ultimate-N 6300 variant 1 */
		case 0x4238:	/* Centrino Ultimate-N 6300 variant 2 */
		case 0x08ae:	/* Centrino Wireless-N 100 */
		case 0x0083:	/* Centrino Wireless-N 1000 */
		case 0x0887:	/* Centrino Wireless-N 2230 */
		case 0x0888:	/* Centrino Wireless-N 2230 */
		case 0x0090:	/* Centrino Advanced-N 6030 */
		case 0x0091:	/* Centrino Advanced-N 6030 */
		case 0x088e:	/* Centrino Advanced-N 6235 */
		case 0x088f:	/* Centrino Advanced-N 6235 */
			break;
		}

		/* Clear device-specific "PCI retry timeout" register (41h). */
		if(pcicfgr8(pdev, 0x41) != 0)
			pcicfgw8(pdev, 0x41, 0);

		/* Clear interrupt disable bit. Hardware bug workaround. */
		if(pdev->pcr & 0x400){
			pdev->pcr &= ~0x400;
			pcicfgw16(pdev, PciPCR, pdev->pcr);
		}

		pcisetbme(pdev);
		pcisetpms(pdev, 0);

		ctlr = malloc(sizeof(Ctlr));
		if(ctlr == nil) {
			print("iwl: unable to alloc Ctlr\n");
			continue;
		}
		ctlr->port = pdev->mem[0].bar & ~0x0F;
		mem = vmap(pdev->mem[0].bar & ~0x0F, pdev->mem[0].size);
		if(mem == nil) {
			print("iwl: can't map %8.8luX\n", pdev->mem[0].bar);
			free(ctlr);
			continue;
		}
		ctlr->nic = mem;
		ctlr->pdev = pdev;
		ctlr->type = (csr32r(ctlr, Rev) >> 4) & 0x1F;

		if(fwname[ctlr->type] == nil){
			print("iwl: unsupported controller type %d\n", ctlr->type);
			vunmap(mem, pdev->mem[0].size);
			free(ctlr);
			continue;
		}

		if(iwlhead != nil)
			iwltail->link = ctlr;
		else
			iwlhead = ctlr;
		iwltail = ctlr;
	}
}

static int
iwlpnp(Ether* edev)
{
	Ctlr *ctlr;
	
	if(iwlhead == nil)
		iwlpci();
again:
	for(ctlr = iwlhead; ctlr != nil; ctlr = ctlr->link){
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
	edev->interrupt = iwlinterrupt;
	edev->attach = iwlattach;
	edev->ifstat = iwlifstat;
	edev->ctl = iwlctl;
	edev->shutdown = iwlshutdown;
	edev->promiscuous = iwlpromiscuous;
	edev->multicast = iwlmulticast;
	edev->mbps = 54;

	if(iwlinit(edev) < 0){
		edev->ctlr = nil;
		goto again;
	}
	
	return 0;
}

void
etheriwllink(void)
{
	addethercard("iwl", iwlpnp);
}
