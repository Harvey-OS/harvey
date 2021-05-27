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
	Nrxlog = 6,
	Nrx    = 1<<Nrxlog,
	Ntx    = 256,

	Rbufsize = 3*1024,
	Rdscsize = 8,

	Tdscsize = 64,
	Tcmdsize = 128,
};

/* registers */
enum {
	Cfg		= 0x000,
		AlmMb		= 1<<8,
		AlmMm		= 1<<9,
		SkuMrc		= 1<<10,
		RevD		= 1<<11,
		TypeB		= 1<<12,
	Isr		= 0x008,
	Imr		= 0x00c,
		Ialive	= 1<<0,
		Iwakeup		= 1<<1,
		Iswrx		= 1<<3,
		Irftoggled	= 1<<7,
		Iswerr		= 1<<25,
		Ifhtx		= 1<<27,
		Ihwerr		= 1<<29,
		Ifhrx		= 1<<31,
		Ierr		= Iswerr | Ihwerr,
		Idefmask	= Ierr | Ifhtx | Ifhrx | Ialive | Iwakeup | Iswrx | Irftoggled,
	FhIsr		= 0x010,
	GpioIn		= 0x018,
	Reset		= 0x020,
		Nevo	= 1<<0,
		SW	= 1<<7,
		MasterDisabled	= 1<<8,
		StopMaster	= 1<<9,

	Gpc		= 0x024,
		MacAccessEna	= 1<<0,
		MacClockReady	= 1<<0,
		InitDone	= 1<<2,
		MacAccessReq	= 1<<3,
		NicSleep	= 1<<4,
		RfKill		= 1<<27,
	Eeprom		= 0x02c,
	EepromGp	= 0x030,

	UcodeGp1Clr	= 0x05c,
		UcodeGp1RfKill		= 1<<1,
		UcodeGp1CmdBlocked	= 1<<2,
	UcodeGp2	= 0x060,

	GioChicken	= 0x100,
		L1AnoL0Srx	= 1<<23,
	AnaPll		= 0x20c,
		Init		= 1<<24,

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
	FhCbbcCtrl	= 0x940,
	FhCbbcBase	= 0x944,
	FhRxConfig	= 0xc00,
		FhRxConfigDmaEna	= 1<<31,
		FhRxConfigRdrbdEna	= 1<<29,
		FhRxConfigWrstatusEna	= 1<<27,
		FhRxConfigMaxfrag	= 1<<24,
		FhRxConfigIrqDstHost	= 1<<12,

		FhRxConfigNrdbShift	= 20,
		FhRxConfigIrqRbthShift	= 4,
	FhRxBase	= 0xc04,
	FhRxWptr	= 0xc20,
	FhRxRptrAddr	= 0xc24,
	FhRssrTbl	= 0xcc0,
	FhRxStatus	= 0xcc4,
	FhTxConfig	= 0xd00,	// +q*32
	FhTxBase	= 0xe80,
	FhMsgConfig	= 0xe88,
	FhTxStatus	= 0xe90,
};

/*
 * NIC internal memory offsets.
 */
enum {
	AlmSchedMode	= 0x2e00,
	AlmSchedArastat	= 0x2e04,
	AlmSchedTxfact	= 0x2e10,
	AlmSchedTxf4mf	= 0x2e14,
	AlmSchedTxf5mf	= 0x2e20,
	AlmSchedBP1	= 0x2e2c,
	AlmSchedBP2	= 0x2e30,
	ApmgClkEna	= 0x3004,
	ApmgClkDis	= 0x3008,
		DmaClkRqt	= 1<<9,
		BsmClkRqt	= 1<<11,
	ApmgPs		= 0x300c,
		PwrSrcVMain	= 0<<24,
		PwrSrcMask	= 3<<24,

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

enum {
	FilterPromisc		= 1<<0,
	FilterCtl		= 1<<1,
	FilterMulticast		= 1<<2,
	FilterNoDecrypt		= 1<<3,
	FilterBSS		= 1<<5,
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
};

typedef struct FWSect FWSect;
typedef struct FWImage FWImage;

typedef struct TXQ TXQ;
typedef struct RXQ RXQ;

typedef struct Shared Shared;
typedef struct Sample Sample;
typedef struct Powergrp Powergrp;

typedef struct Ctlr Ctlr;

struct FWSect
{
	uchar *data;
	uint  size;
};

struct FWImage
{
	struct {
		FWSect text;
		FWSect data;
	} init, main, boot;

	uint  version;
	uchar data[];
};

struct TXQ
{
	uint n;
	uint i;
	Block **b;
	uchar *d;
	uchar *c;

	uint lastcmd;

	Rendez;
	QLock;
};

struct RXQ
{
	uint   i;
	Block  **b;
	u32int *p;
};

struct Shared
{
	u32int txbase[8];
	u32int next;
	u32int reserved[2];
};

struct Sample
{
	uchar index;
	char power;
};

struct Powergrp
{
	uchar chan;
	char maxpwr;
	short temp;
	Sample samples[5];
};

struct Ctlr {
	Lock;
	QLock;

	Ctlr *link;
	Pcidev *pdev;
	Wifi *wifi;

	int port;
	int power;
	int active;
	int broken;
	int attached;

	int temp;
	u32int ie;
	u32int *nic;

	/* assigned node ids in hardware node table or -1 if unassigned */
	int bcastnodeid;
	int bssnodeid;

	/* current receiver settings */
	uchar bssid[Eaddrlen];
	int channel;
	int prom;
	int aid;

	RXQ rx;
	TXQ tx[8];

	struct {
		Rendez;
		u32int	m;
		u32int	w;
	} wait;

	struct {
		uchar cap;
		u16int rev;
		uchar type;

		char regdom[4+1];

		Powergrp pwrgrps[5];
	} eeprom;

	char maxpwr[256];

	Shared *shared;

	FWImage *fw;
};

static void setled(Ctlr *ctlr, int which, int on, int off);

#define csr32r(c, r)	(*((c)->nic+((r)/4)))
#define csr32w(c, r, v)	(*((c)->nic+((r)/4)) = (v))

static uint
get32(uchar *p){
	return *((u32int*)p);
}
static uint
get16(uchar *p)
{
	return *((u16int*)p);
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

static char*
eepromread(Ctlr *ctlr, void *data, int count, uint off)
{
	uchar *out = data;
	char *err;
	u32int w = 0;
	int i;

	if((err = niclock(ctlr)) != nil)
		return err;

	for(; count > 0; count -= 2, off++){
		csr32w(ctlr, Eeprom, off << 2);
		csr32w(ctlr, Eeprom, csr32r(ctlr, Eeprom) & ~(1<<1));

		for(i = 0; i < 10; i++){
			w = csr32r(ctlr, Eeprom);
			if(w & 1)
				break;
			delay(5);
		}
		if(i == 10)
			break;
		*out++ = w >> 16;
		if(count > 1)
			*out++ = w >> 24;
	}
	nicunlock(ctlr);

	if(count > 0)
		return "eeprompread: timeout";
	return nil;
}

static char*
clockwait(Ctlr *ctlr)
{
	int i;

	/* Set "initialization complete" bit. */
	csr32w(ctlr, Gpc, csr32r(ctlr, Gpc) | InitDone);
	for(i=0; i<2500; i++){
		if(csr32r(ctlr, Gpc) & MacClockReady)
			return nil;
		delay(10);
	}
	return "clockwait: timeout";
}

static char*
poweron(Ctlr *ctlr)
{
	char *err;

	if(ctlr->power)
		return nil;

	csr32w(ctlr, AnaPll, csr32r(ctlr, AnaPll) | Init);
	/* Disable L0s. */
	csr32w(ctlr, GioChicken, csr32r(ctlr, GioChicken) | L1AnoL0Srx);

	if((err = clockwait(ctlr)) != nil)
		return err;

	if((err = niclock(ctlr)) != nil)
		return err;

	prphwrite(ctlr, ApmgClkEna, DmaClkRqt | BsmClkRqt);
	delay(20);

	/* Disable L1. */
	prphwrite(ctlr, ApmgPciStt, prphread(ctlr, ApmgPciStt) | (1<<11));

	nicunlock(ctlr);

	ctlr->power = 1;

	return nil;
}

static void
poweroff(Ctlr *ctlr)
{
	int i, j;

	csr32w(ctlr, Reset, Nevo);

	/* Disable interrupts. */
	csr32w(ctlr, Imr, 0);
	csr32w(ctlr, Isr, ~0);
	csr32w(ctlr, FhIsr, ~0);

	if(niclock(ctlr) == nil){
		/* Stop TX scheduler. */
		prphwrite(ctlr, AlmSchedMode, 0);
		prphwrite(ctlr, AlmSchedTxfact, 0);

		/* Stop all DMA channels */
		for(i = 0; i < 6; i++){
			csr32w(ctlr, FhTxConfig + i*32, 0);
			for(j = 0; j < 100; j++){
				if((csr32r(ctlr, FhTxStatus) & (0x1010000<<i)) == (0x1010000<<i))
					break;
				delay(10);
			}
		}
		nicunlock(ctlr);
	}

	/* Stop RX ring. */
	if(niclock(ctlr) == nil){
		csr32w(ctlr, FhRxConfig, 0);
		for(j = 0; j < 100; j++){
			if(csr32r(ctlr, FhRxStatus) & (1<<24))
				break;
			delay(10);
		}
		nicunlock(ctlr);
	}

	if(niclock(ctlr) == nil){
		prphwrite(ctlr, ApmgClkDis, DmaClkRqt);
		nicunlock(ctlr);
	}
	delay(5);

	csr32w(ctlr, Reset, csr32r(ctlr, Reset) | StopMaster);

	if((csr32r(ctlr, Gpc) & (7<<24)) != (4<<24)){
		for(j = 0; j < 100; j++){
			if(csr32r(ctlr, Reset) & MasterDisabled)
				break;
			delay(10);
		}
	}

	csr32w(ctlr, Reset, csr32r(ctlr, Reset) | SW);

	ctlr->power = 0;
}

static struct {
	u32int	addr;	/* offset in EEPROM */
	u8int	nchan;
	u8int	chan[14];
} bands[5] = {
	{ 0x63, 14,
	    { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14 } },
	{ 0x72, 13,
	    { 183, 184, 185, 187, 188, 189, 192, 196, 7, 8, 11, 12, 16 } },
	{ 0x80, 12,
	    { 34, 36, 38, 40, 42, 44, 46, 48, 52, 56, 60, 64 } },
	{ 0x8d, 11,
	    { 100, 104, 108, 112, 116, 120, 124, 128, 132, 136, 140 } },
	{ 0x99, 6,
	    { 145, 149, 153, 157, 161, 165 } }
};

static int
wpiinit(Ether *edev)
{
	Ctlr *ctlr;
	char *err;
	uchar b[64];
	int i, j;
	Powergrp *g;

	ctlr = edev->ctlr;
	if((err = poweron(ctlr)) != nil)
		goto Err;
	if((csr32r(ctlr, EepromGp) & 0x6) == 0){
		err = "bad rom signature";
		goto Err;
	}
	/* Clear HW ownership of EEPROM. */
	csr32w(ctlr, EepromGp, csr32r(ctlr, EepromGp) & ~0x180);

	if((err = eepromread(ctlr, b, 1, 0x45)) != nil)
		goto Err;
	ctlr->eeprom.cap = b[0];
	if((err = eepromread(ctlr, b, 2, 0x35)) != nil)
		goto Err;
	ctlr->eeprom.rev = get16(b);
	if((err = eepromread(ctlr, b, 1, 0x4a)) != nil)
		goto Err;
	ctlr->eeprom.type = b[0];
	if((err = eepromread(ctlr, b, 4, 0x60)) != nil)
		goto Err;
	strncpy(ctlr->eeprom.regdom, (char*)b, 4);
	ctlr->eeprom.regdom[4] = '\0';

	print("wpi: %X %X %X %s\n", ctlr->eeprom.cap, ctlr->eeprom.rev, ctlr->eeprom.type, ctlr->eeprom.regdom);

	if((err = eepromread(ctlr, b, 6, 0x15)) != nil)
		goto Err;
	memmove(edev->ea, b, Eaddrlen);

	memset(ctlr->maxpwr, 0, sizeof(ctlr->maxpwr));
	for(i = 0; i < nelem(bands); i++){
		if((err = eepromread(ctlr, b, 2*bands[i].nchan, bands[i].addr)) != nil)
			goto Err;
		for(j = 0; j < bands[i].nchan; j++){
			if(!(b[j*2] & 1))
				continue;
			ctlr->maxpwr[bands[i].chan[j]] = b[j*2+1];
		}
	}

	for(i = 0; i < nelem(ctlr->eeprom.pwrgrps); i++){
		if((err = eepromread(ctlr, b, 64, 0x100 + i*32)) != nil)
			goto Err;
		g = &ctlr->eeprom.pwrgrps[i];
		g->maxpwr = b[60];
		g->chan = b[61];
		g->temp = get16(b+62);
		for(j = 0; j < 5; j++){
			g->samples[j].index = b[j*4];
			g->samples[j].power = b[j*4+1];
		}
	}

	poweroff(ctlr);
	return 0;
Err:
	print("wpiinit: %s\n", err);
	poweroff(ctlr);
	return -1;
}

static char*
crackfw(FWImage *i, uchar *data, uint size)
{
	uchar *p, *e;

	memset(i, 0, sizeof(*i));
	if(size < 4*6){
Tooshort:
		return "firmware image too short";
	}
	p = data;
	e = p + size;
	i->version = get32(p); p += 4;
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
	return nil;
}

static FWImage*
readfirmware(void)
{
	uchar dirbuf[sizeof(Dir)+100], *data;
	char *err;
	FWImage *fw;
	int n, r;
	Chan *c;
	Dir d;

	if(!iseve())
		error(Eperm);
	if(!waserror()){
		c = namec("/boot/wpi-3945abg", Aopen, OREAD, 0);
		poperror();
	}else
		c = namec("/lib/firmware/wpi-3945abg", Aopen, OREAD, 0);
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
	if((err = crackfw(fw, data, r)) != nil)
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

	b = iallocb(Rbufsize+127);
	if(b == nil)
		return -1;
	b->rp = b->wp = (uchar*)((((uintptr)b->base+127)&~127));
	memset(b->rp, 0, Rdscsize);
	coherence();
	ctlr->rx.b[i] = b;
	ctlr->rx.p[i] = PCIWADDR(b->rp);
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
		rx->p = mallocalign(sizeof(u32int) * Nrx, 16 * 1024, 0, 0);
	if(rx->b == nil || rx->p == nil)
		return "no memory for rx ring";
	for(i = 0; i<Nrx; i++){
		rx->p[i] = 0;
		if(rx->b[i] != nil){
			freeb(rx->b[i]);
			rx->b[i] = nil;
		}
		if(rbplant(ctlr, i) < 0)
			return "no memory for rx descriptors";
	}
	rx->i = 0;

	if(ctlr->shared == nil)
		ctlr->shared = mallocalign(4096, 4096, 0, 0);
	if(ctlr->shared == nil)
		return "no memory for shared buffer";
	memset(ctlr->shared, 0, 4096);

	for(q=0; q<nelem(ctlr->tx); q++){
		tx = &ctlr->tx[q];
		if(tx->b == nil)
			tx->b = malloc(sizeof(Block*) * Ntx);
		if(tx->d == nil)
			tx->d = mallocalign(Tdscsize * Ntx, 16 * 1024, 0, 0);
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
		ctlr->shared->txbase[q] = PCIWADDR(tx->d);
		tx->i = 0;
		tx->n = 0;
		tx->lastcmd = 0;
	}
	return nil;
}

static char*
reset(Ctlr *ctlr)
{
	uchar rev;
	char *err;
	int i;

	if(ctlr->power)
		poweroff(ctlr);
	if((err = initring(ctlr)) != nil)
		return err;
	if((err = poweron(ctlr)) != nil)
		return err;

	/* Select VMAIN power source. */
	if((err = niclock(ctlr)) != nil)
		return err;
	prphwrite(ctlr, ApmgPs, (prphread(ctlr, ApmgPs) & ~PwrSrcMask) | PwrSrcVMain);
	nicunlock(ctlr);
	/* Spin until VMAIN gets selected. */
	for(i = 0; i < 5000; i++){
		if(csr32r(ctlr, GpioIn) & (1 << 9))
			break;
		delay(10);
	}

	/* Perform adapter initialization. */
	rev = ctlr->pdev->rid;
	if((rev & 0xc0) == 0x40)
		csr32w(ctlr, Cfg, csr32r(ctlr, Cfg) | AlmMb);
	else if(!(rev & 0x80))
		csr32w(ctlr, Cfg, csr32r(ctlr, Cfg) | AlmMm);

	if(ctlr->eeprom.cap == 0x80)
		csr32w(ctlr, Cfg, csr32r(ctlr, Cfg) | SkuMrc);

	if((ctlr->eeprom.rev & 0xf0) == 0xd0)
		csr32w(ctlr, Cfg, csr32r(ctlr, Cfg) | RevD);
	else
		csr32w(ctlr, Cfg, csr32r(ctlr, Cfg) & ~RevD);

	if(ctlr->eeprom.type > 1)
		csr32w(ctlr, Cfg, csr32r(ctlr, Cfg) | TypeB);

	/* Initialize RX ring. */
	if((err = niclock(ctlr)) != nil)
		return err;

	coherence();
	csr32w(ctlr, FhRxBase, PCIWADDR(ctlr->rx.p));
	csr32w(ctlr, FhRxRptrAddr, PCIWADDR(&ctlr->shared->next));
	csr32w(ctlr, FhRxWptr, 0);
	csr32w(ctlr, FhRxConfig,
		FhRxConfigDmaEna |
		FhRxConfigRdrbdEna |
		FhRxConfigWrstatusEna |
		FhRxConfigMaxfrag |
		(Nrxlog << FhRxConfigNrdbShift) |
		FhRxConfigIrqDstHost |
		(1 << FhRxConfigIrqRbthShift));
	USED(csr32r(ctlr, FhRssrTbl));
	csr32w(ctlr, FhRxWptr, (Nrx-1) & ~7);
	nicunlock(ctlr);

	/* Initialize TX rings. */
	if((err = niclock(ctlr)) != nil)
		return err;
	prphwrite(ctlr, AlmSchedMode, 2);
	prphwrite(ctlr, AlmSchedArastat, 1);
	prphwrite(ctlr, AlmSchedTxfact, 0x3f);
	prphwrite(ctlr, AlmSchedBP1, 0x10000);
	prphwrite(ctlr, AlmSchedBP2, 0x30002);
	prphwrite(ctlr, AlmSchedTxf4mf, 4);
	prphwrite(ctlr, AlmSchedTxf5mf, 5);
	csr32w(ctlr, FhTxBase, PCIWADDR(ctlr->shared));
	csr32w(ctlr, FhMsgConfig, 0xffff05a5);
	for(i = 0; i < 6; i++){
		csr32w(ctlr, FhCbbcCtrl+i*8, 0);
		csr32w(ctlr, FhCbbcBase+i*8, 0);
		csr32w(ctlr, FhTxConfig+i*32, 0x80200008);
	}
	nicunlock(ctlr);
	USED(csr32r(ctlr, FhTxBase));

	csr32w(ctlr, UcodeGp1Clr, UcodeGp1RfKill);
	csr32w(ctlr, UcodeGp1Clr, UcodeGp1CmdBlocked);

	ctlr->broken = 0;
	ctlr->wait.m = 0;
	ctlr->wait.w = 0;

	ctlr->ie = Idefmask;
	csr32w(ctlr, Imr, ctlr->ie);
	csr32w(ctlr, Isr, ~0);

	csr32w(ctlr, UcodeGp1Clr, UcodeGp1RfKill);
	csr32w(ctlr, UcodeGp1Clr, UcodeGp1RfKill);

	return nil;
}

static char*
postboot(Ctlr *);

static char*
boot(Ctlr *ctlr)
{
	int i, n, size;
	uchar *dma, *p;
	FWImage *fw;
	char *err;

	fw = ctlr->fw;
	/* 16 byte padding may not be necessary. */
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
	prphwrite(ctlr, BsmDramDataAddr, PCIWADDR(p));
	prphwrite(ctlr, BsmDramDataSize, fw->init.data.size);
	p += ROUND(fw->init.data.size, 16);
	memmove(p, fw->init.text.data, fw->init.text.size);
	coherence();
	prphwrite(ctlr, BsmDramTextAddr, PCIWADDR(p));
	prphwrite(ctlr, BsmDramTextSize, fw->init.text.size);

	nicunlock(ctlr);
	if((err = niclock(ctlr)) != nil){
		free(dma);
		return err;
	}

	/* Copy microcode image into NIC memory. */
	p = fw->boot.text.data;
	n = fw->boot.text.size/4;
	for(i=0; i<n; i++, p += 4)
		prphwrite(ctlr, BsmSramBase+i*4, get32(p));

	prphwrite(ctlr, BsmWrMemSrc, 0);
	prphwrite(ctlr, BsmWrMemDst, 0);
	prphwrite(ctlr, BsmWrDwCount, n);

	/* Start boot load now. */
	prphwrite(ctlr, BsmWrCtrl, 1<<31);

	/* Wait for transfer to complete. */
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

	/* Enable boot after power up. */
	prphwrite(ctlr, BsmWrCtrl, 1<<30);
	nicunlock(ctlr);

	/* Now press "execute". */
	csr32w(ctlr, Reset, 0);

	/* Wait at most one second for first alive notification. */
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
	prphwrite(ctlr, BsmDramDataAddr, PCIWADDR(p));
	prphwrite(ctlr, BsmDramDataSize, fw->main.data.size);
	p += ROUND(fw->main.data.size, 16);
	memmove(p, fw->main.text.data, fw->main.text.size);
	coherence();
	prphwrite(ctlr, BsmDramTextAddr, PCIWADDR(p));
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
	int pad;
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

	memset(d, 0, Tdscsize);

	pad = size - 4;
	if(block != nil)
		pad += BLEN(block);
	pad = ((pad + 3) & ~3) - pad;

	put32(d, (pad << 28) | ((1 + (block != nil)) << 24)), d += 4;
	put32(d, PCIWADDR(c)), d += 4;
	put32(d, size), d += 4;

	if(block != nil){
		size = BLEN(block);
		put32(d, PCIWADDR(block->rp)), d += 4;
		put32(d, size), d += 4;
	}

	USED(d);

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
		if(islo() && !waserror()){
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

	if((err = qcmd(ctlr, 4, code, data, size, nil)) != nil)
		return err;
	return flushq(ctlr, 4);
}

static void
setled(Ctlr *ctlr, int which, int on, int off)
{
	uchar c[8];

	memset(c, 0, sizeof(c));
	put32(c, 10000);
	c[4] = which;
	c[5] = on;
	c[6] = off;
	cmd(ctlr, 72, c, sizeof(c));
}

static char*
btcoex(Ctlr *ctlr)
{
	uchar c[Tcmdsize], *p;

	/* configure bluetooth coexistance. */
	p = c;
	*p++ = 3;		/* flags WPI_BT_COEX_MODE_4WIRE */
	*p++ = 30;		/* lead time */
	*p++ = 5;		/* max kill */
	*p++ = 0;		/* reserved */
	put32(p, 0), p += 4;	/* kill_ack */
	put32(p, 0), p += 4;	/* kill_cts */
	return cmd(ctlr, 155, c, p-c);
}

static char*
powermode(Ctlr *ctlr)
{
	uchar c[Tcmdsize];
	int capoff, reg;

	memset(c, 0, sizeof(c));
	capoff = pcicap(ctlr->pdev, PciCapPCIe);
	if(capoff >= 0){
		reg = pcicfgr8(ctlr->pdev, capoff+1);
		if((reg & 1) == 0)	/* PCI_PCIE_LCR_ASPM_L0S */
			c[0] |= 1<<3;	/* WPI_PS_PCI_PMGT */
	}
	return cmd(ctlr, 119, c, 4*(3+5));
}

static char*
postboot(Ctlr *ctlr)
{
	while((ctlr->temp = (int)csr32r(ctlr, UcodeGp2)) == 0)
		delay(10);

if(0){
	char *err;

	if((err = btcoex(ctlr)) != nil)
		print("btcoex: %s\n", err);
	if((err = powermode(ctlr)) != nil)
		print("powermode: %s\n", err);
}

	return nil;
}

static uchar wpirates[] = {
	0x80 | 12,
	0x80 | 18,
	0x80 | 24,
	0x80 | 36,
	0x80 | 48,
	0x80 | 72,
	0x80 | 96,
	0x80 | 108,
	0x80 | 2,
	0x80 | 4,
	0x80 | 11,
	0x80 | 22,

	0
};

static struct {
	uchar	rate;
	uchar	plcp;
} ratetab[] = {
	{  12, 0xd },
	{  18, 0xf },
	{  24, 0x5 },
	{  36, 0x7 },
	{  48, 0x9 },
	{  72, 0xb },
	{  96, 0x1 },
	{ 108, 0x3 },
	{   2,  10 },
	{   4,  20 },
	{  11,  55 },
	{  22, 110 },
};

static u8int rfgain_2ghz[] = {
	0xfb, 0xfb, 0xfb, 0xfb, 0xfb, 0xfb, 0xfb, 0xfb, 0xbb, 0xbb, 0xbb,
	0xbb, 0xf3, 0xf3, 0xf3, 0xf3, 0xf3, 0xd3, 0xd3, 0xb3, 0xb3, 0xb3,
	0x93, 0x93, 0x93, 0x93, 0x93, 0x93, 0x93, 0x73, 0xeb, 0xeb, 0xeb,
	0xcb, 0xcb, 0xcb, 0xcb, 0xcb, 0xcb, 0xcb, 0xab, 0xab, 0xab, 0x8b,
	0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xc3, 0xc3, 0xc3, 0xc3, 0xa3,
	0xa3, 0xa3, 0xa3, 0x83, 0x83, 0x83, 0x83, 0x63, 0x63, 0x63, 0x63,
	0x43, 0x43, 0x43, 0x43, 0x23, 0x23, 0x23, 0x23, 0x03, 0x03, 0x03,
	0x03
};

static  u8int dspgain_2ghz[] = {
	0x7f, 0x7f, 0x7f, 0x7f, 0x7d, 0x6e, 0x69, 0x62, 0x7d, 0x73, 0x6c,
	0x63, 0x77, 0x6f, 0x69, 0x61, 0x5c, 0x6a, 0x64, 0x78, 0x71, 0x6b,
	0x7d, 0x77, 0x70, 0x6a, 0x65, 0x61, 0x5b, 0x6b, 0x79, 0x73, 0x6d,
	0x7f, 0x79, 0x73, 0x6c, 0x66, 0x60, 0x5c, 0x6e, 0x68, 0x62, 0x74,
	0x7d, 0x77, 0x71, 0x6b, 0x65, 0x60, 0x71, 0x6a, 0x66, 0x5f, 0x71,
	0x6a, 0x66, 0x5f, 0x71, 0x6a, 0x66, 0x5f, 0x71, 0x6a, 0x66, 0x5f,
	0x71, 0x6a, 0x66, 0x5f, 0x71, 0x6a, 0x66, 0x5f, 0x71, 0x6a, 0x66,
	0x5f
};

static int
pwridx(Ctlr *ctlr, Powergrp *pwgr, int chan, int rate)
{
/* Fixed-point arithmetic division using a n-bit fractional part. */
#define fdivround(a, b, n)	\
	((((1 << n) * (a)) / (b) + (1 << n) / 2) / (1 << n))

/* Linear interpolation. */
#define interpolate(x, x1, y1, x2, y2, n)	\
	((y1) + fdivround(((x) - (x1)) * ((y2) - (y1)), (x2) - (x1), n))

	int pwr;
	Sample *sample;
	int idx;

	/* Default TX power is group maximum TX power minus 3dB. */
	pwr = pwgr->maxpwr / 2;

	/* Decrease TX power for highest OFDM rates to reduce distortion. */
	switch(rate){
	case 5: /* WPI_RIDX_OFDM36 */
		pwr -= 0;
		break;
	case 6: /* WPI_RIDX_OFDM48 */
		pwr -=7;
		break;
	case 7: /* WPI_RIDX_OFDM54 */
		pwr -= 9;
		break;
	}

	/* Never exceed the channel maximum allowed TX power. */
	pwr = MIN(pwr, ctlr->maxpwr[chan]);

	/* Retrieve TX power index into gain tables from samples. */
	for(sample = pwgr->samples; sample < &pwgr->samples[3]; sample++)
		if(pwr > sample[1].power)
			break;
	/* Fixed-point linear interpolation using a 19-bit fractional part. */
	idx = interpolate(pwr, sample[0].power, sample[0].index,
	    sample[1].power, sample[1].index, 19);

	/*-
	 * Adjust power index based on current temperature:
	 * - if cooler than factory-calibrated: decrease output power
	 * - if warmer than factory-calibrated: increase output power
	 */
	idx -= (ctlr->temp - pwgr->temp) * 11 / 100;

	/* Decrease TX power for CCK rates (-5dB). */
	if (rate >= 8)
		idx += 10;

	/* Make sure idx stays in a valid range. */
	if (idx < 0)
		idx = 0;
	else if (idx >= nelem(rfgain_2ghz))
		idx = nelem(rfgain_2ghz)-1;
	return idx;
#undef fdivround
#undef interpolate
}

static void
addnode(Ctlr *ctlr, uchar id, uchar *addr, int plcp, int antenna)
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
	p += 2;		/* reserved */
	p += 16;	/* key */
	put32(p, 4);	/* action (4 = set_rate) */
	p += 4;		
	p += 4;		/* mask */
	p += 2;		/* tid */
	*p++ = plcp;	/* plcp */
	*p++ = antenna;	/* antenna */
	p++;		/* add_imm */
	p++;		/* del_imm */
	p++;		/* add_imm_start */
	cmd(ctlr, 24, c, p - c);
}

static void
rxon(Ether *edev, Wnode *bss)
{
	uchar c[Tcmdsize], *p;
	int filter, flags, rate;
	Ctlr *ctlr;
	char *err;
	int idx;

	ctlr = edev->ctlr;
	filter = FilterNoDecrypt | FilterMulticast;
	if(ctlr->prom){
		filter |= FilterPromisc;
		if(bss != nil)
			ctlr->channel = bss->channel;
		bss = nil;
	}
	flags = RFlagTSF | RFlag24Ghz | RFlagAuto;
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
			ctlr->bssnodeid = -1;
		}else
			ctlr->bcastnodeid = -1;
	}else{
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

	memset(p = c, 0, sizeof(c));
	memmove(p, edev->ea, 6); p += 8;	/* myaddr */
	memmove(p, ctlr->bssid, 6); p += 16;	/* bssid */
	*p++ = 3;				/* mode (STA) */
	p += 3;
	*p++ = 0xff;				/* ofdm mask (not yet negotiated) */
	*p++ = 0x0f;				/* cck mask (not yet negotiated) */
	put16(p, ctlr->aid & 0x3fff);		/* associd */
	p += 2;
	put32(p, flags);
	p += 4;
	put32(p, filter);
	p += 4;
	*p++ = ctlr->channel;
	p += 3;

	if((err = cmd(ctlr, 16, c, p - c)) != nil){
		print("rxon: %s\n", err);
		return;
	}

	if(ctlr->maxpwr[ctlr->channel] != 0){
		/* tx power */
		memset(p = c, 0, sizeof(c));
		*p++ = 1;	/* band (0 = 5ghz) */
		p++;		/* reserved */
		put16(p, ctlr->channel), p += 2;
		for(rate = 0; rate < nelem(ratetab); rate++){
			idx = pwridx(ctlr, &ctlr->eeprom.pwrgrps[0], ctlr->channel, rate);
			*p++ = ratetab[rate].plcp;
			*p++ = rfgain_2ghz[idx];	/* rf_gain */
			*p++ = dspgain_2ghz[idx];	/* dsp_gain */
			p++;		/* reservd */
		}
		cmd(ctlr, 151, c, p - c);
	}

	if(ctlr->bcastnodeid == -1){
		ctlr->bcastnodeid = 24;
		addnode(ctlr, ctlr->bcastnodeid, edev->bcast, ratetab[0].plcp, 3<<6);
	}
	if(ctlr->bssnodeid == -1 && bss != nil && ctlr->aid != 0){
		ctlr->bssnodeid = 0;
		addnode(ctlr, ctlr->bssnodeid, bss->bssid, ratetab[0].plcp, 3<<6);
	}
}

enum {
	TFlagNeedRTS		= 1<<1,
	TFlagNeedCTS		= 1<<2,
	TFlagNeedACK		= 1<<3,
	TFlagFullTxOp		= 1<<7,
	TFlagBtDis		= 1<<12,
	TFlagAutoSeq		= 1<<13,
	TFlagInsertTs		= 1<<16,
};

static void
transmit(Wifi *wifi, Wnode *wn, Block *b)
{
	uchar c[Tcmdsize], *p;
	Ether *edev;
	Ctlr *ctlr;
	Wifipkt *w;
	int flags, nodeid, rate, timeout;
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
	timeout = 3;
	nodeid = ctlr->bcastnodeid;
	p = wn->minrate;
	w = (Wifipkt*)b->rp;
	if((w->a1[0] & 1) == 0){
		flags |= TFlagNeedACK;

		if(BLEN(b) > 512-4)
			flags |= TFlagNeedRTS|TFlagFullTxOp;

		if((w->fc[0] & 0x0c) == 0x08 &&	ctlr->bssnodeid != -1){
			timeout = 0;
			nodeid = ctlr->bssnodeid;
			p = wn->actrate;
		}
	}
	qunlock(ctlr);

	rate = 0;
	if(p >= wpirates && p < &wpirates[nelem(ratetab)])
		rate = p - wpirates;

	memset(p = c, 0, sizeof(c));
	put16(p, BLEN(b)), p += 2;
	put16(p, 0), p += 2;	/* lnext */
	put32(p, flags), p += 4;
	*p++ = ratetab[rate].plcp;
	*p++ = nodeid;
	*p++ = 0;	/* tid */
	*p++ = 0;	/* security */
	p += 16+8;	/* key/iv */
	put32(p, 0), p += 4;	/* fnext */
	put32(p, 0xffffffff), p += 4;	/* livetime infinite */
	*p++ = 0xff;
	*p++ = 0x0f;
	*p++ = 7;
	*p++ = 15;
	put16(p, timeout), p += 2;
	put16(p, 0), p += 2;	/* txop */

	if((err = qcmd(ctlr, 0, 28, c, p - c, b)) != nil){
		print("transmit: %s\n", err);
		freeb(b);
	}
}

static long
wpictl(Ether *edev, void *buf, long n)
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
wpiifstat(Ether *edev, void *buf, long n, ulong off)
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
wpipromiscuous(void *arg, int on)
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
wpimulticast(void *, uchar*, int)
{
}

static void
wpirecover(void *arg)
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
wpiattach(Ether *edev)
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
			ctlr->wifi->rates = wpirates;
		}

		if(ctlr->fw == nil){
			fw = readfirmware();
			print("#l%d: firmware: %ux, size: %ux+%ux+%ux+%ux+%ux\n",
				edev->ctlrno, fw->version,
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

		kproc("wpirecover", wpirecover, edev);
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
	u32int hw;

	rx = &ctlr->rx;
	if(ctlr->broken || ctlr->shared == nil || rx->b == nil)
		return;

	bb = nil;
	for(hw = ctlr->shared->next % Nrx; rx->i != hw; rx->i = (rx->i + 1) % Nrx){
		uchar type, flags, idx, qid;
		u32int len;

		b = rx->b[rx->i];
		if(b == nil)
			continue;

		d = b->rp;
		len = get32(d); d += 4;
		type = *d++;
		flags = *d++;
		idx = *d++;
		qid = *d++;

		USED(len);
		USED(flags);

if(0) iprint("rxdesc[%d] type=%d len=%d idx=%d qid=%d\n", rx->i, type, len, idx, qid);

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

		switch(type){
		case 1:		/* uc ready */
			break;

		case 24:	/* add node done */
			break;

		case 27:	/* rx done */
			if(d + 1 > b->lim)
				break;
			d += d[0];
			d += 8;
			if(d + 6 + 2 > b->lim){
				break;
			}
			len = get16(d+6);
			d += 8;
			if(d + len + 4 > b->lim){
				break;
			}
			if((get32(d + len) & 3) != 3){
				break;
			}
			if(ctlr->wifi == nil)
				break;
			if(rbplant(ctlr, rx->i) < 0)
				break;
			b->rp = d;
			b->wp = d + len;
			wifiiq(ctlr->wifi, b);
			continue;

		case 28:	/* tx done */
			if(len <= 8 || d[8] == 1)
				break;
			wifitxfail(ctlr->wifi, bb);
			break;

		case 130:	/* start scan */
			break;

		case 132:	/* stop scan */
			break;

		case 161:	/* state change */
			break;
		}
	}
	csr32w(ctlr, FhRxWptr, ((hw+Nrx-1) % Nrx) & ~7);
	if(bb != nil)
		freeb(bb);
}

static void
wpiinterrupt(Ureg*, void *arg)
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
	if((isr & (Iswrx | Ifhrx)) || (fhisr & Ifhrx))
		receive(ctlr);
	if(isr & Ierr){
		ctlr->broken = 1;
		iprint("#l%d: fatal firmware error, lastcmd %ud\n", edev->ctlrno, ctlr->tx[4].lastcmd);
	}
	ctlr->wait.m |= isr;
	if(ctlr->wait.m & ctlr->wait.w)
		wakeup(&ctlr->wait);
done:
	csr32w(ctlr, Imr, ctlr->ie);
	iunlock(ctlr);
}

static void
wpishutdown(Ether *edev)
{
	Ctlr *ctlr;

	ctlr = edev->ctlr;
	if(ctlr->power)
		poweroff(ctlr);
	ctlr->broken = 0;
}

static Ctlr *wpihead, *wpitail;

static void
wpipci(void)
{
	Pcidev *pdev;

	pdev = nil;
	while(pdev = pcimatch(pdev, 0x8086, 0)){
		Ctlr *ctlr;
		void *mem;
		switch(pdev->did){
		default:
			continue;
		case 0x4227:
			break;
		}

		/* Clear device-specific "PCI retry timeout" register (41h). */
		if(pcicfgr8(pdev, 0x41) != 0)
			pcicfgw8(pdev, 0x41, 0);

		pcisetbme(pdev);
		pcisetpms(pdev, 0);

		ctlr = malloc(sizeof(Ctlr));
		if(ctlr == nil) {
			print("wpi: unable to alloc Ctlr\n");
			continue;
		}
		ctlr->port = pdev->mem[0].bar & ~0x0F;
		mem = vmap(pdev->mem[0].bar & ~0x0F, pdev->mem[0].size);
		if(mem == nil) {
			print("wpi: can't map %8.8luX\n", pdev->mem[0].bar);
			free(ctlr);
			continue;
		}
		ctlr->nic = mem;
		ctlr->pdev = pdev;

		if(wpihead != nil)
			wpitail->link = ctlr;
		else
			wpihead = ctlr;
		wpitail = ctlr;
	}
}

static int
wpipnp(Ether *edev)
{
	Ctlr *ctlr;

	if(wpihead == nil)
		wpipci();

again:
	for(ctlr = wpihead; ctlr != nil; ctlr = ctlr->link){
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
	edev->interrupt = wpiinterrupt;
	edev->attach = wpiattach;
	edev->ifstat = wpiifstat;
	edev->ctl = wpictl;
	edev->shutdown = wpishutdown;
	edev->promiscuous = wpipromiscuous;
	edev->multicast = wpimulticast;
	edev->mbps = 54;

	if(wpiinit(edev) < 0){
		edev->ctlr = nil;
		goto again;
	}

	return 0;
}

void
etherwpilink(void)
{
	addethercard("wpi", wpipnp);
}
