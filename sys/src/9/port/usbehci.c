/*
 * USB Enhanced Host Controller Interface (EHCI) driver
 * High speed USB 2.0.
 *
 * Note that all of our unlock routines call coherence.
 *
 * BUGS:
 * - Too many delays and ilocks.
 * - bandwidth admission control must be done per-frame.
 * - requires polling (some controllers miss interrupts).
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
#include	"usbehci.h"
#include	"uncached.h"

#define diprint		if(ehcidebug || iso->debug)print
#define ddiprint	if(ehcidebug>1 || iso->debug>1)print
#define dqprint		if(ehcidebug || (qh->io && qh->io->debug))print
#define ddqprint	if(ehcidebug>1 || (qh->io && qh->io->debug>1))print

#define TRUNC(x, sz)	((x) & ((sz)-1))
#define LPTR(q)		((ulong*)KADDR((q) & ~0x1F))

typedef struct Ctlio Ctlio;
typedef union Ed Ed;
typedef struct Edpool Edpool;
typedef struct Itd Itd;
typedef struct Qio Qio;
typedef struct Qtd Qtd;
typedef struct Sitd Sitd;
typedef struct Td Td;

/*
 * EHCI interface registers and bits
 */
enum
{
	/* Queue states (software) */
	Qidle		= 0,
	Qinstall,
	Qrun,
	Qdone,
	Qclose,
	Qfree,

	Enabledelay	= 100,		/* waiting for a port to enable */
	Abortdelay	= 5,		/* delay after cancelling Tds (ms) */

	Incr		= 64,		/* for pools of Tds, Qhs, etc. */
	Align		= 128,		/* in bytes for all those descriptors */

	/* Keep them as a power of 2, lower than ctlr->nframes */
	/* Also, keep Nisoframes >= Nintrleafs */
	Nintrleafs	= 32,		/* nb. of leaf frames in intr. tree */
	Nisoframes	= 64,		/* nb. of iso frames (in window) */

	/*
	 * HW constants
	 */

	/* Itd bits (csw[]) */
	Itdactive	= 0x80000000,	/* execution enabled */
	Itddberr	= 0x40000000,	/* data buffer error */
	Itdbabble	= 0x20000000,	/* babble error */
	Itdtrerr	= 0x10000000,	/* transaction error */
	Itdlenshift	= 16,		/* transaction length */
	Itdlenmask	= 0xFFF,
	Itdioc		= 0x00008000,	/* interrupt on complete */
	Itdpgshift	= 12,		/* page select field */
	Itdoffshift	= 0,		/* transaction offset */
	/* Itd bits, buffer[] */
	Itdepshift	= 8,		/* endpoint address (buffer[0]) */
	Itddevshift	= 0,		/* device address (buffer[0]) */
	Itdin		= 0x800,	/* is input (buffer[1]) */
	Itdout		= 0,
	Itdmaxpktshift	= 0,		/* max packet (buffer[1]) */
	Itdntdsshift	= 0,		/* nb. of tds per µframe (buffer[2]) */

	Itderrors	= Itddberr|Itdbabble|Itdtrerr,

	/* Sitd bits (epc) */
	Stdin		= 0x80000000,	/* input direction */
	Stdportshift	= 24,		/* hub port number */
	Stdhubshift	= 16,		/* hub address */
	Stdepshift	= 8,		/* endpoint address */
	Stddevshift	= 0,		/* device address */
	/* Sitd bits (mfs) */
	Stdssmshift	= 0,		/* split start mask */
	Stdscmshift	= 8,		/* split complete mask */
	/* Sitd bits (csw) */
	Stdioc		= 0x80000000,	/* interrupt on complete */
	Stdpg		= 0x40000000,	/* page select */
	Stdlenshift	= 16,		/* total bytes to transfer */
	Stdlenmask	= 0x3FF,
	Stdactive	= 0x00000080,	/* active */
	Stderr		= 0x00000040,	/* tr. translator error */
	Stddberr	= 0x00000020,	/* data buffer error */
	Stdbabble	= 0x00000010,	/* babble error */
	Stdtrerr	= 0x00000008,	/* transaction error */
	Stdmmf		= 0x00000004,	/* missed µframe */
	Stddcs		= 0x00000002,	/* do complete split */

	Stderrors	= Stderr|Stddberr|Stdbabble|Stdtrerr|Stdmmf,

	/* Sitd bits buffer[1] */
	Stdtpall	= 0x00000000,	/* all payload here (188 bytes) */
	Stdtpbegin	= 0x00000008,	/* first payload for fs trans. */
	Stdtcntmask	= 0x00000007,	/* T-count */

	/* Td bits (csw) */
	Tddata1		= 0x80000000,	/* data toggle 1 */
	Tddata0		= 0x00000000,	/* data toggle 0 */
	Tdlenshift	= 16,		/* total bytes to transfer */
	Tdlenmask	= 0x7FFF,
	Tdmaxpkt	= 0x5000,	/* max buffer for a Td */
	Tdioc		= 0x00008000,	/* interrupt on complete */
	Tdpgshift	= 12,		/* current page */
	Tdpgmask	= 7,
	Tderr1		= 0x00000400,	/* bit 0 of error counter */
	Tderr2		= 0x00000800,	/* bit 1 of error counter */
	Tdtokout	= 0x00000000,	/* direction out */
	Tdtokin		= 0x00000100,	/* direction in */
	Tdtoksetup	= 0x00000200,	/* setup packet */
	Tdtok		= 0x00000300,	/* token bits */
	Tdactive		= 0x00000080,	/* active */
	Tdhalt		= 0x00000040,	/* halted */
	Tddberr		= 0x00000020,	/* data buffer error */
	Tdbabble	= 0x00000010,	/* babble error */
	Tdtrerr		= 0x00000008,	/* transaction error */
	Tdmmf		= 0x00000004,	/* missed µframe */
	Tddcs		= 0x00000002,	/* do complete split */
	Tdping		= 0x00000001,	/* do ping */

	Tderrors	= Tdhalt|Tddberr|Tdbabble|Tdtrerr|Tdmmf,

	/* Qh bits (eps0) */
	Qhrlcmask	= 0xF,		/* nak reload count */
	Qhrlcshift	= 28,		/* nak reload count */
	Qhnhctl		= 0x08000000,	/* not-high speed ctl */
	Qhmplmask	= 0x7FF,	/* max packet */
	Qhmplshift	= 16,
	Qhhrl		= 0x00008000,	/* head of reclamation list */
	Qhdtc		= 0x00004000,	/* data toggle ctl. */
	Qhint		= 0x00000080,	/* inactivate on next transition */
	Qhspeedmask	= 0x00003000,	/* speed bits */
	Qhfull		= 0x00000000,	/* full speed */
	Qhlow		= 0x00001000,	/* low speed */
	Qhhigh		= 0x00002000,	/* high speed */

	/* Qh bits (eps1) */
	Qhmultshift	= 30,		/* multiple tds per µframe */
	Qhmultmask	= 3,
	Qhportshift	= 23,		/* hub port number */
	Qhhubshift	= 16,		/* hub address */
	Qhscmshift	= 8,		/* split completion mask bits */
	Qhismshift	= 0,		/* interrupt sched. mask bits */
};

/*
 * Endpoint tree (software)
 */
struct Qtree
{
	int	nel;
	int	depth;
	ulong*	bw;
	Qh**	root;
};

/*
 * One per endpoint per direction, to control I/O.
 */
struct Qio
{
	QLock;			/* for the entire I/O process */
	Rendez;			/* wait for completion */
	Qh*	qh;		/* Td list (field const after init) */
	int	usbid;		/* usb address for endpoint/device */
	int	toggle;		/* Tddata0/Tddata1 */
	int	tok;		/* Tdtoksetup, Tdtokin, Tdtokout */
	ulong	iotime;		/* last I/O time; to hold interrupt polls */
	int	debug;		/* debug flag from the endpoint */
	char*	err;		/* error string */
	char*	tag;		/* debug (no room in Qh for this) */
	ulong	bw;
};

struct Ctlio
{
	Qio;			/* a single Qio for each RPC */
	uchar*	data;		/* read from last ctl req. */
	int	ndata;		/* number of bytes read */
};

struct Isoio
{
	QLock;
	Rendez;			/* wait for space/completion/errors */
	int	usbid;		/* address used for device/endpoint */
	int	tok;		/* Tdtokin or Tdtokout */
	int	state;		/* Qrun -> Qdone -> Qrun... -> Qclose */
	int	nframes;	/* number of frames ([S]Itds) used */
	uchar*	data;		/* iso data buffers if not embedded */
	char*	err;		/* error string */
	int	nerrs;		/* nb of consecutive I/O errors */
	ulong	maxsize;	/* ntds * ep->maxpkt */
	long	nleft;		/* number of bytes left from last write */
	int	debug;		/* debug flag from the endpoint */
	int	hs;		/* is high speed? */
	Isoio*	next;		/* in list of active Isoios */
	ulong	td0frno;	/* first frame used in ctlr */
	union{
		Itd*	tdi;	/* next td processed by interrupt */
		Sitd*	stdi;
	};
	union{
		Itd*	tdu;	/* next td for user I/O in tdps */
		Sitd*	stdu;
	};
	union{
		Itd**	itdps;	/* itdps[i]: ptr to Itd for i-th frame or nil */
		Sitd**	sitdps;	/* sitdps[i]: ptr to Sitd for i-th frame or nil */
		ulong**	tdps;	/* same thing, as seen by hw */
	};
};

struct Edpool
{
	Lock;
	Ed*	free;
	int	nalloc;
	int	ninuse;
	int	nfree;
};

/*
 * We use the 64-bit version for Itd, Sitd, Td, and Qh.
 * If the ehci is 64-bit capable it assumes we are using those
 * structures even when the system is 32 bits.
 */

/*
 * Iso transfer descriptor.  hw: 92 bytes, 108 bytes total
 * aligned to 32.
 */
struct Itd
{
	ulong	link;		/* to next hw struct */
	ulong	csw[8];		/* sts/length/pg/off. updated by hw */
	ulong	buffer[7];	/* buffer pointers, addrs, maxsz */
	ulong	xbuffer[7];	/* high 32 bits of buffer for 64-bits */

	ulong	_pad0;		/* pad to next cache line */
	/* cache-line boundary here */

	/* software */
	Itd*	next;
	ulong	ndata;		/* number of bytes in data */
	ulong	mdata;		/* max number of bytes in data */
	uchar*	data;
};

/*
 * Split transaction iso transfer descriptor.
 * hw: 36 bytes, 52 bytes total. aligned to 32.
 */
struct Sitd
{
	ulong	link;		/* to next hw struct */
	ulong	epc;		/* static endpoint state. addrs */
	ulong	mfs;		/* static endpoint state. µ-frame sched. */
	ulong	csw;		/* transfer state. updated by hw */
	ulong	buffer[2];	/* buf. ptr/offset. offset updated by hw */
				/* buf ptr/TP/Tcnt. TP/Tcnt updated by hw */
	ulong	blink;		/* back pointer */
	/* cache-line boundary after xbuffer[0] */
	ulong	xbuffer[2];	/* high 32 bits of buffer for 64-bits */

	/* software */
	Sitd*	next;
	ulong	ndata;		/* number of bytes in data */
	ulong	mdata;		/* max number of bytes in data */
	uchar*	data;
};

/*
 * Queue element transfer descriptor.
 * hw: first 52 bytes, total 68+sbuff bytes.  aligned to 32 bytes.
 */
struct Td
{
	ulong	nlink;		/* to next Td */
	ulong	alink;		/* alternate link to next Td */
	ulong	csw;		/* cmd/sts. updated by hw */
	ulong	buffer[5];	/* buf ptrs. offset updated by hw */
	/* cache-line boundary here */
	ulong	xbuffer[5];	/* high 32 bits of buffer for 64-bits */

	/* software */
	Td*	next;		/* in qh or Isoio or free list */
	ulong	ndata;		/* bytes available/used at data */
	uchar*	data;		/* pointer to actual data */
	uchar*	buff;		/* allocated data buffer or nil */
	uchar	sbuff[1];	/* first byte of embedded buffer */
};

/*
 * Queue head. Aligned to 32 bytes.
 * hw: first 68 bytes, 92 total.
 */
struct Qh
{
	ulong	link;		/* to next Qh in round robin */
	ulong	eps0;		/* static endpoint state. addrs */
	ulong	eps1;		/* static endpoint state. µ-frame sched. */

	/* updated by hw */
	ulong	tclink;		/* current Td (No Term bit here!) */
	ulong	nlink;		/* to next Td */
	ulong	alink;		/* alternate link to next Td */
	ulong	csw;		/* cmd/sts. updated by hw */
	/* cache-line boundary after buffer[0] */
	ulong	buffer[5];	/* buf ptrs. offset updated by hw */
	ulong	xbuffer[5];	/* high 32 bits of buffer for 64-bits */

	/* software */
	Qh*	next;		/* in controller list/tree of Qhs */
	int	state;		/* Qidle -> Qinstall -> Qrun -> Qdone | Qclose */
	Qio*	io;		/* for this queue */
	Td*	tds;		/* for this queue */
	int	sched;		/* slot for for intr. Qhs */
	Qh*	inext;		/* next in list of intr. qhs */
};

/*
 * We can avoid frame span traversal nodes if we don't span frames.
 * Just schedule transfers that can fit on the current frame and
 * wait a little bit otherwise.
 */

/*
 * Software. Ehci descriptors provided by pool.
 * There are soo few because we avoid using Fstn.
 */
union Ed
{
	Ed*	next;		/* in free list */
	Qh	qh;
	Td	td;
	Itd	itd;
	Sitd	sitd;
	uchar	align[Align];
};

int ehcidebug;

static Edpool edpool;
static char Ebug[] = "not yet implemented";
static char* qhsname[] = { "idle", "install", "run", "done", "close", "FREE" };

Ecapio* ehcidebugcapio;
int ehcidebugport;

void
ehcirun(Ctlr *ctlr, int on)
{
	int i;
	Eopio *opio;

	ddprint("ehci %#p %s\n", ctlr->capio, on ? "starting" : "halting");
	opio = ctlr->opio;
	if(on)
		opio->cmd |= Crun;
	else
		opio->cmd = Cstop;
	coherence();
	for(i = 0; i < 100; i++)
		if(on == 0 && (opio->sts & Shalted) != 0)
			break;
		else if(on != 0 && (opio->sts & Shalted) == 0)
			break;
		else
			delay(1);
	if(i == 100)
		print("ehci %#p %s cmd timed out\n",
			ctlr->capio, on ? "run" : "halt");
	ddprint("ehci %#p cmd %#lux sts %#lux\n",
		ctlr->capio, opio->cmd, opio->sts);
}

static void*
edalloc(void)
{
	Ed *ed, *pool;
	int i;

	lock(&edpool);
	if(edpool.free == nil){
		pool = xspanalloc(Incr*sizeof(Ed), Align, 0);
		if(pool == nil)
			panic("edalloc");
		for(i=Incr; --i>=0;){
			pool[i].next = edpool.free;
			edpool.free = &pool[i];
		}
		edpool.nalloc += Incr;
		edpool.nfree += Incr;
		dprint("ehci: edalloc: %d eds\n", edpool.nalloc);
	}
	ed = edpool.free;
	edpool.free = ed->next;
	edpool.ninuse++;
	edpool.nfree--;
	unlock(&edpool);

	memset(ed, 0, sizeof(Ed));	/* safety */
	assert(((ulong)ed & 0xF) == 0);
	return ed;
}

static void
edfree(void *a)
{
	Ed *ed;

	ed = a;
	lock(&edpool);
	ed->next = edpool.free;
	edpool.free = ed;
	edpool.ninuse--;
	edpool.nfree++;
	unlock(&edpool);
}

/*
 * Allocate and do some initialization.
 * Free after releasing buffers used.
 */

static Itd*
itdalloc(void)
{
	Itd *td;

	td = edalloc();
	td->link = Lterm;
	return td;
}

static void
itdfree(Itd *td)
{
	edfree(td);
}

static Sitd*
sitdalloc(void)
{
	Sitd *td;

	td = edalloc();
	td->link = td->blink = Lterm;
	return td;
}

static void
sitdfree(Sitd *td)
{
	edfree(td);
}

static Td*
tdalloc(void)
{
	Td *td;

	td = edalloc();
	td->nlink = td->alink = Lterm;
	return td;
}

static void
tdfree(Td *td)
{
	if(td == nil)
		return;
	free(td->buff);
	edfree(td);
}

static void
tdlinktd(Td *td, Td *next)
{
	td->next = next;
	td->alink = Lterm;
	if(next == nil)
		td->nlink = Lterm;
	else
		td->nlink = PADDR(next);
	coherence();
}

static Qh*
qhlinkqh(Qh *qh, Qh *next)
{
	qh->next = next;
	qh->link = PADDR(next)|Lqh;
	coherence();
	return qh;
}

static void
qhsetaddr(Qh *qh, ulong addr)
{
	ulong eps0;

	eps0 = qh->eps0 & ~((Epmax<<8)|Devmax);
	qh->eps0 = eps0 | addr & Devmax | ((addr >> 7) & Epmax) << 8;
	coherence();
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
schedq(Ctlr *ctlr, Qh *qh, int pollival)
{
	int q;
	Qh *tqh;
	ulong bw;

	bw = qh->io->bw;
	q = pickschedq(ctlr->tree, pollival, 0, ~0);
	ddqprint("ehci: sched %#p q %d, ival %d, bw %uld\n",
		qh->io, q, pollival, bw);
	if(q < 0){
		print("ehci: no room for ed\n");
		return -1;
	}
	ctlr->tree->bw[q] += bw;
	tqh = ctlr->tree->root[q];
	qh->sched = q;
	qhlinkqh(qh, tqh->next);
	qhlinkqh(tqh, qh);
	coherence();
	qh->inext = ctlr->intrqhs;
	ctlr->intrqhs = qh;
	coherence();
	return 0;
}

static void
unschedq(Ctlr *ctlr, Qh *qh)
{
	int q;
	Qh *prev, *this, *next;
	Qh **l;
	ulong bw;

	bw = qh->io->bw;
	q = qh->sched;
	if(q < 0)
		return;
	ctlr->tree->bw[q] -= bw;

	prev = ctlr->tree->root[q];
	this = prev->next;
	while(this != nil && this != qh){
		prev = this;
		this = this->next;
	}
	if(this == nil)
		print("ehci: unschedq %d: not found\n", q);
	else{
		next = this->next;
		qhlinkqh(prev, next);
	}
	for(l = &ctlr->intrqhs; *l != nil; l = &(*l)->inext)
		if(*l == qh){
			*l = (*l)->inext;
			return;
		}
	print("ehci: unschedq: qh %#p not found\n", qh);
}

static ulong
qhmaxpkt(Qh *qh)
{
	return (qh->eps0 >> Qhmplshift) & Qhmplmask;
}

static void
qhsetmaxpkt(Qh *qh, int maxpkt)
{
	ulong eps0;

	eps0 = qh->eps0 & ~(Qhmplmask << Qhmplshift);
	qh->eps0 = eps0 | (maxpkt & Qhmplmask) << Qhmplshift;
	coherence();
}

/*
 * Initialize the round-robin circular list of ctl/bulk Qhs
 * if ep is nil. Otherwise, allocate and link a new Qh in the ctlr.
 */
static Qh*
qhalloc(Ctlr *ctlr, Ep *ep, Qio *io, char* tag)
{
	Qh *qh;
	int ttype;

	qh = edalloc();
	qh->nlink = Lterm;
	qh->alink = Lterm;
	qh->csw = Tdhalt;
	qh->state = Qidle;
	qh->sched = -1;
	qh->io = io;
	if(ep != nil){
		qh->eps0 = 0;
		qhsetmaxpkt(qh, ep->maxpkt);
		if(ep->dev->speed == Lowspeed)
			qh->eps0 |= Qhlow;
		if(ep->dev->speed == Highspeed)
			qh->eps0 |= Qhhigh;
		else if(ep->ttype == Tctl)
			qh->eps0 |= Qhnhctl;
		qh->eps0 |= Qhdtc | 8 << Qhrlcshift;	/* 8 naks max */
		coherence();
		qhsetaddr(qh, io->usbid);
		qh->eps1 = (ep->ntds & Qhmultmask) << Qhmultshift;
		qh->eps1 |= ep->dev->port << Qhportshift;
		qh->eps1 |= ep->dev->hub << Qhhubshift;
		qh->eps1 |= 034 << Qhscmshift;
		if(ep->ttype == Tintr)
			qh->eps1 |= 1 << Qhismshift;	/* intr. start µf. */
		coherence();
		if(io != nil)
			io->tag = tag;
	}
	ilock(ctlr);
	ttype = Tctl;
	if(ep != nil)
		ttype = ep->ttype;
	switch(ttype){
	case Tctl:
	case Tbulk:
		if(ctlr->qhs == nil){
			ctlr->qhs = qhlinkqh(qh, qh);
			qh->eps0 |= Qhhigh | Qhhrl;
			coherence();
			ctlr->opio->link = PADDR(qh)|Lqh;
			coherence();
		}else{
			qhlinkqh(qh, ctlr->qhs->next);
			qhlinkqh(ctlr->qhs, qh);
		}
		break;
	case Tintr:
		schedq(ctlr, qh, ep->pollival);
		break;
	default:
		print("ehci: qhalloc called for ttype != ctl/bulk\n");
	}
	iunlock(ctlr);
	return qh;
}

static int
qhadvanced(void *a)
{
	Ctlr *ctlr;

	ctlr = a;
	return (ctlr->opio->cmd & Ciasync) == 0;
}

/*
 * called when a qh is removed, to be sure the hw is not
 * keeping pointers into it.
 */
static void
qhcoherency(Ctlr *ctlr)
{
	int i;

	qlock(&ctlr->portlck);
	ctlr->opio->cmd |= Ciasync;	/* ask for intr. on async advance */
	coherence();
	for(i = 0; i < 3 && qhadvanced(ctlr) == 0; i++)
		if(!waserror()){
			tsleep(ctlr, qhadvanced, ctlr, Abortdelay);
			poperror();
		}
	dprint("ehci: qhcoherency: doorbell %d\n", qhadvanced(ctlr));
	if(i == 3)
		print("ehci: async advance doorbell did not ring\n");
	ctlr->opio->cmd &= ~Ciasync;	/* try to clean */
	qunlock(&ctlr->portlck);
}

static void
qhfree(Ctlr *ctlr, Qh *qh)
{
	Td *td, *ltd;
	Qh *q;

	if(qh == nil)
		return;
	ilock(ctlr);
	if(qh->sched < 0){
		for(q = ctlr->qhs; q != nil; q = q->next)
			if(q->next == qh)
				break;
		if(q == nil)
			panic("qhfree: nil q");
		q->next = qh->next;
		q->link = qh->link;
		coherence();
	}else
		unschedq(ctlr, qh);
	iunlock(ctlr);

	qhcoherency(ctlr);

	for(td = qh->tds; td != nil; td = ltd){
		ltd = td->next;
		tdfree(td);
	}

	edfree(qh);
}

static void
qhlinktd(Qh *qh, Td *td)
{
	ulong csw;
	int i;

	csw = qh->csw;
	qh->tds = td;
	if(td == nil)
		qh->csw = (csw & ~Tdactive) | Tdhalt;
	else{
		csw &= Tddata1 | Tdping;	/* save */
		qh->csw = Tdhalt;
		coherence();
		qh->tclink = 0;
		qh->alink = Lterm;
		qh->nlink = PADDR(td);
		for(i = 0; i < nelem(qh->buffer); i++)
			qh->buffer[i] = 0;
		coherence();
		qh->csw = csw & ~(Tdhalt|Tdactive);	/* activate next */
	}
	coherence();
}

static char*
seprintlink(char *s, char *se, char *name, ulong l, int typed)
{
	s = seprint(s, se, "%s %ulx", name, l);
	if((l & Lterm) != 0)
		return seprint(s, se, "T");
	if(typed == 0)
		return s;
	switch(l & (3<<1)){
	case Litd:
		return seprint(s, se, "I");
	case Lqh:
		return seprint(s, se, "Q");
	case Lsitd:
		return seprint(s, se, "S");
	default:
		return seprint(s, se, "F");
	}
}

static char*
seprintitd(char *s, char *se, Itd *td)
{
	int i;
	ulong b0, b1;
	char flags[6];
	char *rw;

	if(td == nil)
		return seprint(s, se, "<nil itd>\n");
	b0 = td->buffer[0];
	b1 = td->buffer[1];

	s = seprint(s, se, "itd %#p", td);
	rw = (b1 & Itdin) ? "in" : "out";
	s = seprint(s, se, " %s ep %uld dev %uld max %uld mult %uld",
		rw, (b0>>8)&Epmax, (b0&Devmax),
		td->buffer[1] & 0x7ff, b1 & 3);
	s = seprintlink(s, se, " link", td->link, 1);
	s = seprint(s, se, "\n");
	for(i = 0; i < nelem(td->csw); i++){
		memset(flags, '-', 5);
		if((td->csw[i] & Itdactive) != 0)
			flags[0] = 'a';
		if((td->csw[i] & Itdioc) != 0)
			flags[1] = 'i';
		if((td->csw[i] & Itddberr) != 0)
			flags[2] = 'd';
		if((td->csw[i] & Itdbabble) != 0)
			flags[3] = 'b';
		if((td->csw[i] & Itdtrerr) != 0)
			flags[4] = 't';
		flags[5] = 0;
		s = seprint(s, se, "\ttd%d %s", i, flags);
		s = seprint(s, se, " len %uld", (td->csw[i] >> 16) & 0x7ff);
		s = seprint(s, se, " pg %uld", (td->csw[i] >> 12) & 0x7);
		s = seprint(s, se, " off %uld\n", td->csw[i] & 0xfff);
	}
	s = seprint(s, se, "\tbuffs:");
	for(i = 0; i < nelem(td->buffer); i++)
		s = seprint(s, se, " %#lux", td->buffer[i] >> 12);
	return seprint(s, se, "\n");
}

static char*
seprintsitd(char *s, char *se, Sitd *td)
{
	char rw, pg, ss;
	char flags[8];
	static char pc[4] = { 'a', 'b', 'm', 'e' };

	if(td == nil)
		return seprint(s, se, "<nil sitd>\n");
	s = seprint(s, se, "sitd %#p", td);
	rw = (td->epc & Stdin) ? 'r' : 'w';
	s = seprint(s, se, " %c ep %uld dev %uld",
		rw, (td->epc>>8)&0xf, td->epc&0x7f);
	s = seprint(s, se, " max %uld", (td->csw >> 16) & 0x3ff);
	s = seprint(s, se, " hub %uld", (td->epc >> 16) & 0x7f);
	s = seprint(s, se, " port %uld\n", (td->epc >> 24) & 0x7f);
	memset(flags, '-', 7);
	if((td->csw & Stdactive) != 0)
		flags[0] = 'a';
	if((td->csw & Stdioc) != 0)
		flags[1] = 'i';
	if((td->csw & Stderr) != 0)
		flags[2] = 'e';
	if((td->csw & Stddberr) != 0)
		flags[3] = 'd';
	if((td->csw & Stdbabble) != 0)
		flags[4] = 'b';
	if((td->csw & Stdtrerr) != 0)
		flags[5] = 't';
	if((td->csw & Stdmmf) != 0)
		flags[6] = 'n';
	flags[7] = 0;
	ss = (td->csw & Stddcs) ? 'c' : 's';
	pg = (td->csw & Stdpg) ? '1' : '0';
	s = seprint(s, se, "\t%s %cs pg%c", flags, ss, pg);
	s = seprint(s, se, " b0 %#lux b1 %#lux off %uld\n",
		td->buffer[0] >> 12, td->buffer[1] >> 12, td->buffer[0] & 0xfff);
	s = seprint(s, se, "\ttpos %c tcnt %uld",
		pc[(td->buffer[0]>>3)&3], td->buffer[1] & 7);
	s = seprint(s, se, " ssm %#lux csm %#lux cspm %#lux",
		td->mfs & 0xff, (td->mfs>>8) & 0xff, (td->csw>>8) & 0xff);
	s = seprintlink(s, se, " link", td->link, 1);
	s = seprintlink(s, se, " blink", td->blink, 0);
	return seprint(s, se, "\n");
}

static long
maxtdlen(Td *td)
{
	return (td->csw >> Tdlenshift) & Tdlenmask;
}

static long
tdlen(Td *td)
{
	if(td->data == nil)
		return 0;
	return td->ndata - maxtdlen(td);
}

static char*
seprinttd(char *s, char *se, Td *td, char *tag)
{
	int i;
	char t, ss;
	char flags[9];
	static char *tok[4] = { "out", "in", "setup", "BUG" };

	if(td == nil)
		return seprint(s, se, "%s <nil td>\n", tag);
	s = seprint(s, se, "%s %#p", tag, td);
	s = seprintlink(s, se, " nlink", td->nlink, 0);
	s = seprintlink(s, se, " alink", td->alink, 0);
	s = seprint(s, se, " %s", tok[(td->csw & Tdtok) >> 8]);
	if((td->csw & Tdping) != 0)
		s = seprint(s, se, " png");
	memset(flags, '-', 8);
	if((td->csw & Tdactive) != 0)
		flags[0] = 'a';
	if((td->csw & Tdioc) != 0)
		flags[1] = 'i';
	if((td->csw & Tdhalt) != 0)
		flags[2] = 'h';
	if((td->csw & Tddberr) != 0)
		flags[3] = 'd';
	if((td->csw & Tdbabble) != 0)
		flags[4] = 'b';
	if((td->csw & Tdtrerr) != 0)
		flags[5] = 't';
	if((td->csw & Tdmmf) != 0)
		flags[6] = 'n';
	if((td->csw & (Tderr2|Tderr1)) == 0)
		flags[7] = 'z';
	flags[8] = 0;
	t = (td->csw & Tddata1) ? '1' : '0';
	ss = (td->csw & Tddcs) ? 'c' : 's';
	s = seprint(s, se, "\n\td%c %s %cs", t, flags, ss);
	s = seprint(s, se, " max %uld", maxtdlen(td));
	s = seprint(s, se, " pg %uld off %#lux\n",
		(td->csw >> Tdpgshift) & Tdpgmask, td->buffer[0] & 0xFFF);
	s = seprint(s, se, "\tbuffs:");
	for(i = 0; i < nelem(td->buffer); i++)
		s = seprint(s, se, " %#lux", td->buffer[i]>>12);
	if(td->data != nil)
		s = seprintdata(s, se, td->data, td->ndata);
	return seprint(s, se, "\n");
}

static void
dumptd(Td *td, char *pref)
{
	char buf[256];
	char *se;
	int i;

	i = 0;
	se = buf+sizeof(buf);
	for(; td != nil; td = td->next){
		seprinttd(buf, se, td, pref);
		print("%s", buf);
		if(i++ > 20){
			print("...more tds...\n");
			break;
		}
	}
}

static void
qhdump(Qh *qh)
{
	char buf[256];
	char *s, *se, *tag;
	Td td;
	static char *speed[] = {"full", "low", "high", "BUG"};

	if(qh == nil){
		print("<nil qh>\n");
		return;
	}
	if(qh->io == nil)
		tag = "qh";
	else
		tag = qh->io->tag;
	se = buf+sizeof(buf);
	s = seprint(buf, se, "%s %#p", tag, qh);
	s = seprint(s, se, " ep %uld dev %uld",
		(qh->eps0>>8)&0xf, qh->eps0&0x7f);
	s = seprint(s, se, " hub %uld", (qh->eps1 >> 16) & 0x7f);
	s = seprint(s, se, " port %uld", (qh->eps1 >> 23) & 0x7f);
	s = seprintlink(s, se, " link", qh->link, 1);
	seprint(s, se, "  clink %#lux", qh->tclink);
	print("%s\n", buf);
	s = seprint(buf, se, "\tnrld %uld", (qh->eps0 >> Qhrlcshift) & Qhrlcmask);
	s = seprint(s, se, " nak %uld", (qh->alink >> 1) & 0xf);
	s = seprint(s, se, " max %uld ", qhmaxpkt(qh));
	if((qh->eps0 & Qhnhctl) != 0)
		s = seprint(s, se, "c");
	if((qh->eps0 & Qhhrl) != 0)
		s = seprint(s, se, "h");
	if((qh->eps0 & Qhdtc) != 0)
		s = seprint(s, se, "d");
	if((qh->eps0 & Qhint) != 0)
		s = seprint(s, se, "i");
	s = seprint(s, se, " %s", speed[(qh->eps0 >> 12) & 3]);
	s = seprint(s, se, " mult %uld", (qh->eps1 >> Qhmultshift) & Qhmultmask);
	seprint(s, se, " scm %#lux ism %#lux\n",
		(qh->eps1 >> 8 & 0xff), qh->eps1 & 0xff);
	print("%s\n", buf);
	memset(&td, 0, sizeof(td));
	memmove(&td, &qh->nlink, 32);	/* overlay area */
	seprinttd(buf, se, &td, "\tovl");
	print("%s", buf);
}

static void
isodump(Isoio* iso, int all)
{
	Itd *td, *tdi, *tdu;
	Sitd *std, *stdi, *stdu;
	char buf[256];
	int i;

	if(iso == nil){
		print("<nil iso>\n");
		return;
	}
	print("iso %#p %s %s speed state %d nframes %d maxsz %uld",
		iso, iso->tok == Tdtokin ? "in" : "out",
		iso->hs ? "high" : "full",
		iso->state, iso->nframes, iso->maxsize);
	print(" td0 %uld tdi %#p tdu %#p data %#p\n",
		iso->td0frno, iso->tdi, iso->tdu, iso->data);
	if(iso->err != nil)
		print("\terr %s\n", iso->err);
	if(iso->err != nil)
		print("\terr='%s'\n", iso->err);
	if(all == 0)
		if(iso->hs != 0){
			tdi = iso->tdi;
			seprintitd(buf, buf+sizeof(buf), tdi);
			print("\ttdi %s\n", buf);
			tdu = iso->tdu;
			seprintitd(buf, buf+sizeof(buf), tdu);
			print("\ttdu %s\n", buf);
		}else{
			stdi = iso->stdi;
			seprintsitd(buf, buf+sizeof(buf), stdi);
			print("\tstdi %s\n", buf);
			stdu = iso->stdu;
			seprintsitd(buf, buf+sizeof(buf), stdu);
			print("\tstdu %s\n", buf);
		}
	else
		for(i = 0; i < Nisoframes; i++)
			if(iso->tdps[i] != nil)
				if(iso->hs != 0){
					td = iso->itdps[i];
					seprintitd(buf, buf+sizeof(buf), td);
					if(td == iso->tdi)
						print("i->");
					if(td == iso->tdu)
						print("i->");
					print("[%d]\t%s", i, buf);
				}else{
					std = iso->sitdps[i];
					seprintsitd(buf, buf+sizeof(buf), std);
					if(std == iso->stdi)
						print("i->");
					if(std == iso->stdu)
						print("u->");
					print("[%d]\t%s", i, buf);
				}
}

static void
dump(Hci *hp)
{
	int i;
	char *s, *se;
	char buf[128];
	Ctlr *ctlr;
	Eopio *opio;
	Isoio *iso;
	Qh *qh;

	ctlr = hp->aux;
	opio = ctlr->opio;
	ilock(ctlr);
	print("ehci port %#p frames %#p (%d fr.) nintr %d ntdintr %d",
		ctlr->capio, ctlr->frames, ctlr->nframes,
		ctlr->nintr, ctlr->ntdintr);
	print(" nqhintr %d nisointr %d\n", ctlr->nqhintr, ctlr->nisointr);
	print("\tcmd %#lux sts %#lux intr %#lux frno %uld",
		opio->cmd, opio->sts, opio->intr, opio->frno);
	print(" base %#lux link %#lux fr0 %#lux\n",
		opio->frbase, opio->link, ctlr->frames[0]);
	se = buf+sizeof(buf);
	s = seprint(buf, se, "\t");
	for(i = 0; i < hp->nports; i++){
		s = seprint(s, se, "p%d %#lux ", i, opio->portsc[i]);
		if(hp->nports > 4 && i == hp->nports/2 - 1)
			s = seprint(s, se, "\n\t");
	}
	print("%s\n", buf);
	qh = ctlr->qhs;
	i = 0;
	do{
		qhdump(qh);
		qh = qh->next;
	}while(qh != ctlr->qhs && i++ < 100);
	if(i > 100)
		print("...too many Qhs...\n");
	if(ctlr->intrqhs != nil)
		print("intr qhs:\n");
	for(qh = ctlr->intrqhs; qh != nil; qh = qh->inext)
		qhdump(qh);
	if(ctlr->iso != nil)
		print("iso:\n");
	for(iso = ctlr->iso; iso != nil; iso = iso->next)
		isodump(ctlr->iso, 0);
	print("%d eds in tree\n", ctlr->ntree);
	iunlock(ctlr);
	lock(&edpool);
	print("%d eds allocated = %d in use + %d free\n",
		edpool.nalloc, edpool.ninuse, edpool.nfree);
	unlock(&edpool);
}

static char*
errmsg(int err)
{
	if(err == 0)
		return "ok";
	if(err & Tddberr)
		return "data buffer error";
	if(err & Tdbabble)
		return "babble detected";
	if(err & Tdtrerr)
		return "transaction error";
	if(err & Tdmmf)
		return "missed µframe";
	if(err & Tdhalt)
		return Estalled;	/* [uo]hci report this error */
	return Eio;
}

static char*
ierrmsg(int err)
{
	if(err == 0)
		return "ok";
	if(err & Itddberr)
		return "data buffer error";
	if(err & Itdbabble)
		return "babble detected";
	if(err & Itdtrerr)
		return "transaction error";
	return Eio;
}

static char*
serrmsg(int err)
{
	if(err & Stderr)
		return "translation translator error";
	/* other errors have same numbers than Td errors */
	return errmsg(err);
}

static int
isocanread(void *a)
{
	Isoio *iso;

	iso = a;
	if(iso->state == Qclose)
		return 1;
	if(iso->state == Qrun && iso->tok == Tdtokin){
		if(iso->hs != 0 && iso->tdi != iso->tdu)
			return 1;
		if(iso->hs == 0 && iso->stdi != iso->stdu)
			return 1;
	}
	return 0;
}

static int
isocanwrite(void *a)
{
	Isoio *iso;

	iso = a;
	if(iso->state == Qclose)
		return 1;
	if(iso->state == Qrun && iso->tok == Tdtokout){
		if(iso->hs != 0 && iso->tdu->next != iso->tdi)
			return 1;
		if(iso->hs == 0 && iso->stdu->next != iso->stdi)
			return 1;
	}
	return 0;
}

static void
itdinit(Isoio *iso, Itd *td)
{
	int p, t;
	ulong pa, tsize, size;

	/*
	 * BUG: This does not put an integral number of samples
	 * on each µframe unless samples per packet % 8 == 0
	 * Also, all samples are packed early on each frame.
	 */
	p = 0;
	size = td->ndata = td->mdata;
	pa = PADDR(td->data);
	for(t = 0; size > 0 && t < 8; t++){
		tsize = size;
		if(tsize > iso->maxsize)
			tsize = iso->maxsize;
		size -= tsize;
		assert(p < nelem(td->buffer));
		td->csw[t] = tsize << Itdlenshift | p << Itdpgshift |
			(pa & 0xFFF) << Itdoffshift | Itdactive | Itdioc;
		coherence();
		if(((pa+tsize) & ~0xFFF) != (pa & ~0xFFF))
			p++;
		pa += tsize;
	}
}

static void
sitdinit(Isoio *iso, Sitd *td)
{
	td->ndata = td->mdata & Stdlenmask;
	td->buffer[0] = PADDR(td->data);
	td->buffer[1] = (td->buffer[0] & ~0xFFF) + 0x1000;
	if(iso->tok == Tdtokin || td->ndata <= 188)
		td->buffer[1] |= Stdtpall;
	else
		td->buffer[1] |= Stdtpbegin;
	if(iso->tok == Tdtokin)
		td->buffer[1] |= 1;
	else
		td->buffer[1] |= ((td->ndata + 187) / 188) & Stdtcntmask;
	coherence();
	td->csw = td->ndata << Stdlenshift | Stdactive | Stdioc;
	coherence();
}

static int
itdactive(Itd *td)
{
	int i;

	for(i = 0; i < nelem(td->csw); i++)
		if((td->csw[i] & Itdactive) != 0)
			return 1;
	return 0;
}

static int
isohsinterrupt(Ctlr *ctlr, Isoio *iso)
{
	int err, i, nframes, t;
	Itd *tdi;

	tdi = iso->tdi;
	assert(tdi != nil);
	if(itdactive(tdi))			/* not all tds are done */
		return 0;
	ctlr->nisointr++;
	ddiprint("isohsintr: iso %#p: tdi %#p tdu %#p\n", iso, tdi, iso->tdu);
	if(iso->state != Qrun && iso->state != Qdone)
		panic("isofsintr: iso state");
	if(ehcidebug > 1 || iso->debug > 1)
		isodump(iso, 0);

	nframes = iso->nframes / 2;		/* limit how many we look */
	if(nframes > Nisoframes)
		nframes = Nisoframes;

	if(iso->tok == Tdtokin)
		tdi->ndata = 0;
	/* else, it has the number of bytes transferred */

	for(i = 0; i < nframes && itdactive(tdi) == 0; i++){
		if(iso->tok == Tdtokin)
			tdi->ndata += (tdi->csw[i] >> Itdlenshift) & Itdlenmask;
		err = 0;
		coherence();
		for(t = 0; t < nelem(tdi->csw); t++){
			tdi->csw[t] &= ~Itdioc;
			coherence();
			err |= tdi->csw[t] & Itderrors;
		}
		if(err == 0)
			iso->nerrs = 0;
		else if(iso->nerrs++ > iso->nframes/2){
			if(iso->err == nil){
				iso->err = ierrmsg(err);
				diprint("isohsintr: tdi %#p error %#ux %s\n",
					tdi, err, iso->err);
				diprint("ctlr load %uld\n", ctlr->load);
			}
			tdi->ndata = 0;
		}else
			tdi->ndata = 0;
		if(tdi->next == iso->tdu || tdi->next->next == iso->tdu){
			memset(iso->tdu->data, 0, iso->tdu->mdata);
			itdinit(iso, iso->tdu);
			iso->tdu = iso->tdu->next;
			iso->nleft = 0;
		}
		tdi = tdi->next;
		coherence();
	}
	ddiprint("isohsintr: %d frames processed\n", nframes);
	if(i == nframes){
		tdi->csw[0] |= Itdioc;
		coherence();
	}
	iso->tdi = tdi;
	coherence();
	if(isocanwrite(iso) || isocanread(iso)){
		diprint("wakeup iso %#p tdi %#p tdu %#p\n", iso,
			iso->tdi, iso->tdu);
		wakeup(iso);
	}
	return 1;
}

static int
isofsinterrupt(Ctlr *ctlr, Isoio *iso)
{
	int err, i, nframes;
	Sitd *stdi;

	stdi = iso->stdi;
	assert(stdi != nil);
	if((stdi->csw & Stdactive) != 0)		/* nothing new done */
		return 0;
	ctlr->nisointr++;
	ddiprint("isofsintr: iso %#p: tdi %#p tdu %#p\n", iso, stdi, iso->stdu);
	if(iso->state != Qrun && iso->state != Qdone)
		panic("isofsintr: iso state");
	if(ehcidebug > 1 || iso->debug > 1)
		isodump(iso, 0);

	nframes = iso->nframes / 2;		/* limit how many we look */
	if(nframes > Nisoframes)
		nframes = Nisoframes;

	for(i = 0; i < nframes && (stdi->csw & Stdactive) == 0; i++){
		stdi->csw &= ~Stdioc;
		/* write back csw and see if it produces errors */
		coherence();
		err = stdi->csw & Stderrors;
		if(err == 0){
			iso->nerrs = 0;
			if(iso->tok == Tdtokin)
				stdi->ndata = (stdi->csw>>Stdlenshift)&Stdlenmask;
			/* else len is assumed correct */
		}else if(iso->nerrs++ > iso->nframes/2){
			if(iso->err == nil){
				iso->err = serrmsg(err);
				diprint("isofsintr: tdi %#p error %#ux %s\n",
					stdi, err, iso->err);
				diprint("ctlr load %uld\n", ctlr->load);
			}
			stdi->ndata = 0;
		}else
			stdi->ndata = 0;

		if(stdi->next == iso->stdu || stdi->next->next == iso->stdu){
			memset(iso->stdu->data, 0, iso->stdu->mdata);
			coherence();
			sitdinit(iso, iso->stdu);
			iso->stdu = iso->stdu->next;
			iso->nleft = 0;
		}
		coherence();
		stdi = stdi->next;
	}
	ddiprint("isofsintr: %d frames processed\n", nframes);
	if(i == nframes){
		stdi->csw |= Stdioc;
		coherence();
	}
	iso->stdi = stdi;
	coherence();
	if(isocanwrite(iso) || isocanread(iso)){
		diprint("wakeup iso %#p tdi %#p tdu %#p\n", iso,
			iso->stdi, iso->stdu);
		wakeup(iso);
	}
	return 1;
}

static int
qhinterrupt(Ctlr *ctlr, Qh *qh)
{
	Td *td;
	int err;

	if(qh->state != Qrun)
		panic("qhinterrupt: qh state");
	td = qh->tds;
	if(td == nil)
		panic("qhinterrupt: no tds");
	if((td->csw & Tdactive) == 0)
		ddqprint("qhinterrupt port %#p qh %#p\n", ctlr->capio, qh);
	for(; td != nil; td = td->next){
		if(td->csw & Tdactive)
			return 0;
		err = td->csw & Tderrors;
		if(err != 0){
			if(qh->io->err == nil){
				qh->io->err = errmsg(err);
				dqprint("qhintr: td %#p csw %#lux error %#ux %s\n",
					td, td->csw, err, qh->io->err);
			}
			break;
		}
		td->ndata = tdlen(td);
		coherence();
		if(td->ndata < maxtdlen(td)){	/* EOT */
			td = td->next;
			break;
		}
	}
	/*
	 * Done. Make void the Tds not used (errors or EOT) and wakeup epio.
	 */
	for(; td != nil; td = td->next)
		td->ndata = 0;
	coherence();
	qh->state = Qdone;
	coherence();
	wakeup(qh->io);
	return 1;
}

static int
ehciintr(Hci *hp)
{
	Ctlr *ctlr;
	Eopio *opio;
	Isoio *iso;
	ulong sts;
	Qh *qh;
	int i, some;

	ctlr = hp->aux;
	opio = ctlr->opio;

	/*
	 * Will we know in USB 3.0 who the interrupt was for?.
	 * Do they still teach indexing in CS?
	 * This is Intel's doing.
	 */
	ilock(ctlr);
	ctlr->nintr++;
	sts = opio->sts & Sintrs;
	if(sts == 0){		/* not ours; shared intr. */
		iunlock(ctlr);
		return 0;
	}
	opio->sts = sts;
	coherence();
	if((sts & Sherr) != 0)
		print("ehci: port %#p fatal host system error\n", ctlr->capio);
	if((sts & Shalted) != 0)
		print("ehci: port %#p: halted\n", ctlr->capio);
	if((sts & Sasync) != 0){
		dprint("ehci: doorbell\n");
		wakeup(ctlr);
	}
	/*
	 * We enter always this if, even if it seems the
	 * interrupt does not report anything done/failed.
	 * Some controllers don't post interrupts right.
	 */
	some = 0;
	if((sts & (Serrintr|Sintr)) != 0){
		ctlr->ntdintr++;
		if(ehcidebug > 1){
			print("ehci port %#p frames %#p nintr %d ntdintr %d",
				ctlr->capio, ctlr->frames,
				ctlr->nintr, ctlr->ntdintr);
			print(" nqhintr %d nisointr %d\n",
				ctlr->nqhintr, ctlr->nisointr);
			print("\tcmd %#lux sts %#lux intr %#lux frno %uld",
				opio->cmd, opio->sts, opio->intr, opio->frno);
		}

		/* process the Iso transfers */
		for(iso = ctlr->iso; iso != nil; iso = iso->next)
			if(iso->state == Qrun || iso->state == Qdone)
				if(iso->hs != 0)
					some += isohsinterrupt(ctlr, iso);
				else
					some += isofsinterrupt(ctlr, iso);

		/* process the qhs in the periodic tree */
		for(qh = ctlr->intrqhs; qh != nil; qh = qh->inext)
			if(qh->state == Qrun)
				some += qhinterrupt(ctlr, qh);

		/* process the async Qh circular list */
		qh = ctlr->qhs;
		i = 0;
		do{
			if (qh == nil)
				panic("ehciintr: nil qh");
			if(qh->state == Qrun)
				some += qhinterrupt(ctlr, qh);
			qh = qh->next;
		}while(qh != ctlr->qhs && i++ < 100);
		if(i > 100)
			print("echi: interrupt: qh loop?\n");
	}
//	if (some == 0)
//		panic("ehciintr: no work");
	iunlock(ctlr);
	return some;
}

static void
interrupt(Ureg*, void* a)
{
	ehciintr(a);
}

static int
portenable(Hci *hp, int port, int on)
{
	Ctlr *ctlr;
	Eopio *opio;
	int s;

	ctlr = hp->aux;
	opio = ctlr->opio;
	s = opio->portsc[port-1];
	qlock(&ctlr->portlck);
	if(waserror()){
		qunlock(&ctlr->portlck);
		nexterror();
	}
	dprint("ehci %#p port %d enable=%d; sts %#x\n",
		ctlr->capio, port, on, s);
	ilock(ctlr);
	if(s & (Psstatuschg | Pschange))
		opio->portsc[port-1] = s;
	if(on)
		opio->portsc[port-1] |= Psenable;
	else
		opio->portsc[port-1] &= ~Psenable;
	coherence();
	microdelay(64);
	iunlock(ctlr);
	tsleep(&up->sleep, return0, 0, Enabledelay);
	dprint("ehci %#p port %d enable=%d: sts %#lux\n",
		ctlr->capio, port, on, opio->portsc[port-1]);
	qunlock(&ctlr->portlck);
	poperror();
	return 0;
}

/*
 * If we detect during status that the port is low-speed or
 * during reset that it's full-speed, the device is not for
 * ourselves. The companion controller will take care.
 * Low-speed devices will not be seen by usbd. Full-speed
 * ones are seen because it's only after reset that we know what
 * they are (usbd may notice a device not enabled in this case).
 */
static void
portlend(Ctlr *ctlr, int port, char *ss)
{
	Eopio *opio;
	ulong s;

	opio = ctlr->opio;

	dprint("ehci %#p port %d: %s speed device: no longer owned\n",
		ctlr->capio, port, ss);
	s = opio->portsc[port-1] & ~(Pschange|Psstatuschg);
	opio->portsc[port-1] = s | Psowner;
	coherence();
}

static int
portreset(Hci *hp, int port, int on)
{
	ulong s;
	Eopio *opio;
	Ctlr *ctlr;
	int i;

	if(on == 0)
		return 0;

	ctlr = hp->aux;
	opio = ctlr->opio;
	qlock(&ctlr->portlck);
	if(waserror()){
		iunlock(ctlr);
		qunlock(&ctlr->portlck);
		nexterror();
	}
	s = opio->portsc[port-1];
	dprint("ehci %#p port %d reset; sts %#lux\n", ctlr->capio, port, s);
	ilock(ctlr);
	s &= ~(Psenable|Psreset);
	opio->portsc[port-1] = s | Psreset;	/* initiate reset */
	coherence();

	for(i = 0; i < 50; i++){		/* was 10 */
		delay(10);
		if((opio->portsc[port-1] & Psreset) == 0)
			break;
	}
	if (opio->portsc[port-1] & Psreset)
		iprint("ehci %#p: port %d didn't reset after %d ms; sts %#lux\n",
			ctlr->capio, port, i * 10, opio->portsc[port-1]);
	opio->portsc[port-1] &= ~Psreset;  /* force appearance of reset done */
	coherence();

	delay(10);
	if((opio->portsc[port-1] & Psenable) == 0)
		portlend(ctlr, port, "full");

	iunlock(ctlr);
	dprint("ehci %#p after port %d reset; sts %#lux\n",
		ctlr->capio, port, opio->portsc[port-1]);
	qunlock(&ctlr->portlck);
	poperror();
	return 0;
}

static int
portstatus(Hci *hp, int port)
{
	int s, r;
	Eopio *opio;
	Ctlr *ctlr;

	ctlr = hp->aux;
	opio = ctlr->opio;
	qlock(&ctlr->portlck);
	if(waserror()){
		iunlock(ctlr);
		qunlock(&ctlr->portlck);
		nexterror();
	}
	ilock(ctlr);
	s = opio->portsc[port-1];
	if(s & (Psstatuschg | Pschange)){
		opio->portsc[port-1] = s;
		coherence();
		ddprint("ehci %#p port %d status %#x\n", ctlr->capio, port, s);
	}
	/*
	 * If the port is a low speed port we yield ownership now
	 * to the [uo]hci companion controller and pretend it's not here.
	 */
	if((s & Pspresent) != 0 && (s & Pslinemask) == Pslow){
		portlend(ctlr, port, "low");
		s &= ~Pspresent;		/* not for us this time */
	}
	iunlock(ctlr);
	qunlock(&ctlr->portlck);
	poperror();

	/*
	 * We must return status bits as a
	 * get port status hub request would do.
	 */
	r = 0;
	if(s & Pspresent)
		r |= HPpresent|HPhigh;
	if(s & Psenable)
		r |= HPenable;
	if(s & Pssuspend)
		r |= HPsuspend;
	if(s & Psreset)
		r |= HPreset;
	if(s & Psstatuschg)
		r |= HPstatuschg;
	if(s & Pschange)
		r |= HPchange;
	return r;
}

static char*
seprintio(char *s, char *e, Qio *io, char *pref)
{
	s = seprint(s,e,"%s io %#p qh %#p id %#x", pref, io, io->qh, io->usbid);
	s = seprint(s,e," iot %ld", io->iotime);
	s = seprint(s,e," tog %#x tok %#x err %s", io->toggle, io->tok, io->err);
	return s;
}

static char*
seprintep(char *s, char *e, Ep *ep)
{
	Qio *io;
	Ctlio *cio;
	Ctlr *ctlr;

	ctlr = ep->hp->aux;
	ilock(ctlr);
	if(ep->aux == nil){
		*s = 0;
		iunlock(ctlr);
		return s;
	}
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
		*s = 0;
		break;
	}
	iunlock(ctlr);
	return s;
}

/*
 * halt condition was cleared on the endpoint. update our toggles.
 */
static void
clrhalt(Ep *ep)
{
	Qio *io;

	ep->clrhalt = 0;
	coherence();
	switch(ep->ttype){
	case Tintr:
	case Tbulk:
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

static void
xdump(char* pref, void *qh)
{
	int i;
	ulong *u;

	u = qh;
	print("%s %#p:", pref, u);
	for(i = 0; i < 16; i++)
		if((i%4) == 0)
			print("\n %#8.8ulx", u[i]);
		else
			print(" %#8.8ulx", u[i]);
	print("\n");
}

static long
episohscpy(Ctlr *ctlr, Ep *ep, Isoio* iso, uchar *b, long count)
{
	int nr;
	long tot;
	Itd *tdu;

	for(tot = 0; iso->tdi != iso->tdu && tot < count; tot += nr){
		tdu = iso->tdu;
		if(itdactive(tdu))
			break;
		nr = tdu->ndata;
		if(tot + nr > count)
			nr = count - tot;
		if(nr == 0)
			print("ehci: ep%d.%d: too many polls\n",
				ep->dev->nb, ep->nb);
		else{
			iunlock(ctlr);		/* We could page fault here */
			memmove(b+tot, tdu->data, nr);
			ilock(ctlr);
			if(nr < tdu->ndata)
				memmove(tdu->data, tdu->data+nr, tdu->ndata - nr);
			tdu->ndata -= nr;
			coherence();
		}
		if(tdu->ndata == 0){
			itdinit(iso, tdu);
			iso->tdu = tdu->next;
		}
	}
	return tot;
}

static long
episofscpy(Ctlr *ctlr, Ep *ep, Isoio* iso, uchar *b, long count)
{
	int nr;
	long tot;
	Sitd *stdu;

	for(tot = 0; iso->stdi != iso->stdu && tot < count; tot += nr){
		stdu = iso->stdu;
		if(stdu->csw & Stdactive){
			diprint("ehci: episoread: %#p tdu active\n", iso);
			break;
		}
		nr = stdu->ndata;
		if(tot + nr > count)
			nr = count - tot;
		if(nr == 0)
			print("ehci: ep%d.%d: too many polls\n",
				ep->dev->nb, ep->nb);
		else{
			iunlock(ctlr);		/* We could page fault here */
			memmove(b+tot, stdu->data, nr);
			ilock(ctlr);
			if(nr < stdu->ndata)
				memmove(stdu->data, stdu->data+nr,
					stdu->ndata - nr);
			stdu->ndata -= nr;
			coherence();
		}
		if(stdu->ndata == 0){
			sitdinit(iso, stdu);
			iso->stdu = stdu->next;
		}
	}
	return tot;
}

static long
episoread(Ep *ep, Isoio *iso, void *a, long count)
{
	Ctlr *ctlr;
	uchar *b;
	long tot;

	iso->debug = ep->debug;
	diprint("ehci: episoread: %#p ep%d.%d\n", iso, ep->dev->nb, ep->nb);

	b = a;
	ctlr = ep->hp->aux;
	qlock(iso);
	if(waserror()){
		qunlock(iso);
		nexterror();
	}
	iso->err = nil;
	iso->nerrs = 0;
	ilock(ctlr);
	if(iso->state == Qclose){
		iunlock(ctlr);
		error(iso->err ? iso->err : Eio);
	}
	iso->state = Qrun;
	coherence();
	while(isocanread(iso) == 0){
		iunlock(ctlr);
		diprint("ehci: episoread: %#p sleep\n", iso);
		if(waserror()){
			if(iso->err == nil)
				iso->err = "I/O timed out";
			ilock(ctlr);
			break;
		}
		tsleep(iso, isocanread, iso, ep->tmout);
		poperror();
		ilock(ctlr);
	}
	if(iso->state == Qclose){
		iunlock(ctlr);
		error(iso->err ? iso->err : Eio);
	}
	iso->state = Qdone;
	coherence();
	assert(iso->tdu != iso->tdi);

	if(iso->hs != 0)
		tot = episohscpy(ctlr, ep, iso, b, count);
	else
		tot = episofscpy(ctlr, ep, iso, b, count);
	iunlock(ctlr);
	qunlock(iso);
	poperror();
	diprint("uhci: episoread: %#p %uld bytes err '%s'\n", iso, tot, iso->err);
	if(iso->err != nil)
		error(iso->err);
	return tot;
}

/*
 * iso->tdu is the next place to put data. When it gets full
 * it is activated and tdu advanced.
 */
static long
putsamples(Isoio *iso, uchar *b, long count)
{
	long tot, n;

	for(tot = 0; isocanwrite(iso) && tot < count; tot += n){
		n = count-tot;
		if(iso->hs != 0){
			if(n > iso->tdu->mdata - iso->nleft)
				n = iso->tdu->mdata - iso->nleft;
			memmove(iso->tdu->data + iso->nleft, b + tot, n);
			coherence();
			iso->nleft += n;
			if(iso->nleft == iso->tdu->mdata){
				itdinit(iso, iso->tdu);
				iso->nleft = 0;
				iso->tdu = iso->tdu->next;
			}
		}else{
			if(n > iso->stdu->mdata - iso->nleft)
				n = iso->stdu->mdata - iso->nleft;
			memmove(iso->stdu->data + iso->nleft, b + tot, n);
			coherence();
			iso->nleft += n;
			if(iso->nleft == iso->stdu->mdata){
				sitdinit(iso, iso->stdu);
				iso->nleft = 0;
				iso->stdu = iso->stdu->next;
			}
		}
	}
	return tot;
}

/*
 * Queue data for writing and return error status from
 * last writes done, to maintain buffered data.
 */
static long
episowrite(Ep *ep, Isoio *iso, void *a, long count)
{
	Ctlr *ctlr;
	uchar *b;
	int tot, nw;
	char *err;

	iso->debug = ep->debug;
	diprint("ehci: episowrite: %#p ep%d.%d\n", iso, ep->dev->nb, ep->nb);

	ctlr = ep->hp->aux;
	qlock(iso);
	if(waserror()){
		qunlock(iso);
		nexterror();
	}
	ilock(ctlr);
	if(iso->state == Qclose){
		iunlock(ctlr);
		error(iso->err ? iso->err : Eio);
	}
	iso->state = Qrun;
	coherence();
	b = a;
	for(tot = 0; tot < count; tot += nw){
		while(isocanwrite(iso) == 0){
			iunlock(ctlr);
			diprint("ehci: episowrite: %#p sleep\n", iso);
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
		nw = putsamples(iso, b+tot, count-tot);
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
	diprint("ehci: episowrite: %#p %d bytes\n", iso, tot);
	return tot;
}

static int
nexttoggle(int toggle, int count, int maxpkt)
{
	int np;

	np = count / maxpkt;
	if(np == 0)
		np = 1;
	if((np % 2) == 0)
		return toggle;
	if(toggle == Tddata1)
		return Tddata0;
	else
		return Tddata1;
}

static Td*
epgettd(Qio *io, int flags, void *a, int count, int maxpkt)
{
	Td *td;
	ulong pa;
	int i;

	if(count > Tdmaxpkt)
		panic("ehci: epgettd: too many bytes");
	td = tdalloc();
	td->csw = flags | io->toggle | io->tok | count << Tdlenshift |
		Tderr2 | Tderr1;
	coherence();

	/*
	 * use the space wasted by alignment as an
	 * embedded buffer if count bytes fit in there.
	 */
	assert(Align > sizeof(Td));
	if(count <= Align - sizeof(Td)){
		td->data = td->sbuff;
		td->buff = nil;
	}else
		td->data = td->buff = smalloc(Tdmaxpkt);

	pa = PADDR(td->data);
	for(i = 0; i < nelem(td->buffer); i++){
		td->buffer[i] = pa;
		if(i > 0)
			td->buffer[i] &= ~0xFFF;
		pa += 0x1000;
	}
	td->ndata = count;
	if(a != nil && count > 0)
		memmove(td->data, a, count);
	coherence();
	io->toggle = nexttoggle(io->toggle, count, maxpkt);
	coherence();
	return td;
}

/*
 * Try to get them idle
 */
static void
aborttds(Qh *qh)
{
	Td *td;

	qh->state = Qdone;
	coherence();
	if(qh->sched >= 0 && (qh->eps0 & Qhspeedmask) != Qhhigh)
		qh->eps0 |= Qhint;	/* inactivate on next pass */
	coherence();
	for(td = qh->tds; td != nil; td = td->next){
		if(td->csw & Tdactive)
			td->ndata = 0;
		td->csw |= Tdhalt;
		coherence();
	}
}

/*
 * Some controllers do not post the usb/error interrupt after
 * the work has been done. It seems that we must poll for them.
 */
static int
workpending(void *a)
{
	Ctlr *ctlr;

	ctlr = a;
	return ctlr->nreqs > 0;
}

static void
ehcipoll(void* a)
{
	Hci *hp;
	Ctlr *ctlr;
	Poll *poll;
	int i;

	hp = a;
	ctlr = hp->aux;
	poll = &ctlr->poll;
	for(;;){
		if(ctlr->nreqs == 0){
			if(0)ddprint("ehcipoll %#p sleep\n", ctlr->capio);
			sleep(poll, workpending, ctlr);
			if(0)ddprint("ehcipoll %#p awaken\n", ctlr->capio);
		}
		for(i = 0; i < 16 && ctlr->nreqs > 0; i++)
			if(ehciintr(hp) == 0)
				 break;
		do{
			tsleep(&up->sleep, return0, 0, 1);
			ehciintr(hp);
		}while(ctlr->nreqs > 0);
	}
}

static void
pollcheck(Hci *hp)
{
	Ctlr *ctlr;
	Poll *poll;

	ctlr = hp->aux;
	poll = &ctlr->poll;

	if(poll->must != 0 && poll->does == 0){
		lock(poll);
		if(poll->must != 0 && poll->does == 0){
			poll->does++;
			print("ehci %#p: polling\n", ctlr->capio);
			kproc("ehcipoll", ehcipoll, hp);
		}
		unlock(poll);
	}
}

static int
epiodone(void *a)
{
	Qh *qh;

	qh = a;
	return qh->state != Qrun;
}

static void
epiowait(Hci *hp, Qio *io, int tmout, ulong load)
{
	Qh *qh;
	int timedout;
	Ctlr *ctlr;

	ctlr = hp->aux;
	qh = io->qh;
	ddqprint("ehci io %#p sleep on qh %#p state %s\n",
		io, qh, qhsname[qh->state]);
	timedout = 0;
	if(waserror()){
		dqprint("ehci io %#p qh %#p timed out\n", io, qh);
		timedout++;
	}else{
		if(tmout == 0)
			sleep(io, epiodone, qh);
		else
			tsleep(io, epiodone, qh, tmout);
		poperror();
	}

	ilock(ctlr);
	/* Are we missing interrupts? */
	if(qh->state == Qrun){
		iunlock(ctlr);
		ehciintr(hp);
		ilock(ctlr);
		if(qh->state == Qdone){
			dqprint("ehci %#p: polling required\n", ctlr->capio);
			ctlr->poll.must = 1;
			pollcheck(hp);
		}
	}

	if(qh->state == Qrun){
		dqprint("ehci io %#p qh %#p timed out (no intr?)\n", io, qh);
		timedout = 1;
	}else if(qh->state != Qdone && qh->state != Qclose)
		panic("ehci: epio: queue state %d", qh->state);
	if(timedout){
		aborttds(io->qh);
		io->err = "request timed out";
		iunlock(ctlr);
		if(!waserror()){
			tsleep(&up->sleep, return0, 0, Abortdelay);
			poperror();
		}
		ilock(ctlr);
	}
	if(qh->state != Qclose)
		qh->state = Qidle;
	coherence();
	qhlinktd(qh, nil);
	ctlr->load -= load;
	ctlr->nreqs--;
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
	int saved, ntds, tmout;
	long n, tot;
	ulong load;
	char *err;
	char buf[128];
	uchar *c;
	Ctlr *ctlr;
	Qh* qh;
	Td *td, *ltd, *td0, *ntd;

	qh = io->qh;
	ctlr = ep->hp->aux;
	io->debug = ep->debug;
	tmout = ep->tmout;
	ddeprint("epio: %s ep%d.%d io %#p count %ld load %uld\n",
		io->tok == Tdtokin ? "in" : "out",
		ep->dev->nb, ep->nb, io, count, ctlr->load);
	if((ehcidebug > 1 || ep->debug > 1) && io->tok != Tdtokin){
		seprintdata(buf, buf+sizeof(buf), a, count);
		print("echi epio: user data: %s\n", buf);
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
	if(qh->state == Qclose){	/* Tds released by cancelio */
		iunlock(ctlr);
		error(io->err ? io->err : Eio);
	}
	if(qh->state != Qidle)
		panic("epio: qh not idle");
	qh->state = Qinstall;
	iunlock(ctlr);

	c = a;
	td0 = ltd = nil;
	load = tot = 0;
	do{
		n = (Tdmaxpkt / ep->maxpkt) * ep->maxpkt;
		if(count-tot < n)
			n = count-tot;
		if(c != nil && io->tok != Tdtokin)
			td = epgettd(io, Tdactive, c+tot, n, ep->maxpkt);
		else
			td = epgettd(io, Tdactive, nil, n, ep->maxpkt);
		if(td0 == nil)
			td0 = td;
		else
			tdlinktd(ltd, td);
		ltd = td;
		tot += n;
		load += ep->load;
	}while(tot < count);
	if(td0 == nil || ltd == nil)
		panic("epio: no td");

	ltd->csw |= Tdioc;		/* the last one interrupts */
	coherence();

	ddeprint("ehci: load %uld ctlr load %uld\n", load, ctlr->load);
	if(ehcidebug > 1 || ep->debug > 1)
		dumptd(td0, "epio: put: ");

	ilock(ctlr);
	if(qh->state != Qclose){
		io->iotime = TK2MS(MACHP(0)->ticks);
		qh->state = Qrun;
		coherence();
		qhlinktd(qh, td0);
		ctlr->nreqs++;
		ctlr->load += load;
	}
	iunlock(ctlr);

	if(ctlr->poll.does)
		wakeup(&ctlr->poll);

	epiowait(ep->hp, io, tmout, load);
	if(ehcidebug > 1 || ep->debug > 1){
		dumptd(td0, "epio: got: ");
		qhdump(qh);
	}

	tot = 0;
	c = a;
	saved = 0;
	ntds = 0;
	for(td = td0; td != nil; td = ntd){
		ntds++;
		/*
		 * Use td tok, not io tok, because of setup packets.
		 * Also, if the Td was stalled or active (previous Td
		 * was a short packet), we must save the toggle as it is.
		 */
		if(td->csw & (Tdhalt|Tdactive)){
			if(saved++ == 0) {
				io->toggle = td->csw & Tddata1;
				coherence();
			}
		}else{
			tot += td->ndata;
			if(c != nil && (td->csw & Tdtok) == Tdtokin && td->ndata > 0){
				memmove(c, td->data, td->ndata);
				c += td->ndata;
			}
		}
		ntd = td->next;
		tdfree(td);
	}
	err = io->err;
	if(mustlock){
		qunlock(io);
		poperror();
	}
	ddeprint("epio: io %#p: %d tds: return %ld err '%s'\n",
		io, ntds, tot, err);
	if(err == Estalled)
		return 0;	/* that's our convention */
	if(err != nil)
		error(err);
	if(tot < 0)
		error(Eio);
	return tot;
}

static long
epread(Ep *ep, void *a, long count)
{
	Ctlio *cio;
	Qio *io;
	Isoio *iso;
	char buf[160];
	ulong delta;

	ddeprint("ehci: epread\n");
	if(ep->aux == nil)
		panic("epread: not open");

	pollcheck(ep->hp);

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
		if(ehcidebug>1 || ep->debug){
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
		iso = ep->aux;
		return episoread(ep, iso, a, count);
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
		error("short usb comand");
	qlock(cio);
	free(cio->data);
	cio->data = nil;
	cio->ndata = 0;
	if(waserror()){
		free(cio->data);
		cio->data = nil;
		cio->ndata = 0;
		qunlock(cio);
		nexterror();
	}

	/* set the address if unset and out of configuration state */
	if(ep->dev->state != Dconfig && ep->dev->state != Dreset)
		if(cio->usbid == 0){
			cio->usbid = (ep->nb&Epmax) << 7 | ep->dev->nb&Devmax;
			coherence();
			qhsetaddr(cio->qh, cio->usbid);
		}
	/* adjust maxpkt if the user has learned a different one */
	if(qhmaxpkt(cio->qh) != ep->maxpkt)
		qhsetmaxpkt(cio->qh, ep->maxpkt);
	c = a;
	cio->tok = Tdtoksetup;
	cio->toggle = Tddata0;
	coherence();
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
			error("d2h data too large to fit in ehci");
		a = cio->data = smalloc(len+1);
	}else{
		cio->tok = Tdtokout;
		len = count;
	}
	coherence();
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
	coherence();
	epio(ep, cio, nil, 0, 0);
	qunlock(cio);
	poperror();
	ddeprint("epctlio cio %#p return %ld\n", cio, count);
	return count;
}

static long
epwrite(Ep *ep, void *a, long count)
{
	Qio *io;
	Ctlio *cio;
	Isoio *iso;
	ulong delta;

	pollcheck(ep->hp);

	ddeprint("ehci: epwrite ep%d.%d\n", ep->dev->nb, ep->nb);
	if(ep->aux == nil)
		panic("ehci: epwrite: not open");
	switch(ep->ttype){
	case Tctl:
		cio = ep->aux;
		return epctlio(ep, cio, a, count);
	case Tbulk:
		io = ep->aux;
		if(ep->clrhalt)
			clrhalt(ep);
		return epio(ep, &io[OWRITE], a, count, 1);
	case Tintr:
		io = ep->aux;
		delta = TK2MS(MACHP(0)->ticks) - io[OWRITE].iotime + 1;
		if(delta < ep->pollival)
			tsleep(&up->sleep, return0, 0, ep->pollival - delta);
		if(ep->clrhalt)
			clrhalt(ep);
		return epio(ep, &io[OWRITE], a, count, 1);
	case Tiso:
		iso = ep->aux;
		return episowrite(ep, iso, a, count);
	}
	return -1;
}

static void
isofsinit(Ep *ep, Isoio *iso)
{
	long left;
	Sitd *td, *ltd;
	int i;
	ulong frno;

	left = 0;
	ltd = nil;
	frno = iso->td0frno;
	for(i = 0; i < iso->nframes; i++){
		td = sitdalloc();
		td->data = iso->data + i * ep->maxpkt;
		td->epc = ep->dev->port << Stdportshift;
		td->epc |= ep->dev->hub << Stdhubshift;
		td->epc |= ep->nb << Stdepshift;
		td->epc |= ep->dev->nb << Stddevshift;
		td->mfs = 034 << Stdscmshift | 1 << Stdssmshift;
		if(ep->mode == OREAD){
			td->epc |= Stdin;
			td->mdata = ep->maxpkt;
		}else{
			td->mdata = (ep->hz+left) * ep->pollival / 1000;
			td->mdata *= ep->samplesz;
			left = (ep->hz+left) * ep->pollival % 1000;
			if(td->mdata > ep->maxpkt){
				print("ehci: ep%d.%d: size > maxpkt\n",
					ep->dev->nb, ep->nb);
				print("size = %ld max = %ld\n",
					td->mdata,ep->maxpkt);
				td->mdata = ep->maxpkt;
			}
		}
		coherence();

		iso->sitdps[frno] = td;
		coherence();
		sitdinit(iso, td);
		if(ltd != nil)
			ltd->next = td;
		ltd = td;
		frno = TRUNC(frno+ep->pollival, Nisoframes);
	}
	ltd->next = iso->sitdps[iso->td0frno];
	coherence();
}

static void
isohsinit(Ep *ep, Isoio *iso)
{
	int ival, p;
	long left;
	ulong frno, i, pa;
	Itd *ltd, *td;

	iso->hs = 1;
	ival = 1;
	if(ep->pollival > 8)
		ival = ep->pollival/8;
	left = 0;
	ltd = nil;
	frno = iso->td0frno;
	for(i = 0; i < iso->nframes; i++){
		td = itdalloc();
		td->data = iso->data + i * 8 * iso->maxsize;
		pa = PADDR(td->data) & ~0xFFF;
		for(p = 0; p < 8; p++)
			td->buffer[i] = pa + p * 0x1000;
		td->buffer[0] = PADDR(iso->data) & ~0xFFF |
			ep->nb << Itdepshift | ep->dev->nb << Itddevshift;
		if(ep->mode == OREAD)
			td->buffer[1] |= Itdin;
		else
			td->buffer[1] |= Itdout;
		td->buffer[1] |= ep->maxpkt << Itdmaxpktshift;
		td->buffer[2] |= ep->ntds << Itdntdsshift;

		if(ep->mode == OREAD)
			td->mdata = 8 * iso->maxsize;
		else{
			td->mdata = (ep->hz + left) * ep->pollival / 1000;
			td->mdata *= ep->samplesz;
			left = (ep->hz + left) * ep->pollival % 1000;
		}
		coherence();
		iso->itdps[frno] = td;
		coherence();
		itdinit(iso, td);
		if(ltd != nil)
			ltd->next = td;
		ltd = td;
		frno = TRUNC(frno + ival, Nisoframes);
	}
}

static void
isoopen(Ctlr *ctlr, Ep *ep)
{
	int ival;		/* pollival in ms */
	int tpf;		/* tds per frame */
	int i, n, w, woff;
	ulong frno;
	Isoio *iso;

	iso = ep->aux;
	switch(ep->mode){
	case OREAD:
		iso->tok = Tdtokin;
		break;
	case OWRITE:
		iso->tok = Tdtokout;
		break;
	default:
		error("iso i/o is half-duplex");
	}
	iso->usbid = ep->nb << 7 | ep->dev->nb & Devmax;
	iso->state = Qidle;
	coherence();
	iso->debug = ep->debug;
	ival = ep->pollival;
	tpf = 1;
	if(ep->dev->speed == Highspeed){
		tpf = 8;
		if(ival <= 8)
			ival = 1;
		else
			ival /= 8;
	}
	assert(ival != 0);
	iso->nframes = Nisoframes / ival;
	if(iso->nframes < 3)
		error("uhci isoopen bug");	/* we need at least 3 tds */
	iso->maxsize = ep->ntds * ep->maxpkt;
	if(ctlr->load + ep->load > 800)
		print("usb: ehci: bandwidth may be exceeded\n");
	ilock(ctlr);
	ctlr->load += ep->load;
	ctlr->isoload += ep->load;
	ctlr->nreqs++;
	dprint("ehci: load %uld isoload %uld\n", ctlr->load, ctlr->isoload);
	diprint("iso nframes %d pollival %uld ival %d maxpkt %uld ntds %d\n",
		iso->nframes, ep->pollival, ival, ep->maxpkt, ep->ntds);
	iunlock(ctlr);
	if(ctlr->poll.does)
		wakeup(&ctlr->poll);

	/*
	 * From here on this cannot raise errors
	 * unless we catch them and release here all memory allocated.
	 */
	assert(ep->maxpkt > 0 && ep->ntds > 0 && ep->ntds < 4);
	assert(ep->maxpkt <= 1024);
	iso->tdps = smalloc(sizeof(uintptr) * Nisoframes);
	iso->data = smalloc(iso->nframes * tpf * ep->ntds * ep->maxpkt);
	iso->td0frno = TRUNC(ctlr->opio->frno + 10, Nisoframes);
	/* read: now; write: 1s ahead */

	if(ep->dev->speed == Highspeed)
		isohsinit(ep, iso);
	else
		isofsinit(ep, iso);
	iso->tdu = iso->tdi = iso->itdps[iso->td0frno];
	iso->stdu = iso->stdi = iso->sitdps[iso->td0frno];
	coherence();

	ilock(ctlr);
	frno = iso->td0frno;
	for(i = 0; i < iso->nframes; i++){
		*iso->tdps[frno] = ctlr->frames[frno];
		frno = TRUNC(frno+ival, Nisoframes);
	}

	/*
	 * Iso uses a virtual frame window of Nisoframes, and we must
	 * fill the actual ctlr frame array by placing ctlr->nframes/Nisoframes
	 * copies of the window in the frame array.
	 */
	assert(ctlr->nframes >= Nisoframes && Nisoframes >= iso->nframes);
	assert(Nisoframes >= Nintrleafs);
	n = ctlr->nframes / Nisoframes;
	for(w = 0; w < n; w++){
		frno = iso->td0frno;
		woff = w * Nisoframes;
		for(i = 0; i < iso->nframes ; i++){
			assert(woff+frno < ctlr->nframes);
			assert(iso->tdps[frno] != nil);
			if(ep->dev->speed == Highspeed)
				ctlr->frames[woff+frno] = PADDR(iso->tdps[frno])
					|Litd;
			else
				ctlr->frames[woff+frno] = PADDR(iso->tdps[frno])
					|Lsitd;
			coherence();
			frno = TRUNC(frno+ep->pollival, Nisoframes);
		}
	}
	coherence();
	iso->next = ctlr->iso;
	ctlr->iso = iso;
	coherence();
	iso->state = Qdone;
	iunlock(ctlr);
	if(ehcidebug > 1 || iso->debug >1)
		isodump(iso, 0);
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
	Ctlio *cio;
	Qio *io;
	int usbid;

	ctlr = ep->hp->aux;
	deprint("ehci: epopen ep%d.%d\n", ep->dev->nb, ep->nb);
	if(ep->aux != nil)
		panic("ehci: epopen called with open ep");
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
		if(ep->dev->isroot != 0 && ep->nb == 0)	/* root hub */
			break;
		cio->qh = qhalloc(ctlr, ep, cio, "epc");
		break;
	case Tbulk:
		ep->pollival = 1;	/* assume this; doesn't really matter */
		/* and fall... */
	case Tintr:
		io = ep->aux = smalloc(sizeof(Qio)*2);
		io[OREAD].debug = io[OWRITE].debug = ep->debug;
		usbid = (ep->nb&Epmax) << 7 | ep->dev->nb &Devmax;
		assert(ep->pollival != 0);
		if(ep->mode != OREAD){
			if(ep->toggle[OWRITE] != 0)
				io[OWRITE].toggle = Tddata1;
			else
				io[OWRITE].toggle = Tddata0;
			io[OWRITE].tok = Tdtokout;
			io[OWRITE].usbid = usbid;
			io[OWRITE].bw = ep->maxpkt*1000/ep->pollival; /* bytes/s */
			io[OWRITE].qh = qhalloc(ctlr, ep, io+OWRITE, "epw");
		}
		if(ep->mode != OWRITE){
			if(ep->toggle[OREAD] != 0)
				io[OREAD].toggle = Tddata1;
			else
				io[OREAD].toggle = Tddata0;
			io[OREAD].tok = Tdtokin;
			io[OREAD].usbid = usbid;
			io[OREAD].bw = ep->maxpkt*1000/ep->pollival; /* bytes/s */
			io[OREAD].qh = qhalloc(ctlr, ep, io+OREAD, "epr");
		}
		break;
	}
	coherence();
	if(ehcidebug>1 || ep->debug)
		dump(ep->hp);
	deprint("ehci: epopen done\n");
	poperror();
}

static void
cancelio(Ctlr *ctlr, Qio *io)
{
	Qh *qh;

	ilock(ctlr);
	qh = io->qh;
	if(io == nil || io->qh == nil || io->qh->state == Qclose){
		iunlock(ctlr);
		return;
	}
	dqprint("ehci: cancelio for qh %#p state %s\n",
		qh, qhsname[qh->state]);
	aborttds(qh);
	qh->state = Qclose;
	iunlock(ctlr);
	if(!waserror()){
		tsleep(&up->sleep, return0, 0, Abortdelay);
		poperror();
	}
	wakeup(io);
	qlock(io);
	/* wait for epio if running */
	qunlock(io);

	qhfree(ctlr, qh);
	io->qh = nil;
}

static void
cancelisoio(Ctlr *ctlr, Isoio *iso, int pollival, ulong load)
{
	int frno, i, n, t, w, woff;
	ulong *lp, *tp;
	Isoio **il;
	Itd *td;
	Sitd *std;

	ilock(ctlr);
	if(iso->state == Qclose){
		iunlock(ctlr);
		return;
	}
	ctlr->nreqs--;
	if(iso->state != Qrun && iso->state != Qdone)
		panic("bad iso state");
	iso->state = Qclose;
	coherence();
	if(ctlr->isoload < load)
		panic("ehci: low isoload");
	ctlr->isoload -= load;
	ctlr->load -= load;
	for(il = &ctlr->iso; *il != nil; il = &(*il)->next)
		if(*il == iso)
			break;
	if(*il == nil)
		panic("cancleiso: not found");
	*il = iso->next;

	frno = iso->td0frno;
	for(i = 0; i < iso->nframes; i++){
		tp = iso->tdps[frno];
		if(iso->hs != 0){
			td = iso->itdps[frno];
			for(t = 0; t < nelem(td->csw); t++)
				td->csw[t] &= ~(Itdioc|Itdactive);
		}else{
			std = iso->sitdps[frno];
			std->csw &= ~(Stdioc|Stdactive);
		}
		coherence();
		for(lp = &ctlr->frames[frno]; !(*lp & Lterm);
		    lp = &LPTR(*lp)[0])
			if(LPTR(*lp) == tp)
				break;
		if(*lp & Lterm)
			panic("cancelisoio: td not found");
		*lp = tp[0];
		/*
		 * Iso uses a virtual frame window of Nisoframes, and we must
		 * restore pointers in copies of the window kept at ctlr->frames.
		 */
		if(lp == &ctlr->frames[frno]){
			n = ctlr->nframes / Nisoframes;
			for(w = 1; w < n; w++){
				woff = w * Nisoframes;
				ctlr->frames[woff+frno] = *lp;
			}
		}
		coherence();
		frno = TRUNC(frno+pollival, Nisoframes);
	}
	iunlock(ctlr);

	/*
	 * wakeup anyone waiting for I/O and
	 * wait to be sure no I/O is in progress in the controller.
	 * and then wait to be sure episo* is no longer running.
	 */
	wakeup(iso);
	diprint("cancelisoio iso %#p waiting for I/O to cease\n", iso);
	tsleep(&up->sleep, return0, 0, 5);
	qlock(iso);
	qunlock(iso);
	diprint("cancelisoio iso %#p releasing iso\n", iso);

	frno = iso->td0frno;
	for(i = 0; i < iso->nframes; i++){
		if(iso->hs != 0)
			itdfree(iso->itdps[frno]);
		else
			sitdfree(iso->sitdps[frno]);
		iso->tdps[frno] = nil;
		frno = TRUNC(frno+pollival, Nisoframes);
	}
	free(iso->tdps);
	iso->tdps = nil;
	free(iso->data);
	iso->data = nil;
	coherence();
}

static void
epclose(Ep *ep)
{
	Qio *io;
	Ctlio *cio;
	Isoio *iso;
	Ctlr *ctlr;

	ctlr = ep->hp->aux;
	deprint("ehci: epclose ep%d.%d\n", ep->dev->nb, ep->nb);

	if(ep->aux == nil)
		panic("ehci: epclose called with closed ep");
	switch(ep->ttype){
	case Tctl:
		cio = ep->aux;
		cancelio(ctlr, cio);
		free(cio->data);
		cio->data = nil;
		break;
	case Tintr:
	case Tbulk:
		io = ep->aux;
		ep->toggle[OREAD] = ep->toggle[OWRITE] = 0;
		if(ep->mode != OWRITE){
			cancelio(ctlr, &io[OREAD]);
			if(io[OREAD].toggle == Tddata1)
				ep->toggle[OREAD] = 1;
		}
		if(ep->mode != OREAD){
			cancelio(ctlr, &io[OWRITE]);
			if(io[OWRITE].toggle == Tddata1)
				ep->toggle[OWRITE] = 1;
		}
		coherence();
		break;
	case Tiso:
		iso = ep->aux;
		cancelisoio(ctlr, iso, ep->pollival, ep->load);
		break;
	default:
		panic("epclose: bad ttype");
	}
	free(ep->aux);
	ep->aux = nil;
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
 * build the periodic scheduling tree:
 * framesize must be a multiple of the tree size
 */
static void
mkqhtree(Ctlr *ctlr)
{
	int i, n, d, o, leaf0, depth;
	ulong leafs[Nintrleafs];
	Qh *qh;
	Qh **tree;
	Qtree *qt;

	depth = flog2(Nintrleafs);
	n = (1 << (depth+1)) - 1;
	qt = mallocz(sizeof(*qt), 1);
	if(qt == nil)
		panic("ehci: mkqhtree: no memory");
	qt->nel = n;
	qt->depth = depth;
	qt->bw = mallocz(n * sizeof(qt->bw), 1);
	qt->root = tree = mallocz(n * sizeof(Qh *), 1);
	if(qt->bw == nil || tree == nil)
		panic("ehci: mkqhtree: no memory");
	for(i = 0; i < n; i++){
		tree[i] = qh = edalloc();
		if(qh == nil)
			panic("ehci: mkqhtree: no memory");
		qh->nlink = qh->alink = qh->link = Lterm;
		qh->csw = Tdhalt;
		qh->state = Qidle;
		coherence();
		if(i > 0)
			qhlinkqh(tree[i], tree[(i-1)/2]);
	}
	ctlr->ntree = i;
	dprint("ehci: tree: %d endpoints allocated\n", i);

	/* distribute leaves evenly round the frame list */
	leaf0 = n / 2;
	for(i = 0; i < Nintrleafs; i++){
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
		leafs[i] = PADDR(tree[leaf0 + o]) | Lqh;
	}
	assert((ctlr->nframes % Nintrleafs) == 0);
	for(i = 0; i < ctlr->nframes; i += Nintrleafs){
		memmove(ctlr->frames + i, leafs, sizeof leafs);
		coherence();
	}
	ctlr->tree = qt;
	coherence();
}

void
ehcimeminit(Ctlr *ctlr)
{
	int i, frsize;
	Eopio *opio;

	opio = ctlr->opio;
	frsize = ctlr->nframes * sizeof(ulong);
	assert((frsize & 0xFFF) == 0);		/* must be 4k aligned */
	ctlr->frames = xspanalloc(frsize, frsize, 0);
	if(ctlr->frames == nil)
		panic("ehci reset: no memory");

	for (i = 0; i < ctlr->nframes; i++)
		ctlr->frames[i] = Lterm;
	opio->frbase = PADDR(ctlr->frames);
	opio->frno = 0;
	coherence();

	qhalloc(ctlr, nil, nil, nil);	/* init async list */
	mkqhtree(ctlr);			/* init sync list */
	edfree(edalloc());		/* try to get some ones pre-allocated */

	dprint("ehci %#p flb %#lux frno %#lux\n",
		ctlr->capio, opio->frbase, opio->frno);
}

static void
init(Hci *hp)
{
	Ctlr *ctlr;
	Eopio *opio;
	int i;

	hp->highspeed = 1;
	ctlr = hp->aux;
	opio = ctlr->opio;
	dprint("ehci %#p init\n", ctlr->capio);

	ilock(ctlr);
	/*
	 * Unless we activate frroll interrupt
	 * some machines won't post other interrupts.
	 */
	opio->intr = Iusb|Ierr|Iportchg|Ihcerr|Iasync;
	coherence();
	opio->cmd |= Cpse;
	coherence();
	opio->cmd |= Case;
	coherence();
	ehcirun(ctlr, 1);
	opio->config = Callmine;	/* reclaim all ports */
	coherence();

	for (i = 0; i < hp->nports; i++)
		opio->portsc[i] = Pspower;
	iunlock(ctlr);
	if(ehcidebug > 1)
		dump(hp);
}

void
ehcilinkage(Hci *hp)
{
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
//	hp->shutdown = shutdown;
//	hp->debug = setdebug;
	hp->type = "ehci";
}
