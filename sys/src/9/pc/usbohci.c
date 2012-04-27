/*
 * USB Open Host Controller Interface (Ohci) driver
 *
 * BUGS:
 * - Missing isochronous input streams.
 * - Too many delays and ilocks.
 * - bandwidth admission control must be done per-frame.
 * - Buffering could be handled like in uhci, to avoid
 * needed block allocation and avoid allocs for small Tds.
 * - must warn of power overruns.
 */

#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"

#include	"../port/usb.h"

typedef struct Ctlio Ctlio;
typedef struct Ctlr Ctlr;
typedef struct Ed Ed;
typedef struct Edpool Edpool;
typedef struct Epx Epx;
typedef struct Hcca Hcca;
typedef struct Isoio Isoio;
typedef struct Ohci Ohci;
typedef struct Qio Qio;
typedef struct Qtree Qtree;
typedef struct Td Td;
typedef struct Tdpool Tdpool;

enum
{
	Incr		= 64,		/* for Td and Ed pools */

	Align		= 0x20,		/* OHCI only requires 0x10 */
					/* use always a power of 2 */

	Abortdelay	= 1,		/* delay after cancelling Tds (ms) */
	Tdatomic		= 8,		/* max nb. of Tds per bulk I/O op. */
	Enabledelay	= 100,		/* waiting for a port to enable */


	/* Queue states (software) */
	Qidle		= 0,
	Qinstall,
	Qrun,
	Qdone,
	Qclose,
	Qfree,

	/* Ed control bits */
	Edmpsmask	= 0x7ff,	/* max packet size */
	Edmpsshift	= 16,
	Edlow		= 1 << 13,	/* low speed */
	Edskip		= 1 << 14,	/* skip this ed */
	Ediso		= 1 << 15,	/* iso Tds used */
	Edtddir		= 0,		/* get dir from td */
	Edin		= 2 << 11,	/* direction in */
	Edout		= 1 << 11,	/* direction out */
	Eddirmask	= 3 << 11,	/* direction bits */
	Edhalt		= 1,		/* halted (in head ptr) */
	Edtoggle	= 2,		/* toggle (in head ptr) 1 == data1 */

	/* Td control bits */
	Tdround		= 1<<18,	/* (rounding) short packets ok */
	Tdtoksetup	= 0<<19,	/* setup packet */
	Tdtokin		= 2<<19,	/* in packet */
	Tdtokout	= 1<<19,	/* out packet */
	Tdtokmask	= 3<<19,	/* in/out/setup bits */
	Tdnoioc		= 7<<21,	/* intr. cnt. value for no interrupt */
	Tdusetog	= 1<<25,	/* use toggle from Td (1) or Ed (0) */
	Tddata1		= 1<<24,	/* data toggle (1 == data1) */
	Tddata0		= 0<<24,
	Tdfcmask	= 7,		/* frame count (iso) */
	Tdfcshift	= 24,
	Tdsfmask	= 0xFFFF,	/* starting frame (iso) */
	Tderrmask	= 3,		/* error counter */
	Tderrshift	= 26,
	Tdccmask	= 0xf,		/* condition code (status) */
	Tdccshift	= 28,
	Tdiccmask	= 0xf,		/* condition code (iso, offsets) */
	Tdiccshift	= 12,

	Ntdframes	= 0x10000,	/* # of different iso frame numbers */

	/* Td errors (condition code) */
	Tdok		= 0,
	Tdcrc		= 1,
	Tdbitstuff	= 2,
	Tdbadtog	= 3,
	Tdstalled	= 4,
	Tdtmout		= 5,
	Tdpidchk	= 6,
	Tdbadpid	= 7,
	Tddataovr	= 8,
	Tddataund	= 9,
	Tdbufovr	= 0xC,
	Tdbufund	= 0xD,
	Tdnotacc	= 0xE,

	/* control register */
	Cple		= 0x04,		/* periodic list enable */
	Cie		= 0x08,		/* iso. list enable */
	Ccle		= 0x10,		/* ctl list enable */
	Cble		= 0x20,		/* bulk list enable */
	Cfsmask		= 3 << 6,	/* functional state... */
	Cfsreset	= 0 << 6,
	Cfsresume	= 1 << 6,
	Cfsoper		= 2 << 6,
	Cfssuspend	= 3 << 6,

	/* command status */
	Sblf =	1 << 2,			/* bulk list (load) flag */
	Sclf =	1 << 1,			/* control list (load) flag */
	Shcr =	1 << 0,			/* host controller reset */

	/* intr enable */
	Mie =	1 << 31,
	Oc =	1 << 30,
	Rhsc =	1 << 6,
	Fno =	1 << 5,
	Ue =	1 << 4,
	Rd =	1 << 3,
	Sf =	1 << 2,
	Wdh =	1 << 1,
	So =	1 << 0,

	Fmaxpktmask = 0x7fff,
	Fmaxpktshift = 16,
	HcRhDescA_POTPGT_MASK =	0xff << 24,
	HcRhDescA_POTPGT_SHIFT =	24,

	/* Rh status */
	Lps =	1 << 0,
	Cgp =	1 << 0,
	Oci =	1 << 1,
	Psm =	1 << 8,
	Nps =	1 << 9,
	Drwe =	1 << 15,
	Srwe =	1 << 15,
	Lpsc =	1 << 16,
	Ccic =	1 << 17,
	Crwe =	1 << 31,

	/* port status */
	Ccs =	0x00001,	/* current connect status */
	Pes =	0x00002,	/* port enable status */
	Pss =	0x00004,	/* port suspend status */
	Poci =	0x00008,	/* over current indicator */
	Prs =	0x00010,	/* port reset status */
	Pps =	0x00100,	/* port power status */
	Lsda =	0x00200,	/* low speed device attached */
	Csc =	0x10000,	/* connect status change */
	Pesc =	0x20000,	/* enable status change */
	Pssc =	0x40000,	/* suspend status change */
	Ocic =	0x80000,	/* over current ind. change */
	Prsc =	0x100000,	/* reset status change */

	/* port status write bits */
	Cpe =	0x001,		/* clear port enable */
	Spe =	0x002,		/* set port enable */
	Spr =	0x010,		/* set port reset */
	Spp =	0x100,		/* set port power */
	Cpp =	0x200,		/* clear port power */

};

/*
 * Endpoint descriptor. (first 4 words used by hardware)
 */
struct Ed {
	ulong	ctrl;
	ulong	tail;		/* transfer descriptor */
	ulong	head;
	ulong	nexted;

	Ed*	next;		/* sw; in free list or next in list */
	Td*	tds;		/* in use by current xfer; all for iso */
	Ep*	ep;		/* debug/align */
	Ed*	inext;		/* debug/align (dump interrupt eds). */
};

/*
 * Endpoint I/O state (software), per direction.
 */
struct Qio
{
	QLock;			/* for the entire I/O process */
	Rendez;			/* wait for completion */
	Ed*	ed;		/* to place Tds on it */
	int	sched;		/* queue number (intr/iso) */
	int	toggle;		/* Tddata0/Tddata1 */
	ulong	usbid;		/* device/endpoint address */
	int	tok;		/* Tdsetup, Tdtokin, Tdtokout */
	long	iotime;		/* last I/O time; to hold interrupt polls */
	int	debug;		/* for the endpoint */
	char*	err;		/* error status */
	int	state;		/* Qidle -> Qinstall -> Qrun -> Qdone | Qclose */
	long	bw;		/* load (intr/iso) */
};

struct Ctlio
{
	Qio;			/* single Ed for all transfers */
	uchar*	data;		/* read from last ctl req. */
	int	ndata;		/* number of bytes read */
};

struct Isoio
{
	Qio;
	int	nframes;	/* number of frames for a full second */
	Td*	atds;		/* Tds avail for further I/O */
	int	navail;		/* number of avail Tds */
	ulong	frno;		/* next frame number avail for I/O */
	ulong	left;		/* remainder after rounding Hz to samples/ms */
	int	nerrs;		/* consecutive errors on iso I/O */
};

/*
 * Transfer descriptor. Size must be multiple of 32
 * First block is used by hardware (aligned to 32).
 */
struct Td
{
	ulong	ctrl;
	ulong	cbp;		/* current buffer pointer */
	ulong	nexttd;
	ulong	be;
	ushort	offsets[8];	/* used by Iso Tds only */

	Td*	next;		/* in free or Ed tds list */
	Td*	anext;		/* in avail td list (iso) */
	Ep*	ep;		/* using this Td for I/O */
	Qio*	io;		/* using this Td for I/O */
	Block*	bp;		/* data for this Td */
	ulong	nbytes;		/* bytes in this Td */
	ulong	cbp0;		/* initial value for cbp */
	ulong	last;		/* true for last Td in Qio */
};

/*
 * Host controller communication area (hardware)
 */
struct Hcca
{
	ulong	intrtable[32];
	ushort	framenumber;
	ushort	pad1;
	ulong	donehead;
	uchar	reserved[116];
};

/*
 * I/O registers
 */
struct Ohci
{
	/* control and status group */
	ulong	revision;		/*00*/
	ulong	control;		/*04*/
	ulong	cmdsts;			/*08*/
	ulong	intrsts;			/*0c*/
	ulong	intrenable;		/*10*/
	ulong	intrdisable;		/*14*/

	/* memory pointer group */
	ulong	hcca;			/*18*/
	ulong	periodcurred;		/*1c*/
	ulong	ctlheaded;		/*20*/
	ulong	ctlcurred;		/*24*/
	ulong	bulkheaded;		/*28*/
	ulong	bulkcurred;		/*2c*/
	ulong	donehead;		/*30*/

	/* frame counter group */
	ulong	fminterval;		/*34*/
	ulong	fmremaining;		/*38*/
	ulong	fmnumber;		/*3c*/
	ulong	periodicstart;		/*40*/
	ulong	lsthreshold;		/*44*/

	/* root hub group */
	ulong	rhdesca;		/*48*/
	ulong	rhdescb;		/*4c*/
	ulong	rhsts;			/*50*/
	ulong	rhportsts[15];		/*54*/
	ulong	pad25[20];		/*90*/

	/* unknown */
	ulong	hostueaddr;		/*e0*/
	ulong	hostuests;		/*e4*/
	ulong	hosttimeoutctrl;		/*e8*/
	ulong	pad59;			/*ec*/
	ulong	pad60;			/*f0*/
	ulong	hostrevision;		/*f4*/
	ulong	pad62[2];
					/*100*/
};

/*
 * Endpoint tree (software)
 */
struct Qtree
{
	int	nel;
	int	depth;
	ulong*	bw;
	Ed**	root;
};

struct Tdpool
{
	Lock;
	Td*	free;
	int	nalloc;
	int	ninuse;
	int	nfree;
};

struct Edpool
{
	Lock;
	Ed*	free;
	int	nalloc;
	int	ninuse;
	int	nfree;
};

struct Ctlr
{
	Lock;			/* for ilock; lists and basic ctlr I/O */
	QLock	resetl;		/* lock controller during USB reset */
	int	active;
	Ctlr*	next;
	int	nports;

	Ohci*	ohci;		/* base I/O address */
	Hcca*	hcca;		/* intr/done Td lists (used by hardware) */
	int	overrun;	/* sched. overrun */
	Ed*	intrhd;		/* list of intr. eds in tree */
	Qtree*	tree;		/* tree for t Ep i/o */
	int	ntree;		/* number of dummy Eds in tree */
	Pcidev*	pcidev;
};

#define dqprint		if(debug || io && io->debug)print
#define ddqprint		if(debug>1 || (io && io->debug>1))print
#define diprint		if(debug || iso && iso->debug)print
#define ddiprint		if(debug>1 || (iso && iso->debug>1))print
#define TRUNC(x, sz)	((x) & ((sz)-1))

static int ohciinterrupts[Nttypes];
static char* iosname[] = { "idle", "install", "run", "done", "close", "FREE" };

static int debug;
static Edpool edpool;
static Tdpool tdpool;
static Ctlr* ctlrs[Nhcis];

static	char	EnotWritten[] = "usb write unfinished";
static	char	EnotRead[] = "usb read unfinished";
static	char	Eunderrun[] = "usb endpoint underrun";

static	QLock	usbhstate;	/* protects name space state */

static int	schedendpt(Ctlr *ub, Ep *ep);
static void	unschedendpt(Ctlr *ub, Ep *ep);
static long	qtd(Ctlr*, Ep*, int, Block*, uchar*, uchar*, int, ulong);

static char* errmsgs[] =
{
[Tdcrc]		"crc error",
[Tdbitstuff]	"bit stuffing error",
[Tdbadtog]	"bad toggle",
[Tdstalled]	Estalled,
[Tdtmout]	"timeout error",
[Tdpidchk]	"pid check error",
[Tdbadpid]	"bad pid",
[Tddataovr]	"data overrun",
[Tddataund]	"data underrun",
[Tdbufovr]	"buffer overrun",
[Tdbufund]	"buffer underrun",
[Tdnotacc]	"not accessed"
};

static void*
pa2ptr(ulong pa)
{
	if(pa == 0)
		return nil;
	else
		return KADDR(pa);
}

static ulong
ptr2pa(void *p)
{
	if(p == nil)
		return 0;
	else
		return PADDR(p);
}

static void
waitSOF(Ctlr *ub)
{
	int frame = ub->hcca->framenumber & 0x3f;

	do {
		delay(2);
	} while(frame == (ub->hcca->framenumber & 0x3f));
}

static char*
errmsg(int err)
{

	if(err < nelem(errmsgs))
		return errmsgs[err];
	return nil;
}

static Ed*
ctlhd(Ctlr *ctlr)
{
	return pa2ptr(ctlr->ohci->ctlheaded);
}

static Ed*
bulkhd(Ctlr *ctlr)
{
	return pa2ptr(ctlr->ohci->bulkheaded);
}

static void
edlinked(Ed *ed, Ed *next)
{
	if(ed == nil)
		print("edlinked: nil ed: pc %#p\n", getcallerpc(&ed));
	ed->nexted = ptr2pa(next);
	ed->next = next;
}

static void
setctlhd(Ctlr *ctlr, Ed *ed)
{
	ctlr->ohci->ctlheaded = ptr2pa(ed);
	if(ed != nil)
		ctlr->ohci->cmdsts |= Sclf;	/* reload it on next pass */
}

static void
setbulkhd(Ctlr *ctlr, Ed *ed)
{
	ctlr->ohci->bulkheaded = ptr2pa(ed);
	if(ed != nil)
		ctlr->ohci->cmdsts |= Sblf;	/* reload it on next pass */
}

static void
unlinkctl(Ctlr *ctlr, Ed *ed)
{
	Ed *this, *prev, *next;

	ctlr->ohci->control &= ~Ccle;
	waitSOF(ctlr);
	this = ctlhd(ctlr);
	ctlr->ohci->ctlcurred = 0;
	prev = nil;
	while(this != nil && this != ed){
		prev = this;
		this = this->next;
	}
	if(this == nil){
		print("unlinkctl: not found\n");
		return;
	}
	next = this->next;
	if(prev == nil)
		setctlhd(ctlr, next);
	else
		edlinked(prev, next);
	ctlr->ohci->control |= Ccle;
	edlinked(ed, nil);		/* wipe out next field */
}

static void
unlinkbulk(Ctlr *ctlr, Ed *ed)
{
	Ed *this, *prev, *next;

	ctlr->ohci->control &= ~Cble;
	waitSOF(ctlr);
	this = bulkhd(ctlr);
	ctlr->ohci->bulkcurred = 0;
	prev = nil;
	while(this != nil && this != ed){
		prev = this;
		this = this->next;
	}
	if(this == nil){
		print("unlinkbulk: not found\n");
		return;
	}
	next = this->next;
	if(prev == nil)
		setbulkhd(ctlr, next);
	else
		edlinked(prev, next);
	ctlr->ohci->control |= Cble;
	edlinked(ed, nil);		/* wipe out next field */
}

static void
edsetaddr(Ed *ed, ulong addr)
{
	ulong ctrl;

	ctrl = ed->ctrl & ~((Epmax<<7)|Devmax);
	ctrl |= (addr & ((Epmax<<7)|Devmax));
	ed->ctrl = ctrl;
}

static void
edsettog(Ed *ed, int c)
{
	if(c != 0)
		ed->head |= Edtoggle;
	else
		ed->head &= ~Edtoggle;
}

static int
edtoggle(Ed *ed)
{
	return ed->head & Edtoggle;
}

static int
edhalted(Ed *ed)
{
	return ed->head & Edhalt;
}

static int
edmaxpkt(Ed *ed)
{
	return (ed->ctrl >> Edmpsshift) & Edmpsmask;
}

static void
edsetmaxpkt(Ed *ed, int m)
{
	ulong c;

	c = ed->ctrl & ~(Edmpsmask << Edmpsshift);
	ed->ctrl = c | ((m&Edmpsmask) << Edmpsshift);
}

static int
tderrs(Td *td)
{
	return (td->ctrl >> Tdccshift) & Tdccmask;
}

static int
tdtok(Td *td)
{
	return (td->ctrl & Tdtokmask);
}

static Td*
tdalloc(void)
{
	Td *td;
	Td *pool;
	int i;

	lock(&tdpool);
	if(tdpool.free == nil){
		ddprint("ohci: tdalloc %d Tds\n", Incr);
		pool = xspanalloc(Incr*sizeof(Td), Align, 0);
		if(pool == nil)
			panic("tdalloc");
		for(i=Incr; --i>=0;){
			pool[i].next = tdpool.free;
			tdpool.free = &pool[i];
		}
		tdpool.nalloc += Incr;
		tdpool.nfree += Incr;
	}
	tdpool.ninuse++;
	tdpool.nfree--;
	td = tdpool.free;
	tdpool.free = td->next;
	memset(td, 0, sizeof(Td));
	unlock(&tdpool);

	assert(((uintptr)td & 0xF) == 0);
	return td;
}

static void
tdfree(Td *td)
{
	if(td == 0)
		return;
	freeb(td->bp);
	td->bp = nil;
	lock(&tdpool);
	if(td->nexttd == 0x77777777)
		panic("ohci: tdfree: double free");
	memset(td, 7, sizeof(Td));	/* poison */
	td->next = tdpool.free;
	tdpool.free = td;
	tdpool.ninuse--;
	tdpool.nfree++;
	unlock(&tdpool);
}

static Ed*
edalloc(void)
{
	Ed *ed, *pool;
	int i;

	lock(&edpool);
	if(edpool.free == nil){
		ddprint("ohci: edalloc %d Eds\n", Incr);
		pool = xspanalloc(Incr*sizeof(Ed), Align, 0);
		if(pool == nil)
			panic("edalloc");
		for(i=Incr; --i>=0;){
			pool[i].next = edpool.free;
			edpool.free = &pool[i];
		}
		edpool.nalloc += Incr;
		edpool.nfree += Incr;
	}
	edpool.ninuse++;
	edpool.nfree--;
	ed = edpool.free;
	edpool.free = ed->next;
	memset(ed, 0, sizeof(Ed));
	unlock(&edpool);

	return ed;
}

static void
edfree(Ed *ed)
{
	Td *td, *next;
	int i;

	if(ed == 0)
		return;
	i = 0;
	for(td = ed->tds; td != nil; td = next){
		next = td->next;
		tdfree(td);
		if(i++ > 2000){
			print("ohci: bug: ed with more than 2000 tds\n");
			break;
		}
	}
	lock(&edpool);
	if(ed->nexted == 0x99999999)
		panic("ohci: edfree: double free");
	memset(ed, 9, sizeof(Ed));	/* poison */
	ed->next = edpool.free;
	edpool.free = ed;
	edpool.ninuse--;
	edpool.nfree++;
	unlock(&edpool);
	ddprint("edfree: ed %#p\n", ed);
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
pickschedq(Qtree *qt, int pollival, ulong bw, ulong limit)
{
	int i, j, d, upperb, q;
	ulong best, worst, total;

	d = flog2lower(pollival);
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
schedq(Ctlr *ctlr, Qio *io, int pollival)
{
	int q;
	Ed *ted;

	q = pickschedq(ctlr->tree, pollival, io->bw, ~0);
	ddqprint("ohci: sched %#p q %d, ival %d, bw %ld\n", io, q, pollival, io->bw);
	if(q < 0){
		print("ohci: no room for ed\n");
		return -1;
	}
	ctlr->tree->bw[q] += io->bw;
	ted = ctlr->tree->root[q];
	io->sched = q;
	edlinked(io->ed, ted->next);
	edlinked(ted, io->ed);
	io->ed->inext = ctlr->intrhd;
	ctlr->intrhd = io->ed;
	return 0;
}

static void
unschedq(Ctlr *ctlr, Qio *qio)
{
	int q;
	Ed *prev, *this, *next;
	Ed **l;

	q = qio->sched;
	if(q < 0)
		return;
	ctlr->tree->bw[q] -= qio->bw;

	prev = ctlr->tree->root[q];
	this = prev->next;
	while(this != nil && this != qio->ed){
		prev = this;
		this = this->next;
	}
	if(this == nil)
		print("ohci: unschedq %d: not found\n", q);
	else{
		next = this->next;
		edlinked(prev, next);
	}
	waitSOF(ctlr);
	for(l = &ctlr->intrhd; *l != nil; l = &(*l)->inext)
		if(*l == qio->ed){
			*l = (*l)->inext;
			return;
		}
	print("ohci: unschedq: ed %#p not found\n", qio->ed);
}

static char*
seprinttdtok(char *s, char *e, int tok)
{
	switch(tok){
	case Tdtoksetup:
		s = seprint(s, e, " setup");
		break;
	case Tdtokin:
		s = seprint(s, e, " in");
		break;
	case Tdtokout:
		s = seprint(s, e, " out");
		break;
	}
	return s;
}


static char*
seprinttd(char *s, char *e, Td *td, int iso)
{
	int i;
	Block *bp;

	if(td == nil)
		return seprint(s, e, "<nil td>\n");
	s = seprint(s, e, "%#p ep %#p ctrl %#p", td, td->ep, td->ctrl);
	s = seprint(s, e, " cc=%#ulx", (td->ctrl >> Tdccshift) & Tdccmask);
	if(iso == 0){
		if((td->ctrl & Tdround) != 0)
			s = seprint(s, e, " rnd");
		s = seprinttdtok(s, e, td->ctrl & Tdtokmask);
		if((td->ctrl & Tdusetog) != 0)
			s = seprint(s, e, " d%d", (td->ctrl & Tddata1) ? 1 : 0);
		else
			s = seprint(s, e, " d-");
		s = seprint(s, e, " ec=%uld", (td->ctrl >> Tderrshift) & Tderrmask);
	}else{
		s = seprint(s, e, " fc=%uld", (td->ctrl >> Tdfcshift) & Tdfcmask);
		s = seprint(s, e, " sf=%uld", td->ctrl & Tdsfmask);
	}
	s = seprint(s, e, " cbp0 %#p cbp %#p next %#p be %#p %s",
		td->cbp0, td->cbp, td->nexttd, td->be, td->last ? "last" : "");
	s = seprint(s, e, "\n\t\t%ld bytes", td->nbytes);
	if((bp = td->bp) != nil){
		s = seprint(s, e, " rp %#p wp %#p ", bp->rp, bp->wp);
		if(BLEN(bp) > 0)
			s = seprintdata(s, e, bp->rp, bp->wp - bp->rp);
	}
	if(iso == 0)
		return seprint(s, e, "\n");
	s = seprint(s, e, "\n\t\t");
	/* we use only offsets[0] */
	i = 0;
	s = seprint(s, e, "[%d] %#ux cc=%#ux sz=%ud\n", i, td->offsets[i],
		(td->offsets[i] >> Tdiccshift) & Tdiccmask,
		td->offsets[i] & 0x7FF);
	return s;
}

static void
dumptd(Td *td, char *p, int iso)
{
	static char buf[512];	/* Too much */
	char *s;

	s = seprint(buf, buf+sizeof(buf), "%s: ", p);
	s = seprinttd(s, buf+sizeof(buf), td, iso);
	if(s > buf && s[-1] != '\n')
		s[-1] = '\n';
	print("\t%s", buf);
}

static void
dumptds(Td *td, char *p, int iso)
{
	int i;

	for(i = 0; td != nil; td = td->next){
		dumptd(td, p, iso);
		if(td->last)
			break;
		if(tdtok(td) == Tdtokin && ++i > 2){
			print("\t\t...\n");
			break;
		}
	}
}

static void
dumped(Ed *ed)
{
	char *buf, *s, *e;

	if(ed == nil){
		print("<null ed>\n");
		return;
	}
	buf = malloc(512);
	/* no waserror; may want to use from interrupt context */
	if(buf == nil)
		return;
	e = buf+512;
	s = seprint(buf, e, "\ted %#p: ctrl %#p", ed, ed->ctrl);
	if((ed->ctrl & Edskip) != 0)
		s = seprint(s, e, " skip");
	if((ed->ctrl & Ediso) != 0)
		s = seprint(s, e, " iso");
	if((ed->ctrl & Edlow) != 0)
		s = seprint(s, e, " low");
	s = seprint(s, e, " d%d", (ed->head & Edtoggle) ? 1 : 0);
	if((ed->ctrl & Eddirmask) == Edin)
		s = seprint(s, e, " in");
	if((ed->ctrl & Eddirmask) == Edout)
		s = seprint(s, e, " out");
	if(edhalted(ed))
		s = seprint(s, e, " hlt");
	s = seprint(s, e, " ep%uld.%uld", (ed->ctrl>>7)&Epmax, ed->ctrl&0x7f);
	s = seprint(s, e, " maxpkt %uld", (ed->ctrl>>Edmpsshift)&Edmpsmask);
	seprint(s, e, " tail %#p head %#p next %#p\n",ed->tail,ed->head,ed->nexted);
	print("%s", buf);
	free(buf);
	if(ed->tds != nil && (ed->ctrl & Ediso) == 0)
		dumptds(ed->tds, "td", 0);
}

static char*
seprintio(char *s, char *e, Qio *io, char *pref)
{
	s = seprint(s, e, "%s qio %#p ed %#p", pref, io, io->ed);
	s = seprint(s, e, " tog %d iot %ld err %s id %#ulx",
		io->toggle, io->iotime, io->err, io->usbid);
	s = seprinttdtok(s, e, io->tok);
	s = seprint(s, e, " %s\n", iosname[io->state]);
	return s;
}

static char*
seprintep(char* s, char* e, Ep *ep)
{
	Isoio *iso;
	Qio *io;
	Ctlio *cio;

	if(ep == nil)
		return seprint(s, e, "<nil ep>\n");
	if(ep->aux == nil)
		return seprint(s, e, "no mdep\n");
	switch(ep->ttype){
	case Tctl:
		cio = ep->aux;
		s = seprintio(s, e, cio, "c");
		s = seprint(s, e, "\trepl %d ndata %d\n", ep->rhrepl, cio->ndata);
		break;
	case Tbulk:
	case Tintr:
		io = ep->aux;
		if(ep->mode != OWRITE)
			s = seprintio(s, e, &io[OREAD], "r");
		if(ep->mode != OREAD)
			s = seprintio(s, e, &io[OWRITE], "w");
		break;
	case Tiso:
		iso = ep->aux;
		s = seprintio(s, e, iso, "w");
		s = seprint(s, e, "\tntds %d avail %d frno %uld left %uld next avail %#p\n",
			iso->nframes, iso->navail, iso->frno, iso->left, iso->atds);
		break;
	}
	return s;
}

static char*
seprintctl(char *s, char *se, ulong ctl)
{
	s = seprint(s, se, "en=");
	if((ctl&Cple) != 0)
		s = seprint(s, se, "p");
	if((ctl&Cie) != 0)
		s = seprint(s, se, "i");
	if((ctl&Ccle) != 0)
		s = seprint(s, se, "c");
	if((ctl&Cble) != 0)
		s = seprint(s, se, "b");
	switch(ctl & Cfsmask){
	case Cfsreset:
		return seprint(s, se, " reset");
	case Cfsresume:
		return seprint(s, se, " resume");
	case Cfsoper:
		return seprint(s, se, " run");
	case Cfssuspend:
		return seprint(s, se, " suspend");
	default:
		return seprint(s, se, " ???");
	}
}

static void
dump(Hci *hp)
{
	Ctlr *ctlr;
	Ed *ed;
	char cs[20];

	ctlr = hp->aux;
	ilock(ctlr);
	seprintctl(cs, cs+sizeof(cs), ctlr->ohci->control);
	print("ohci ctlr %#p: frno %#ux ctl %#lux %s sts %#lux intr %#lux\n",
		ctlr, ctlr->hcca->framenumber, ctlr->ohci->control, cs,
		ctlr->ohci->cmdsts, ctlr->ohci->intrsts);
	print("ctlhd %#ulx cur %#ulx bulkhd %#ulx cur %#ulx done %#ulx\n",
		ctlr->ohci->ctlheaded, ctlr->ohci->ctlcurred,
		ctlr->ohci->bulkheaded, ctlr->ohci->bulkcurred,
		ctlr->ohci->donehead);
	if(ctlhd(ctlr) != nil)
		print("[ctl]\n");
	for(ed = ctlhd(ctlr); ed != nil; ed = ed->next)
		dumped(ed);
	if(bulkhd(ctlr) != nil)
		print("[bulk]\n");
	for(ed = bulkhd(ctlr); ed != nil; ed = ed->next)
		dumped(ed);
	if(ctlr->intrhd != nil)
		print("[intr]\n");
	for(ed = ctlr->intrhd; ed != nil; ed = ed->inext)
		dumped(ed);
	if(ctlr->tree->root[0]->next != nil)
		print("[iso]");
	for(ed = ctlr->tree->root[0]->next; ed != nil; ed = ed->next)
		dumped(ed);
	print("%d eds in tree\n", ctlr->ntree);
	iunlock(ctlr);
	lock(&tdpool);
	print("%d tds allocated = %d in use + %d free\n",
		tdpool.nalloc, tdpool.ninuse, tdpool.nfree);
	unlock(&tdpool);
	lock(&edpool);
	print("%d eds allocated = %d in use + %d free\n",
		edpool.nalloc, edpool.ninuse, edpool.nfree);
	unlock(&edpool);
}

/*
 * Compute size for the next iso Td and setup its
 * descriptor for I/O according to the buffer size.
 */
static void
isodtdinit(Ep *ep, Isoio *iso, Td *td)
{
	Block *bp;
	long size;
	int i;

	bp = td->bp;
	assert(bp != nil && BLEN(bp) == 0);
	size = (ep->hz+iso->left) * ep->pollival / 1000;
	iso->left = (ep->hz+iso->left) * ep->pollival % 1000;
	size *= ep->samplesz;
	if(size > ep->maxpkt){
		print("ohci: ep%d.%d: size > maxpkt\n",
			ep->dev->nb, ep->nb);
		print("size = %uld max = %ld\n", size, ep->maxpkt);
		size = ep->maxpkt;
	}
	td->nbytes = size;
	memset(bp->wp, 0, size);	/* in case we don't fill it on time */
	td->cbp0 = td->cbp = ptr2pa(bp->rp) & ~0xFFF;
	td->ctrl = TRUNC(iso->frno, Ntdframes);
	td->offsets[0] = (ptr2pa(bp->rp) & 0xFFF);
	td->offsets[0] |= (Tdnotacc << Tdiccshift);
	/* in case the controller checks out the offests... */
	for(i = 1; i < nelem(td->offsets); i++)
		td->offsets[i] = td->offsets[0];
	td->be = ptr2pa(bp->rp + size - 1);
	td->ctrl |= (0 << Tdfcshift);	/* frame count is 1 */

	iso->frno = TRUNC(iso->frno + ep->pollival, Ntdframes);
}

/*
 * start I/O on the dummy td and setup a new dummy to fill up.
 */
static void
isoadvance(Ep *ep, Isoio *iso, Td *td)
{
	Td *dtd;

	dtd = iso->atds;
	iso->atds = dtd->anext;
	iso->navail--;
	dtd->anext = nil;
	dtd->bp->wp = dtd->bp->rp;
	dtd->nexttd = 0;
	td->nexttd = ptr2pa(dtd);
	isodtdinit(ep, iso, dtd);
	iso->ed->tail = ptr2pa(dtd);
}

static int
isocanwrite(void *a)
{
	Isoio *iso;

	iso = a;
	return iso->state == Qclose || iso->err != nil ||
		iso->navail > iso->nframes / 2;
}

/*
 * Service a completed/failed Td from the done queue.
 * It may be of any transfer type.
 * The queue is not in completion order.
 * (It's actually in reverse completion order).
 *
 * When an error, a short packet, or a last Td is found
 * we awake the process waiting for the transfer.
 * Although later we will process other Tds completed
 * before, epio won't be able to touch the current Td
 * until interrupt returns and releases the lock on the
 * controller.
 */
static void
qhinterrupt(Ctlr *, Ep *ep, Qio *io, Td *td, int)
{
	Block *bp;
	int mode, err;
	Ed *ed;

	ed = io->ed;
	if(io->state != Qrun)
		return;
	if(tdtok(td) == Tdtokin)
		mode = OREAD;
	else
		mode = OWRITE;
	bp = td->bp;
	err = tderrs(td);

	switch(err){
	case Tddataovr:			/* Overrun is not an error */
		break;
	case Tdok:
		/* virtualbox doesn't always report underflow on short packets */
		if(td->cbp == 0)
			break;
		/* fall through */
	case Tddataund:
		/* short input packets are ok */
		if(mode == OREAD){
			if(td->cbp == 0)
				panic("ohci: short packet but cbp == 0");
			/*
			 * td->cbp and td->cbp0 are the real addresses
			 * corresponding to virtual addresses bp->wp and
			 * bp->rp respectively.
			 */
			bp->wp = bp->rp + (td->cbp - td->cbp0);
			if(bp->wp < bp->rp)
				panic("ohci: wp < rp");
			/*
			 * It's ok. clear error and flag as last in xfer.
			 * epio must ignore following Tds.
			 */
			td->last = 1;
			td->ctrl &= ~(Tdccmask << Tdccshift);
			break;
		}
		/* else fall; it's an error */
	case Tdcrc:
	case Tdbitstuff:
	case Tdbadtog:
	case Tdstalled:
	case Tdtmout:
	case Tdpidchk:
	case Tdbadpid:
		bp->wp = bp->rp;	/* no bytes in xfer. */
		io->err = errmsg(err);
		if(debug || ep->debug){
			print("tdinterrupt: failed err %d (%s)\n", err, io->err);
			dumptd(td, "failed", ed->ctrl & Ediso);
		}
		td->last = 1;
		break;
	default:
		panic("ohci: td cc %ud unknown", err);
	}

	if(td->last != 0){
		/*
		 * clear td list and halt flag.
		 */
		ed->head = (ed->head & Edtoggle) | ed->tail;
		ed->tds = pa2ptr(ed->tail);
		io->state = Qdone;
		wakeup(io);
	}
}

/*
 * BUG: Iso input streams are not implemented.
 */
static void
isointerrupt(Ctlr *ctlr, Ep *ep, Qio *io, Td *td, int)
{
	Isoio *iso;
	Block *bp;
	Ed *ed;
	int err, isoerr;

	iso = ep->aux;
	ed = io->ed;
	if(io->state == Qclose)
		return;
	bp = td->bp;
	/*
	 * When we get more than half the frames consecutive errors
	 * we signal an actual error. Errors in the entire Td are
	 * more serious and are always singaled.
	 * Errors like overrun are not really errors. In fact, for
	 * output, errors cannot be really detected. The driver will
	 * hopefully notice I/O errors on input endpoints and detach the device.
	 */
	err = tderrs(td);
	isoerr = (td->offsets[0] >> Tdiccshift) & Tdiccmask;
	if(isoerr == Tdok || isoerr == Tdnotacc)
		iso->nerrs = 0;
	else if(iso->nerrs++ > iso->nframes/2)
		err = Tdstalled;
	if(err != Tdok && err != Tddataovr){
		bp->wp = bp->rp;
		io->err = errmsg(err);
		if(debug || ep->debug){
			print("ohci: isointerrupt: ep%d.%d: err %d (%s) frnum 0x%lux\n",
				ep->dev->nb, ep->nb,
				err, errmsg(err), ctlr->ohci->fmnumber);
			dumptd(td, "failed", ed->ctrl & Ediso);
		}
	}
	td->bp->wp = td->bp->rp;
	td->nbytes = 0;
	td->anext = iso->atds;
	iso->atds = td;
	iso->navail++;
	/*
	 * If almost all Tds are avail the user is not doing I/O at the
	 * required rate. We put another Td in place to keep the polling rate.
	 */
	if(iso->err == nil && iso->navail > iso->nframes - 10)
		isoadvance(ep, iso, pa2ptr(iso->ed->tail));
	/*
	 * If there's enough buffering futher I/O can be done.
	 */
	if(isocanwrite(iso))
		wakeup(iso);
}

static void
interrupt(Ureg *, void *arg)
{
	Td *td, *ntd;
	Hci *hp;
	Ctlr *ctlr;
	ulong status, curred;
	int i, frno;

	hp = arg;
	ctlr = hp->aux;
	ilock(ctlr);
	ctlr->ohci->intrdisable = Mie;
	coherence();
	status = ctlr->ohci->intrsts & ctlr->ohci->intrenable;
	status &= Oc|Rhsc|Fno|Ue|Rd|Sf|Wdh|So;
	frno = TRUNC(ctlr->ohci->fmnumber, Ntdframes);
	if(status & Wdh){
		/* lsb of donehead has bit to flag other intrs.  */
		td = pa2ptr(ctlr->hcca->donehead & ~0xF);

		for(i = 0; td != nil && i < 1024; i++){
			if(0)ddprint("ohci tdinterrupt: td %#p\n", td);
			ntd = pa2ptr(td->nexttd & ~0xF);
			td->nexttd = 0;
			if(td->ep == nil || td->io == nil)
				panic("ohci: interrupt: ep %#p io %#p",
					td->ep, td->io);
			ohciinterrupts[td->ep->ttype]++;
			if(td->ep->ttype == Tiso)
				isointerrupt(ctlr, td->ep, td->io, td, frno);
			else
				qhinterrupt(ctlr, td->ep, td->io, td, frno);
			td = ntd;
		}
		if(i >= 1024)
			print("ohci: bug: more than 1024 done Tds?\n");
		ctlr->hcca->donehead = 0;
	}

	ctlr->ohci->intrsts = status;
	status &= ~Wdh;
	status &= ~Sf;
	if(status & So){
		print("ohci: sched overrun: too much load\n");
		ctlr->overrun++;
		status &= ~So;
	}
	if((status & Ue) != 0){
		curred = ctlr->ohci->periodcurred;
		print("ohci: unrecoverable error frame 0x%.8lux ed 0x%.8lux, "
			"ints %d %d %d %d\n",
			ctlr->ohci->fmnumber, curred,
			ohciinterrupts[Tctl], ohciinterrupts[Tintr],
			ohciinterrupts[Tbulk], ohciinterrupts[Tiso]);
		if(curred != 0)
			dumped(pa2ptr(curred));
		status &= ~Ue;
	}
	if(status != 0)
		print("ohci interrupt: unhandled sts 0x%.8lux\n", status);
	ctlr->ohci->intrenable = Mie | Wdh | Ue;
	iunlock(ctlr);
}

/*
 * The old dummy Td is used to implement the new Td.
 * A new dummy is linked at the end of the old one and
 * returned, to link further Tds if needed.
 */
static Td*
epgettd(Ep *ep, Qio *io, Td **dtdp, int flags, void *a, int count)
{
	Td *td, *dtd;
	Block *bp;

	if(count <= BY2PG)
		bp = allocb(count);
	else{
		if(count > 2*BY2PG)
			panic("ohci: transfer > two pages");
		/* maximum of one physical page crossing allowed */
		bp = allocb(count+BY2PG);
		bp->rp = (uchar*)PGROUND((uintptr)bp->rp);
		bp->wp = bp->rp;
	}
	dtd = *dtdp;
	td = dtd;
	td->bp = bp;
	if(count > 0){
		td->cbp0 = td->cbp = ptr2pa(bp->wp);
		td->be = ptr2pa(bp->wp + count - 1);
		if(a != nil){
			/* validaddr((uintptr)a, count, 0); DEBUG */
			memmove(bp->wp, a, count);
		}
		bp->wp += count;
	}
	td->nbytes = count;
	td->ctrl = io->tok|Tdusetog|io->toggle|flags;
	if(io->toggle == Tddata0)
		io->toggle = Tddata1;
	else
		io->toggle = Tddata0;
	assert(td->ep == ep);
	td->io = io;
 	dtd = tdalloc();	/* new dummy */
	dtd->ep = ep;
	td->nexttd = ptr2pa(dtd);
	td->next = dtd;
	*dtdp = dtd;
	return td;
}

/*
 * Try to get them idle
 */
static void
aborttds(Qio *io)
{
	Ed *ed;
	Td *td;

	ed = io->ed;
	if(ed == nil)
		return;
	ed->ctrl |= Edskip;
	for(td = ed->tds; td != nil; td = td->next)
		if(td->bp != nil)
			td->bp->wp = td->bp->rp;
	ed->head = (ed->head&0xF) | ed->tail;
	if((ed->ctrl & Ediso) == 0)
		ed->tds = pa2ptr(ed->tail);
}

static int
epiodone(void *a)
{
	Qio *io;

	io = a;
	return io->state != Qrun;
}

static void
epiowait(Ctlr *ctlr, Qio *io, int tmout, ulong)
{
	Ed *ed;
	int timedout;

	ed = io->ed;
	if(0)ddqprint("ohci io %#p sleep on ed %#p state %s\n",
		io, ed, iosname[io->state]);
	timedout = 0;
	if(waserror()){
		dqprint("ohci io %#p ed %#p timed out\n", io, ed);
		timedout++;
	}else{
		if(tmout == 0)
			sleep(io, epiodone, io);
		else
			tsleep(io, epiodone, io, tmout);
		poperror();
	}
	ilock(ctlr);
	if(io->state == Qrun)
		timedout = 1;
	else if(io->state != Qdone && io->state != Qclose)
		panic("epio: ed not done and not closed");
	if(timedout){
		aborttds(io);
		io->err = "request timed out";
		iunlock(ctlr);
		if(!waserror()){
			tsleep(&up->sleep, return0, 0, Abortdelay);
			poperror();
		}
		ilock(ctlr);
	}
	if(io->state != Qclose)
		io->state = Qidle;
	iunlock(ctlr);
}

/*
 * Non iso I/O.
 * To make it work for control transfers, the caller may
 * lock the Qio for the entire control transfer.
 */
static long
epio(Ep *ep, Qio *io, void *a, long count, int mustlock)
{
	Ed *ed;
	Ctlr *ctlr;
	char buf[80];
	char *err;
	uchar *c;
	Td *td, *ltd, *ntd, *td0;
	int last, ntds, tmout;
	long tot, n;
	ulong load;

	ed = io->ed;
	ctlr = ep->hp->aux;
	io->debug = ep->debug;
	tmout = ep->tmout;
	ddeprint("ohci: %s ep%d.%d io %#p count %ld\n",
		io->tok == Tdtokin ? "in" : "out",
		ep->dev->nb, ep->nb, io, count);
	if((debug > 1 || ep->debug > 1) && io->tok != Tdtokin){
		seprintdata(buf, buf+sizeof(buf), a, count);
		print("\t%s\n", buf);
	}
	if(mustlock){
		qlock(io);
		if(waserror()){
			qunlock(io);
			nexterror();
		}
	}
	io->err = nil;
	ilock(ctlr);
	if(io->state == Qclose){	/* Tds released by cancelio */
		iunlock(ctlr);
		error(io->err ? io->err : Eio);
	}
	if(io->state != Qidle)
		panic("epio: qio not idle");
	io->state = Qinstall;

	c = a;
	ltd = td0 = ed->tds;
	load = tot = 0;
	do{
		n = 2*BY2PG;
		if(count-tot < n)
			n = count-tot;
		if(c != nil && io->tok != Tdtokin)
			td = epgettd(ep, io, &ltd, 0, c+tot, n);
		else
			td = epgettd(ep, io, &ltd, 0, nil, n);
		tot += n;
		load += ep->load;
	}while(tot < count);
	if(td0 == nil || ltd == nil || td0 == ltd)
		panic("epio: no td");
	td->last = 1;
	if(debug > 2 || ep->debug > 2)
		dumptds(td0, "put td", ep->ttype == Tiso);
	iunlock(ctlr);

	ilock(ctlr);
	if(io->state != Qclose){
		io->iotime = TK2MS(MACHP(0)->ticks);
		io->state = Qrun;
		ed->tail = ptr2pa(ltd);
		if(ep->ttype == Tctl)
			ctlr->ohci->cmdsts |= Sclf;
		else if(ep->ttype == Tbulk)
			ctlr->ohci->cmdsts |= Sblf;
	}
	iunlock(ctlr);

	epiowait(ctlr, io, tmout, load);
	ilock(ctlr);
	if(debug > 1 || ep->debug > 1)
		dumptds(td0, "got td", 0);
	iunlock(ctlr);

	tot = 0;
	c = a;
	ntds = last = 0;
	for(td = td0; td != ltd; td = ntd){
		ntds++;
		/*
		 * If the Td is flagged as last we must
		 * ignore any following Td. The block may
		 * seem to have bytes but interrupt has not seen
		 * those Tds through the done queue, and they are void.
		 */
		if(last == 0 && tderrs(td) == Tdok){
			n = BLEN(td->bp);
			tot += n;
			if(c != nil && tdtok(td) == Tdtokin && n > 0){
				memmove(c, td->bp->rp, n);
				c += n;
			}
		}
		last |= td->last;
		ntd = td->next;
		tdfree(td);
	}
	if(edtoggle(ed) == 0)
		io->toggle = Tddata0;
	else
		io->toggle = Tddata1;

	err = io->err;
	if(mustlock){
		qunlock(io);
		poperror();
	}
	ddeprint("ohci: io %#p: %d tds: return %ld err '%s'\n\n",
		io, ntds, tot, err);
	if(err != nil)
		error(err);
	if(tot < 0)
		error(Eio);
	return tot;
}

/*
 * halt condition was cleared on the endpoint. update our toggles.
 */
static void
clrhalt(Ep *ep)
{
	Qio *io;

	ep->clrhalt = 0;
	switch(ep->ttype){
	case Tbulk:
	case Tintr:
		io = ep->aux;
		if(ep->mode != OREAD){
			qlock(&io[OWRITE]);
			io[OWRITE].toggle = Tddata0;
			deprint("ep clrhalt for io %#p\n", io+OWRITE);
			qunlock(&io[OWRITE]);
		}
		if(ep->mode != OWRITE){
			qlock(&io[OREAD]);
			io[OREAD].toggle = Tddata0;
			deprint("ep clrhalt for io %#p\n", io+OREAD);
			qunlock(&io[OREAD]);
		}
		break;
	}
}

static long
epread(Ep *ep, void *a, long count)
{
	Ctlio *cio;
	Qio *io;
	char buf[80];
	ulong delta;

	if(ep->aux == nil)
		panic("epread: not open");

	switch(ep->ttype){
	case Tctl:
		cio = ep->aux;
		qlock(cio);
		if(waserror()){
			qunlock(cio);
			nexterror();
		}
		ddeprint("epread ctl ndata %d\n", cio->ndata);
		if(cio->ndata < 0)
			error("request expected");
		else if(cio->ndata == 0){
			cio->ndata = -1;
			count = 0;
		}else{
			if(count > cio->ndata)
				count = cio->ndata;
			if(count > 0)
				memmove(a, cio->data, count);
			/* BUG for big transfers */
			free(cio->data);
			cio->data = nil;
			cio->ndata = 0;	/* signal EOF next time */
		}
		qunlock(cio);
		poperror();
		if(debug>1 || ep->debug){
			seprintdata(buf, buf+sizeof(buf), a, count);
			print("epread: %s\n", buf);
		}
		return count;
	case Tbulk:
		io = ep->aux;
		if(ep->clrhalt)
			clrhalt(ep);
		return epio(ep, &io[OREAD], a, count, 1);
	case Tintr:
		io = ep->aux;
		delta = TK2MS(MACHP(0)->ticks) - io[OREAD].iotime + 1;
		if(delta < ep->pollival / 2)
			tsleep(&up->sleep, return0, 0, ep->pollival/2 - delta);
		if(ep->clrhalt)
			clrhalt(ep);
		return epio(ep, &io[OREAD], a, count, 1);
	case Tiso:
		panic("ohci: iso read not implemented");
		break;
	default:
		panic("epread: bad ep ttype %d", ep->ttype);
	}
	return -1;
}

/*
 * Control transfers are one setup write (data0)
 * plus zero or more reads/writes (data1, data0, ...)
 * plus a final write/read with data1 to ack.
 * For both host to device and device to host we perform
 * the entire transfer when the user writes the request,
 * and keep any data read from the device for a later read.
 * We call epio three times instead of placing all Tds at
 * the same time because doing so leads to crc/tmout errors
 * for some devices.
 * Upon errors on the data phase we must still run the status
 * phase or the device may cease responding in the future.
 */
static long
epctlio(Ep *ep, Ctlio *cio, void *a, long count)
{
	uchar *c;
	long len;

	ddeprint("epctlio: cio %#p ep%d.%d count %ld\n",
		cio, ep->dev->nb, ep->nb, count);
	if(count < Rsetuplen)
		error("short usb command");
	qlock(cio);
	free(cio->data);
	cio->data = nil;
	cio->ndata = 0;
	if(waserror()){
		qunlock(cio);
		free(cio->data);
		cio->data = nil;
		cio->ndata = 0;
		nexterror();
	}

	/* set the address if unset and out of configuration state */
	if(ep->dev->state != Dconfig && ep->dev->state != Dreset)
		if(cio->usbid == 0){
			cio->usbid = (ep->nb<<7)|(ep->dev->nb & Devmax);
			edsetaddr(cio->ed, cio->usbid);
		}
	/* adjust maxpkt if the user has learned a different one */
	if(edmaxpkt(cio->ed) != ep->maxpkt)
		edsetmaxpkt(cio->ed, ep->maxpkt);
	c = a;
	cio->tok = Tdtoksetup;
	cio->toggle = Tddata0;
	if(epio(ep, cio, a, Rsetuplen, 0) < Rsetuplen)
		error(Eio);

	a = c + Rsetuplen;
	count -= Rsetuplen;

	cio->toggle = Tddata1;
	if(c[Rtype] & Rd2h){
		cio->tok = Tdtokin;
		len = GET2(c+Rcount);
		if(len <= 0)
			error("bad length in d2h request");
		if(len > Maxctllen)
			error("d2h data too large to fit in ohci");
		a = cio->data = smalloc(len+1);
	}else{
		cio->tok = Tdtokout;
		len = count;
	}
	if(len > 0)
		if(waserror())
			len = -1;
		else{
			len = epio(ep, cio, a, len, 0);
			poperror();
		}
	if(c[Rtype] & Rd2h){
		count = Rsetuplen;
		cio->ndata = len;
		cio->tok = Tdtokout;
	}else{
		if(len < 0)
			count = -1;
		else
			count = Rsetuplen + len;
		cio->tok = Tdtokin;
	}
	cio->toggle = Tddata1;
	epio(ep, cio, nil, 0, 0);
	qunlock(cio);
	poperror();
	ddeprint("epctlio cio %#p return %ld\n", cio, count);
	return count;
}

/*
 * Put new samples in the dummy Td.
 * BUG: This does only a transfer per Td. We could do up to 8.
 */
static long
putsamples(Ctlr *ctlr, Ep *ep, Isoio *iso, uchar *b, long count)
{
	Td *td;
	ulong n;

	td = pa2ptr(iso->ed->tail);
	n = count;
	if(n > td->nbytes - BLEN(td->bp))
		n = td->nbytes - BLEN(td->bp);
	assert(td->bp->wp + n <= td->bp->lim);
	memmove(td->bp->wp, b, n);
	td->bp->wp += n;
	if(BLEN(td->bp) == td->nbytes){	/* full Td: activate it */
		ilock(ctlr);
		isoadvance(ep, iso, td);
		iunlock(ctlr);
	}
	return n;
}

static long
episowrite(Ep *ep, void *a, long count)
{
	long tot, nw;
	char *err;
	uchar *b;
	Ctlr *ctlr;
	Isoio *iso;

	ctlr = ep->hp->aux;
	iso = ep->aux;
	iso->debug = ep->debug;

	qlock(iso);
	if(waserror()){
		qunlock(iso);
		nexterror();
	}
	diprint("ohci: episowrite: %#p ep%d.%d\n", iso, ep->dev->nb, ep->nb);
	ilock(ctlr);
	if(iso->state == Qclose){
		iunlock(ctlr);
		error(iso->err ? iso->err : Eio);
	}
	iso->state = Qrun;
	b = a;
	for(tot = 0; tot < count; tot += nw){
		while(isocanwrite(iso) == 0){
			iunlock(ctlr);
			diprint("ohci: episowrite: %#p sleep\n", iso);
			if(waserror()){
				if(iso->err == nil)
					iso->err = "I/O timed out";
				ilock(ctlr);
				break;
			}
			tsleep(iso, isocanwrite, iso, ep->tmout);
			poperror();
			ilock(ctlr);
		}
		err = iso->err;
		iso->err = nil;
		if(iso->state == Qclose || err != nil){
			iunlock(ctlr);
			error(err ? err : Eio);
		}
		if(iso->state != Qrun)
			panic("episowrite: iso not running");
		iunlock(ctlr);		/* We could page fault here */
		nw = putsamples(ctlr, ep, iso, b+tot, count-tot);
		ilock(ctlr);
	}
	if(iso->state != Qclose)
		iso->state = Qdone;
	iunlock(ctlr);
	err = iso->err;		/* in case it failed early */
	iso->err = nil;
	qunlock(iso);
	poperror();
	if(err != nil)
		error(err);
	diprint("ohci: episowrite: %#p %ld bytes\n", iso, tot);
	return tot;
}

static long
epwrite(Ep *ep, void *a, long count)
{
	Qio *io;
	Ctlio *cio;
	ulong delta;
	uchar *b;
	long tot, nw;

	if(ep->aux == nil)
		panic("ohci: epwrite: not open");
	switch(ep->ttype){
	case Tctl:
		cio = ep->aux;
		return epctlio(ep, cio, a, count);
	case Tbulk:
		io = ep->aux;
		if(ep->clrhalt)
			clrhalt(ep);
		/*
		 * Put at most Tdatomic Tds (512 bytes) at a time.
		 * Otherwise some devices produce babble errors.
		 */
		b = a;
		assert(a != nil);
		for(tot = 0; tot < count ; tot += nw){
			nw = count - tot;
			if(nw > Tdatomic * ep->maxpkt)
				nw = Tdatomic * ep->maxpkt;
			nw = epio(ep, &io[OWRITE], b+tot, nw, 1);
		}
		return tot;
	case Tintr:
		io = ep->aux;
		delta = TK2MS(MACHP(0)->ticks) - io[OWRITE].iotime + 1;
		if(delta < ep->pollival)
			tsleep(&up->sleep, return0, 0, ep->pollival - delta);
		if(ep->clrhalt)
			clrhalt(ep);
		return epio(ep, &io[OWRITE], a, count, 1);
	case Tiso:
		return episowrite(ep, a, count);
	default:
		panic("ohci: epwrite: bad ep ttype %d", ep->ttype);
	}
	return -1;
}

static Ed*
newed(Ctlr *ctlr, Ep *ep, Qio *io, char *)
{
	Ed *ed;
	Td *td;

	ed = io->ed = edalloc();	/* no errors raised here, really */
	td = tdalloc();
	td->ep = ep;
	td->io = io;
	ed->tail =  ptr2pa(td);
	ed->head = ptr2pa(td);
	ed->tds = td;
	ed->ep = ep;
	ed->ctrl = (ep->maxpkt & Edmpsmask) << Edmpsshift;
	if(ep->ttype == Tiso)
		ed->ctrl |= Ediso;
	if(waserror()){
		edfree(ed);
		io->ed = nil;
		nexterror();
	}
	/* For setup endpoints we start with the config address */
	if(ep->ttype != Tctl)
		edsetaddr(io->ed, io->usbid);
	if(ep->dev->speed == Lowspeed)
		ed->ctrl |= Edlow;
	switch(io->tok){
	case Tdtokin:
		ed->ctrl |= Edin;
		break;
	case Tdtokout:
		ed->ctrl |= Edout;
		break;
	default:
		ed->ctrl |= Edtddir;	/* Td will say */
		break;
	}

	switch(ep->ttype){
	case Tctl:
		ilock(ctlr);
		edlinked(ed, ctlhd(ctlr));
		setctlhd(ctlr, ed);
		iunlock(ctlr);
		break;
	case Tbulk:
		ilock(ctlr);
		edlinked(ed, bulkhd(ctlr));
		setbulkhd(ctlr, ed);
		iunlock(ctlr);
		break;
	case Tintr:
	case Tiso:
		ilock(ctlr);
		schedq(ctlr, io, ep->pollival);
		iunlock(ctlr);
		break;
	default:
		panic("ohci: newed: bad ttype");
	}
	poperror();
	return ed;
}

static void
isoopen(Ctlr *ctlr, Ep *ep)
{
	Td *td, *edtds;
	Isoio *iso;
	int i;

	iso = ep->aux;
	iso->usbid = (ep->nb<<7)|(ep->dev->nb & Devmax);
	iso->bw = ep->hz * ep->samplesz;	/* bytes/sec */
	if(ep->mode != OWRITE){
		print("ohci: bug: iso input streams not implemented\n");
		error("ohci iso input streams not implemented");
	}else
		iso->tok = Tdtokout;

	iso->left = 0;
	iso->nerrs = 0;
	iso->frno = TRUNC(ctlr->ohci->fmnumber + 10, Ntdframes);
	iso->nframes = 1000 / ep->pollival;
	if(iso->nframes < 10){
		print("ohci: isoopen: less than 10 frames; using 10.\n");
		iso->nframes = 10;
	}
	iso->navail = iso->nframes;
	iso->atds = edtds = nil;
	for(i = 0; i < iso->nframes-1; i++){	/* -1 for dummy */
		td = tdalloc();
		td->ep = ep;
		td->io = iso;
		td->bp = allocb(ep->maxpkt);
		td->anext = iso->atds;		/* link as avail */
		iso->atds = td;
		td->next = edtds;
		edtds = td;
	}
	newed(ctlr, ep, iso, "iso");		/* allocates a dummy td */
	iso->ed->tds->bp = allocb(ep->maxpkt);	/* but not its block */
	iso->ed->tds->next = edtds;
	isodtdinit(ep, iso, iso->ed->tds);
}

/*
 * Allocate the endpoint and set it up for I/O
 * in the controller. This must follow what's said
 * in Ep regarding configuration, including perhaps
 * the saved toggles (saved on a previous close of
 * the endpoint data file by epclose).
 */
static void
epopen(Ep *ep)
{
	Ctlr *ctlr;
	Qio *io;
	Ctlio *cio;
	ulong usbid;

	ctlr = ep->hp->aux;
	deprint("ohci: epopen ep%d.%d\n", ep->dev->nb, ep->nb);
	if(ep->aux != nil)
		panic("ohci: epopen called with open ep");
	if(waserror()){
		free(ep->aux);
		ep->aux = nil;
		nexterror();
	}
	switch(ep->ttype){
	case Tnone:
		error("endpoint not configured");
	case Tiso:
		ep->aux = smalloc(sizeof(Isoio));
		isoopen(ctlr, ep);
		break;
	case Tctl:
		cio = ep->aux = smalloc(sizeof(Ctlio));
		cio->debug = ep->debug;
		cio->ndata = -1;
		cio->data = nil;
		cio->tok = -1;	/* invalid; Tds will say */
		if(ep->dev->isroot != 0 && ep->nb == 0)	/* root hub */
			break;
		newed(ctlr, ep, cio, "epc");
		break;
	case Tbulk:
		ep->pollival = 1;	/* assume this; doesn't really matter */
		/* and fall... */
	case Tintr:
		io = ep->aux = smalloc(sizeof(Qio)*2);
		io[OREAD].debug = io[OWRITE].debug = ep->debug;
		usbid = (ep->nb<<7)|(ep->dev->nb & Devmax);
		if(ep->mode != OREAD){
			if(ep->toggle[OWRITE] != 0)
				io[OWRITE].toggle = Tddata1;
			else
				io[OWRITE].toggle = Tddata0;
			io[OWRITE].tok = Tdtokout;
			io[OWRITE].usbid = usbid;
			io[OWRITE].bw = ep->maxpkt*1000/ep->pollival; /* bytes/s */
			newed(ctlr, ep, io+OWRITE, "epw");
		}
		if(ep->mode != OWRITE){
			if(ep->toggle[OREAD] != 0)
				io[OREAD].toggle = Tddata1;
			else
				io[OREAD].toggle = Tddata0;
			io[OREAD].tok = Tdtokin;
			io[OREAD].usbid = usbid;
			io[OREAD].bw = ep->maxpkt*1000/ep->pollival; /* bytes/s */
			newed(ctlr, ep, io+OREAD, "epr");
		}
		break;
	}
	deprint("ohci: epopen done:\n");
	if(debug || ep->debug)
		dump(ep->hp);
	poperror();
}

static void
cancelio(Ep *ep, Qio *io)
{
	Ed *ed;
	Ctlr *ctlr;

	ctlr = ep->hp->aux;

	ilock(ctlr);
	if(io == nil || io->state == Qclose){
		assert(io == nil || io->ed == nil);
		iunlock(ctlr);
		return;
	}
	ed = io->ed;
	io->state = Qclose;
	io->err = Eio;
	aborttds(io);
	iunlock(ctlr);
	if(!waserror()){
		tsleep(&up->sleep, return0, 0, Abortdelay);
		poperror();
	}

	wakeup(io);
	qlock(io);
	/* wait for epio if running */
	qunlock(io);

	ilock(ctlr);
	switch(ep->ttype){
	case Tctl:
		unlinkctl(ctlr, ed);
		break;
	case Tbulk:
		unlinkbulk(ctlr, ed);
		break;
	case Tintr:
	case Tiso:
		unschedq(ctlr, io);
		break;
	default:
		panic("ohci cancelio: bad ttype");
	}
	iunlock(ctlr);
	edfree(io->ed);
	io->ed = nil;
}

static void
epclose(Ep *ep)
{
	Ctlio *cio;
	Isoio *iso;
	Qio *io;

	deprint("ohci: epclose ep%d.%d\n", ep->dev->nb, ep->nb);
	if(ep->aux == nil)
		panic("ohci: epclose called with closed ep");
	switch(ep->ttype){
	case Tctl:
		cio = ep->aux;
		cancelio(ep, cio);
		free(cio->data);
		cio->data = nil;
		break;
	case Tbulk:
	case Tintr:
		io = ep->aux;
		if(ep->mode != OWRITE){
			cancelio(ep, &io[OREAD]);
			if(io[OREAD].toggle == Tddata1)
				ep->toggle[OREAD] = 1;
		}
		if(ep->mode != OREAD){
			cancelio(ep, &io[OWRITE]);
			if(io[OWRITE].toggle == Tddata1)
				ep->toggle[OWRITE] = 1;
		}
		break;
	case Tiso:
		iso = ep->aux;
		cancelio(ep, iso);
		break;
	default:
		panic("epclose: bad ttype %d", ep->ttype);
	}

	deprint("ohci: epclose ep%d.%d: done\n", ep->dev->nb, ep->nb);
	free(ep->aux);
	ep->aux = nil;
}

static int
portreset(Hci *hp, int port, int on)
{
	Ctlr *ctlr;
	Ohci *ohci;

	if(on == 0)
		return 0;

	ctlr = hp->aux;
	qlock(&ctlr->resetl);
	if(waserror()){
		qunlock(&ctlr->resetl);
		nexterror();
	}
	ilock(ctlr);
	ohci = ctlr->ohci;
	ohci->rhportsts[port - 1] = Spp;
	if((ohci->rhportsts[port - 1] & Ccs) == 0){
		iunlock(ctlr);
		error("port not connected");
	}
	ohci->rhportsts[port - 1] = Spr;
	while((ohci->rhportsts[port - 1] & Prsc) == 0){
		iunlock(ctlr);
		dprint("ohci: portreset, wait for reset complete\n");
		ilock(ctlr);
	}
	ohci->rhportsts[port - 1] = Prsc;
	iunlock(ctlr);
	poperror();
	qunlock(&ctlr->resetl);
	return 0;
}

static int
portenable(Hci *hp, int port, int on)
{
	Ctlr *ctlr;

	ctlr = hp->aux;
	dprint("ohci: %#p port %d enable=%d\n", ctlr->ohci, port, on);
	qlock(&ctlr->resetl);
	if(waserror()){
		qunlock(&ctlr->resetl);
		nexterror();
	}
	ilock(ctlr);
	if(on)
		ctlr->ohci->rhportsts[port - 1] = Spe | Spp;
	else
		ctlr->ohci->rhportsts[port - 1] = Cpe;
	iunlock(ctlr);
	tsleep(&up->sleep, return0, 0, Enabledelay);
	poperror();
	qunlock(&ctlr->resetl);
	return 0;
}

static int
portstatus(Hci *hp, int port)
{
	int v;
	Ctlr *ub;
	ulong ohcistatus;

	/*
	 * We must return status bits as a
	 * get port status hub request would do.
	 */
	ub = hp->aux;
	ohcistatus = ub->ohci->rhportsts[port - 1];
	v = 0;
	if(ohcistatus & Ccs)
		v |= HPpresent;
	if(ohcistatus & Pes)
		v |= HPenable;
	if(ohcistatus & Pss)
		v |= HPsuspend;
	if(ohcistatus & Prs)
		v |= HPreset;
	else {
		/* port is not in reset; these potential writes are ok */
		if(ohcistatus & Csc){
			v |= HPstatuschg;
			ub->ohci->rhportsts[port - 1] = Csc;
		}
		if(ohcistatus & Pesc){
			v |= HPchange;
			ub->ohci->rhportsts[port - 1] = Pesc;
		}
	}
	if(ohcistatus & Lsda)
		v |= HPslow;
	if(v & (HPstatuschg|HPchange))
		ddprint("ohci port %d sts %#ulx hub sts %#x\n", port, ohcistatus, v);
	return v;
}

static void
dumpohci(Ctlr *ctlr)
{
	int i;
	ulong *ohci;

	ohci = &ctlr->ohci->revision;
	print("ohci registers: \n");
	for(i = 0; i < sizeof(Ohci)/sizeof(ulong); i++)
		if(i < 3 || ohci[i] != 0)
			print("\t[%#2.2x]\t%#8.8ulx\n", i * 4, ohci[i]);
	print("\n");
}

static void
init(Hci *hp)
{
	Ctlr *ctlr;
	Ohci *ohci;
	int i;
	ulong ival, ctrl, fmi;

	ctlr = hp->aux;
	dprint("ohci %#p init\n", ctlr->ohci);
	ohci = ctlr->ohci;

	fmi =  ctlr->ohci->fminterval;
	ctlr->ohci->cmdsts = Shcr;         /* reset the block */
	while(ctlr->ohci->cmdsts & Shcr)
		delay(1);  /* wait till reset complete, Ohci says 10us max. */
	ctlr->ohci->fminterval = fmi;

	/*
	 * now that soft reset is done we are in suspend state.
	 * Setup registers which take in suspend state
	 * (will only be here for 2ms).
	 */

	ctlr->ohci->hcca = ptr2pa(ctlr->hcca);
	setctlhd(ctlr, nil);
	ctlr->ohci->ctlcurred = 0;
	setbulkhd(ctlr, nil);
	ctlr->ohci->bulkcurred = 0;

	ohci->intrenable = Mie | Wdh | Ue;
	ohci->control |= Ccle | Cble | Cple | Cie | Cfsoper;

	/* set frame after operational */
	ohci->rhdesca = Nps;	/* no power switching */
	if(ohci->rhdesca & Nps){
		dprint("ohci: ports are not power switched\n");
	}else{
		dprint("ohci: ports are power switched\n");
		ohci->rhdesca &= ~Psm;
		ohci->rhsts &= ~Lpsc;
	}
	for(i = 0; i < ctlr->nports; i++)	/* paranoia */
		ohci->rhportsts[i] = 0;		/* this has no effect */
	delay(50);

	for(i = 0; i < ctlr->nports; i++){
		ohci->rhportsts[i] =  Spp;
		if((ohci->rhportsts[i] & Ccs) != 0)
			ohci->rhportsts[i] |= Spr;
	}
	delay(100);

	ctrl = ohci->control;
	if((ctrl & Cfsmask) != Cfsoper){
		ctrl = (ctrl & ~Cfsmask) | Cfsoper;
		ohci->control = ctrl;
		ohci->rhsts = Lpsc;
	}
	ival = ohci->fminterval & ~(Fmaxpktmask << Fmaxpktshift);
	ohci->fminterval = ival | (5120 << Fmaxpktshift);

	if(debug > 1)
		dumpohci(ctlr);
}

static void
scanpci(void)
{
	ulong mem;
	Ctlr *ctlr;
	Pcidev *p;
	int i;
	static int already = 0;

	if(already)
		return;
	already = 1;
	p = nil;
	while(p = pcimatch(p, 0, 0)) {
		/*
		 * Find Ohci controllers (Programming Interface = 0x10).
		 */
		if(p->ccrb != Pcibcserial || p->ccru != Pciscusb ||
		    p->ccrp != 0x10)
			continue;
		mem = p->mem[0].bar & ~0x0F;
		dprint("ohci: %x/%x port 0x%lux size 0x%x irq %d\n",
			p->vid, p->did, mem, p->mem[0].size, p->intl);
		if(mem == 0){
			print("ohci: failed to map registers\n");
			continue;
		}
		if(p->intl == 0xFF || p->intl == 0) {
			print("ohci: no irq assigned for port %#lux\n", mem);
			continue;
		}

		ctlr = malloc(sizeof(Ctlr));
		if (ctlr == nil)
			panic("ohci: out of memory");
		ctlr->pcidev = p;
		ctlr->ohci = vmap(mem, p->mem[0].size);
		dprint("scanpci: ctlr %#p, ohci %#p\n", ctlr, ctlr->ohci);
		pcisetbme(p);
		pcisetpms(p, 0);
		for(i = 0; i < Nhcis; i++)
			if(ctlrs[i] == nil){
				ctlrs[i] = ctlr;
				break;
			}
		if(i == Nhcis)
			print("ohci: bug: no more controllers\n");
	}
}

static void
usbdebug(Hci*, int d)
{
	debug = d;
}

/*
 * build the periodic scheduling tree:
 * framesize must be a multiple of the tree size
 */
static void
mkqhtree(Ctlr *ctlr)
{
	int i, n, d, o, leaf0, depth;
	Ed **tree;
	Qtree *qt;

	depth = flog2(32);
	n = (1 << (depth+1)) - 1;
	qt = mallocz(sizeof(*qt), 1);
	if(qt == nil)
		panic("usb: can't allocate scheduling tree");
	qt->nel = n;
	qt->depth = depth;
	qt->bw = mallocz(n * sizeof(qt->bw), 1);
	qt->root = tree = mallocz(n * sizeof(Ed *), 1);
	if(qt->bw == nil || qt->root == nil)
		panic("usb: can't allocate scheduling tree");
	for(i = 0; i < n; i++){
		if((tree[i] = edalloc()) == nil)
			panic("mkqhtree");
		tree[i]->ctrl = (8 << Edmpsshift);	/* not needed */
		tree[i]->ctrl |= Edskip;

		if(i > 0)
			edlinked(tree[i], tree[(i-1)/2]);
		else
			edlinked(tree[i], nil);
	}
	ctlr->ntree = i;
	dprint("ohci: tree: %d endpoints allocated\n", i);

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
		ctlr->hcca->intrtable[i] = ptr2pa(tree[leaf0 + o]);
	}
	ctlr->tree = qt;
}

static void
ohcimeminit(Ctlr *ctlr)
{
	Hcca *hcca;

	edfree(edalloc());	/* allocate pools now */
	tdfree(tdalloc());

	hcca = xspanalloc(sizeof(Hcca), 256, 0);
	if(hcca == nil)
		panic("usbhreset: no memory for Hcca");
	memset(hcca, 0, sizeof(*hcca));
	ctlr->hcca = hcca;

	mkqhtree(ctlr);
}

static void
ohcireset(Ctlr *ctlr)
{
	ilock(ctlr);
	dprint("ohci %#p reset\n", ctlr->ohci);

	/*
	 * usually enter here in reset, wait till its through,
	 * then do our own so we are on known timing conditions.
	 * Is this needed?
	 */
	delay(100);
	ctlr->ohci->control = 0;
	delay(100);

	/* legacy support register: turn off lunacy mode */
	pcicfgw16(ctlr->pcidev, 0xc0, 0x2000);

	iunlock(ctlr);
}

static void
shutdown(Hci *hp)
{
	Ctlr *ctlr;

	ctlr = hp->aux;

	ilock(ctlr);
	ctlr->ohci->intrdisable = Mie;
	ctlr->ohci->control = 0;
	coherence();
	delay(100);
	iunlock(ctlr);
}

static int
reset(Hci *hp)
{
	int i;
	Ctlr *ctlr;
	Pcidev *p;
	static Lock resetlck;

	if(getconf("*nousbohci"))
		return -1;
	ilock(&resetlck);
	scanpci();

	/*
	 * Any adapter matches if no hp->port is supplied,
	 * otherwise the ports must match.
	 */
	ctlr = nil;
	for(i = 0; i < Nhcis && ctlrs[i] != nil; i++){
		ctlr = ctlrs[i];
		if(ctlr->active == 0)
		if(hp->port == 0 || hp->port == (uintptr)ctlr->ohci){
			ctlr->active = 1;
			break;
		}
	}
	iunlock(&resetlck);
	if(ctlrs[i] == nil || i == Nhcis)
		return -1;
	if(ctlr->ohci->control == ~0)
		return -1;


	p = ctlr->pcidev;
	hp->aux = ctlr;
	hp->port = (uintptr)ctlr->ohci;
	hp->irq = p->intl;
	hp->tbdf = p->tbdf;
	ctlr->nports = hp->nports = ctlr->ohci->rhdesca & 0xff;

	ohcireset(ctlr);
	ohcimeminit(ctlr);

	/*
	 * Linkage to the generic HCI driver.
	 */
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
	hp->debug = usbdebug;
	hp->type = "ohci";
	return 0;
}

void
usbohcilink(void)
{
	addhcitype("ohci", reset);
}
