/*
 * from 9front ../port/usbxhci.c
 */

#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"pci.h"
#include	"../port/error.h"
#include	"usb.h"

extern void dmaflush(int, void*, ulong);

enum {
	/* Capability Registers */
	CAPLENGTH	= 0x00/4,	// 1
	HCIVERSION	= 0x02/4,	// 2
	HCSPARAMS1	= 0x04/4,
	HCSPARAMS2	= 0x08/4,
	HCSPARAMS3	= 0x0C/4,

	HCCPARAMS	= 0x10/4,
		AC64	= 1<<0,
		BNC	= 1<<1,
		CSZ	= 1<<2,
		PPC	= 1<<3,
		PIND	= 1<<4,
		LHRC	= 1<<5,
		LTC	= 1<<6,
		NSS	= 1<<7,

	DBOFF		= 0x14/4,
	RTSOFF		= 0x18/4,

	HCCPARAMS2	= 0x1C/4,

	/* Operational Registers */
	USBCMD		= 0x00/4,	/* USB Command Register */
		RUNSTOP	= 1<<0,		/* Run/Stop - RW */
		HCRST	= 1<<1,		/* Host Controller Reset - RW */
		INTE	= 1<<2,		/* Interrupter Enable - RW */
		HSEE	= 1<<3,		/* Host System Error Enable - RW */
		LHCRST	= 1<<7,		/* Light Host Controller Reset - RO/RW */
		CSS	= 1<<8,		/* Controller Save State - RW */
		CRS	= 1<<9,		/* Controller Restore State - RW */
		EWE	= 1<<10,	/* Enable Wrap Event - RW */
		EU3S	= 1<<11,	/* Enable U3 MFINDEX Stop - RW */

	USBSTS		= 0x04/4,	/* USB Status Register */
		HCH	= 1<<0,		/* HCHalted - RO */
		HSE	= 1<<2,		/* Host System Error - RW1C */
		EINT	= 1<<3,		/* Event Interrupt - RW1C */
		PCD	= 1<<4,		/* Port Change Detect - RW1C */
		SSS	= 1<<8,		/* Save State Status - RO */
		RSS	= 1<<9,		/* Restore State Status - RO */
		SRE	= 1<<10,	/* Save/Restore Error - RW1C */
		CNR	= 1<<11,	/* Controller Not Ready - RO */
		HCE	= 1<<12,	/* Host Controller Error - RO */

	PAGESIZE	= 0x08/4,	/* Page Size - RO */

	DNCTRL		= 0x14/4,	/* Device Notification Control Register - RW */

	CRCR		= 0x18/4,	/* Command Ring Control Register - RW */
		RCS	= 1<<0,		/* Ring Cycle State - RW */
		CS	= 1<<1,		/* Command Stop - RW1S */
		CA	= 1<<2,		/* Command Abort - RW1S */
		CRR	= 1<<3,		/* Command Ring Running - RO */

	DCBAAP		= 0x30/4,	// 8

	CONFIG		= 0x38/4,	/* Configure Register (MaxSlotEn[7:0]) */

	/* Port Register Set */
	PORTSC		= 0x00/4,	/* Port status and Control Register */
		CCS	= 1<<0,		/* Current Connect Status - ROS */
		PED	= 1<<1,		/* Port Enable/Disabled - RW1CS */
		OCA	= 1<<3,		/* Over-current Active - RO */
		PR	= 1<<4,		/* Port Reset - RW1S */
		PLS	= 15<<5,	/* Port Link State - RWS */
		PP	= 1<<9,		/* Port Power - RWS */
		PS	= 15<<10,	/* Port Speed - ROS */
		PIC	= 3<<14,	/* Port Indicator Control - RWS */
		LWS	= 1<<16,	/* Port Link Write Strobe - RW */
		CSC	= 1<<17,	/* Connect Status Change - RW1CS */
		PEC	= 1<<18,	/* Port Enabled/Disabled Change - RW1CS */
		WRC	= 1<<19,	/* Warm Port Reset Change - RW1CS */
		OCC	= 1<<20,	/* Over-current Change - RW1CS */
		PRC	= 1<<21,	/* Port Reset Change - RW1CS */
		PLC	= 1<<22,	/* Port Link State Change - RW1CS */
		CEC	= 1<<23,	/* Port Config Error Change - RW1CS */
		CAS	= 1<<24,	/* Cold Attach Status - RO */
		WCE	= 1<<25,	/* Wake on Connect Enable - RWS */
		WDE	= 1<<26,	/* Wake on Disconnect Enable - RWS */
		WOE	= 1<<27,	/* Wake on Over-current Enable - RWS */
		DR	= 1<<30,	/* Device Removable - RO */
		WPR	= 1<<31,	/* Warm Port Reset - RW1S */

	PORTPMSC	= 0x04/4,
	PORTLI		= 0x08/4,

	/* Host Controller Runtime Register */
	MFINDEX		= 0x0000/4,	/* Microframe Index */
	IR0		= 0x0020/4,	/* Interrupt Register Set 0 */

	/* Interrupter Registers */
	IMAN		= 0x00/4,	/* Interrupter Management */
	IMOD		= 0x04/4,	/* Interrupter Moderation */
	ERSTSZ		= 0x08/4,	/* Event Ring Segment Table Size */
	ERSTBA		= 0x10/4,	/* Event Ring Segment Table Base Address */
	ERDP		= 0x18/4,	/* Event Ring Dequeue Pointer */

	/* TRB flags */
	TR_ENT		= 1<<1,
	TR_ISP		= 1<<2,
	TR_NS		= 1<<3,
	TR_CH		= 1<<4,
	TR_IOC		= 1<<5,
	TR_IDT		= 1<<6,
	TR_BEI		= 1<<9,
	
	/* TRB types */
	TR_RESERVED	= 0<<10,
	TR_NORMAL	= 1<<10,
	TR_SETUPSTAGE	= 2<<10,
	TR_DATASTAGE	= 3<<10,
	TR_STATUSSTAGE	= 4<<10,
	TR_ISOCH	= 5<<10,
	TR_LINK		= 6<<10,
	TR_EVENTDATA	= 7<<10,
	TR_NOOP		= 8<<10,

	CR_ENABLESLOT	= 9<<10,
	CR_DISABLESLOT	= 10<<10,
	CR_ADDRESSDEV	= 11<<10,
	CR_CONFIGEP	= 12<<10,
	CR_EVALCTX	= 13<<10,
	CR_RESETEP	= 14<<10,
	CR_STOPEP	= 15<<10,
	CR_SETTRDQP	= 16<<10,
	CR_RESETDEV	= 17<<10,
	CR_FORCECMD	= 18<<10,
	CR_NEGBW	= 19<<10,
	CR_SETLAT	= 20<<10,
	CR_GETPORTBW	= 21<<10,
	CR_FORCEHDR	= 22<<10,
	CR_NOOP		= 23<<10,

	ER_TRANSFER	= 32<<10,
	ER_CMDCOMPL	= 33<<10,
	ER_PORTSC	= 34<<10,
	ER_BWREQ	= 35<<10,
	ER_DOORBELL	= 36<<10,
	ER_HCE		= 37<<10,
	ER_DEVNOTE	= 38<<10,
	ER_MFINDEXWRAP	= 39<<10,
};

typedef struct Ctlr Ctlr;
typedef struct Wait Wait;
typedef struct Ring Ring;
typedef struct Slot Slot;
typedef struct Epio Epio;
typedef struct Port Port;

struct Wait
{
	Wait	*next;
	Ring	*ring;
	u32int	*td;
	u32int	er[4];
	Rendez	*z;
};

struct Ring
{
	int	id;

	Slot	*slot;

	u32int	*base;

	u32int	mask;
	u32int	shift;

	u32int	rp;
	u32int	wp;

	u32int	*ctx;
	u32int	*doorbell;

	int	stopped;

	Wait	*pending;
	Lock;
};

struct Slot
{
	int	id;

	int	confval; // bConfigurationValue of SET_CONFIGURATION
	int	iface;	// bInterfaceNumber of SET_INTERFACE
	int	altc;	// bAlternateSetting of SET_INTERFACE

	Ctlr	*ctlr;
	Udev	*dev;

	u32int	*ibase;
	u32int	*obase;

	/* endpoint rings */
	int	nep;
	Ring	epr[32];
};

struct Port
{
	char	spec[4];
	int	proto;

	u32int	*reg;
};

struct Ctlr
{
	Pcidev	*pcidev;

	u32int	*mmio;

	u32int	*opr;	/* operational registers */
	u32int	*rts;	/* runtime registers */
	u32int	*dba;	/* doorbell array */

	u64int	*dcba;	/* device context base array */

	u64int	*sba;	/* scratchpad buffer array */
	void	*sbp;	/* scratchpad buffer pages */

	u32int	*erst[1];	/* event ring segment table */
	Ring	er[1];		/* event ring segment */
	Ring	cr[1];		/* command ring segment */
	QLock	cmdlock;

	u32int	µframe;

	QLock	slotlock;
	Slot	**slot;		/* slots by slot id */
	Port	*port;
	
	u32int	hccparams;

	int	csz;
	int	pagesize;
	int	nscratch;
	int	nintrs;
	int	nslots;

	Rendez	recover;	
	void	*active;
	uintptr	base;
};

struct Epio
{
	QLock;

	Ring	*ring;
	Block	*b;

	/* iso */
	u32int	frame;
	u32int	period;
	u32int	incr;
	u32int	tdsz;

	int	nleft;
};

static char Ebadlen[] = "bad usb request length";
static char Enotconfig[] = "usb endpoint not configured";
static char Erecover[] = "xhci controller needs reset";

static char*
ctlrcmd(Ctlr *ctlr, u32int c, u32int s, u64int p, u32int *er);

static void
setrptr(u32int *reg, u64int pa)
{
	coherence();
	reg[0] = pa;
	reg[1] = pa>>32;
}

static u32int
µframe(Ctlr *ctlr)
{
	u32int µ;
	do {
		µ = (ctlr->rts[MFINDEX] & (1<<14)-1) |
			(ctlr->µframe & ~((1<<14)-1));
	} while((int)(µ - ctlr->µframe) < 0);
	return µ;
}

static void
freering(Ring *r)
{
	if(r == nil)
		return;
	if(r->base != nil){
		dmaflush(0, r->base, 4*4<<r->shift);
		free(r->base);
	}
	memset(r, 0, sizeof(*r));
}

static Ring*
initring(Ring *r, int shift)
{
	r->id = 0;
	r->ctx = nil;
	r->slot = nil;
	r->doorbell = nil;
	r->pending = nil;
	r->stopped = 0;
	r->shift = shift;
	r->mask = (1<<shift)-1;
	r->rp = r->wp = 0;
	r->base = mallocalign(4*4<<shift, 64, 0, 64*1024);
	if(r->base == nil){
		freering(r);
		error(Enomem);
	}
	dmaflush(1, r->base, 4*4<<shift);
	return r;
}

static void
flushring(Ring *r)
{
	Rendez *z;
	Wait *w;

	while((w = r->pending) != nil){
		r->pending = w->next;
		w->next = nil;
		if((z = w->z) != nil){
			w->z = nil;
			wakeup(z);
		}
	}
}

static u64int
resetring(Ring *r)
{
	u64int pa;

	ilock(r);
	flushring(r);
	r->rp = r->wp;
	pa = PCIWADDR(&r->base[4*(r->wp & r->mask)]) | ((~r->wp>>r->shift) & 1);
	iunlock(r);

	return pa;
}

static u32int*
xecp(Ctlr *ctlr, uchar id, u32int *p)
{
	u32int x, *e;

	e = &ctlr->mmio[ctlr->pcidev->mem[0].size/4];
	if(p == nil){
		p = ctlr->mmio;
		x = ctlr->hccparams>>16;
	} else {
		assert(p < e);
		x = (*p>>8) & 255;
	}
	while(x != 0){
		p += x;
		if(p >= e)
			break;
		x = *p;
		if((x & 255) == id)
			return p;
		x >>= 8;
		x &= 255;
	}
	return nil;
}

static void
handoff(Ctlr *ctlr)
{
	u32int *r;
	int i;

	if((r = xecp(ctlr, 1, nil)) == nil)
		return;
	if(getconf("*noxhcihandoff") == nil){
		r[0] |= 1<<24;		/* request ownership */
		for(i = 0; (r[0] & (1<<16)) != 0 && i<100; i++)
			tsleep(&up->sleep, return0, nil, 10);
	}
	/* disable SMI interrupts */
	r[1] &= 7<<1 | 255<<5 | 7<<17 | 7<<29;

	/* clear BIOS ownership in case of timeout */
	r[0] &= ~(1<<16);
}

static void
shutdown(Hci *hp)
{
	Ctlr *ctlr = hp->aux;
	int i;

	ctlr->opr[USBCMD] = 0;
	for(i=0; (ctlr->opr[USBSTS] & HCH) == 0 && i < 10; i++)
		delay(10);
	pciintrdisable(ctlr->pcidev->tbdf, hp->interrupt, hp);
	pcidisable(ctlr->pcidev);
}

static void
release(Ctlr *ctlr)
{
	int i;

	freering(ctlr->cr);
	for(i=0; i<nelem(ctlr->er); i++){
		freering(&ctlr->er[i]);
		free(ctlr->erst[i]);
		ctlr->erst[i] = nil;
	}
	free(ctlr->port), ctlr->port = nil;
	free(ctlr->slot), ctlr->slot = nil;
	free(ctlr->dcba), ctlr->dcba = nil;
	free(ctlr->sba), ctlr->sba = nil;
	if(ctlr->sbp != nil){
		dmaflush(0, ctlr->sbp, ctlr->nscratch*ctlr->pagesize);
		free(ctlr->sbp);
		ctlr->sbp = nil;
	}
}

static void recover(void *arg);

static void
init(Hci *hp)
{
	Ctlr *ctlr;
	Port *pp;
	u32int *x;
	uchar *p;
	int i, j;

	ctlr = hp->aux;
	pcienable(ctlr->pcidev);
	if(ctlr->mmio[CAPLENGTH] == -1){
		pcidisable(ctlr->pcidev);
		error("controller vanished");
	}

	ctlr->opr = &ctlr->mmio[(ctlr->mmio[CAPLENGTH]&0xFF)/4];
	ctlr->dba = &ctlr->mmio[ctlr->mmio[DBOFF]/4];
	ctlr->rts = &ctlr->mmio[ctlr->mmio[RTSOFF]/4];

	ctlr->hccparams = ctlr->mmio[HCCPARAMS];
	handoff(ctlr);
	xhcireset(BUSBNO(hp->tbdf)<<20 | BUSDNO(hp->tbdf)<<15 | BUSFNO(hp->tbdf)<<12);

	for(i=0; (ctlr->opr[USBSTS] & CNR) != 0 && i<100; i++)
		tsleep(&up->sleep, return0, nil, 10);
	if(i == 100)
		print("%s: controller not ready, status %ux\n", hp->type, ctlr->opr[USBSTS]);

	ctlr->opr[USBCMD] = HCRST;
	delay(1);
	for(i=0; (ctlr->opr[USBSTS] & (CNR|HCH)) != HCH && i<100; i++)
		tsleep(&up->sleep, return0, nil, 10);
	if(i == 100)
		print("%s: controller not halted, status %ux\n", hp->type, ctlr->opr[USBSTS]);

	pcisetbme(ctlr->pcidev);
	pciintrenable(ctlr->pcidev->tbdf, hp->interrupt, hp);

	if(waserror()){
		shutdown(hp);
		release(ctlr);
		nexterror();
	}

	ctlr->csz = (ctlr->hccparams & CSZ) != 0;
	ctlr->pagesize = (ctlr->opr[PAGESIZE] & 0xFFFF) << 12;

	ctlr->nscratch = (ctlr->mmio[HCSPARAMS2] >> 27) & 0x1F | (ctlr->mmio[HCSPARAMS2] >> 16) & 0x3E0;
	ctlr->nintrs = (ctlr->mmio[HCSPARAMS1] >> 8) & 0x7FF;
	ctlr->nslots = (ctlr->mmio[HCSPARAMS1] >> 0) & 0xFF;

	hp->highspeed = 1;
	hp->superspeed = 0;
	hp->nports = (ctlr->mmio[HCSPARAMS1] >> 24) & 0xFF;
	ctlr->port = malloc(hp->nports * sizeof(Port));
	if(ctlr->port == nil)
		error(Enomem);
	for(i=0; i<hp->nports; i++)
		ctlr->port[i].reg = &ctlr->opr[0x400/4 + i*4];

	x = nil;
	while((x = xecp(ctlr, 2, x)) != nil){
		i = x[2]&255;
		j = (x[2]>>8)&255;
		while(j--){
			if(i < 1 || i > hp->nports)
				break;
			pp = &ctlr->port[i-1];
			pp->proto = x[0]>>16;
			memmove(pp->spec, &x[1], 4);
			if(memcmp(pp->spec, "USB ", 4) == 0 && pp->proto >= 0x0300)
				hp->superspeed |= 1<<(i-1);
			i++;
		}
	}

	ctlr->slot = malloc((1+ctlr->nslots)*sizeof(ctlr->slot[0]));
	ctlr->dcba = mallocalign((1+ctlr->nslots)*sizeof(ctlr->dcba[0]), 64, 0, ctlr->pagesize);
	if(ctlr->slot == nil || ctlr->dcba == nil)
		error(Enomem);
	if(ctlr->nscratch != 0){
		ctlr->sba = mallocalign(ctlr->nscratch*8, 64, 0, ctlr->pagesize);
		ctlr->sbp = mallocalign(ctlr->nscratch*ctlr->pagesize, ctlr->pagesize, 0, 0);
		if(ctlr->sba == nil || ctlr->sbp == nil)
			error(Enomem);
		for(i=0, p = ctlr->sbp; i<ctlr->nscratch; i++, p += ctlr->pagesize){
			memset(p, 0, ctlr->pagesize);
			ctlr->sba[i] = PCIWADDR(p);
		}
		dmaflush(1, ctlr->sbp, ctlr->nscratch*ctlr->pagesize);
		dmaflush(1, ctlr->sba, ctlr->nscratch*8);
		ctlr->dcba[0] = PCIWADDR(ctlr->sba);
	} else {
		ctlr->dcba[0] = 0;
	}
	for(i=1; i<=ctlr->nslots; i++)
		ctlr->dcba[i] = 0;

	ctlr->opr[CONFIG] = (ctlr->opr[CONFIG] & 0xFFFFFC00) | ctlr->nslots;	/* MaxSlotsEn */

	dmaflush(1, ctlr->dcba, (1+ctlr->nslots)*sizeof(ctlr->dcba[0]));
	setrptr(&ctlr->opr[DCBAAP], PCIWADDR(ctlr->dcba));

	initring(ctlr->cr, 8);		/* 256 entries */
	ctlr->cr->id = 0;
	ctlr->cr->doorbell = &ctlr->dba[0];
	setrptr(&ctlr->opr[CRCR], resetring(ctlr->cr));

	for(i=0; i<ctlr->nintrs; i++){
		u32int *irs = &ctlr->rts[IR0 + i*8];

		if(i >= nelem(ctlr->er)){
			irs[ERSTSZ] = 0;	/* disable ring */
			irs[IMAN] = 1;
			irs[IMOD] = 0;
			setrptr(&irs[ERSTBA], 0);
			setrptr(&irs[ERDP], 0);
			continue;
		}

		/* allocate and link into event ring segment table */
		initring(&ctlr->er[i], 8);	/* 256 entries */
		ctlr->erst[i] = mallocalign(4*4, 64, 0, 0);
		if(ctlr->erst[i] == nil)
			error(Enomem);
		*((u64int*)ctlr->erst[i]) = PCIWADDR(ctlr->er[i].base);
		ctlr->erst[i][2] = ctlr->er[i].mask+1;
		ctlr->erst[i][3] = 0;
		dmaflush(1, ctlr->erst[i], 4*4);

		irs[ERSTSZ] = 1;	/* just one segment */
		irs[IMAN] = 3;
		irs[IMOD] = 0;
		setrptr(&irs[ERSTBA], PCIWADDR(ctlr->erst[i]));
		setrptr(&irs[ERDP], PCIWADDR(ctlr->er[i].base) | (1<<3));
	}
	poperror();

	ctlr->µframe = 0;
	ctlr->opr[USBSTS] = ctlr->opr[USBSTS] & (HSE|EINT|PCD|SRE);
	coherence();

	ctlr->opr[USBCMD] = RUNSTOP|INTE|HSEE|EWE;
	for(i=0; (ctlr->opr[USBSTS] & (CNR|HCH)) != 0 && i<100; i++)
		tsleep(&up->sleep, return0, nil, 10);

	kproc("xhcirecover", recover, hp);
}

static int
needrecover(void *arg)
{
	Ctlr *ctlr = arg;
	return 	ctlr->er->stopped || 
		(ctlr->opr[USBSTS] & (HCH|HCE|HSE)) != 0;
}

static void
recover(void *arg)
{
	Hci *hp = arg;
	Ctlr *ctlr = hp->aux;

	while(waserror())
		;
	while(!needrecover(ctlr))
		tsleep(&ctlr->recover, needrecover, ctlr, 10);
	shutdown(hp);

	/*
	 * flush all transactions and wait until all devices have
	 * been detached by usbd.
	 */
	for(;;){
		int i, j, active;

		ilock(ctlr->cr);
		ctlr->cr->stopped = 1;
		flushring(ctlr->cr);
		iunlock(ctlr->cr);

		active = 0;
		qlock(&ctlr->slotlock);
		for(i=1; i<=ctlr->nslots; i++){
			Slot *slot = ctlr->slot[i];
			if(slot == nil)
				continue;
			active++;
			for(j=0; j < slot->nep; j++){
				Ring *ring = &slot->epr[j];
				if(ring->base == nil)
					continue;
				ilock(ring);
				ring->stopped = 1;
				flushring(ring);
				iunlock(ring);
			}
		}
		qunlock(&ctlr->slotlock);
		if(active == 0)
			break;

		tsleep(&up->sleep, return0, nil, 100);
	}

	qlock(&ctlr->slotlock);
	qlock(&ctlr->cmdlock);

	release(ctlr);
	if(waserror()) {
		print("xhci recovery failed: %s\n", up->errstr);
	} else {
		init(hp);
		poperror();
	}

	qunlock(&ctlr->cmdlock);
	qunlock(&ctlr->slotlock);

	pexit("", 1);
}

static void
dump(Hci *)
{
}

static void
queuetd(Ring *r, u32int c, u32int s, u64int p, Wait *w)
{
	u32int *td, x;

	x = r->wp++;
	if((x & r->mask) == r->mask){
		td = r->base + 4*(x & r->mask);
		*(u64int*)td = PCIWADDR(r->base);
		td[2] = 0;
		td[3] = ((~x>>r->shift)&1) | (1<<1) | TR_LINK;
		dmaflush(1, td, 4*4);
		x = r->wp++;
	}
	td = r->base + 4*(x & r->mask);
	if(w != nil){
		w->er[0] = w->er[1] = w->er[2] = w->er[3] = 0;
		w->ring = r;
		w->td = td;
		w->z = &up->sleep;

		ilock(r);
		w->next = r->pending;
		r->pending = w;
		iunlock(r);
	}
	coherence();
	*(u64int*)td = p;
	td[2] = s;
	td[3] = ((~x>>r->shift)&1) | c;
	dmaflush(1, td, 4*4);
}

static char *ccerrtab[] = {
[2]	"Data Buffer Error",
[3]	"Babble Detected Error",
[4]	"USB Transaction Error",
[5]	"TRB Error",
[6]	"Stall Error",
[7]	"Resume Error",
[8]	"Bandwidth Error",
[9]	"No Slots Available",
[10]	"Invalid Stream Type",
[11]	"Slot Not Enabled",
[12]	"Endpoint Not Enabled",
[13]	"Short Packet",
[14]	"Ring Underrun",
[15]	"Ring Overrun",
[16]	"VF Event Ring Full",
[17]	"Parameter Error",
[18]	"Bandwidth Overrun Error",
[19]	"Context State Error",
[20]	"No Ping Response",
[21]	"Event Ring Full",
[22]	"Incompatible Device",
[23]	"Missed Service Error",
[24]	"Command Ring Stopped",
[25]	"Command Aborted",
[26]	"Stopped",
[27]	"Stopped - Length Invalid",
[29]	"Max Exit Latency Too Large",
[31]	"Isoch Buffer Overrun",
[32]	"Event Lost Error",
[33]	"Undefined Error",
[34]	"Invalid Stream ID",
[35]	"Secondary Bandwidth Error",
[36]	"Split Transaction Error",
};

static char*
ccerrstr(u32int cc)
{
	char *s;

	if(cc == 1 || cc == 13)
		return nil;
	if(cc < nelem(ccerrtab) && ccerrtab[cc] != nil)
		s = ccerrtab[cc];
	else
		s = "???";
	return s;
}

static int
waitdone(void *a)
{
	return ((Wait*)a)->z == nil;
}

static char*
waittd(Ctlr *ctlr, Wait *w, int tmout)
{
	Ring *r = w->ring;

	coherence();
	*r->doorbell = r->id;

	while(waserror()){
		if(r->stopped) {
			ctlr->er->stopped = 1;
			wakeup(&ctlr->recover);

			/* wait for rescue */
			tmout = 0;
			continue;
		}

		if(r == ctlr->cr)
			ctlr->opr[CRCR] |= CA;
		else
			ctlrcmd(ctlr, CR_STOPEP | (r->id<<16) | (r->slot->id<<24), 0, 0, nil);
		r->stopped = 1;

		/* time to abort the transaction */
		tmout = 5000;
	}
	if(tmout > 0){
		tsleep(&up->sleep, waitdone, w, tmout);
		if(!waitdone(w))
			error("timed out");
	} else {
		while(!waitdone(w))
			sleep(&up->sleep, waitdone, w);
	}
	poperror();
	return ccerrstr(w->er[2]>>24);
}

static char*
ctlrcmd(Ctlr *ctlr, u32int c, u32int s, u64int p, u32int *er)
{
	Wait w[1];
	char *err;

	qlock(&ctlr->cmdlock);
	if(needrecover(ctlr)){
		qunlock(&ctlr->cmdlock);
		return Erecover;
	}
	ctlr->cr->stopped = 0;
	queuetd(ctlr->cr, c, s, p, w);
	err = waittd(ctlr, w, 5000);
	qunlock(&ctlr->cmdlock);

	if(er != nil)
		memmove(er, w->er, 4*4);

	return err;
}

static void
completering(Ring *r, u32int *er)
{
	Wait *w, **wp;
	u32int *td, x;
	u64int pa;

	pa = (*(u64int*)er) & ~15ULL;
	ilock(r);

	for(x = r->rp; (int)(r->wp - x) > 0;){
		td = &r->base[4*(x++ & r->mask)];
		if((u64int)PCIWADDR(td) == pa){
			r->rp = x;
			break;
		}
	}

	wp = &r->pending;
	while(w = *wp){
		if((u64int)PCIWADDR(w->td) == pa){
			Rendez *z = w->z;

			memmove(w->er, er, 4*4);
			*wp = w->next;
			w->next = nil;

			if(z != nil){
				w->z = nil;
				wakeup(z);
			}
			break;
		} else {
			wp = &w->next;
		}
	}

	iunlock(r);
}

static void
interrupt(Ureg*, void *arg)
{
	Hci *hp = arg;
	Ctlr *ctlr = hp->aux;
	Ring *ring = ctlr->er;
	Slot *slot;
	u32int *irs, *td, x;

	if(ring->base == nil)
		return;

	irs = &ctlr->rts[IR0];
	x = irs[IMAN];
	if(x & 1) irs[IMAN] = x & 3;

	for(x = ring->rp;; x=++ring->rp){
		td = ring->base + 4*(x & ring->mask);
		dmaflush(0, td, 4*4);

		if((((x>>ring->shift)^td[3])&1) == 0)
			break;

		switch(td[3] & 0xFC00){
		case ER_CMDCOMPL:
			completering(ctlr->cr, td);
			break;
		case ER_TRANSFER:
			x = td[3]>>24;
			if(x == 0 || x > ctlr->nslots)
				break;
			slot = ctlr->slot[x];
			if(slot == nil)
				break;
			completering(&slot->epr[(td[3]>>16)-1&31], td);
			break;
		case ER_MFINDEXWRAP:
			ctlr->µframe = (ctlr->rts[MFINDEX] & (1<<14)-1) | 
				(ctlr->µframe+(1<<14) & ~((1<<14)-1));
			break;
		case ER_HCE:
			iprint("xhci: host controller error: %ux %ux %ux %ux\n",
				td[0], td[1], td[2], td[3]);
			ctlr->er->stopped = 1;
			wakeup(&ctlr->recover);
			return;
		case ER_PORTSC:
			break;
		case ER_BWREQ:
		case ER_DOORBELL:
		case ER_DEVNOTE:
		default:
			iprint("xhci: event %ud: %ux %ux %ux %ux\n",
				x, td[0], td[1], td[2], td[3]);
		}
	}

	setrptr(&irs[ERDP], PCIWADDR(td) | (1<<3));
}

static void
freeslot(void *arg)
{
	Slot *slot;

	if(arg == nil)
		return;
	slot = arg;
	if(slot->id != 0){
		Ctlr *ctlr = slot->ctlr;
		qlock(&ctlr->slotlock);
		if(ctlr->slot != nil && ctlr->slot[slot->id] == slot){
			ctlrcmd(ctlr, CR_DISABLESLOT | (slot->id<<24), 0, 0, nil);
			dmaflush(0, slot->obase, 32*32 << ctlr->csz);
			ctlr->dcba[slot->id] = 0;
			dmaflush(1, &ctlr->dcba[slot->id], sizeof(ctlr->dcba[0]));
			ctlr->slot[slot->id] = nil;
		}
		qunlock(&ctlr->slotlock);
	}
	freering(&slot->epr[0]);
	free(slot->ibase);
	free(slot->obase);
	free(slot);
}

static Slot*
allocslot(Ctlr *ctlr, Udev *dev)
{
	u32int r[4];
	Slot *slot;
	char *err;

	slot = malloc(sizeof(Slot));
	if(slot == nil)
		error(Enomem);

	slot->ctlr = ctlr;
	slot->dev = dev;
	slot->nep = 0;
	slot->id = 0;

	slot->confval = 0;
	slot->iface = 0;
	slot->altc = 0;

	qlock(&ctlr->slotlock);
	if(waserror()){
		qunlock(&ctlr->slotlock);
		freeslot(slot);
		nexterror();
	}
	if(ctlr->slot == nil)
		error(Erecover);
	slot->ibase = mallocalign(32*33 << ctlr->csz, 64, 0, ctlr->pagesize);
	slot->obase = mallocalign(32*32 << ctlr->csz, 64, 0, ctlr->pagesize);
	if(slot->ibase == nil || slot->obase == nil)
		error(Enomem);

	if((err = ctlrcmd(ctlr, CR_ENABLESLOT, 0, 0, r)) != nil)
		error(err);
	slot->id = r[3]>>24;
	if(slot->id <= 0 || slot->id > ctlr->nslots || ctlr->slot[slot->id] != nil){
		slot->id = 0;
		error("bad slot id from controller");
	}
	poperror();

	dmaflush(1, slot->obase, 32*32 << ctlr->csz);
	ctlr->dcba[slot->id] = PCIWADDR(slot->obase);
	dmaflush(1, &ctlr->dcba[slot->id], sizeof(ctlr->dcba[0]));

	ctlr->slot[slot->id] = slot;

	qunlock(&ctlr->slotlock);

	return slot;
}

static void
setdebug(Hci *, int)
{
}

static void
epclose(Ep *ep)
{
	Ctlr *ctlr;
	Slot *slot;
	Ring *ring;
	Epio *io;

	if(ep->dev->isroot)
		return;

	io = ep->aux;
	if(io == nil)
		return;
	ep->aux = nil;

	ctlr = ep->hp->aux;
	slot = ep->dev->aux;

	if(ep->nb > 0 && (io[OREAD].ring != nil || io[OWRITE].ring != nil)){
		u32int *w;

		/* input control context */
		w = slot->ibase;
		memset(w, 0, 32<<ctlr->csz);
		w[1] = 1;
		if((ring = io[OREAD].ring) != nil){
			w[0] |= 1 << ring->id;
			if(ring->id == slot->nep)
				slot->nep--;
			ctlrcmd(ctlr, CR_STOPEP | (ring->id<<16) | (slot->id<<24), 0, 0, nil);
		}
		if((ring = io[OWRITE].ring) != nil){
			w[0] |= 1 << ring->id;
			if(ring->id == slot->nep)
				slot->nep--;
			ctlrcmd(ctlr, CR_STOPEP | (ring->id<<16) | (slot->id<<24), 0, 0, nil);
		}

		/* (input) slot context */
		w += 8<<ctlr->csz;
		w[0] = (w[0] & ~(0x1F<<27)) | slot->nep<<27;

		/* (input) ep context */
		w += ep->nb*2*8<<ctlr->csz;
		memset(w, 0, 2*32<<ctlr->csz);

		dmaflush(1, slot->ibase, 32*33 << ctlr->csz);
		ctlrcmd(ctlr, CR_CONFIGEP | (slot->id<<24), 0, PCIWADDR(slot->ibase), nil);
		dmaflush(0, slot->obase, 32*32 << ctlr->csz);

		freering(io[OREAD].ring);
		freering(io[OWRITE].ring);
	}
	freeb(io[OREAD].b);
	freeb(io[OWRITE].b);
	free(io);
}

static void
initepctx(u32int *w, Ring *r, Ep *ep)
{
	int ival;

	if(ep->dev->speed == Lowspeed || ep->dev->speed == Fullspeed){
		for(ival=3; ival < 11 && (1<<ival) < ep->pollival; ival++)
			;
	} else {
		for(ival=0; ival < 15 && (1<<ival) < ep->pollival; ival++)
			;
	}
	w[0] = ival<<16;
	w[1] = ((ep->ttype-Tctl) | (r->id&1)<<2)<<3 | (ep->ntds-1)<<8 | ep->maxpkt<<16;
	if(ep->ttype != Tiso)
		w[1] |= 3<<1;
	*((u64int*)&w[2]) = PCIWADDR(r->base) | 1;
	w[4] = 2*ep->maxpkt;
	if(ep->ttype == Tintr || ep->ttype == Tiso)
		w[4] |= (ep->maxpkt*ep->ntds)<<16;
}

static void
initisoio(Epio *io, Ep *ep)
{
	if(io->ring == nil)
		return;
	io->frame = 0;
	io->period = ep->pollival<<3*(ep->dev->speed == Fullspeed);
	io->incr = (ep->hz*io->period<<8)/8000;
	io->tdsz = (io->incr+255>>8)*ep->samplesz;
	io->b = allocb((io->ring->mask+1)*io->tdsz);
}

static void
initep(Ep *ep)
{
	Epio *io;
	Ctlr *ctlr;
	Slot *slot;
	Ring *ring;
	u32int *w;
	char *err;

	io = ep->aux;
	ctlr = ep->hp->aux;
	slot = ep->dev->aux;

	io[OREAD].ring = io[OWRITE].ring = nil;
	if(ep->nb == 0){
		io[OWRITE].ring = &slot->epr[0];
		return;
	}

	/* (input) control context */
	w = slot->ibase;
	memset(w, 0, 32<<ctlr->csz);
	w[1] = 1;
	w[31] = slot->altc<<16 | slot->iface<<8 | slot->confval;

	if(waserror()){
		freering(io[OWRITE].ring), io[OWRITE].ring = nil;
		freering(io[OREAD].ring), io[OREAD].ring = nil;
		nexterror();
	}
	if(ep->mode != OREAD){
		ring = initring(io[OWRITE].ring = &slot->epr[ep->nb*2-1], 8);
		ring->id = ep->nb*2;
		if(ring->id > slot->nep)
			slot->nep = ring->id;
		ring->slot = slot;
		ring->doorbell = &ctlr->dba[slot->id];
		ring->ctx = &slot->obase[ring->id*8<<ctlr->csz];
		w[1] |= 1 << ring->id;
	}
	if(ep->mode != OWRITE){
		ring = initring(io[OREAD].ring = &slot->epr[ep->nb*2], 8);
		ring->id = ep->nb*2+1;
		if(ring->id > slot->nep)
			slot->nep = ring->id;
		ring->slot = slot;
		ring->doorbell = &ctlr->dba[slot->id];
		ring->ctx = &slot->obase[ring->id*8<<ctlr->csz];
		w[1] |= 1 << ring->id;
	}

	/* (input) slot context */
	w += 8<<ctlr->csz;
	w[0] = (w[0] & ~(0x1F<<27)) | slot->nep<<27;
	if(!ep->dev->ishub)
		w[0] &= ~(1<<25);	// MTT

	/* (input) ep context */
	w += ep->nb*2*8<<ctlr->csz;
	if(io[OWRITE].ring != nil){
		memset(w, 0, 5*4);
		initepctx(w, io[OWRITE].ring, ep);
	}

	w += 8<<ctlr->csz;
	if(io[OREAD].ring != nil){
		memset(w, 0, 5*4);
		initepctx(w, io[OREAD].ring, ep);
	}

	dmaflush(1, slot->ibase, 32*33 << ctlr->csz);
	err = ctlrcmd(ctlr, CR_CONFIGEP | (slot->id<<24), 0, PCIWADDR(slot->ibase), nil);
	dmaflush(0, slot->obase, 32*32 << ctlr->csz);
	if(err != nil)
		error(err);

	if(ep->ttype == Tiso){
		initisoio(io+OWRITE, ep);
		initisoio(io+OREAD, ep);
	}
	poperror();
}

static int
speedid(int speed)
{
	switch(speed){
	case Fullspeed:		return 1;
	case Lowspeed:		return 2;
	case Highspeed:		return 3;
	case Superspeed:	return 4;
	}
	return 0;
}

static void
epopen(Ep *ep)
{
	Ctlr *ctlr = ep->hp->aux;
	Slot *slot, *hub;
	Ring *ring;
	Epio *io;
	Udev *dev;
	char *err;
	u32int *w;
	int i;

	if(ep->dev->isroot)
		return;
	if(needrecover(ctlr))
		error(Erecover);
	io = malloc(sizeof(Epio)*2);
	if(io == nil)
		error(Enomem);
	ep->aux = io;
	if(waserror()){
		epclose(ep);
		nexterror();
	}
	dev = ep->dev;
	slot = dev->aux;
	if(slot != nil && slot->dev == dev){
		initep(ep);
		poperror();
		return;
	}

	/* first open has to be control endpoint */
	if(ep->nb != 0)
		error(Egreg);

	slot = allocslot(ctlr, dev);
	if(waserror()){
		freeslot(slot);
		nexterror();
	}

	/* allocate control ep 0 ring */
	ring = initring(io[OWRITE].ring = &slot->epr[0], 4);
	ring->id = 1;
	slot->nep = 1;
	ring->slot = slot;
	ring->doorbell = &ctlr->dba[slot->id];
	ring->ctx = &slot->obase[8];

	/* (input) control context */
	w = slot->ibase;
	memset(w, 0, 3*32<<ctlr->csz);
	w[1] = 3;	/* A0, A1 */
		
	/* (input) slot context */
	w += 8<<ctlr->csz;
	w[2] = w[3] = 0;
	w[0] = dev->routestr | speedid(dev->speed)<<20 |
		(dev->speed == Highspeed && dev->ishub != 0)<<25 |	// MTT
		(dev->ishub != 0)<<26 | slot->nep<<27;
	w[1] = dev->rootport<<16;

	/* find the parent hub that this device is conected to */
	qlock(&ctlr->slotlock);
	for(i=1; i<=ctlr->nslots; i++){
		hub = ctlr->slot[i];
		if(hub == nil || hub->dev == nil || hub->dev->aux != hub)
			continue;
		if(hub == slot || hub->dev == dev)
			continue;
		if(!hub->dev->ishub)
			continue;
		if(hub->dev->addr != dev->hub)
			continue;
		if(hub->dev->rootport != dev->rootport)
			continue;

		if(dev->speed < Highspeed && hub->dev->speed == Highspeed){
			w[0] |= 1<<25;	// MTT
			w[2] = hub->id | dev->port<<8;
		}
		break;
	}
	qunlock(&ctlr->slotlock);

	/* (input) ep context 0 */
	w += 8<<ctlr->csz;
	initepctx(w, io[OWRITE].ring, ep);

	dmaflush(1, slot->ibase, 32*33 << ctlr->csz);
	err = ctlrcmd(ctlr, CR_ADDRESSDEV | (slot->id<<24), 0, PCIWADDR(slot->ibase), nil);
	dmaflush(0, slot->obase, 32*32 << ctlr->csz);
	if(err != nil)
		error(err);

	/* (output) slot context */
	w = slot->obase;

	dev->addr = w[3] & 0xFF;

	dev->aux = slot;
	dev->free = freeslot;

	poperror();
	poperror();
}

static long
isoread(Ep *, uchar *, long)
{
	error(Egreg);
	return 0;
}

static long
isowrite(Ep *ep, uchar *p, long n)
{
	uchar *s, *d;
	Ctlr *ctlr;
	Epio *io;
	u32int i, µ;
	long m;

	s = p;
	io = (Epio*)ep->aux + OWRITE;
	qlock(io);
	if(waserror()){
		qunlock(io);
		nexterror();
	}
	µ = io->period;
	ctlr = ep->hp->aux;
	if(needrecover(ctlr))
		error(Erecover);
	for(i = io->frame;; i++){
		for(;;){
			m = (int)(io->ring->wp - io->ring->rp);
			if(m <= 0)
				i = (80 + µframe(ctlr))/µ;
			if(m < io->ring->mask)
				break;
			*io->ring->doorbell = io->ring->id;
			tsleep(&up->sleep, return0, nil, 5);
		}
		m = ((io->incr + (i*io->incr&255))>>8)*ep->samplesz;
		d = io->b->rp + (i&io->ring->mask)*io->tdsz;
		m -= io->nleft, d += io->nleft;
		if(n < m){
			memmove(d, p, n);
			p += n;
			io->nleft += n;
			break;
		}
		memmove(d, p, m);
		p += m, n -= m;
		m += io->nleft, d -= io->nleft;
		io->nleft = 0;
		dmaflush(1, d, m);
		queuetd(io->ring, TR_ISOCH | (i*µ/8 & 0x7ff)<<20 | TR_IOC, m, PCIWADDR(d), nil);
	}
	io->frame = i;
	while(io->ring->rp != io->ring->wp){
		int d = (int)(i*µ - µframe(ctlr))/8;
		//d -= ep->sampledelay*1000 / ep->hz;
		if(d < 5)
			break;
		*io->ring->doorbell = io->ring->id;
		tsleep(&up->sleep, return0, nil, d);
	}
	qunlock(io);
	poperror();

	return p - s;
}

static char*
unstall(Ep *ep, Ring *r)
{
	char *err;

	switch(r->ctx[0]&7){
	case 2:	/* halted */
	case 4:	/* error */
		ep->clrhalt = 1;
	}
	if(ep->clrhalt){
		ep->clrhalt = 0;
		err = ctlrcmd(r->slot->ctlr, CR_RESETEP | (r->id<<16) | (r->slot->id<<24), 0, 0, nil);
		dmaflush(0, r->ctx, 8*4 << r->slot->ctlr->csz);
		if(err != nil)
			return err;
		r->stopped = 1;
	}
	if(r->stopped){
		err = ctlrcmd(r->slot->ctlr, CR_SETTRDQP | (r->id<<16) | (r->slot->id<<24), 0, resetring(r), nil);
		dmaflush(0, r->ctx, 8*4 << r->slot->ctlr->csz);
		if(err != nil)
			return err;
		r->stopped = 0;
	}
	if(r->wp - r->rp >= r->mask)
		return "Ring Full";
	return nil;
}

static long
epread(Ep *ep, void *va, long n)
{
	Epio *io;
	Ctlr *ctlr;
	uchar *p;
	char *err;
	Wait w[1];

	if(ep->dev->isroot)
		error(Egreg);

	p = va;
	if(ep->ttype == Tctl){
		io = (Epio*)ep->aux + OREAD;
		qlock(io);
		if(io->b == nil || BLEN(io->b) == 0){
			qunlock(io);
			return 0;
		}
		if(n > BLEN(io->b))
			n = BLEN(io->b);
		memmove(p, io->b->rp, n);
		io->b->rp += n;
		qunlock(io);
		return n;
	} else if(ep->ttype == Tiso)
		return isoread(ep, p, n);

	if((uintptr)p <= KZERO){
		Block *b;

		b = allocb(n);
		if(waserror()){
			freeb(b);
			nexterror();
		}
		n = epread(ep, b->rp, n);
		memmove(p, b->rp, n);
		freeb(b);
		poperror();
		return n;
	}

	ctlr = (Ctlr*)ep->hp->aux;
	io = (Epio*)ep->aux + OREAD;
	qlock(io);
	if(waserror()){
		dmaflush(0, io->ring->ctx, 8*4 << ctlr->csz);
		qunlock(io);
		nexterror();
	}

	if((err = unstall(ep, io->ring)) != nil)
		error(err);

	dmaflush(1, p, n);
	queuetd(io->ring, TR_NORMAL | TR_IOC, n, PCIWADDR(p), w);
	err = waittd(ctlr, w, ep->tmout);
	dmaflush(0, p, n);
	if(err != nil)
		error(err);

	qunlock(io);
	poperror();

	n -= (w->er[2] & 0xFFFFFF);
	if(n < 0)
		n = 0;

	return n;
}

static long
epwrite(Ep *ep, void *va, long n)
{
	Wait w[3];
	Ctlr *ctlr;
	Epio *io;
	uchar *p;
	char *err;

	if(ep->dev->isroot)
		error(Egreg);

	p = va;
	if(ep->ttype == Tctl){
		int dir, len;
		Ring *ring;
		Slot *slot;

		if(n < 8)
			error(Eshort);

		if(p[0] == 0x00 && p[1] == 0x05)
			return n;

		ctlr = (Ctlr*)ep->hp->aux;
		io = (Epio*)ep->aux + OREAD;
		ring = io[OWRITE-OREAD].ring;
		slot = ring->slot;
		qlock(io);
		if(waserror()){
			ilock(ring);
			ring->pending = nil;
			iunlock(ring);
			dmaflush(0, ring->ctx, 8*4 << ctlr->csz);
			qunlock(io);
			nexterror();
		}
		if(io->b != nil){
			freeb(io->b);
			io->b = nil;
		}
		len = GET2(&p[6]);
		dir = (p[0] & Rd2h) != 0;
		if(len > 0){
			io->b = allocb(len);		
			if(dir == 0){	/* out */
				assert(len >= n-8);
				memmove(io->b->wp, p+8, n-8);
			} else {
				memset(io->b->wp, 0, len);
				io->b->wp += len;
			}
		}
		if((err = unstall(ep, ring)) != nil)
			error(err);

		if((ring->ctx[1]>>16) != ep->maxpkt){
			u32int *w = slot->ibase;
			w[0] = 0;
			w[1] = 1<<ring->id;
			w += (ring->id+1)*8<<ctlr->csz;
			initepctx(w, ring, ep);
			dmaflush(1, slot->ibase, 32*33 << ctlr->csz);
			err = ctlrcmd(ctlr, CR_EVALCTX | (slot->id<<24), 0, PCIWADDR(slot->ibase), nil);
			dmaflush(0, slot->obase, 32*32 << ctlr->csz);
			if(err != nil)
				error(err);
		}

		queuetd(ring, TR_SETUPSTAGE | (len > 0 ? 2+dir : 0)<<16 | TR_IDT | TR_IOC, 8,
			p[0] | p[1]<<8 | GET2(&p[2])<<16 |
			(u64int)(GET2(&p[4]) | len<<16)<<32, &w[0]);
		if(len > 0){
			dmaflush(1, io->b->rp, len);
			queuetd(ring, TR_DATASTAGE | dir<<16 | TR_IOC, len,
				PCIWADDR(io->b->rp), &w[1]);
		}
		queuetd(ring, TR_STATUSSTAGE | (len == 0 || !dir)<<16 | TR_IOC, 0, 0, &w[2]);

		if((err = waittd(ctlr, &w[0], ep->tmout)) != nil)
			error(err);
		if(len > 0){
			if((err = waittd(ctlr, &w[1], ep->tmout)) != nil)
				error(err);
			if(dir != 0){
				dmaflush(0, io->b->rp, len);
				io->b->wp -= (w[1].er[2] & 0xFFFFFF);
				if(io->b->wp < io->b->rp)
					io->b->wp = io->b->rp;
			}
		}
		if((err = waittd(ctlr, &w[2], ep->tmout)) != nil)
			error(err);

		if(p[0] == 0x00 && p[1] == 0x09){
			slot->confval = GET2(&p[2]);
		} else if(p[0] == 0x01 && p[1] == 0x0d){
			slot->altc = GET2(&p[2]);
			slot->iface = GET2(&p[4]);
		}

		qunlock(io);
		poperror();

		return n;
	} else if(ep->ttype == Tiso)
		return isowrite(ep, p, n);

	if((uintptr)p <= KZERO){
		Block *b;

		b = allocb(n);
		if(waserror()){
			freeb(b);
			nexterror();
		}
		memmove(b->wp, p, n);
		n = epwrite(ep, b->wp, n);
		freeb(b);
		poperror();
		return n;
	}

	ctlr = (Ctlr*)ep->hp->aux;
	io = (Epio*)ep->aux + OWRITE;
	qlock(io);
	if(waserror()){
		dmaflush(0, io->ring->ctx, 8*4 << ctlr->csz);
		qunlock(io);
		nexterror();
	}

	if((err = unstall(ep, io->ring)) != nil)
		error(err);

	dmaflush(1, p, n);
	queuetd(io->ring, TR_NORMAL | TR_IOC, n, PCIWADDR(p), w);
	if((err = waittd(ctlr, w, ep->tmout)) != nil)
		error(err);

	qunlock(io);
	poperror();

	return n;
}

static char*
seprintep(char *s, char*, Ep*)
{
	return s;
}

static int
portstatus(Hci *hp, int port)
{
	Ctlr *ctlr = hp->aux;
	u32int psc, ps;

	if(ctlr->port == nil || needrecover(ctlr))
		return 0;

	ps = 0;
	psc = ctlr->port[port-1].reg[PORTSC];
	if(psc & CCS)	ps |= HPpresent;
	if(psc & PED)	ps |= HPenable;
	if(psc & OCA)	ps |= HPovercurrent;
	if(psc & PR)	ps |= HPreset;

	if((hp->superspeed & (1<<(port-1))) != 0){
		ps |= psc & (PLS|PP);
		if(psc & CSC)	ps |= 1<<0+16;
		if(psc & OCC)	ps |= 1<<3+16;
		if(psc & PRC)	ps |= 1<<4+16;
		if(psc & WRC)	ps |= 1<<5+16;
		if(psc & PLC)	ps |= 1<<6+16;
		if(psc & CEC)	ps |= 1<<7+16;
	} else {
		if((ps & HPreset) == 0){
			switch((psc>>10)&15){
			case 1:
				/* full speed */
				break;
			case 2:
				ps |= HPslow;
				break;
			case 3:
				ps |= HPhigh;
				break;
			}
		}
		if(psc & PP)	ps |= HPpower;
		if(psc & CSC)	ps |= HPstatuschg;
		if(psc & PRC)	ps |= HPchange;
	}

	return ps;
}
	
static int
portenable(Hci*, int, int)
{
	return 0;
}

static int
portreset(Hci *hp, int port, int on)
{
	Ctlr *ctlr = hp->aux;

	if(ctlr->port == nil || needrecover(ctlr))
		return 0;

	if(on){
		ctlr->port[port-1].reg[PORTSC] |= PR;
		tsleep(&up->sleep, return0, nil, 200);
	}
	return 0;
}


static Ctlr *ctlrs[Nhcis];

static void
scanpci(void)
{
	static int already = 0;
	int i;
	uintpci io;
	Ctlr *ctlr;
	Pcidev *p;
	u32int *mmio; 

	if(already)
		return;
	already = 1;
	p = nil;
	while ((p = pcimatch(p, 0, 0)) != nil) {
		/*
		 * Find XHCI controllers (Programming Interface = 0x30).
		 */
		if(p->ccrb != Pcibcserial || p->ccru != Pciscusb || p->ccrp != 0x30)
			continue;
		io = p->mem[0].bar & ~0x0f;
		if(io == 0)
			continue;
		print("usbxhci: %#x %#x: port %llux size %#x irq %d\n",
			p->vid, p->did, io, p->mem[0].size, p->intl);
		mmio = (u32int*)mmukmapx(VIRTPCI, io, p->mem[0].size);
		if(mmio == nil){
			print("usbxhci: cannot map registers\n");
			continue;
		}
		ctlr = malloc(sizeof(Ctlr));
		if(ctlr == nil){
			print("usbxhci: no memory\n");
			continue;
		}
		ctlr->base = io;
		ctlr->active = nil;
		ctlr->pcidev = p;
		ctlr->mmio = mmio;
		for(i = 0; i < nelem(ctlrs); i++)
			if(ctlrs[i] == nil){
				ctlrs[i] = ctlr;
				break;
			}
		if(i >= nelem(ctlrs))
			print("xhci: bug: more than %d controllers\n", nelem(ctlrs));
	}
}

static int
reset(Hci *hp)
{
	Ctlr *ctlr;
	int i;

	if(getconf("*nousbxhci"))
		return -1;

	scanpci();

	/*
	 * Any adapter matches if no hp->port is supplied,
	 * otherwise the ports must match.
	 */
	for(i = 0; i < nelem(ctlrs) && ctlrs[i] != nil; i++){
		ctlr = ctlrs[i];
		if(ctlr->active == nil)
		if(hp->port == 0 || hp->port == ctlr->base){
			ctlr->active = hp;
			goto Found;
		}
	}
	return -1;

Found:
	hp->aux = ctlr;
	hp->port = ctlr->base;
	hp->irq = ctlr->pcidev->intl;
	hp->tbdf = ctlr->pcidev->tbdf;

	hp->init = init;
	hp->dump = dump;
	hp->interrupt = interrupt;
	hp->epopen = epopen;
	hp->epclose = epclose;
	hp->epread = epread;
	hp->epwrite = epwrite;
	hp->seprintep = seprintep;
	hp->portenable = portenable;
	hp->portreset = portreset;
	hp->portstatus = portstatus;
	hp->shutdown = shutdown;
	hp->debug = setdebug;
	hp->type = "xhci";

	return 0;
}

void
usbxhcilink(void)
{
	addhcitype("xhci", reset);
}

void
dmaflush(int clean, void *p, ulong len)
{
	uintptr s = (uintptr)p;
	uintptr e = (uintptr)p + len;

	if(clean){
		s &= ~(BLOCKALIGN-1);
		e += BLOCKALIGN-1;
		e &= ~(BLOCKALIGN-1);
		cachedwbse((void*)s, e - s);
		return;
	}
	if(s & BLOCKALIGN-1){
		s &= ~(BLOCKALIGN-1);
		cachedwbinvse((void*)s, BLOCKALIGN);
		s += BLOCKALIGN;
	}
	if(e & BLOCKALIGN-1){
		e &= ~(BLOCKALIGN-1);
		if(e < s)
			return;
		cachedwbinvse((void*)e, BLOCKALIGN);
	}
	if(s < e)
		cachedinvse((void*)s, e - s);
}
