/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * Broadcom BCM57xx
 * Not implemented:
 *  proper fatal error handling
 *  multiple rings
 *  checksum offloading
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
#include "../port/ethermii.h"

#define dprint(...)	do{ if(debug)print(__VA_ARGS__); }while(0)
#define Rbsz		ROUNDUP(sizeof(Etherpkt)+4, 4)

typedef struct Ctlr Ctlr;
struct Ctlr {
	Lock	txlock, imlock;
	Ether	*ether;
	Ctlr	*next;
	Pcidev	*pdev;
	u32int	*nic, *status;

	u32int	*recvret, *recvprod, *sendr;
	ulong	port;
	uint	recvreti, recvprodi, sendri, sendcleani;
	Block	**sends;
	Block	**rxs;
	int	active, duplex;
	int	type;

	uint	nobuf;
	uint	partial;
	uint	rxerr;
	uint	qfull;
	uint	dmaerr;
};

enum {
	/* configurable constants */
	RxRetRingLen 		= 0x200,
	RxProdRingLen 		= 0x200,
	SendRingLen 		= 0x200,

	Reset 			= 1<<0,
	Enable 			= 1<<1,
	Attn 			= 1<<2,

	Pwrctlstat 		= 0x4C,

	MiscHostCtl 		= 0x68,
	TaggedStatus		= 1<<9,
	IndirAccessEn		= 1<<7,
	EnableClockCtl		= 1<<5,
	PCIStateRegEn		= 1<<4,
	WordSwap		= 1<<3,
	ByteSwap		= 1<<2,
	MaskPCIInt		= 1<<1,
	ClearIntA		= 1<<0,

	Fwmbox			= 0x0b50,	/* magic value exchange */
	Fwmagic			= 0x4b657654,

	Dmarwctl 		= 0x6C,
	DMAWaterMask		= ~(7<<19),
	DMAWaterValue		= 3<<19,

	Memwind		= 0x7C,
	MemwindData		= 0x84,

	SendRCB 		= 0x100,
	RxRetRCB 		= 0x200,

	InterruptMailbox 		= 0x204,

	RxProdBDRingIdx	= 0x26c,
	RxBDRetRingIdx		= 0x284,
	SendBDRingHostIdx	= 0x304,

	MACMode		= 0x400,
	MACPortMask		= ~(1<<3 | 1<<2),
	MACPortGMII		= 1<<3,
	MACPortMII		= 1<<2,
	MACEnable		= 1<<23 | 1<<22 | 1<<21 | 1 << 15 | 1 << 14 | 1<<12 | 1<<11,
	MACHalfDuplex		= 1<<1,
	
	MACEventStatus		= 0x404,
	MACEventEnable	= 0x408,
	MACAddress		= 0x410,
	RandomBackoff		= 0x438,
	RxMTU			= 0x43C,
	MIComm		= 0x44C,
	MIStatus		= 0x450,
	MIMode			= 0x454,
	RxMACMode		= 0x468,
	TxMACMode		= 0x45C,
	TxMACLengths		= 0x464,
	MACHash		= 0x470,
	RxRules			= 0x480,
	
	RxRulesConf		= 0x500,
	LowWaterMax		= 0x504,
	LowWaterMaxMask	= ~0xFFFF,
	LowWaterMaxValue	= 2,

	SendDataInitiatorMode	= 0xC00,
	SendInitiatorConf	= 0x0C08,
	SendStats		= 1<<0,
	SendInitiatorMask	= 0x0C0C,

	SendDataCompletionMode = 0x1000,
	SendBDSelectorMode	= 0x1400,
	SendBDInitiatorMode	= 0x1800,
	SendBDCompletionMode	= 0x1C00,
	
	RxListPlacementMode	= 0x2000,
	RxListPlacement		= 0x2010,
	RxListPlacementConf	= 0x2014,
	RxStats			= 1<<0,
	RxListPlacementMask	= 0x2018,
	
	RxDataBDInitiatorMode	= 0x2400,
	RxBDHostAddr		= 0x2450,
	RxBDFlags		= 0x2458,
	RxBDNIC		= 0x245C,
	RxDataCompletionMode	= 0x2800,
	RxBDInitiatorMode	= 0x2C00,
	RxBDRepl		= 0x2C18,
	
	RxBDCompletionMode	= 0x3000,
	HostCoalMode		= 0x3C00,
	HostCoalRxTicks		= 0x3C08,
	HostCoalSendTicks	= 0x3C0C,
	RxMaxCoalFrames	= 0x3C10,
	SendMaxCoalFrames	= 0x3C14,
	RxMaxCoalFramesInt	= 0x3C20,
	SendMaxCoalFramesInt	= 0x3C24,
	StatusBlockHostAddr	= 0x3C38,
	FlowAttention		= 0x3C48,

	MemArbiterMode		= 0x4000,
	
	BufferManMode		= 0x4400,
	
	MBUFLowWater		= 0x4414,
	MBUFHighWater		= 0x4418,
	
	ReadDMAMode		= 0x4800,
	ReadDMAStatus		= 0x4804,
	WriteDMAMode		= 0x4C00,
	WriteDMAStatus		= 0x4C04,
	
	RISCState		= 0x5004,
	FTQReset		= 0x5C00,
	MSIMode		= 0x6000,
	
	ModeControl		= 0x6800,
	ByteWordSwap		= 1<<4 | 1<<5 | 1<<2, // | 1<<1,
	HostStackUp		= 1<<16,
	HostSendBDs		= 1<<17,
	InterruptOnMAC		= 1<<26,
	
	MiscConf		= 0x6804,
	CoreClockBlocksReset	= 1<<0,
	GPHYPwrdnOverride	= 1<<26,
	DisableGRCRstOnPpcie	= 1<<29,
	TimerMask		= ~0xFF,
	TimerValue		= 65<<1,
	MiscLocalControl		= 0x6808,
	InterruptOnAttn		= 1<<3,
	AutoSEEPROM		= 1<<24,
	
	SwArbitration		= 0x7020,
	SwArbitSet1		= 1<<1,
	SwArbitWon1		= 1<<9,
	Pcitlplpl			= 0x7C00,	/* "lower 1k of the pcie pl regs" ?? */

	PhyAuxControl		= 0x18,
	PhyIntStatus		= 0x1A,
	PhyIntMask		= 0x1B,
	
	Updated			= 1<<0,
	LinkStateChange		= 1<<1,
	Error			= 1<<2,
	
	PacketEnd		= 1<<2,
	FrameError		= 1<<10,
};

enum {
	b5722,
	b5751,
	b5754,
	b5755,
	b5756,
	b5782,
	b5787,
	b5906,
	Nctlrtype,
};

typedef struct Ctlrtype Ctlrtype;
struct Ctlrtype {
	int	mtu;
	int	flag;
	char	*name;
};

static Ctlrtype cttab[Nctlrtype] = {
[b5722]	1514,	0,	"b5722",
[b5751]	1514,	0,	"b5751",
[b5754]	1514,	0,	"b5754",
[b5755]	1514,	0,	"b5755",
[b5756]	1514,	0,	"b5756",
[b5782]	1514,	0,	"b5782",
[b5787]	1514,	0,	"b5787",
[b5906]	1514,	0,	"b5906",
};

#define csr32(c, r)	((c)->nic[(r)/4])

static Ctlr *bcmhead;
static int debug=1;

static char*
cname(Ctlr *c)
{
	return cttab[c->type].name;
}

static long
bcmifstat(Ether *edev, void *a, long n, ulong offset)
{
	char *s, *p, *e;
	Ctlr *c;

	c = edev->ctlr;
	p = s = malloc(READSTR);
	e = p + READSTR;

	p = seprint(p, e, "nobuf	%ud\n", c->nobuf);
	p = seprint(p, e, "partial	%ud\n", c->partial);
	p = seprint(p, e, "rxerr	%ud\n", c->rxerr);
	p = seprint(p, e, "qfull	%ud\n", c->qfull);
	p = seprint(p, e, "dmaerr	%ud\n", c->dmaerr);
	p = seprint(p, e, "type: %s\n", cname(c));

	USED(p);
	n = readstr(offset, a, n, s);
	free(s);

	return n;
}

enum {
	Phybusy		= 1<<29,
	Phyrdfail		= 1<<28,
	Phyrd		= 1<<27,
	Phywr		= 1<<26,
};
Lock miilock;

static uint
miiwait(Ctlr *ctlr)
{
	uint i, v;

	for(i = 0; i < 100; i += 5){
		microdelay(10);
		v = csr32(ctlr, MIComm);
		if((v & Phybusy) == 0){
			microdelay(5);
			return csr32(ctlr, MIComm);
		}
		microdelay(5);
	}
	print("#l%d: bcm: miiwait: timeout\n", ctlr->ether->ctlrno);
	return ~0;
}

static int
miir(Ctlr *ctlr, int r)
{
	uint v, phyno;

	phyno = 1;
	lock(&miilock);
	csr32(ctlr, MIComm) = r<<16 | phyno<<21 | Phyrd | Phybusy;
	v = miiwait(ctlr);
	unlock(&miilock);
	if(v == ~0)
		return -1;
	if(v & Phyrdfail){
		print("#l%d: bcm: miir: fail\n", ctlr->ether->ctlrno);
		return -1;
	}
	return v & 0xffff;
}

static int
miiw(Ctlr *ctlr, int r, int v)
{
	uint phyno, w;

	phyno = 1;
	lock(&miilock);
	csr32(ctlr, MIComm) = r<<16 | v&0xffff | phyno<<21 | Phywr | Phybusy;
	w = miiwait(ctlr);
	unlock(&miilock);
	if(w == ~0)
		return -1;
	return 0;
}

static void
checklink(Ether *edev)
{
	uint i;
	Ctlr *ctlr;

	ctlr = edev->ctlr;
	miir(ctlr, Bmsr); /* read twice for current status as per 802.3 */
	if(!(miir(ctlr, Bmsr) & BmsrLs)) {
		edev->link = 0;
		edev->mbps = 1000;
		ctlr->duplex = 1;
		dprint("bcm: no link\n");
		goto out;
	}
	edev->link = 1;
	while((miir(ctlr, Bmsr) & BmsrAnc) == 0)
		;
	i = miir(ctlr, Mssr);
	if(i & (Mssr1000THD | Mssr1000TFD)) {
		edev->mbps = 1000;
		ctlr->duplex = (i & Mssr1000TFD) != 0;
	} else if(i = miir(ctlr, Anlpar), i & (AnaTXHD | AnaTXFD)) {
		edev->mbps = 100;
		ctlr->duplex = (i & AnaTXFD) != 0;
	} else if(i & (Ana10HD | Ana10FD)) {
		edev->mbps = 10;
		ctlr->duplex = (i & Ana10FD) != 0;
	} else {
		edev->link = 0;
		edev->mbps = 1000;
		ctlr->duplex = 1;
		dprint("bcm: link partner supports neither 10/100/1000 Mbps\n");
		goto out;
	}
	dprint("bcm: %d Mbps link, %s duplex\n", edev->mbps, ctlr->duplex ? "full" : "half");
out:
	if(ctlr->duplex)
		csr32(ctlr, MACMode) &= ~MACHalfDuplex;
	else
		csr32(ctlr, MACMode) |= MACHalfDuplex;
	if(edev->mbps >= 1000)
		csr32(ctlr, MACMode) = (csr32(ctlr, MACMode) & MACPortMask) | MACPortGMII;
	else
		csr32(ctlr, MACMode) = (csr32(ctlr, MACMode) & MACPortMask) | MACPortMII;
	csr32(ctlr, MACEventStatus) |= (1<<4) | (1<<3); /* undocumented bits (sync and config changed) */
}

static uint*
currentrecvret(Ctlr *ctlr)
{
	if(ctlr->recvreti == (ctlr->status[4] & 0xFFFF))
		return 0;
	return ctlr->recvret + ctlr->recvreti * 8;
}

static void
consumerecvret(Ctlr *ctlr)
{
	ctlr->recvreti = ctlr->recvreti+1 & RxRetRingLen-1;
	csr32(ctlr, RxBDRetRingIdx) = ctlr->recvreti;
}

static int
replenish(Ctlr *ctlr)
{
	uint incr;
	u32int *next;
	Block *bp;
	
	incr = (ctlr->recvprodi + 1) & (RxProdRingLen - 1);
	if(incr == (ctlr->status[2] >> 16))
		return -1;
	bp = iallocb(Rbsz);
	if(bp == nil) {
		/* iallocb never fails.  this code is unnecessary */
		dprint("bcm: out of memory for receive buffers\n");
		ctlr->nobuf++;
		return -1;
	}
	next = ctlr->recvprod + ctlr->recvprodi * 8;
	memset(next, 0, 32);
	next[0] = Pciwaddrh(bp->rp);
	next[1] = Pciwaddrl(bp->rp);
	next[2] = Rbsz;
	next[7] = ctlr->recvprodi;
	ctlr->rxs[ctlr->recvprodi] = bp;
	coherence();
	csr32(ctlr, RxProdBDRingIdx) = ctlr->recvprodi = incr;
	return 0;
}

static void
bcmreceive(Ether *edev)
{
	uint len;
	u32int *pkt;
	Ctlr *ctlr;
	Block *bp;
	
	ctlr = edev->ctlr;
	for(; pkt = currentrecvret(ctlr); replenish(ctlr), consumerecvret(ctlr)) {
		bp = ctlr->rxs[pkt[7]];
		len = pkt[2] & 0xFFFF;
		bp->wp = bp->rp + len;
		if((pkt[3] & PacketEnd) == 0){
			dprint("bcm: partial frame received -- shouldn't happen\n");
			ctlr->partial++;
			freeb(bp);
			continue;
		}
		if(pkt[3] & FrameError){
			ctlr->rxerr++;
			freeb(bp);
			continue;
		}
		etheriq(edev, bp, 1);
	}
}

static void
bcmtransclean(Ether *edev)
{
	Ctlr *ctlr;
	
	ctlr = edev->ctlr;
	ilock(&ctlr->txlock);
	while(ctlr->sendcleani != (ctlr->status[4] >> 16)) {
		freeb(ctlr->sends[ctlr->sendcleani]);
		ctlr->sends[ctlr->sendcleani] = nil;
		ctlr->sendcleani = (ctlr->sendcleani + 1) & (SendRingLen - 1);
	}
	iunlock(&ctlr->txlock);
}

static void
bcmtransmit(Ether *edev)
{
	uint incr;
	u32int *next;
	Ctlr *ctlr;
	Block *bp;
	
	ctlr = edev->ctlr;
	ilock(&ctlr->txlock);
	for(;;){
		incr = (ctlr->sendri + 1) & (SendRingLen - 1);
		if(incr == ctlr->sendcleani) {
			dprint("bcm: send queue full\n");
			ctlr->qfull++;
			break;
		}
		bp = qget(edev->oq);
		if(bp == nil)
			break;
		next = ctlr->sendr + ctlr->sendri * 4;
		next[0] = Pciwaddrh(bp->rp);
		next[1] = Pciwaddrl(bp->rp);
		next[2] = (BLEN(bp) << 16) | PacketEnd;
		next[3] = 0;
		ctlr->sends[ctlr->sendri] = bp;
		coherence();
		csr32(ctlr, SendBDRingHostIdx) = ctlr->sendri = incr;
	}
	iunlock(&ctlr->txlock);
}

static void
bcmerror(Ether *edev)
{
	Ctlr *ctlr;
	
	ctlr = edev->ctlr;
	if(csr32(ctlr, FlowAttention)) {
		if(csr32(ctlr, FlowAttention) & 0xf8ff8080)
			print("bcm: fatal error %#.8ux", csr32(ctlr, FlowAttention));
		csr32(ctlr, FlowAttention) = 0;
	}
	csr32(ctlr, MACEventStatus) = 0; /* worth ignoring */
	if(csr32(ctlr, ReadDMAStatus) || csr32(ctlr, WriteDMAStatus)) {
		dprint("bcm: DMA error\n");
		ctlr->dmaerr++;
		csr32(ctlr, ReadDMAStatus) = 0;
		csr32(ctlr, WriteDMAStatus) = 0;
	}
	if(csr32(ctlr, RISCState)) {
		if(csr32(ctlr, RISCState) & 0x78000403)
			print("bcm: RISC halted %#.8ux", csr32(ctlr, RISCState));
		csr32(ctlr, RISCState) = 0;
	}
}

static void
bcminterrupt(Ureg*, void *arg)
{
	u32int status, tag, dummy;
	Ether *edev;
	Ctlr *ctlr;
	
	edev = arg;
	ctlr = edev->ctlr;
	ilock(&ctlr->imlock);
	dummy = csr32(ctlr, InterruptMailbox);
	USED(dummy);
	csr32(ctlr, InterruptMailbox) = 1;
	status = ctlr->status[0];
	tag = ctlr->status[1];
	ctlr->status[0] = 0;
	if(status & Error)
		bcmerror(edev);
	if(status & LinkStateChange)
		checklink(edev);
	if(0)
		iprint("bcm: interrupt %.8ux %.8ux\n", ctlr->status[2], ctlr->status[4]);
	bcmreceive(edev);
	bcmtransclean(edev);
	bcmtransmit(edev);
	csr32(ctlr, InterruptMailbox) = tag << 24;
	iunlock(&ctlr->imlock);
}

static void
mem32w(Ctlr *c, uint r, uint v)
{
	pcicfgw32(c->pdev, Memwind, r);
	pcicfgw32(c->pdev, MemwindData, v);
}

static u32int
mem32r(Ctlr *c, uint r)
{
	u32int v;

	pcicfgw32(c->pdev, Memwind, r);
	v = pcicfgr32(c->pdev, MemwindData);
	pcicfgw32(c->pdev, Memwind, 0);
	return v;
}

static int
bcmµwait(Ctlr *ctlr, uint to, uint r, uint m, uint v)
{
	int i;

	for(i = 0;; i += 100){
		if((csr32(ctlr, r) & m) == v)
			return 0;
		if(i == to /* µs */)
			return -1;
		microdelay(100);
	}
}

static int
bcminit(Ether *edev)
{
	uint i;
	u32int j;
	Ctlr *ctlr;
	
	ctlr = edev->ctlr;
	dprint("bcm: reset\n");
	/* initialization procedure according to the datasheet */
	csr32(ctlr, MiscHostCtl) |= MaskPCIInt | ClearIntA | WordSwap | IndirAccessEn;
	csr32(ctlr, SwArbitration) |= SwArbitSet1;
	if(bcmµwait(ctlr, 2000, SwArbitration, SwArbitWon1, SwArbitWon1) == -1){
		print("bcm: arbiter failed to respond\n");
		return -1;
	}
	csr32(ctlr, MemArbiterMode) |= Enable;
	csr32(ctlr, MiscHostCtl) = WordSwap | IndirAccessEn | PCIStateRegEn | EnableClockCtl
		| MaskPCIInt | ClearIntA;
	csr32(ctlr, Memwind) = 0;
	mem32w(ctlr, Fwmbox, Fwmagic);
	csr32(ctlr, MiscConf) |= GPHYPwrdnOverride | DisableGRCRstOnPpcie | CoreClockBlocksReset;
	delay(100);
	pcicfgw32(ctlr->pdev, PciPCR, ctlr->pdev->pcr);	/* restore pci bits lost */
	csr32(ctlr, MiscHostCtl) |= MaskPCIInt | ClearIntA;
	csr32(ctlr, MemArbiterMode) |= Enable;
	csr32(ctlr, MiscHostCtl) |= WordSwap | IndirAccessEn | PCIStateRegEn | EnableClockCtl | TaggedStatus;
	csr32(ctlr, ModeControl) |= ByteWordSwap;
	csr32(ctlr, MACMode) = (csr32(ctlr, MACMode) & MACPortMask) | MACPortGMII;
	delay(40);
	for(i = 0;; i += 100){
		if(mem32r(ctlr, Fwmbox) == ~Fwmagic)
			break;
		if(i == 20*10000 /* µs */){
			print("bcm: fw failed to respond %#.8ux\n", mem32r(ctlr, Fwmbox));
			break; //return -1;
		}
		microdelay(100);
	}
	/*
	 * there appears to be no justification for setting these bits in any driver
	 * i can find.  nor to i have a datasheet that recommends this.  - quanstro
	 * csr32(ctlr, Pcitlplpl) |= 1<<25 | 1<<29;
	 */
	memset(ctlr->status, 0, 20);
	csr32(ctlr, Dmarwctl) = (csr32(ctlr, Dmarwctl) & DMAWaterMask) | DMAWaterValue;
	csr32(ctlr, ModeControl) |= HostSendBDs | HostStackUp | InterruptOnMAC;
	csr32(ctlr, MiscConf) = (csr32(ctlr, MiscConf) & TimerMask) | TimerValue;
	csr32(ctlr, MBUFLowWater) = 0x20;
	csr32(ctlr, MBUFHighWater) = 0x60;
	csr32(ctlr, LowWaterMax) = (csr32(ctlr, LowWaterMax) & LowWaterMaxMask) | LowWaterMaxValue;
	csr32(ctlr, BufferManMode) |= Enable | Attn;
	if(bcmµwait(ctlr, 2000, BufferManMode, Enable, Enable) == -1){
		print("bcm: failed to enable buffers\n");
		return -1;
	}
	csr32(ctlr, FTQReset) = ~0;
	csr32(ctlr, FTQReset) = 0;
	if(bcmµwait(ctlr, 2000, FTQReset, ~0, 0) == -1){
		print("bcm: failed to bring ftq out of reset\n");
		return -1;
	}
	csr32(ctlr, RxBDHostAddr) = Pciwaddrh(ctlr->recvprod);
	csr32(ctlr, RxBDHostAddr + 4) = Pciwaddrl(ctlr->recvprod);
	csr32(ctlr, RxBDFlags) = RxProdRingLen << 16;
	csr32(ctlr, RxBDNIC) = 0x6000;
	csr32(ctlr, RxBDRepl) = 25;
	csr32(ctlr, SendBDRingHostIdx) = 0;
	csr32(ctlr, SendBDRingHostIdx+4) = 0;
	mem32w(ctlr, SendRCB, Pciwaddrh(ctlr->sendr));
	mem32w(ctlr, SendRCB + 4, Pciwaddrl(ctlr->sendr));
	mem32w(ctlr, SendRCB + 8, SendRingLen << 16);
	mem32w(ctlr, SendRCB + 12, 0x4000);
	for(i=1; i<4; i++)
		mem32w(ctlr, RxRetRCB + i * 0x10 + 8, 2);
	mem32w(ctlr, RxRetRCB, Pciwaddrh(ctlr->recvret));
	mem32w(ctlr, RxRetRCB + 4, Pciwaddrl(ctlr->recvret));
	mem32w(ctlr, RxRetRCB + 8, RxRetRingLen << 16);
	csr32(ctlr, RxProdBDRingIdx) = 0;
	csr32(ctlr, RxProdBDRingIdx+4) = 0;
	/* this delay is not in the datasheet, but necessary */
	delay(1);
	i = csr32(ctlr, MACAddress);
	j = edev->ea[0] = i >> 8;
	j += edev->ea[1] = i;
	i = csr32(ctlr, MACAddress + 4);
	j += edev->ea[2] = i >> 24;
	j += edev->ea[3] = i >> 16;
	j += edev->ea[4] = i >> 8;
	j += edev->ea[5] = i;
	csr32(ctlr, RandomBackoff) = j & 0x3FF;
	csr32(ctlr, RxMTU) = Rbsz;
	csr32(ctlr, TxMACLengths) = 0x2620;
	csr32(ctlr, RxListPlacement) = 1<<3; /* one list */
	csr32(ctlr, RxListPlacementMask) = 0xFFFFFF;
	csr32(ctlr, RxListPlacementConf) |= RxStats;
	csr32(ctlr, SendInitiatorMask) = 0xFFFFFF;
	csr32(ctlr, SendInitiatorConf) |= SendStats;
	csr32(ctlr, HostCoalMode) = 0;
	if(bcmµwait(ctlr, 2000, HostCoalMode, ~0, 0) == -1){
		print("bcm: failed to unset coalescing\n");
		return -1;
	}
	csr32(ctlr, HostCoalRxTicks) = 150;
	csr32(ctlr, HostCoalSendTicks) = 150;
	csr32(ctlr, RxMaxCoalFrames) = 10;
	csr32(ctlr, SendMaxCoalFrames) = 10;
	csr32(ctlr, RxMaxCoalFramesInt) = 0;
	csr32(ctlr, SendMaxCoalFramesInt) = 0;
	csr32(ctlr, StatusBlockHostAddr) = Pciwaddrh(ctlr->status);
	csr32(ctlr, StatusBlockHostAddr + 4) = Pciwaddrl(ctlr->status);
	csr32(ctlr, HostCoalMode) |= Enable;
	csr32(ctlr, RxBDCompletionMode) |= Enable | Attn;
	csr32(ctlr, RxListPlacementMode) |= Enable;
	csr32(ctlr, MACMode) |= MACEnable;
	csr32(ctlr, MiscLocalControl) |= InterruptOnAttn | AutoSEEPROM;
	csr32(ctlr, InterruptMailbox) = 0;
	csr32(ctlr, WriteDMAMode) |= 0x200003fe; /* pulled out of my nose */
	csr32(ctlr, ReadDMAMode) |= 0x3fe;
	csr32(ctlr, RxDataCompletionMode) |= Enable | Attn;
	csr32(ctlr, SendDataCompletionMode) |= Enable;
	csr32(ctlr, SendBDCompletionMode) |= Enable | Attn;
	csr32(ctlr, RxBDInitiatorMode) |= Enable | Attn;
	csr32(ctlr, RxDataBDInitiatorMode) |= Enable | (1<<4);
	csr32(ctlr, SendDataInitiatorMode) |= Enable;
	csr32(ctlr, SendBDInitiatorMode) |= Enable | Attn;
	csr32(ctlr, SendBDSelectorMode) |= Enable | Attn;
	ctlr->recvprodi = 0;
	while(replenish(ctlr) >= 0)
		;
	csr32(ctlr, TxMACMode) |= Enable;
	csr32(ctlr, RxMACMode) |= Enable;
	csr32(ctlr, Pwrctlstat) &= ~3;
	csr32(ctlr, MIStatus) |= 1<<0;
	csr32(ctlr, MACEventEnable) = 0;
	csr32(ctlr, MACEventStatus) |= (1<<12);
	csr32(ctlr, MIMode) = 0xC0000;		/* set base mii clock */
	microdelay(40);

	if(0){
		/* bug (ours): can't reset phy without dropping into 100mbit mode */
		miiw(ctlr, Bmcr, BmcrR);
		for(i = 0;; i += 100){
			if((miir(ctlr, Bmcr) & BmcrR) == 0)
				break;
			if(i == 10000 /* µs */){
				print("bcm: phy reset failure\n");
				return -1;
			}
			microdelay(100);
		}
	}
	miiw(ctlr, Bmcr, BmcrAne | BmcrRan);

	miiw(ctlr, PhyAuxControl, 2);
	miir(ctlr, PhyIntStatus);
	miir(ctlr, PhyIntStatus);
	miiw(ctlr, PhyIntMask, ~(1<<1));
	csr32(ctlr, MACEventEnable) |= 1<<12;
	for(i = 0; i < 4; i++)
		csr32(ctlr, MACHash + 4*i) = ~0;
	for(i = 0; i < 8; i++)
		csr32(ctlr, RxRules + 8 * i) = 0;
	csr32(ctlr, RxRulesConf) = 1 << 3;
	csr32(ctlr, MSIMode) |= Enable;
	csr32(ctlr, MiscHostCtl) &= ~(MaskPCIInt | ClearIntA);
	dprint("bcm: reset: fin\n");
	return 0;
}

static int
didtype(Pcidev *p)
{
	if(p->vid != 0x14e4)
		return -1;
	
	switch(p->did){
	default:
		return -1;
	case 0x165a:		/* 5722 gbe */
		return b5722;
	case 0x1670:		/* ?? */
		return b5751;
	case 0x1672:		/* 5754m */
		return b5754;
	case 0x1673:		/* 5755m gbe */
		return b5755;
	case 0x1674:		/* 5756me gbe */
		return b5756;
	case 0x1677:		/* 5751 gbe */
		return b5751;
	case 0x167a:		/* 5754 gbe */
		return b5754;
	case 0x167b:		/* 5755 gbe */
		return b5755;
	case 0x1693:		/* 5787m gbe */
		return b5787;
	case 0x1696:		/* 5782 gbe; steve */
		return b5782;
	case 0x169b:		/* 5787 gbe */
		return b5787;
	case 0x1712:		/* 5906 fast */
	case 0x1713:		/* 5906m fast */
		return b5906;
	case 0x167d:		/* 5751m gbe */
	case 0x167e:		/* 5751f fast */
		return b5751;
	}
}

static void
bcmpci(void)
{
	int type;
	void *mem;
	Ctlr *ctlr, **xx;
	Pcidev *p;

	xx = &bcmhead;
	for(p = nil; p = pcimatch(p, 0, 0); ) {
		if(p->ccrb != 2 || p->ccru != 0 || (type = didtype(p)) == -1)
			continue;
		pcisetbme(p);
		pcisetpms(p, 0);
		ctlr = malloc(sizeof(Ctlr));
		if(ctlr == nil)
			continue;
		ctlr->type = type;
		ctlr->port = p->mem[0].bar & ~0x0F;
		mem = vmap(ctlr->port, p->mem[0].size);
		if(mem == nil) {
			print("bcm: can't map %#p\n", (uvlong)ctlr->port);
			free(ctlr);
			continue;
		}
		ctlr->pdev = p;
		ctlr->nic = mem;
		ctlr->status = mallocalign(20, 16, 0, 0);
		ctlr->recvprod = mallocalign(32 * RxProdRingLen, 16, 0, 0);
		ctlr->recvret = mallocalign(32 * RxRetRingLen, 16, 0, 0);
		ctlr->sendr = mallocalign(16 * SendRingLen, 16, 0, 0);
		ctlr->sends = malloc(sizeof *ctlr->sends * SendRingLen);
		ctlr->rxs = malloc(sizeof *ctlr->sends * SendRingLen);
		*xx = ctlr;
		xx = &ctlr->next;
	}
}

static void
bcmpromiscuous(void* arg, int on)
{
	Ctlr *ctlr;
	
	ctlr = ((Ether*)arg)->ctlr;
	if(on)
		csr32(ctlr, RxMACMode) |= 1<<8;
	else
		csr32(ctlr, RxMACMode) &= ~(1<<8);
}

static void
bcmmulticast(void*, uchar*, int)
{
}

static int
bcmpnp(Ether* edev)
{
	Ctlr *ctlr;
	static int done;

	if(done == 0){
		bcmpci();
		done = 1;
	}
	
redux:
	for(ctlr = bcmhead; ; ctlr = ctlr->next) {
		if(ctlr == nil)
			return -1;
		if(ctlr->active)
			continue;
		if(edev->port == 0 || edev->port == ctlr->port) {
			ctlr->active = 1;
			break;
		}
	}

	ctlr->ether = edev;
	edev->ctlr = ctlr;
	edev->port = ctlr->port;
	edev->irq = ctlr->pdev->intl;
	edev->tbdf = ctlr->pdev->tbdf;
	edev->interrupt = bcminterrupt;
	edev->ifstat = bcmifstat;
	edev->transmit = bcmtransmit;
	edev->multicast = bcmmulticast;
	edev->promiscuous = bcmpromiscuous;
	edev->arg = edev;
	edev->mbps = 1000;

	if(bcminit(edev) == -1)
		goto redux;
	return 0;
}

void
etherbcmlink(void)
{
	addethercard("bcm57xx", bcmpnp);
}
