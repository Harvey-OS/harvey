/*
 * USB Open Host Controller Interface (OHCI) driver
 * from devohci.c provided by Charles Forsyth, 5 Aug 2006.
 */
#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"

#include	"usb.h"

#define XPRINT	if(usbhdebug) print
#define XIPRINT	if(usbhdebug) iprint
#define XEPRINT	if(usbhdebug || ep->debug) print
#define XEIPRINT if(usbhdebug || ep->debug) iprint

#define	IPRINT(x) iprint x

static int usbhdebug = 0;
static int dcls;

enum {
	Ned = 63 + 32,
	Ntd = 256,
};

/*
 * USB packet definitions
 */
enum {
	Otoksetup = 0,
	Otokout = 1,
	Otokin = 2,

	/* port status - UHCI style */
	Suspend =	1<<12,
	PortReset =	1<<9,
	SlowDevice =	1<<8,
	ResumeDetect =	1<<6,
	PortEnableChange = 1<<3,	/* write 1 to clear */
	PortEnable = 	1<<2,
	ConnectStatusChange = 1<<1,	/* write 1 to clear */
	DevicePresent =	1<<0,
};

typedef struct Ctlr Ctlr;
typedef struct QTree QTree;

enum {
	ED_MPS_MASK = 0x7ff,
	ED_MPS_SHIFT = 16,
	ED_C_MASK  = 1,
	ED_C_SHIFT = 1,
	ED_F_BIT = 1 << 15,
	ED_S_MASK  = 1,
	ED_S_SHIFT = 13,
	ED_D_MASK  = 3,
	ED_D_SHIFT = 11,
	ED_H_MASK  = 1,
	ED_H_SHIFT = 0,
};

typedef struct Endptx Endptx;
typedef struct TD TD;

struct Endptx
{
	Lock;		/* for manipulating ed */
	ED	*ed;	/* Single endpoint descriptor */
	int	ntd;	/* Number of TDs in use */
	int	overruns;
};

struct TD {
	ulong	ctrl;
	ulong	cbp;
	ulong	nexttd;
	ulong	be;
	ushort	offsets[8];	/* Iso TDs only */
	/* driver specific; pad to multiple of 32 */
	TD*	next;
	Endpt	*ep;
	Block	*bp;
	ulong	flags;
	ulong	offset;		/* offset associated with end of data */
	ulong	bytes;		/* bytes in this TD */
	ulong	pad[2];
};

enum {
	TD_R_SHIFT = 18,
	TD_DP_MASK = 3,
	TD_DP_SHIFT = 19,
	TD_CC_MASK = 0xf,
	TD_CC_SHIFT = 28,
	TD_EC_MASK = 3,
	TD_EC_SHIFT = 26,

	TD_FLAGS_LAST = 1 << 0,
};

typedef struct HCCA HCCA;
struct HCCA {
	ulong	intrtable[32];
	ushort	framenumber;
	ushort	pad1;
	ulong	donehead;
	uchar	reserved[116];
};

/* OHCI registers */
typedef struct OHCI OHCI;
struct OHCI {
	/* control and status group */
/*00*/	ulong	revision;
	ulong	control;
	ulong	cmdsts;
	ulong	intrsts;

/*10*/	ulong	intrenable;
	ulong	intrdisable;
	/* memory pointer group */
	ulong	hcca;
	ulong	periodcurred;

/*20*/	ulong	ctlheaded;
	ulong	ctlcurred;
	ulong	bulkheaded;
	ulong	bulkcurred;

/*30*/	ulong	donehead;
	/* frame counter group */
	ulong	fminterval;
	ulong	fmremaining;
	ulong	fmnumber;

/*40*/	ulong	periodicstart;
	ulong	lsthreshold;
	/* root hub group */
	ulong	rhdesca;
	ulong	rhdescb;

/*50*/	ulong	rhsts;
	ulong	rhportsts[15];

/*90*/	ulong	pad25[20];

	/* unknown */
/*e0*/	ulong	hostueaddr;
	ulong	hostuests;
	ulong	hosttimeoutctrl;
	ulong	pad59;

/*f0*/	ulong	pad60;
	ulong	hostrevision;
	ulong	pad62[2];
/*100*/
};

/*
 * software structures
 */

static struct {
	int	bit;
	char	*name;
} portstatus[] = {
	{ Suspend,		"suspend", },
	{ PortReset,		"reset", },
	{ SlowDevice,		"lowspeed", },
	{ ResumeDetect,		"resume", },
	{ PortEnableChange,	"portchange", },
	{ PortEnable,		"enable", },
	{ ConnectStatusChange,	"statuschange", },
	{ DevicePresent,	"present", },
};

struct QTree {
	QLock;
	int	nel;
	int	depth;
	ulong*	bw;
	ED	**root;
};

/* device parameters */
static char *devstates[] = {
	[Disabled]		"Disabled",
	[Attached]		"Attached",
	[Enabled]		"Enabled",
};

struct Ctlr {
	Lock;	/* protects state shared with interrupt (eg, free list) */
	int	active;
	Pcidev*	pcidev;
	int	irq;
	ulong	tbdf;
	Ctlr*	next;
	int	nports;

	OHCI	*base;		/* equiv to io in uhci */
	HCCA	*uchcca;
	int	idgen;		/* version # to distinguish new connections */
	QLock	resetl;		/* lock controller during USB reset */

	struct {
		Lock;
		TD*	pool;
		TD*	free;
		int	alloced;
	} td;

	struct {
		QLock;
		ED*	pool;
		ED*	free;
		int	alloced;
	} ed;

	/* TODO: what happened to ctlq, etc. from uhci? */

	QTree*	tree;		/* tree for t Endpt i/o */

	struct {
		QLock;
		Endpt*	f;
	} activends;
};

enum {
	HcRevision =	0x00,
	HcControl =	0x01,
		HcfsMask =	3 << 6,
		HcfsReset =	0 << 6,
		HcfsResume =	1 << 6,
		HcfsOperational=2 << 6,
		HcfsSuspend =	3 << 6,
		Ble =	1 << 5,
		Cle =	1 << 4,
		Ie =	1 << 3,
		Ple =	1 << 2,
		Cbsr_MASK = 3,
	HcCommandStatus = 0x02,
		Ocr =	1 << 3,
		Blf =	1 << 2,
		Clf =	1 << 1,
		Hcr =	1 << 0,
	HcIntrStatus =	0x03,
	HcIntrEnable =	0x04,
		Mie =	1 << 31,
		Oc =	1 << 30,
		Rhsc =	1 << 6,
		Fno =	1 << 5,
		Ue =	1 << 4,
		Rd =	1 << 3,
		Sf =	1 << 2,
		Wdh =	1 << 1,
		So =	1 << 0,
	HcIntrDisable =	0x05,
	HcFmIntvl =	0x0d,
		HcFmIntvl_FSMaxpack_MASK = 0x7fff,
		HcFmIntvl_FSMaxpack_SHIFT = 16,
	HcFmRemaining =	0x0e,
	HcFmNumber =	0x0f,
	HcLSThreshold =	0x11,
	HcRhDescA =	0x12,
		HcRhDescA_POTPGT_MASK =	0xff << 24,
		HcRhDescA_POTPGT_SHIFT =	24,
	HcRhDescB =	0x13,
	HcRhStatus =	0x14,
		Lps =	1 << 0,
		Cgp =	1 << 0,
		Oci =	1 << 1,
		Drwe =	1 << 15,
		Srwe =	1 << 15,
		LpsC =	1 << 16,
		Sgp =	1 << 16,
		Ccic =	1 << 17,
		Crwe =	1 << 31,
	HcRhPortStatus1 = 0x15,
		Ccs =	1 << 0,
		Cpe =	1 << 0,
		Pes =	1 << 1,
		Spe =	1 << 1,
		Pss =	1 << 2,
		Poci =	1 << 3,
		Prs =	1 << 4,
		Spr =	1 << 4,
		Pps =	1 << 8,
		Spp=	1 << 8,
		Lsda =	1 << 9,
		Cpp =	1 << 9,
		Csc =	1 << 16,
		Pesc =	1 << 17,
		Pssc =	1 << 18,
		Ocic =	1 << 19,
		Prsc =	1 << 20,
	HcRhPortStatus2 =	0x16,

	L2NFRAME =	5,
	NFRAME =	1 << L2NFRAME,
	/* TODO: from UHCI; correct for OHCI? */
	FRAMESIZE = NFRAME*sizeof(ulong), /* fixed by hardware; aligned to same */
};

char *usbmode[] = {
[Ctlmode]=	"Ctl",
[Bulkmode] =	"Bulk",
[Intrmode] =	"Intr",
[Isomode] =	"Iso",
};

static char *ousbmode[] = {
[OREAD] = "r",
[OWRITE] = "w",
[ORDWR] = "rw",
};

int ohciinterrupts[Nmodes];

static Ctlr* ctlrhead;
static Ctlr* ctlrtail;

static	char	Estalled[] = "usb endpoint stalled";
static	char	EnotWritten[] = "usb write unfinished";
static	char	EnotRead[] = "usb read unfinished";
static	char	Eunderrun[] = "usb endpoint underrun";

static	QLock	usbhstate;	/* protects name space state */

static void	eptactivate(Ctlr *ub, Endpt *ep);
static void	eptdeactivate(Ctlr *ub, Endpt *e);
static long	read (Usbhost *, Endpt*, void*, long, vlong);
static void	scanpci(void);
static int	schedendpt(Ctlr *ub, Endpt *ep, int direction);
static void	unschedendpt(Ctlr *ub, Endpt *ep, int);
static long	write(Usbhost *, Endpt*, void*, long, vlong, int);
static long	qtd(Ctlr*, Endpt*, int, Block*, uchar*, uchar*, int, ulong);

static short
refcnt(Block *b, int i)
{
	short v;
	static Lock l;

	ilock(&l);
	v = (b->flag += i);
	iunlock(&l);
	if(v < 0)
		iprint("refcnt 0x%lux %d\n", b, v);
	return v;
}

static void
freewb(Block *b)
{
	if(b == nil || refcnt(b, -1) > 0)
		return;

	if(b->base > b->rp || b->rp > b->wp || b->wp > b->lim)
		iprint("freebw: %lux %lux %lux %lux\n",
			b->base, b->rp, b->wp, b->lim);
	/* poison the block in case someone is still holding onto it */
	b->next = (Block*)0xdeadcafe;
	b->rp = (uchar*)0xdeadcafe;
	b->wp = (uchar*)0xdeadcafe;
	b->lim = (uchar*)0xdeadcafe;
	b->base = (uchar*)0xdeadcafe;

	free(b);
}

Block *
allocwb(long size)
{
	Block *b;

	b = allocb(size);
	b->flag = 1;
	b->free = freewb;
	return b;
}

void
printdata(void *pdata, int itemsize, int nitems)
{
	int i;
	uchar *p1;
	ushort *p2;
	ulong *p4;

	if(!usbhdebug)
		return;
	p1 = pdata;
	p2 = pdata;
	p4 = pdata;
	i = 0;
	for(;;){
		switch(itemsize){
		default:
			assert(0);
		case 1:
			print("%2.2ux ", *p1++);
			break;
		case 2:
			print("%4.4ux ", *p2++);
			break;
		case 4:
			print("%8.8lux ", *p4++);
			break;
		}
		if(++i >= nitems || (i & ((0x40 >> itemsize) - 1)) == 0){
			print("\n");
			if(i >= nitems)
				break;
		}
	}
}

/*
 * i left these in so that we could use the same
 * driver on several other platforms (in principle).
 * the processor on which it was originally developed
 * had an IO MMU and thus another address space.
 * it's nothing to do with USB as such.
 */
ulong
va2hcva(void *va)
{
	if(va == nil)
		return 0;
	return PADDR(va);
}

void *
hcva2va(ulong hcva)
{
	if(hcva == 0)
		return nil;
	return KADDR(hcva);
}

void *
va2ucva(void *va)
{
	return va;
}

void *
hcva2ucva(ulong hcva)
{
	if(hcva == 0)
		return nil;
	if(hcva & 0xf0000000){
		iprint("hcva2ucva: bad 0x%lux, called from 0x%lux\n",
			hcva, getcallerpc(&hcva));
		return nil;
	}
	return KADDR(hcva);
}

#define	IOCACHED	0
#define	invalidatedcacheva(va)
#define	dcclean(p, n)

static void
EDinit(ED *ed, int mps, int f, int k, int s, int d, int en, int fa,
	TD *tail, TD *head, int c, int h, ED *next)
{
	/* check nothing is running? */
	ed->ctrl = (mps & ED_MPS_MASK) << ED_MPS_SHIFT
		| (f & 1) << 15
		| (k & 1) << 14
		| (s & ED_S_MASK) << ED_S_SHIFT
		| (d & 3) << 11		/* 00 is obtained from TD (used here) */
		| (en & 0xf) << 7
		| (fa & 0x7f);
	ed->tail =  va2hcva(tail) & ~0xF;
	ed->head = (va2hcva(head) & ~0xF)
		| (c & ED_C_MASK) << ED_C_SHIFT
		| (h & ED_H_MASK) << ED_H_SHIFT;
	ed->next = va2hcva(next) & ~0xF;
}

static void
EDsetS(ED *ed, int s)
{
	XIPRINT("EDsetS: %s speed\n", s == Lowspeed ? "low" : "high");
	if(s == Lowspeed)
		ed->ctrl |= 1 << ED_S_SHIFT;
	else
		ed->ctrl &= ~(1 << ED_S_SHIFT);
}

static void
EDsetMPS(ED *ed, int mps)
{
	ed->ctrl = (ed->ctrl & ~(ED_MPS_MASK << ED_MPS_SHIFT)) |
		(mps & ED_MPS_MASK) << ED_MPS_SHIFT;
}

static void
EDsetC(ED *ed, int c)
{
	ed->head = (ed->head & ~(ED_C_MASK << ED_C_SHIFT)) |
		(c & ED_C_MASK) << ED_C_SHIFT;
}

static void
EDsetH(ED *ed, int h)
{
	ed->head = (ed->head & ~(ED_H_MASK << ED_H_SHIFT)) |
		(h & ED_H_MASK) << ED_H_SHIFT;
}

static int
EDgetH(ED *ed)
{
	return (ed->head >> ED_H_SHIFT) & ED_H_MASK;
}

static int
EDgetC(ED *ed)
{
	return (ed->head >> ED_C_SHIFT) & ED_C_MASK;
}

static void
EDsetnext(ED *ed, void *va)
{
	ed->next = va2hcva(va) & ~0xF;
}

static ED *
EDgetnext(ED *ed)
{
	return hcva2ucva(ed->next & ~0xF);
}

static void
EDsettail(ED *ed, void *va)
{
	ed->tail = va2hcva(va) & ~0xF;
}

static TD *
EDgettail(ED *ed)
{
	return hcva2ucva(ed->tail & ~0xF);
}

static void
EDsethead(ED *ed, void *va)
{
	ed->head = (ed->head & 0xf) | (va2hcva(va) & ~0xF);
}

static TD *
EDgethead(ED *ed)
{
	return hcva2ucva(ed->head & ~0xF);
}

static ED *
EDalloc(Ctlr *ub)
{
	ED *t;

	qlock(&ub->ed);
	t = ub->ed.free;
	if(t == nil){
		qunlock(&ub->ed);
		return nil;
	}
	ub->ed.free = (ED *)t->next;
	ub->ed.alloced++;
	if (0)
		print("%d endpoints allocated\n", ub->ed.alloced);
	qunlock(&ub->ed);
	t->next = 0;
	return t;
}

void
TDsetnexttd(TD *td, TD *va)
{
	td->nexttd = va2hcva(va) & ~0xF;
}

TD *
TDgetnexttd(TD *td)
{
	return hcva2ucva(td->nexttd & ~0xF);
}

void
OHCIsetControlHeadED(OHCI *ohci, ED *va)
{
	ohci->ctlheaded = va2hcva(va) & ~0xF;
}

ED *
OHCIgetControlHeadED(OHCI *ohci)
{
	return hcva2ucva(ohci->ctlheaded);
}

void
OHCIsetBulkHeadED(OHCI *ohci, ED *va)
{
	ohci->bulkheaded = va2hcva(va) & ~0xF;
}

ED *
OHCIgetBulkHeadED(OHCI *ohci)
{
	return hcva2ucva(ohci->bulkheaded);
}

static TD *
TDalloc(Ctlr *ub, Endpt *ep, int musthave)		/* alloctd */
{
	TD *t;
	Endptx *epx;

	for(;;){
		ilock(ub);
		t = ub->td.free;
		if(t)
			break;
		iunlock(ub);
		if(up == nil){
			if(musthave)
				panic("TDalloc: out of descs");
			return nil;
		}
		tsleep(&up->sleep, return0, 0, 100);
	}

	ub->td.free = t->next;
	epx = ep->private;
	epx->ntd++;
	ub->td.alloced++;
	iunlock(ub);
	memset(t, 0, sizeof(TD));
	t->ep = ep;
	return t;
}

/* call under ilock */
static void
TDfree(Ctlr *ub, TD *t)					/* freetd */
{
	Endptx *epx;

	if(t == 0)
		return;

	if(t->ep){
		epx = t->ep->private;
		epx->ntd--;
	} else
		t->ep = nil;			/* redundant? */
	t->bp = nil;
	t->next = ub->td.free;
	ub->td.free = t;
	ub->td.alloced--;
}

static void
EDfree(Ctlr *ub, ED *t)
{
	TD *td, *next;

	if(t == 0)
		return;
	qlock(&ub->ed);
	t->next = (ulong)ub->ed.free;
	ub->ed.free = t;
	ub->ed.alloced--;
	if (0)
		print("%d endpoints allocated\n", ub->ed.alloced);
	ilock(ub);
	for(td = EDgethead(t); td; td = next){
		next = TDgetnexttd(td);
		TDfree(ub, td);
	}
	iunlock(ub);
	EDsethead(t, 0);
	EDsettail(t, 0);
	qunlock(&ub->ed);
}

static void
waitSOF(Ctlr *ub)
{
	/*
	 * wait for SOF - interlock with interrupt handler so
	 * done queue processed first.
	 */
	int frame = ub->uchcca->framenumber & 0x3f;

	do {
		delay(2);
	} while(frame == (ub->uchcca->framenumber & 0x3f));
}

static void
dumptd(TD *td, char *s)
{
	int i;
	Endpt *ep;

	ep = td->ep;
	print("\t%s: 0x%.8lux ctrl 0x%.8lux cbp 0x%.8lux "
		"nexttd 0x%.8lux be 0x%.8lux, flags %lux\n",
		s, td, td->ctrl, td->cbp, td->nexttd, td->be, td->flags);
	if(ep->epmode != Isomode){
		print("\t\tbytes: %ld\n", td->be + 1 - td->cbp);
		return;
	}
	print("\t\t0x%ux 0x%ux 0x%ux 0x%ux 0x%ux 0x%ux 0x%ux 0x%ux\n",
		td->offsets[0], td->offsets[1], td->offsets[2], td->offsets[3],
		td->offsets[4], td->offsets[5], td->offsets[6], td->offsets[7]);
	print("\t\tbytes:");
	for(i = 0; i < td->ctrl >> 24 & 0x7; i++)
		print(" %d", (td->offsets[i+1]-td->offsets[i])&0xfff);
	print(" %ld\n", (td->be + 1 - td->offsets[i]) & 0xfff);
}

static void
dumped(ED *ed)
{
	TD *tailp, *td;

	tailp = EDgettail(ed);
	td = EDgethead(ed);
	print("dumpED 0x%lux: ctrl 0x%lux tail 0x%lux head 0x%lux next 0x%lux\n",
		ed, ed->ctrl, ed->tail, ed->head, ed->next);
	if(tailp == td)
		return;
	do {
		dumptd(td, "td");
	} while((td = TDgetnexttd(td)) != tailp);
}

static void
dumpstatus(Ctlr *ub)
{
	ED *ed;

	print("dumpstatus 0x%lux, frame 0x%ux:\n", ub, ub->uchcca->framenumber);
	print("control 0x%lux, cmdstat 0x%lux, intrsts 0x%lux\n",
		ub->base->control, ub->base->cmdsts, ub->base->intrsts);
	print("Control:\n");
	for(ed = OHCIgetControlHeadED(ub->base); ed; ed = EDgetnext(ed))
		dumped(ed);
	print("Bulk:\n");
	for(ed = OHCIgetBulkHeadED(ub->base); ed; ed = EDgetnext(ed))
		dumped(ed);
	print("Iso:\n");
	for(ed = ub->tree->root[0]; ed; ed = EDgetnext(ed))
		dumped(ed);
	print("frame 0x%ux:\n", ub->uchcca->framenumber);
}

/*
 * halt the ED and free input or output transfer descs
 * called when the relevant lock in the enclosing Endpt is held
 */

static void
EDcancel(Ctlr *ub, ED *ed, int dirin)
{
	int tddir, iso, n;
	TD *tailp, *headp, *td, *prev, *next;
	Endpt *ep;

	if(ed == nil)
		return;

	/* halt ED if not already halted */
	if(EDgetH(ed) != 1){
		EDsetH(ed, 1);
		waitSOF(ub);
	}

	SET(tddir);
	if((iso = ed->ctrl & ED_F_BIT) != 0)
		switch((ed->ctrl >> 11) & 0x3){
		default:
			panic("ED iso direction unset");
		case Otokin:	tddir = Dirin;	break;
		case Otokout:	tddir = Dirout;	break;
		}

	/* can now clean up TD list of ED */
	tailp = EDgettail(ed);
	headp = EDgethead(ed);
	n = 0;
	prev = nil;
	td = headp;
	while(td != tailp){
		ep = td->ep;
		if(iso == 0)
			switch((td->ctrl >> TD_DP_SHIFT) & TD_DP_MASK){
			default:
				panic("TD direction unset");
			case Otoksetup:	tddir = Dirout;	break;
			case Otokin:	tddir = Dirin;	break;
			case Otokout:	tddir = Dirout;	break;
			}
		else if(usbhdebug || ep->debug)
			print("EDcancel: buffered: %d, bytes %ld\n",
				ep->buffered, td->bytes);
		next = TDgetnexttd(td);
		if(dirin == 2 || dirin == tddir){
			XEPRINT("%d/%d: EDcancel %d\n", ep->dev->x, ep->x, tddir);
			/* Remove this sucker */
			ep->buffered -= td->bytes;
			if(ep->buffered < 0)
				ep->buffered = 0;
			ilock(ub);
			ep->dir[tddir].queued--;
			if(tddir == Dirout){
				freeb(td->bp);
				td->bp = nil;
			}
			if(prev)
				TDsetnexttd(prev, next);
			else
				EDsethead(ed, next);
			TDfree(ub, td);
			n++;
			iunlock(ub);
		}else{
			XEPRINT("%d/%d: EDcancel skip %d\n", ep->dev->x, ep->x,
				tddir);
			prev = td;
		}
		td = next;
	}
	XPRINT("EDcancel: %d\n", n);
}

static void
eptactivate(Ctlr *ub, Endpt *ep)
{
	Endptx *epx;

	qlock(&ub->activends);
	if(ep->active == 0){
		epx = ep->private;
		XEPRINT("%d/%d: activate\n", ep->dev->x, ep->x);
		ep->active = 1;
		/*
		 * set the right speed
		 */
		EDsetS(epx->ed, ep->dev->speed);
		switch(ep->epmode){
		case Ctlmode:
			/*
			 * chain the two descs together, and
			 * bind to beginning of control queue
			 */
			EDsetnext(epx->ed, OHCIgetControlHeadED(ub->base));
			OHCIsetControlHeadED(ub->base, epx->ed);
			/*
			 * prompt controller to absorb new queue on next pass
			 */
			ub->base->cmdsts |= Clf;
			XEPRINT("%d/%d: activated in control queue\n",
				ep->dev->x, ep->x);
			break;
		case Bulkmode:
			EDsetnext(epx->ed, OHCIgetBulkHeadED(ub->base));
			OHCIsetBulkHeadED(ub->base, epx->ed);
			ub->base->cmdsts |= Blf;
			XEPRINT("%d/%d: activated %s in bulk input queue\n",
				ep->dev->x, ep->x, ousbmode[ep->mode]);
			break;
		case Isomode:
			if(ep->mode != OWRITE)
				schedendpt(ub, ep, Dirin);
			if(ep->mode != OREAD)
				schedendpt(ub, ep, Dirout);
			ep->buffered = 0;
			ep->partial = 0;
			break;
		case Intrmode:
			if(ep->mode != OWRITE)
				schedendpt(ub, ep, Dirin);
			if(ep->mode != OREAD)
				schedendpt(ub, ep, Dirout);
			break;
		case Nomode:
			break;
		default:
			panic("eptactivate: wierd epmode %d\n", ep->epmode);
		}
		ep->dir[Dirin].xdone  = ep->dir[Dirin].xstarted =  0;
		ep->dir[Dirout].xdone = ep->dir[Dirout].xstarted = 0;
		ep->activef = ub->activends.f;
		ub->activends.f = ep;
	}
	qunlock(&ub->activends);
}

static void
EDpullfrombulk(Ctlr *ub, ED *ed)
{
	ED *this, *prev, *next;

	this = OHCIgetBulkHeadED(ub->base);
	ub->base->bulkcurred = 0;
	prev = nil;
	while(this != nil && this != ed){
		prev = this;
		this = EDgetnext(this);
	}
	if(this == nil){
		print("EDpullfrombulk: not found\n");
		return;
	}
	next = EDgetnext(this);
	if(prev == nil)
		OHCIsetBulkHeadED(ub->base, next);
	else
		EDsetnext(prev, next);
	EDsetnext(ed, nil);		/* wipe out next field */
}

static void
EDpullfromctl(Ctlr *ub, ED *ed)
{
	ED *this, *prev, *next;

	this = OHCIgetControlHeadED(ub->base);
	ub->base->ctlcurred = 0;
	prev = nil;
	while(this != nil && this != ed){
		prev = this;
		this = EDgetnext(this);
	}
	if(this == nil)
		panic("EDpullfromctl: not found\n");
	next = EDgetnext(this);
	if(prev == nil)
		OHCIsetControlHeadED(ub->base, next);
	else
		EDsetnext(prev, next);
	EDsetnext(ed, nil);		/* wipe out next field */
}

static void
eptdeactivate(Ctlr *ub, Endpt *ep)
{
	ulong ctrl;
	Endpt **l;
	Endptx *epx;

	/* could be O(1) but not worth it yet */
	qlock(&ub->activends);
	if(ep->active){
		epx = ep->private;
		XEPRINT("ohci: eptdeactivate %d/%d\n", ep->dev->x, ep->x);
		ep->active = 0;
		for(l = &ub->activends.f; *l != ep; l = &(*l)->activef)
			if(*l == nil){
				qunlock(&ub->activends);
				panic("usb eptdeactivate");
			}
		*l = ep->activef;
		/* pull it from the appropriate queue */
		ctrl = ub->base->control;
		switch(ep->epmode){
		case Ctlmode:
			if(ctrl & Cle){
				ub->base->control &= ~Cle;
				waitSOF(ub);
			}
			EDpullfromctl(ub, epx->ed);
			if(ctrl & Cle){
				ub->base->control |= Cle;
				/*
				 * don't fill it if there is nothing in it -
				 * shouldn't be necessary according to the
				 * spec., but practice is different
				 */
				if(OHCIgetControlHeadED(ub->base))
					ub->base->cmdsts |= Clf;
			}
			break;
		case Bulkmode:
			if(ctrl & Ble){
				ub->base->control &= ~Ble;
				waitSOF(ub);
			}
			EDpullfrombulk(ub, epx->ed);
			if(ctrl & Ble){
				ub->base->control |= Ble;
				/*
				 * don't fill it if there is nothing in it -
				 * shouldn't be necessary according to the
				 * spec., but practice is different
				 */
				if(OHCIgetBulkHeadED(ub->base))
					ub->base->cmdsts |= Blf;
			}
			break;
		case Intrmode:
		case Isomode:
			if(ep->mode != OWRITE)
				unschedendpt(ub, ep, Dirin);
			if(ep->mode != OREAD)
				unschedendpt(ub, ep, Dirout);
			waitSOF(ub);
			break;
		case Nomode:
			break;
		default:
			panic("eptdeactivate: wierd in.epmode %d\n",
				ep->epmode);
		}
	}
	qunlock(&ub->activends);
}

static void
kickappropriatequeue(Ctlr *ub, Endpt *ep, int)
{
	switch(ep->epmode){
	case Nomode:
		break;
	case Ctlmode:
		ub->base->cmdsts |= Clf;
		break;
	case Bulkmode:
		ub->base->cmdsts |= Blf;
		break;
	case Intrmode:
	case Isomode:
		/* no kicking required */
		break;
	default:
		panic("wierd epmode %d\n", ep->epmode);
	}
}

static void
eptenable(Ctlr *ub, Endpt *ep, int dirin)
{
	ED *ed;
	Endptx *epx;

	epx = ep->private;
	ed = epx->ed;
	if(EDgetH(ed) == 1){
		EDsetH(ed, 0);
		kickappropriatequeue(ub, ep, dirin);
		if(ep->epmode == Isomode || ep->epmode == Intrmode)
			waitSOF(ub);
	}
}

/*
 * return smallest power of 2 >= n
 */
static int
flog2(int n)
{
	int i;

	for(i = 0; (1 << i) < n; i++)
		;
	return i;
}

/*
 * return smallest power of 2 <= n
 */
static int
flog2lower(int n)
{
	int i;

	for(i = 0; (1 << (i + 1)) <= n; i++)
		;
	return i;
}

static int
pickschedq(QTree *qt, int pollms, ulong bw, ulong limit)
{
	int i, j, d, upperb, q;
	ulong best, worst, total;

	d = flog2lower(pollms);
	if(d > qt->depth)
		d = qt->depth;
	q = -1;
	worst = 0;
	best = ~0;
	upperb = (1 << (d+1)) - 1;
	for(i = (1 << d) - 1; i < upperb; i++){
		total = qt->bw[0];
		for(j = i; j > 0; j = (j - 1) / 2)
			total += qt->bw[j];
		if(total < best){
			best = total;
			q = i;
		}
		if(total > worst)
			worst = total;
	}
	if(worst + bw >= limit)
		return -1;
	return q;
}

static int
schedendpt(Ctlr *ub, Endpt *ep, int dirin)
{
	int q;
	ED *ed;
	Endptx *epx;

	epx = ep->private;
	qlock(ub->tree);
	/* TO DO: bus bandwidth limit */
	q = pickschedq(ub->tree, ep->pollms, ep->bw, ~0);
	XEPRINT("schedendpt, dir %d Q index %d, ms %d, bw %ld\n",
		dirin, q, ep->pollms, ep->bw);
	if(q < 0){
		qunlock(ub->tree);
		return -1;
	}
	ub->tree->bw[q] += ep->bw;
	ed = ub->tree->root[q];
	ep->sched = q;
	EDsetnext(epx->ed, EDgetnext(ed));
	EDsetnext(ed, epx->ed);
	XEPRINT("%d/%d: sched on q %d pollms %d\n",
		ep->dev->x, ep->x, q, ep->pollms);
	qunlock(ub->tree);
	return 0;
}

static void
unschedendpt(Ctlr *ub, Endpt *ep, int dirin)
{
	int q;
	ED *prev, *this, *next;
	Endptx *epx;

	epx = ep->private;
	if((q = ep->sched) < 0)
		return;
	qlock(ub->tree);
	ub->tree->bw[q] -= ep->bw;

	prev = ub->tree->root[q];
	this = EDgetnext(prev);
	while(this != nil && this != epx->ed){
		prev = this;
		this = EDgetnext(this);
	}
	if(this == nil)
		print("unschedendpt %d %d: not found\n", dirin, q);
	else{
		next = EDgetnext(this);
		EDsetnext(prev, next);
	}
	qunlock(ub->tree);
}

/* at entry, *e is partly populated */
static void
epalloc(Usbhost *uh, Endpt *ep)
{
	int id;
	Endptx *epx;
	Ctlr *ctlr;
	Udev *d;
	TD *dtd;

	XEPRINT("ohci: epalloc from devusb\n");
	ctlr = uh->ctlr;
	id = ep->id;
	d = ep->dev;

	epx = malloc(sizeof(Endptx));
	memset(epx, 0, sizeof(Endptx));
	ep->private = epx;

	dtd = nil;
	if(waserror()){
		XEPRINT("ohci: epalloc error\n");
		EDfree(ctlr, epx->ed);
		epx->ed = nil;
		TDfree(ctlr, dtd);
		nexterror();
	}
	if(epx->ed)
		error("usb: already allocated");
	if((epx->ed = EDalloc(ctlr)) == nil)
		error(Enomem);
	ep->bw = 1;			/* all looks the same currently */
	if((dtd = TDalloc(ctlr, ep, 0)) == nil)
		error(Enomem);
	EDinit(epx->ed, ep->maxpkt, 0, 0, 0, 0, id, d->id, dtd, dtd, 0, 0, 0);
	XEPRINT("ohci: epalloc done\n");
	poperror();
}

static void
epfree(Usbhost *uh, Endpt *ep)
{
	Endptx *epx;
	Ctlr *ctlr;

	epx = ep->private;
	ctlr = uh->ctlr;
	XEPRINT("ohci: epfree %d/%d from devusb\n", ep->dev->x, ep->x);

	if(ep->active)
		panic("epfree: active");
	EDfree(ctlr, epx->ed);
	epx->ed = nil;
	free(epx);
	ep->private = nil;
}

static void
epopen(Usbhost *uh, Endpt *ep)
{
	Ctlr *ctlr;

	XEPRINT("ohci: epopen %d/%d from devusb\n", ep->dev->x, ep->x);
	ctlr = uh->ctlr;
	if((ep->epmode == Isomode || ep->epmode == Intrmode) && ep->active)
		error("already open");
	eptactivate(ctlr, ep);
}

static int
setfrnum(Ctlr *ub, Endpt *ep)
{
	short frnum, d;
	static int adj;

	/* adjust frnum as necessary... */
	frnum = ub->base->fmnumber + (ep->buffered*1000)/ep->bw;
	d = frnum - ep->frnum;
	if(d < -100 || d > 100){
		/* We'd play in the past */
		if(0 && d > 1000)
			/* We're more than a second off: */
			print("d %d, done %d, started %d, buffered %d\n", d,
				ep->dir[Dirout].xdone, ep->dir[Dirout].xstarted,
				ep->buffered);
		if(ep->dir[Dirout].xdone == ep->dir[Dirout].xstarted)
			ep->buffered = adj = 0;
		if(0 && (adj++ & 0xff) == 0)
			print("adj %d %d\n", d, ep->buffered);
		ep->frnum = ub->base->fmnumber + 10 + (ep->buffered*1000)/ep->bw;
		ep->partial = 0;
		return 1;
	}
	return 0;
}

static int
ceptdone(void *arg)
{
	Endpt *ep;

	ep = arg;
	return ep->dir[Dirout].xdone - ep->dir[Dirout].xstarted >= 0
		|| ep->dir[Dirout].err;
}

static void
epclose(Usbhost *uh, Endpt *ep)
{
	Ctlr *ctlr;
	int xdone, part;
	Endptx *epx;

	XEPRINT("ohci: epclose %d/%d from devusb, %d buffered\n",
		ep->dev->x, ep->x, ep->buffered);
	ctlr = uh->ctlr;
	epx = ep->private;
	if(ep->epmode == Isomode && ep->active){
		qlock(&ep->wlock);
		if(ep->partial && setfrnum(ctlr, ep) == 0){
			part = ep->partial;
			memset(ep->bpartial->wp, 0, ep->maxpkt - ep->partial);
			ep->bpartial->wp = ep->bpartial->rp + ep->maxpkt;
			qtd(uh->ctlr, ep, Dirout, nil, ep->bpartial->rp,
				ep->bpartial->wp, Otokout, TD_FLAGS_LAST);
			XEPRINT("epclose: wrote partial block %d\n", part);
			ep->partial = 0;
		}
		qunlock(&ep->wlock);
		XEPRINT("epclose: wait for outstanding TDs, xdone %d"
			", xstarted %d, buffered %d, queued %d\n",
			ep->dir[Dirout].xdone, ep->dir[Dirout].xstarted,
			ep->buffered, ep->dir[Dirout].queued);
		while(ep->dir[Dirout].err == nil
		&& (xdone = ep->dir[Dirout].xdone) - ep->dir[Dirout].xstarted < 0){
			tsleep(&ep->dir[Dirout].rend, ceptdone, ep, 500);
			if(xdone == ep->dir[Dirout].xdone){
				print("no progress\n");
				break;
			}
		}
		if(ep->dir[Dirout].err)
			XEPRINT("error: %s\n", ep->dir[Dirout].err);
		if(ep->buffered)
			XEPRINT("epclose: done waiting, xdone %d, xstarted %d, "
				"buffered %d, queued %d\n",
				ep->dir[Dirout].xdone, ep->dir[Dirout].xstarted,
				ep->buffered, ep->dir[Dirout].queued);
	}
	lock(epx);
	EDcancel(ctlr, epx->ed, 2);
	unlock(epx);
	if(ep->epmode == Isomode && ep->buffered)
		XEPRINT("epclose: after cancelling, xdone %d, xstarted %d"
			", buffered %d, queued %d\n",
			ep->dir[Dirout].xdone, ep->dir[Dirout].xstarted,
			ep->buffered, ep->dir[Dirout].queued);
	eptdeactivate(ctlr, ep);
}

static void
epmaxpkt(Usbhost *, Endpt *ep)
{
	Endptx *epx;

	epx = ep->private;
	XEPRINT("ohci: epmaxpkt %d/%d: %d\n",
		ep->dev->x, ep->x, ep->maxpkt);
	EDsetMPS(epx->ed, ep->maxpkt);
}

static void
epmode(Usbhost *uh, Endpt *ep)
{
	int tok, reactivate;
	Ctlr *ctlr;
	Endptx *epx;

	epx = ep->private;
	ctlr = uh->ctlr;
	XEPRINT("ohci: epmode %d/%d %s → %s\n",
		ep->dev->x, ep->x, usbmode[ep->epmode], usbmode[ep->epnewmode]);
	reactivate = 0;
	if(ep->epnewmode != ep->epmode)
		if(reactivate = ep->active){
			XEPRINT("ohci: epmode %d/%d: already open\n",
				ep->dev->x, ep->x);
			eptdeactivate(ctlr, ep);
		}
	EDsetS(epx->ed, ep->dev->speed);
	switch(ep->epnewmode){
	default:
		panic("devusb is sick");
	case Intrmode:
//		ep->debug++;
		ep->bw = ep->maxpkt*1000/ep->pollms;	/* bytes/sec */
		XEPRINT("ohci: epmode %d/%d %s, intr: maxpkt %d, pollms %d, bw %ld\n",
			ep->dev->x, ep->x, ousbmode[ep->mode],
			ep->maxpkt, ep->pollms, ep->bw);
		break;
	case Isomode:
//		ep->debug++;
		ep->rem = 999;
		switch(ep->mode){
		default:
			panic("ep mode");
		case ORDWR:
			error("iso unidirectional only");
		case OREAD:
			tok = Otokin;
			error("iso read not implemented");
			break;
		case OWRITE:
			tok = Otokout;
			break;
		}
		XEPRINT("ohci: epmode %d/%d %s, iso: maxpkt %d, pollms %d, hz %d, samp %d\n",
			ep->dev->x, ep->x, ousbmode[ep->mode],
			ep->maxpkt, ep->pollms, ep->hz, ep->samplesz);
		ep->bw = ep->hz * ep->samplesz;		/* bytes/sec */
		/* Use Iso TDs: */
		epx->ed->ctrl |= ED_F_BIT;
		/* Set direction in ED, no room in an Iso TD for this */
		epx->ed->ctrl &= ~(ED_D_MASK << ED_D_SHIFT);
		epx->ed->ctrl |= tok << ED_D_SHIFT;
		break;
	case Bulkmode:
//		ep->debug++;
		/*
		 * Each Bulk device gets a queue head hanging off the
		 * bulk queue head
		 */
		break;
	case Ctlmode:
		break;
	}
	ep->epmode = ep->epnewmode;
	epmaxpkt(uh, ep);
	if(reactivate){
		XEPRINT("ohci: epmode %d/%d: reactivate\n", ep->dev->x, ep->x);
		eptactivate(ctlr, ep);
	}
}

static long
qtd(Ctlr *ub, Endpt *ep, int dirin , Block *bp, uchar *base, uchar *limit,
	int pid, ulong flags)
{
	int fc, mps;
	ulong x;
	uchar *p;
	ED *ed;
	Endptx *epx;
	TD *dummytd, *td;

	epx = ep->private;
	ed = epx->ed;
	td = hcva2ucva(ed->tail);
	td->flags = flags;
	if(ep->epmode == Isomode){
		x = va2hcva(base);
		td->cbp = x & ~0xfff;
		x &= 0xfff;
		p = base;
		setfrnum(ub, ep);
		td->ctrl = ep->frnum & 0xffff;
		fc = 0;
		for(;;){
			/* Calculate number of samples in next packet */
			mps = (ep->hz + ep->rem)/1000;
			/* rem is the partial sample left over */
			ep->rem += ep->hz - 1000*mps;
			mps *= ep->samplesz;
			if(mps > ep->maxpkt)
				panic("Packet size");
			if(ep->partial == 0 && mps > limit - p){
				/* Save this data for later ... */
				ep->partial = limit - p;
				if(fc-- == 0)
					return p - base;	/* No TD */
				/* We do have a TD, send this one off normally */
				td->flags |= TD_FLAGS_LAST;
				break;
			}else if(mps >= limit - p){
				td->flags |= TD_FLAGS_LAST;
				mps = limit - p;
				ep->partial = 0;
			}
			td->offsets[fc] = 0xe000 | x;
			x += mps;
			p += mps;
			ep->frnum++;
			if(fc == 7 || limit - p == 0)
				break;
			fc++;
		}
		td->ctrl |= fc << 24;
	}else{
		td->cbp = va2hcva(base);
		td->ctrl = (pid & TD_DP_MASK) << TD_DP_SHIFT;
		p = base;
		mps = 0x2000 - ((ulong)p & 0xfff);
		if(mps > ep->maxpkt)
			mps = ep->maxpkt;
		if(mps >= limit - p){
			mps = limit - base;
			td->flags |= TD_FLAGS_LAST;
		}
		p += mps;
	}
	td->be = va2hcva(p == nil? nil: p - 1);
	td->ep = ep;
	td->bytes = p - base;
	ep->buffered += td->bytes;
	td->bp = bp;
	if(td->flags & TD_FLAGS_LAST)
		ep->dir[dirin].xstarted++;
	if(dirin == Dirout && bp)
		refcnt(bp, 1);
	dummytd = TDalloc(ub, ep, 1);
	TDsetnexttd(td, dummytd);
	ep->dir[dirin].queued++;
	EDsettail(ed, dummytd);
	if(usbhdebug || ep->debug)
		dumptd(td, "qtd: before");
	kickappropriatequeue(ub, ep, dirin);
	return p - base;
}

Block*
allocrcvb(long size)
{
	Block *b;
	int asize;

	asize = ROUND(size, dcls) + dcls - 1;
	/*
	 * allocate enough to align rp to dcls, and have an integral number
	 * of cache lines in the buffer
	 */
	while(waserror())
		tsleep(&up->sleep, return0, 0, 100);
	b = allocb(asize);
	poperror();
	/*
	 * align the rp and wp
	 */
	b->rp = b->wp = (uchar *)ROUND((ulong)b->rp, dcls);
	/*
	 * invalidate the cache lines which enclose the buffer
	 */
	if(IOCACHED){
		uchar *p;

		p = b->rp;
		while(size > 0){
			invalidatedcacheva((ulong)p);
			p += dcls;
			size -= dcls;
		}
	}
	return b;
}

/*
 * build the periodic scheduling tree:
 * framesize must be a multiple of the tree size
 */
static QTree *
mkqhtree(Ctlr *ub)
{
	int i, n, d, o, leaf0, depth;
	ED **tree;
	QTree *qt;

	depth = flog2(32);
	n = (1 << (depth+1)) - 1;
	qt = mallocz(sizeof(*qt), 1);
	if(qt == nil)
		return nil;
	qt->nel = n;
	qt->depth = depth;
	qt->bw = mallocz(n * sizeof(qt->bw), 1);
	if(qt->bw == nil){
		free(qt);
		return nil;
	}
	tree = mallocz(n * sizeof(ED *), 1);
	if(tree == nil){
		free(qt->bw);
		free(qt);
		return nil;
	}
	for(i = 0; i < n; i++)
		if((tree[i] = EDalloc(ub)) == nil)
			break;

	if(i < n){
		int j;

		for(j = 0; j < i; j++)
			EDfree(ub, tree[j]);
		free(tree);
		free(qt->bw);
		free(qt);
		return nil;
	}

	qt->root = tree;
	EDinit(qt->root[0], 8, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0);

	for(i = 1; i < n; i++)
		EDinit(tree[i], 8, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, tree[(i-1)/2]);

	/* distribute leaves evenly round the frame list */
	leaf0 = n / 2;
	for(i = 0; i < 32; i++){
		o = 0;
		for(d = 0; d < depth; d++){
			o <<= 1;
			if(i & (1 << d))
				o |= 1;
		}
		if(leaf0 + o >= n){
			print("leaf0=%d o=%d i=%d n=%d\n", leaf0, o, i, n);
			break;
		}
		ub->uchcca->intrtable[i] = va2hcva(tree[leaf0 + o]);
	}
	return qt;
}

static void
portreset(Usbhost *uh, int port)
{
	Ctlr *ctlr;

	XIPRINT("ohci: portreset(port %d) from devusb\n", port);
	ctlr = uh->ctlr;
	/* should check that device not being configured on other port? */
	qlock(&ctlr->resetl);
	if(waserror()){
		qunlock(&ctlr->resetl);
		nexterror();
	}
	ilock(ctlr);
	ctlr->base->rhportsts[port - 1] = Spp | Spr;
	while((ctlr->base->rhportsts[port - 1] & Prsc) == 0){
		iunlock(ctlr);
		XIPRINT("ohci: portreset, wait for reset complete\n");
		ilock(ctlr);
	}
	ctlr->base->rhportsts[port - 1] = Prsc;
	iunlock(ctlr);
	poperror();
	qunlock(&ctlr->resetl);
}

static void
portenable(Usbhost *uh, int port, int on)
{
	Ctlr *ctlr;

	XIPRINT("ohci: portenable(port %d, on %d) from devusb\n", port, on);
	ctlr = uh->ctlr;
	/* should check that device not being configured on other port? */
	qlock(&ctlr->resetl);
	if(waserror()){
		qunlock(&ctlr->resetl);
		nexterror();
	}
	ilock(ctlr);
	if(on)
		ctlr->base->rhportsts[port - 1] = Spe | Spp;
	else
		ctlr->base->rhportsts[port - 1] = Cpe;
	iunlock(ctlr);
	poperror();
	qunlock(&ctlr->resetl);
}

static int
getportstatus(Ctlr *ub, int port)
{
	int v;
	ulong ohcistatus;

	ohcistatus = ub->base->rhportsts[port - 1];
	v = 0;
	if(ohcistatus & Ccs)
		v |= DevicePresent;
	if(ohcistatus & Pes)
		v |= PortEnable;
	if(ohcistatus & Pss)
		v |= Suspend;
	if(ohcistatus & Prs)
		v |= PortReset;
	else {
		/* port is not in reset; these potential writes are ok */
		if(ohcistatus & Csc){
			/* TODO: could notify usbd equivalent here */
			v |= ConnectStatusChange;
			ub->base->rhportsts[port - 1] = Csc;
		}
		if(ohcistatus & Pesc){
			/* TODO: could notify usbd equivalent here */
			v |= PortEnableChange;
			ub->base->rhportsts[port - 1] = Pesc;
		}
	}
	if(ohcistatus & Lsda)
		v |= SlowDevice;
	if(ohcistatus & Ccs)
		XIPRINT("portstatus(%d) = OHCI 0x%.8lux UHCI 0x%.8ux\n",
			port, ohcistatus, v);
	return v;
}

/* print any interesting stuff here for debugging purposes */
static void
usbdebug(Usbhost *uh, char *s, char *se)
{
	Udev *dev;
	Endpt *ep;
	int n, i, j;

	n = 0;
	for(i = 0; i < MaxUsbDev; i++)
		if(uh->dev[i])
			n++;
	s = seprint(s, se, "OHCI, %d devices\n", n);
	for(i = 0; i < MaxUsbDev; i++){
		if((dev = uh->dev[i]) == nil)
			continue;
		s = seprint(s, se, "dev 0x%6.6lux, %d epts\n", dev->csp, dev->npt);
		for(j = 0; j < nelem(dev->ep); j++){
			if((ep = dev->ep[j]) == nil)
				continue;
			if(ep->epmode >= 0 && ep->epmode < Nmodes)
				s = seprint(s, se, "ept %d/%d: %s 0x%6.6lux "
					"maxpkt %d %s\n",
					dev->x, ep->x, usbmode[ep->epmode],
					ep->csp, ep->maxpkt, ep->active ? "active" : "idle");
			else{
				s = seprint(s, se, "ept %d/%d: bad mode 0x%6.6lux\n",
					dev->x, ep->x, ep->csp);
				continue;
			}
			
			switch(ep->epmode){
			case Nomode:
				break;
			case Ctlmode:
				break;
			case Bulkmode:
				break;
			case Intrmode:
				s = seprint(s, se, "\t%d ms\n",
					ep->pollms);
				break;
			case Isomode:
				s = seprint(s, se, "\t%d ms, remain %d, "
					"partial %d, buffered %d, "
					"xdone %d, xstarted %d, err %s\n",
					ep->pollms, ep->remain, ep->partial,
					ep->buffered,
					ep->dir[ep->mode == OREAD].xdone,
					ep->dir[ep->mode == OREAD].xstarted,
					ep->dir[ep->mode == OREAD].err
					? ep->dir[ep->mode == OREAD].err : "no");
				break;
			}
		}
	}
}

/* this is called several times every few seconds, possibly due to usbd */
static void
portinfo(Usbhost *uh, char *s, char *se)
{
	int x, i, j;
	Ctlr *ctlr;

	XIPRINT("ohci: portinfo from devusb\n");
	ctlr = uh->ctlr;
	for(i = 1; i <= ctlr->nports; i++){
		ilock(ctlr);
		x = getportstatus(ctlr, i);
		iunlock(ctlr);
		s = seprint(s, se, "%d %ux", i, x);
		for(j = 0; j < nelem(portstatus); j++)
			if((x & portstatus[j].bit) != 0)
				s = seprint(s, se, " %s", portstatus[j].name);
		s = seprint(s, se, "\n");
	}
}

void
interrupt(Ureg *, void *arg)
{
	int dirin, cc;
	ulong ctrl, status;
	uchar *p;
	Block *bp;
	Ctlr *ub;
	Endpt *ep;
	Endptx *epx;
	TD *donetd, *nexttd;
	Usbhost *eh;

	XIPRINT("ohci: interrupt\n");
	eh = arg;
	ub = eh->ctlr;
	status = ub->base->intrsts;
	status &= ub->base->intrenable;
	status &= Oc | Rhsc | Fno
		| Ue
		| Rd | Sf | Wdh
		| So;
	if(status & Wdh){
		/* LSb of donehead has bit that says there are other interrupts */
		donetd = hcva2ucva(ub->uchcca->donehead & ~0xf);
		XIPRINT("donetd 0x%.8lux\n", donetd);
	}else
		donetd = 0;
	ub->base->intrsts = status;
	status &= ~Wdh;
	while(donetd){
		ctrl = donetd->ctrl;
		ep = donetd->ep;
		bp = donetd->bp;
		donetd->bp = nil;
		epx = ep->private;

		ohciinterrupts[ep->epmode]++;
		dirin = ((ctrl >> TD_DP_SHIFT) & TD_DP_MASK) == Otokin;
		ep->buffered -= donetd->bytes;
		if(ep->epmode == Isomode){
			dirin = Dirout;
			if(ep->buffered < 0){
				print("intr: buffered %d bytes %ld\n",
					ep->buffered, donetd->bytes);
				ep->buffered = 0;
			}
		}
		cc = (ctrl >> TD_CC_SHIFT) & TD_CC_MASK;
		if((usbhdebug || ep->debug) && (cc != 0 && cc != 9)){
			print("%d/%d: cc %d frnum 0x%lux\n",
				ep->dev->x, ep->x, cc, ub->base->fmnumber);
			dumptd(donetd, "after");
		}
		switch(cc){
		case 8:			/* Overrun, Not an error */
			epx->overruns++;
			/* fall through to no error code */
		case 0:			/* noerror */
			if((donetd->flags & TD_FLAGS_LAST) == 0)
				break;
			if(dirin){
				if(bp){
					p = hcva2va(donetd->be + 1);
					if(p < bp->wp)
						print("interrupt: bp: rp 0x%lux"
							", wp 0x%lux→0x%lux\n",
							bp->rp, bp->wp, p);
					bp->wp = p;
				}
			}
			ep->dir[dirin].xdone++;
			wakeup(&ep->dir[dirin].rend);
			break;
		case 9:			/* underrun */
			if(bp){
				p = hcva2va(donetd->cbp);
				XEIPRINT("interrupt: bp: rp 0x%lux, wp "
					"0x%lux→0x%lux\n", bp->rp, bp->wp, p);
				bp->wp = p;
			}
			if((donetd->flags & TD_FLAGS_LAST) == 0){
				XEIPRINT("Underrun\n");
				ep->dir[dirin].err = Eunderrun;
			}
			ep->dir[dirin].xdone++;
			wakeup(&ep->dir[dirin].rend);
			break;
		case 1:			/* CRC */
			ep->dir[dirin].err = "CRC error";
			goto error;
		case 2:			/* Bitstuff */
			ep->dir[dirin].err = "Bitstuff error";
			goto error;
		case 3:
			ep->dir[dirin].err = "data toggle mismatch";
			goto error;
		case 4:			/* Stall */
			ep->dir[dirin].err = Estalled;
			goto error;
		case 5:			/* No response */
			ep->dir[dirin].err = "No response";
			goto error;
		case 6:			/* PIDcheck */
			ep->dir[dirin].err = "PIDcheck";
			goto error;
		case 7:			/* UnexpectedPID */
			ep->dir[dirin].err = "badPID";
			goto error;
		error:
			XEPRINT("fail %d (%lud)\n", cc,
				(ctrl >> TD_EC_SHIFT) & TD_EC_MASK);
			ep->dir[dirin].xdone++;
			wakeup(&ep->dir[dirin].rend);
			break;
		default:
			panic("cc %lud unimplemented\n", cc);
		}
		ep->dir[dirin].queued--;
		/* Clean up blocks used for transfers */
		if(dirin == Dirout)
			freeb(bp);

		nexttd = TDgetnexttd(donetd);
		TDfree(ub, donetd);
		donetd = nexttd;
	}
	if(status & Sf){
		if (0)
			XIPRINT(("sof!!\n"));
		// wakeup(&ub->sofr);	/* sofr doesn't exist anywhere! */
		status &= ~Sf;
	}
	if(status & Ue){
		ulong curred;

		// usbhdbg();		/* TODO */
		curred = ub->base->periodcurred;
		print("usbh: unrecoverable error frame 0x%.8lux ed 0x%.8lux, "
			"ints %d %d %d %d\n",
			ub->base->fmnumber, curred,
			ohciinterrupts[1], ohciinterrupts[2],
			ohciinterrupts[3], ohciinterrupts[4]);
		if(curred)
			dumped(hcva2ucva(curred));
	}
	if(status)
		IPRINT(("interrupt: unhandled interrupt 0x%.8lux\n", status));
}

static void
usbhattach(Ctlr *ub)		/* TODO: is unused now, but it fiddles ctlr */
{
	ulong ctrl;

	if(ub == nil || ub->base == 0)
		error(Enodev);
	ctrl = ub->base->control;
	if((ctrl & HcfsMask) != HcfsOperational){
		ctrl = (ctrl & ~HcfsMask) | HcfsOperational;
		ub->base->control = ctrl;
		ub->base->rhsts = Sgp;
	}
}

static int
reptdone(void *arg)
{
	Endpt *ep;

	ep = arg;
	return ep->dir[Dirin].err
		/* Expression crafted to deal with wrap around: */
		|| ep->dir[Dirin].xdone - ep->dir[Dirin].xstarted >= 0;
}

Block *
breadusbh(Ctlr *ub, Endpt *ep, long n)		/* guts of read() */
{
	long in, l;
	uchar *p;
	Block *bp;
	Endptx *epx;

	epx = ep->private;
	qlock(&ep->rlock);
	EDsetC(epx->ed, ep->rdata01);
	XEPRINT("breadusbh(%d/%d, %ld, dt %d)\n", ep->dev->x, ep->x, n, ep->rdata01);
	eptenable(ub, ep, Dirin);
	if(waserror()){
		EDcancel(ub, epx->ed, Dirin);
		ep->dir[Dirin].err = nil;
		qunlock(&ep->rlock);
		nexterror();
	}
	if(ep->dir[Dirin].err != nil)
		error("usb: can't happen");
	bp = allocrcvb(n);
	in = n;
	if(in > bp->lim - bp->wp){
		print("usb: read larger than block\n");
		in = bp->lim - bp->wp;
	}
	p = bp->rp;
	do{
		l = qtd(ub, ep, Dirin, bp, p, p+in, Otokin, 0);
		p += l;
		in -= l;
	}while(in > 0);
	sleep(&ep->dir[Dirin].rend, reptdone, ep);
	if(ep->dir[Dirin].err){
		EDcancel(ub, epx->ed, Dirin);
		if(ep->dir[Dirin].err == Eunderrun)
			ep->dir[Dirin].err = nil;
		else
			error(ep->dir[Dirin].err);
	}
	XEPRINT("breadusbh(%d/%d, %ld) returned %ld\n", ep->dev->x, ep->x, n,
		BLEN(bp));
	poperror();
	qunlock(&ep->rlock);
	ep->rdata01 = EDgetC(epx->ed);
	return bp;
}

static long
read(Usbhost *uh, Endpt *ep, void *a, long n, vlong off) /* TODO off */
{
	long l;
	Block *bp;
	Ctlr *ub;

	XEPRINT("ohci: read from devusb\n");
	USED(off);
	ub = uh->ctlr;
	XEPRINT("%d/%d: read 0x%.8lux %ld\n", ep->dev->x, ep->x, a, n);
	bp = breadusbh(ub, ep, n);
	l = BLEN(bp);
	memmove(a, bp->rp, l);
	printdata(bp->rp, 1, l);
	XEPRINT("ohci: read %ld\n\n", l);
	freeb(bp);
	return l;
}

static int
weptdone(void *arg)
{
	Endpt *ep;

	ep = arg;
	/*
	 * success when all operations are done or when less than
	 * a second is buffered in iso connections
	 */
	return	ep->dir[Dirout].xdone - ep->dir[Dirout].xstarted >= 0
		|| (ep->epmode == Isomode && ep->buffered <= ep->bw)
		|| ep->dir[Dirout].err;
}

/* TODO: honour off */
static long
write(Usbhost *uh, Endpt *ep, void *a, long n, vlong off, int tok)
{
	long m;
	short frnum;
	uchar *p = a;
	Block *b;
	Ctlr *ub;
	Endptx *epx;

	XEPRINT("ohci: write(addr %p, bytes %ld, off %lld, tok %d) from devusb\n",
		a, n, off, tok);
	epx = ep->private;
	ub = uh->ctlr;
	qlock(&ep->wlock);
	XEPRINT("%d/%d: write 0x%.8lux %ld %s\n", ep->dev->x, ep->x, a, n,
		tok == Otoksetup? "setup": "out");
	if(ep->dir[Dirout].xdone - ep->dir[Dirout].xstarted > 0){
		print("done > started, %d %d\n",
			ep->dir[Dirout].xdone, ep->dir[Dirout].xstarted);
		ep->dir[Dirout].xdone = ep->dir[Dirout].xstarted;
	}
	if(waserror()){
		lock(epx);
		EDcancel(ub, epx->ed, Dirout);
		unlock(epx);
		ep->dir[Dirout].err = nil;
		qunlock(&ep->wlock);
		nexterror();
	}
	eptenable(ub, ep, Dirout);
	EDsetC(epx->ed, ep->wdata01);
	if(ep->dir[Dirout].err)
		error(ep->dir[Dirout].err);
	if((m = n) == 0 || p == nil)
		qtd(ub, ep, Dirout, 0, 0, 0, tok, TD_FLAGS_LAST);
	else{
		b = allocwb(m+ep->partial);
		if(ep->partial){
			memmove(b->wp, ep->bpartial->rp, ep->partial);
			b->wp += ep->partial;
			ep->partial = 0;
		}
		validaddr((uintptr)p, m, 0);		/* DEBUG */
		memmove(b->wp, a, m);
		b->wp += m;
		printdata(b->rp, 1, m);
		m = BLEN(b);
		dcclean(b->rp, m);
		if(ep->epmode == Isomode && ep->buffered <= ep->bw<<1){
			sleep(&ep->dir[Dirout].rend, weptdone, ep);
			if(ep->dir[Dirout].err)
				error(ep->dir[Dirout].err);
		}
		while(m > 0){
			int l;

			l = qtd(ub, ep, Dirout, b, b->rp, b->wp, tok, 0);
			b->rp += l;
			m -= l;
			tok = Otokout;
			if(ep->partial){
				/* We have some data to save */
				if(ep->bpartial == nil)
					ep->bpartial = allocb(ep->maxpkt);
				if(ep->partial != m)
					print("curious: %d != %ld\n",
						ep->partial, m);
				memmove(ep->bpartial->rp, b->rp, ep->partial);
				ep->bpartial->wp = ep->bpartial->rp + ep->partial;
				break;
			}
		}
		freeb(b);
	}
	if(ep->epmode != Isomode){
		sleep(&ep->dir[Dirout].rend, weptdone, ep);
		if(ep->dir[Dirout].err)
			error(ep->dir[Dirout].err);
	}else if(0 && (frnum = ep->frnum - ub->base->fmnumber) < 0)
			print("too late %d\n", frnum);
	poperror();
	qunlock(&ep->wlock);
	XEPRINT("ohci: wrote %ld\n\n", n);
	ep->wdata01 = EDgetC(epx->ed);
	return n;
}

static void
init(Usbhost*)
{
	XIPRINT("ohci: init from devusb\n");
}

static void
scanpci(void)
{
	ulong mem;
	Ctlr *ctlr;
	Pcidev *p;
	static int already = 0;

	if(already)
		return;
	already = 1;
	p = nil;
	while(p = pcimatch(p, 0, 0)) {
		/*
		 * Find OHCI controllers (Programming Interface = 0x10).
		 */
		if(p->ccrb != Pcibcserial || p->ccru != Pciscusb ||
		    p->ccrp != 0x10)
			continue;
		mem = p->mem[0].bar & ~0x0F;
		XPRINT("usbohci: %x/%x port 0x%lux size 0x%x irq %d\n",
			p->vid, p->did, mem, p->mem[0].size, p->intl);
		if(mem == 0){
			print("usbohci: failed to map registers\n");
			continue;
		}
		if(p->intl == 0xFF || p->intl == 0) {
			print("usbohci: no irq assigned for port %#lux\n", mem);
			continue;
		}

		ctlr = malloc(sizeof(Ctlr));
		ctlr->pcidev = p;
		ctlr->base = vmap(mem, p->mem[0].size);
		XPRINT("scanpci: ctlr 0x%lux, base 0x%lux\n", ctlr, ctlr->base);
		pcisetbme(p);
		pcisetpms(p, 0);
		if(ctlrhead != nil)
			ctlrtail->next = ctlr;
		else
			ctlrhead = ctlr;
		ctlrtail = ctlr;
	}
}

static int
reset(Usbhost *uh)
{
	int i, linesize;
	ulong io, fminterval, ctrl;
	Ctlr *ctlr;
	HCCA *atmp;
	OHCI *ohci;
	Pcidev *p;
	QTree *qt;

	/*
	 * data cache line size; probably doesn't matter on pc
	 * except that it must be a power of 2 for xspanalloc.
	 */
	dcls = 32;

	scanpci();

	/*
	 * Any adapter matches if no uh->port is supplied,
	 * otherwise the ports must match.
	 */
	for(ctlr = ctlrhead; ctlr != nil; ctlr = ctlr->next){
		if(ctlr->active)
			continue;
		if(uh->port == 0 || uh->port == (uintptr)ctlr->base){
			ctlr->active = 1;
			break;
		}
	}
	if(ctlr == nil)
		return -1;

	io = (uintptr)ctlr->base;	/* TODO: correct? */
	ohci = ctlr->base;

	XPRINT("OHCI ctlr 0x%lux, base 0x%lux\n", ctlr, ohci);

	p = ctlr->pcidev;

	uh->ctlr = ctlr;
	uh->port = io;
	uh->irq = p->intl;
	uh->tbdf = p->tbdf;

	XPRINT("OHCI revision %ld.%ld\n", (ohci->revision >> 4) & 0xf,
		ohci->revision & 0xf);
	XPRINT("Host revision %ld.%ld\n", (ohci->hostrevision >> 4) & 0xf,
		ohci->hostrevision & 0xf);
	ctlr->nports = ohci->rhdesca & 0xff;
	XPRINT("HcControl 0x%.8lux, %d ports\n", ohci->control, ctlr->nports);
	delay(100);  /* anything greater than 50 should ensure reset is done */
	if(ohci->control == ~0){
		ctlrhead = nil;
		return -1;
	}

	/*
	 * usually enter here in reset, wait till its through,
	 * then do our own so we are on known timing conditions.
	 */

	ohci->control = 0;
	delay(100);

	fminterval = ohci->fminterval;

	/* legacy support register: turn off lunacy mode */
	pcicfgw16(p, 0xc0, 0x2000);

	/*
	 * transfer descs need at least 16 byte alignment, but
	 * align to dcache line size since the access will always be uncached.
	 * linesize must be a power of 2 for xspanalloc.
	 */
	linesize = dcls;
	if(linesize < 0x20)
		linesize = 0x20;

	ctlr->td.pool = va2ucva(xspanalloc(Ntd * sizeof(TD), linesize, 0));
	if(ctlr->td.pool == nil)
		panic("usbohci: no memory for TD pool");
	for(i = Ntd - 1; --i >= 0;){
		ctlr->td.pool[i].next = ctlr->td.free;
		ctlr->td.free = &ctlr->td.pool[i];
	}
	ctlr->td.alloced = 0;

	ctlr->ed.pool = va2ucva(xspanalloc(Ned*sizeof(ED), linesize, 0));
	if(ctlr->ed.pool == nil)
		panic("usbohci: no memory for ED pool");
	for(i = Ned - 1; --i >= 0;){
		ctlr->ed.pool[i].next = (ulong)ctlr->ed.free;
		ctlr->ed.free = &ctlr->ed.pool[i];
	}
	ctlr->ed.alloced = 0;

	atmp = xspanalloc(sizeof(HCCA), 256, 0);
	if(atmp == nil)
		panic("usbhreset: no memory for HCCA");
	memset(atmp, 0, sizeof(*atmp));
	ctlr->uchcca = atmp;

	qt = mkqhtree(ctlr);
	if(qt == nil){
		panic("usb: can't allocate scheduling tree");
		return -1;
	}
	ctlr->tree = qt;

	ctlr->base = ohci;

	/* time to move to rest then suspend mode. */
	ohci->cmdsts = 1;         /* reset the block */
	while(ohci->cmdsts == 1)
		continue;  /* wait till reset complete, OHCI says 10us max. */
	/*
	 * now that soft reset is done we are in suspend state.
	 * Setup registers which take in suspend state
	 * (will only be here for 2ms).
	 */

	ohci->hcca = va2hcva(ctlr->uchcca);
	OHCIsetControlHeadED(ctlr->base, 0);
	ctlr->base->ctlcurred = 0;
	OHCIsetBulkHeadED(ctlr->base, 0);
	ctlr->base->bulkcurred = 0;

	ohci->intrenable = Mie | Wdh | Ue;
	ohci->control |= Cle | Ble | Ple | Ie | HcfsOperational;

	/* set frame after operational */
	ohci->fminterval = (fminterval &
		~(HcFmIntvl_FSMaxpack_MASK << HcFmIntvl_FSMaxpack_SHIFT)) |
		5120 << HcFmIntvl_FSMaxpack_SHIFT;
	ohci->rhdesca = 1 << 9;

	for(i = 0; i < ctlr->nports; i++)
		ohci->rhportsts[i] = Spp | Spr;

	delay(100);

	ctrl = ohci->control;
	if((ctrl & HcfsMask) != HcfsOperational){
		XIPRINT("ohci: reset, take ctlr out of Suspend\n");
		ctrl = (ctrl & ~HcfsMask) | HcfsOperational;
		ohci->control = ctrl;
		ohci->rhsts = Sgp;
	}

	p = ctlr->pcidev;
	ctlr->irq = p->intl;
	ctlr->tbdf = p->tbdf;

	/*
	 * Linkage to the generic USB driver.
	 */
	uh->init = init;
	uh->interrupt = interrupt;

	uh->debug = usbdebug;
	uh->portinfo = portinfo;
	uh->portreset = portreset;
	uh->portenable = portenable;

	uh->epalloc = epalloc;
	uh->epfree = epfree;
	uh->epopen = epopen;
	uh->epclose = epclose;
	uh->epmode = epmode;
	uh->epmaxpkt = epmaxpkt;

	uh->read = read;
	uh->write = write;

	uh->tokin = Otokin;
	uh->tokout = Otokout;
	uh->toksetup = Otoksetup;

	return 0;
}

void
usbohcilink(void)
{
	addusbtype("ohci", reset);
}
