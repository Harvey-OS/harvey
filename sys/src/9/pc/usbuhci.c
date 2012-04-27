/*
 * USB Universal Host Controller Interface (sic) driver.
 *
 * BUGS:
 * - Too many delays and ilocks.
 * - bandwidth admission control must be done per-frame.
 * - interrupt endpoints should go on a tree like [oe]hci.
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
typedef struct Isoio Isoio;
typedef struct Qh Qh;
typedef struct Qhpool Qhpool;
typedef struct Qio Qio;
typedef struct Td Td;
typedef struct Tdpool Tdpool;

enum
{
	Resetdelay	= 100,		/* delay after a controller reset (ms) */
	Enabledelay	= 100,		/* waiting for a port to enable */
	Abortdelay	= 5,		/* delay after cancelling Tds (ms) */
	Incr		= 64,		/* for Td and Qh pools */

	Tdatomic	= 8,		/* max nb. of Tds per bulk I/O op. */

	/* Queue states (software) */
	Qidle		= 0,
	Qinstall,
	Qrun,
	Qdone,
	Qclose,
	Qfree,

	/*
	 * HW constants
	 */

	Nframes		= 1024,		/* 2ⁿ for xspanalloc; max 1024 */
	Align		= 16,		/* for data structures */

	/* Size of small buffer kept within Tds. (software) */
	/* Keep as a multiple of Align to maintain alignment of Tds in pool */
	Tdndata		= 1*Align,

	/* i/o space
	 * Some ports are short, some are long, some are byte.
	 * We use ins[bsl] and not vmap.
	 */
	Cmd		= 0,
		Crun		= 0x01,
		Chcreset	= 0x02,	/* host controller reset */
		Cgreset		= 0x04,	/* global reset */
		Cegsm		= 0x08,	/* enter global suspend */
		Cfgr		= 0x10,	/* forge global resume */
		Cdbg		= 0x20,	/* single step, debug */
		Cmaxp		= 0x80,	/* max packet */

	Status		= 2,
		Susbintr		= 0x01,	/* interrupt */
		Seintr		= 0x02, /* error interrupt */
		Sresume		= 0x04, /* resume detect */
		Shserr		= 0x08, /* host system error */
		Shcerr		= 0x10, /* host controller error */
		Shalted		= 0x20, /* controller halted */
		Sall		= 0x3F,

	Usbintr 		= 4,
		Itmout		= 0x01, /* timeout or crc */
		Iresume		= 0x02, /* resume interrupt enable */
		Ioc		= 0x04, /* interrupt on complete */
		Ishort		= 0x08, /* short packet interrupt */
		Iall		= 0x0F,
	Frnum		= 6,
	Flbaseadd 	= 8,
	SOFmod		= 0xC,		/* start of frame modifier register */

	Portsc0		= 0x10,
		PSpresent	= 0x0001,	/* device present */
		PSstatuschg	= 0x0002,	/* PSpresent changed */
		PSenable	= 0x0004,	/* device enabled */
		PSchange	= 0x0008,	/* PSenable changed */
		PSresume	= 0x0040,	/* resume detected */
		PSreserved1	= 0x0080,	/* always read as 1; reserved */
		PSslow		= 0x0100,	/* device has low speed */
		PSreset		= 0x0200,	/* port reset */
		PSsuspend	= 0x1000,	/* port suspended */

	/* Transfer descriptor link */
	Tdterm		= 0x1,		/* nil (terminate) */
	Tdlinkqh	= 0x2,			/* link refers to a QH */
	Tdvf		= 0x4,		/* run linked Tds first (depth-first)*/

	/* Transfer status bits */
	Tdbitstuff	= 0x00020000,	/* bit stuffing error */
	Tdcrcto		= 0x00040000,	/* crc or timeout error */
	Tdnak		= 0x00080000,	/* nak packet received */
	Tdbabble	= 0x00100000,	/* babble detected */
	Tddberr		= 0x00200000,	/* data buf. error */
	Tdstalled	= 0x00400000,	/* serious error to ep. */
	Tdactive		= 0x00800000,	/* enabled/in use by hw */
	/* Transfer control bits */
	Tdioc		= 0x01000000,	/* interrupt on complete */
	Tdiso		= 0x02000000,	/* isochronous select */
	Tdlow		= 0x04000000,	/* low speed device */
	Tderr1		= 0x08000000,	/* bit 0 of error counter */
	Tderr2		= 0x10000000,	/* bit 1 of error counter */
	Tdspd		= 0x20000000,	/* short packet detect */

	Tdlen		= 0x000003FF,	/* actual length field */

	Tdfatalerr	= Tdnak|Tdbabble|Tdstalled, /* hw retries others */
	Tderrors	= Tdfatalerr|Tdbitstuff|Tdcrcto|Tddberr,

	/* Transfer descriptor token bits */
	Tddata0		= 0,
	Tddata1		= 0x80000,	/* data toggle (1==DATA1) */
	Tdtokin		= 0x69,
	Tdtokout	= 0xE1,
	Tdtoksetup	= 0x2D,

	Tdmaxpkt	= 0x800,	/* max packet size */

	/* Queue head bits */
	QHterm		= 1<<0,		/* nil (terminate) */
	QHlinkqh		= 1<<1,		/* link refers to a QH */
	QHvf		= 1<<2,		/* vertical first (depth first) */
};

struct Ctlr
{
	Lock;			/* for ilock. qh lists and basic ctlr I/O */
	QLock	portlck;	/* for port resets/enable... */
	Pcidev*	pcidev;
	int	active;
	int	port;		/* I/O address */
	Qh*	qhs;		/* list of Qhs for this controller */
	Qh*	qh[Tmax];	/* Dummy Qhs to insert Qhs after */
	Isoio*	iso;		/* list of active iso I/O */
	ulong*	frames;		/* frame list (used by hw) */
	ulong	load;		/* max load for a single frame */
	ulong	isoload;		/* max iso load for a single frame */
	int	nintr;		/* number of interrupts attended */
	int	ntdintr;		/* number of intrs. with something to do */
	int	nqhintr;		/* number of intrs. for Qhs */
	int	nisointr;	/* number of intrs. for iso transfers */
};

struct Qio
{
	QLock;			/* for the entire I/O process */
	Rendez;			/* wait for completion */
	Qh*	qh;		/* Td list (field const after init) */
	int	usbid;		/* usb address for endpoint/device */
	int	toggle;		/* Tddata0/Tddata1 */
	int	tok;		/* Tdtoksetup, Tdtokin, Tdtokout */
	ulong	iotime;		/* time of last I/O */
	int	debug;		/* debug flag from the endpoint */
	char*	err;		/* error string */
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
	int	nframes;	/* Nframes/ep->pollival */
	uchar*	data;		/* iso data buffers if not embedded */
	int	td0frno;	/* frame number for first Td */
	Td*	tdu;		/* next td for user I/O in tdps */
	Td*	tdi;		/* next td processed by interrupt */
	char*	err;		/* error string */
	int	nerrs;		/* nb of consecutive I/O errors */
	long	nleft;		/* number of bytes left from last write */
	int	debug;		/* debug flag from the endpoint */
	Isoio*	next;		/* in list of active Isoios */
	Td*	tdps[Nframes];	/* pointer to Td used for i-th frame or nil */
};

struct Tdpool
{
	Lock;
	Td*	free;
	int	nalloc;
	int	ninuse;
	int	nfree;
};

struct Qhpool
{
	Lock;
	Qh*	free;
	int	nalloc;
	int	ninuse;
	int	nfree;
};

/*
 * HW data structures
 */

/*
 * Queue header (known by hw).
 * 16-byte aligned. first two words used by hw.
 * They are taken from the pool upon endpoint opening and
 * queued after the dummy queue header for the endpoint type
 * in the controller. Actual I/O happens as Tds are linked into it.
 * The driver does I/O in lock-step.
 * The user builds a list of Tds and links it into the Qh,
 * then the Qh goes from Qidle to Qrun and nobody touches it until
 * it becomes Qdone at interrupt time.
 * At that point the user collects the Tds and it goes Qidle.
 * A premature cancel may set the state to Qclose and abort I/O.
 * The Ctlr lock protects change of state for Qhs in use.
 */
struct Qh
{
	ulong	link;		/* link to next horiz. item (eg. Qh) */
	ulong	elink;		/* link to element (eg. Td; updated by hw) */

	ulong	state;		/* Qidle -> Qinstall -> Qrun -> Qdone | Qclose */
	Qio*	io;		/* for this queue */

	Qh*	next;		/* in active or free list */
	Td*	tds;		/* Td list in this Qh (initially, elink) */
	char*	tag;		/* debug and align, mostly */
	ulong	align;
};

/*
 * Transfer descriptor.
 * 16-byte aligned. first two words used by hw. Next 4 by sw.
 * We keep an embedded buffer for small I/O transfers.
 * They are taken from the pool when buffers are needed for I/O
 * and linked at the Qh/Isoio for the endpoint and direction requiring it.
 * The block keeps actual data. They are protected from races by
 * the queue or the pool keeping it. The owner of the link to the Td
 * is free to use it and can be the only one using it.
 */
struct Td
{
	ulong	link;		/* Link to next Td or Qh */
	ulong	csw;		/* control and status word (updated by hw) */
	ulong	token;		/* endpt, device, pid */
	ulong	buffer;		/* buffer pointer */

	Td*	next;		/* in qh or Isoio or free list */
	ulong	ndata;		/* bytes available/used at data */
	uchar*	data;		/* pointer to actual data */
	void*	buff;		/* allocated data, for large transfers */

	uchar	sbuff[Tdndata];	/* embedded buffer, for small transfers */
};

#define INB(x)		inb(ctlr->port+(x))
#define	INS(x)		ins(ctlr->port+(x))
#define INL(x)		inl(ctlr->port+(x))
#define OUTB(x, v)	outb(ctlr->port+(x), (v))
#define	OUTS(x, v)	outs(ctlr->port+(x), (v))
#define OUTL(x, v)	outl(ctlr->port+(x), (v))
#define TRUNC(x, sz)	((x) & ((sz)-1))
#define PTR(q)		((void*)KADDR((ulong)(q) & ~ (0xF|PCIWINDOW)))
#define QPTR(q)		((Qh*)PTR(q))
#define TPTR(q)		((Td*)PTR(q))
#define PORT(p)		(Portsc0 + 2*(p))
#define diprint		if(debug || iso->debug)print
#define ddiprint		if(debug>1 || iso->debug>1)print
#define dqprint		if(debug || (qh->io && qh->io->debug))print
#define ddqprint		if(debug>1 || (qh->io && qh->io->debug>1))print

static Ctlr* ctlrs[Nhcis];

static Tdpool tdpool;
static Qhpool qhpool;
static int debug;

static char* qhsname[] = { "idle", "install", "run", "done", "close", "FREE" };

static void
uhcicmd(Ctlr *ctlr, int c)
{
	OUTS(Cmd, c);
}

static void
uhcirun(Ctlr *ctlr, int on)
{
	int i;

	ddprint("uhci %#ux setting run to %d\n", ctlr->port, on);

	if(on)
		uhcicmd(ctlr, INS(Cmd)|Crun);
	else
		uhcicmd(ctlr, INS(Cmd) & ~Crun);
	for(i = 0; i < 100; i++)
		if(on == 0 && (INS(Status) & Shalted) != 0)
			break;
		else if(on != 0 && (INS(Status) & Shalted) == 0)
			break;
		else
			delay(1);
	if(i == 100)
		dprint("uhci %#x run cmd timed out\n", ctlr->port);
	ddprint("uhci %#ux cmd %#ux sts %#ux\n",
		ctlr->port, INS(Cmd), INS(Status));
}

static int
tdlen(Td *td)
{
	return (td->csw+1) & Tdlen;
}

static int
maxtdlen(Td *td)
{
	return ((td->token>>21)+1) & (Tdmaxpkt-1);
}

static int
tdtok(Td *td)
{
	return td->token & 0xFF;
}

static char*
seprinttd(char *s, char *se, Td *td)
{
	s = seprint(s, se, "%#p link %#ulx", td, td->link);
	if((td->link & Tdvf) != 0)
		s = seprint(s, se, "V");
	if((td->link & Tdterm) != 0)
		s = seprint(s, se, "T");
	if((td->link & Tdlinkqh) != 0)
		s = seprint(s, se, "Q");
	s = seprint(s, se, " csw %#ulx ", td->csw);
	if(td->csw & Tdactive)
		s = seprint(s, se, "a");
	if(td->csw & Tdiso)
		s = seprint(s, se, "I");
	if(td->csw & Tdioc)
		s = seprint(s, se, "i");
	if(td->csw & Tdlow)
		s = seprint(s, se, "l");
	if((td->csw & (Tderr1|Tderr2)) == 0)
		s = seprint(s, se, "z");
	if(td->csw & Tderrors)
		s = seprint(s, se, " err %#ulx", td->csw & Tderrors);
	if(td->csw & Tdstalled)
		s = seprint(s, se, "s");
	if(td->csw & Tddberr)
		s = seprint(s, se, "d");
	if(td->csw & Tdbabble)
		s = seprint(s, se, "b");
	if(td->csw & Tdnak)
		s = seprint(s, se, "n");
	if(td->csw & Tdcrcto)
		s = seprint(s, se, "c");
	if(td->csw & Tdbitstuff)
		s = seprint(s, se, "B");
	s = seprint(s, se, " stslen %d", tdlen(td));

	s = seprint(s, se, " token %#ulx", td->token);
	if(td->token == 0)		/* the BWS loopback Td, ignore rest */
		return s;
	s = seprint(s, se, " maxlen %d", maxtdlen(td));
	if(td->token & Tddata1)
		s = seprint(s, se, " d1");
	else
		s = seprint(s, se, " d0");
	s = seprint(s, se, " id %#ulx:", (td->token>>15) & Epmax);
	s = seprint(s, se, "%#ulx", (td->token>>8) & Devmax);
	switch(tdtok(td)){
	case Tdtokin:
		s = seprint(s, se, " in");
		break;
	case Tdtokout:
		s = seprint(s, se, " out");
		break;
	case Tdtoksetup:
		s = seprint(s, se, " setup");
		break;
	default:
		s = seprint(s, se, " BADPID");
	}
	s = seprint(s, se, "\n\t  buffer %#ulx data %#p", td->buffer, td->data);
	s = seprint(s, se, " ndata %uld sbuff %#p buff %#p",
		td->ndata, td->sbuff, td->buff);
	if(td->ndata > 0)
		s = seprintdata(s, se, td->data, td->ndata);
	return s;
}

static void
isodump(Isoio *iso, int all)
{
	char buf[256];
	Td *td;
	int i;

	print("iso %#p %s state %d nframes %d"
		" td0 %#p tdu %#p tdi %#p data %#p\n",
		iso, iso->tok == Tdtokin ? "in" : "out",
		iso->state, iso->nframes, iso->tdps[iso->td0frno],
		iso->tdu, iso->tdi, iso->data);
	if(iso->err != nil)
		print("\terr='%s'\n", iso->err);
	if(all == 0){
		seprinttd(buf, buf+sizeof(buf), iso->tdu);
		print("\ttdu %s\n", buf);
		seprinttd(buf, buf+sizeof(buf), iso->tdi);
		print("\ttdi %s\n", buf);
	}else{
		td = iso->tdps[iso->td0frno];
		for(i = 0; i < iso->nframes; i++){
			seprinttd(buf, buf+sizeof(buf), td);
			if(td == iso->tdi)
				print("i->");
			if(td == iso->tdu)
				print("u->");
			print("\t%s\n", buf);
			td = td->next;
		}
	}
}

static int
sameptr(void *p, ulong l)
{
	if(l & QHterm)
		return p == nil;
	return PTR(l) == p;
}

static void
dumptd(Td *td, char *pref)
{
	char buf[256];
	char *s;
	char *se;
	int i;

	i = 0;
	se = buf+sizeof(buf);
	for(; td != nil; td = td->next){
		s = seprinttd(buf, se, td);
		if(!sameptr(td->next, td->link))
			seprint(s, se, " next %#p != link %#ulx %#p",
				td->next, td->link, TPTR(td->link));
		print("%std %s\n", pref, buf);
		if(i++ > 20){
			print("...more tds...\n");
			break;
		}
	}
}

static void
qhdump(Qh *qh, char *pref)
{
	char buf[256];
	char *s;
	char *se;
	ulong td;
	int i;

	s = buf;
	se = buf+sizeof(buf);
	s = seprint(s, se, "%sqh %s %#p state %s link %#ulx", pref,
		qh->tag, qh, qhsname[qh->state], qh->link);
	if(!sameptr(qh->tds, qh->elink))
		s = seprint(s, se, " [tds %#p != elink %#ulx %#p]",
			qh->tds, qh->elink, TPTR(qh->elink));
	if(!sameptr(qh->next, qh->link))
		s = seprint(s, se, " [next %#p != link %#ulx %#p]",
			qh->next, qh->link, QPTR(qh->link));
	if((qh->link & Tdterm) != 0)
		s = seprint(s, se, "T");
	if((qh->link & Tdlinkqh) != 0)
		s = seprint(s, se, "Q");
	s = seprint(s, se, " elink %#ulx", qh->elink);
	if((qh->elink & Tdterm) != 0)
		s = seprint(s, se, "T");
	if((qh->elink & Tdlinkqh) != 0)
		s = seprint(s, se, "Q");
	s = seprint(s, se, " io %#p", qh->io);
	if(qh->io != nil && qh->io->err != nil)
		seprint(s, se, " err='%s'", qh->io->err);
	print("%s\n", buf);
	dumptd(qh->tds, "\t");
	if((qh->elink & QHterm) == 0){
		print("\thw tds:");
		i = 0;
		for(td = qh->elink; (td & Tdterm) == 0; td = TPTR(td)->link){
			print(" %#ulx", td);
			if(td == TPTR(td)->link)	/* BWS Td */
				break;
			if(i++ > 40){
				print("...");
				break;
			}
		}
		print("\n");
	}
}

static void
xdump(Ctlr *ctlr, int doilock)
{
	Isoio *iso;
	Qh *qh;
	int i;

	if(doilock){
		if(ctlr == ctlrs[0]){
			lock(&tdpool);
			print("tds: alloc %d = inuse %d + free %d\n",
				tdpool.nalloc, tdpool.ninuse, tdpool.nfree);
			unlock(&tdpool);
			lock(&qhpool);
			print("qhs: alloc %d = inuse %d + free %d\n",
				qhpool.nalloc, qhpool.ninuse, qhpool.nfree);
			unlock(&qhpool);
		}
		ilock(ctlr);
	}
	print("uhci port %#x frames %#p nintr %d ntdintr %d",
		ctlr->port, ctlr->frames, ctlr->nintr, ctlr->ntdintr);
	print(" nqhintr %d nisointr %d\n", ctlr->nqhintr, ctlr->nisointr);
	print("cmd %#ux sts %#ux fl %#ulx ps1 %#ux ps2 %#ux frames[0] %#ulx\n",
		INS(Cmd), INS(Status),
		INL(Flbaseadd), INS(PORT(0)), INS(PORT(1)),
		ctlr->frames[0]);
	for(iso = ctlr->iso; iso != nil; iso = iso->next)
		isodump(iso, 1);
	i = 0;
	for(qh = ctlr->qhs; qh != nil; qh = qh->next){
		qhdump(qh, "");
		if(i++ > 20){
			print("qhloop\n");
			break;
		}
	}
	print("\n");
	if(doilock)
		iunlock(ctlr);
}

static void
dump(Hci *hp)
{
	xdump(hp->aux, 1);
}

static Td*
tdalloc(void)
{
	int i;
	Td *td;
	Td *pool;

	lock(&tdpool);
	if(tdpool.free == nil){
		ddprint("uhci: tdalloc %d Tds\n", Incr);
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
	td = tdpool.free;
	tdpool.free = td->next;
	tdpool.ninuse++;
	tdpool.nfree--;
	unlock(&tdpool);

	memset(td, 0, sizeof(Td));
	td->link = Tdterm;
	assert(((ulong)td & 0xF) == 0);
	return td;
}

static void
tdfree(Td *td)
{
	if(td == nil)
		return;
	free(td->buff);
	td->buff = nil;
	lock(&tdpool);
	td->next = tdpool.free;
	tdpool.free = td;
	tdpool.ninuse--;
	tdpool.nfree++;
	unlock(&tdpool);
}

static void
qhlinkqh(Qh* qh, Qh* next)
{
	if(next == nil)
		qh->link = QHterm;
	else{
		next->link = qh->link;
		next->next = qh->next;
		qh->link = PCIWADDR(next)|QHlinkqh;
	}
	qh->next = next;
}

static void
qhlinktd(Qh *qh, Td *td)
{
	qh->tds = td;
	if(td == nil)
		qh->elink = QHvf|QHterm;
	else
		qh->elink = PCIWADDR(td);
}

static void
tdlinktd(Td *td, Td *next)
{
	td->next = next;
	if(next == nil)
		td->link = Tdterm;
	else
		td->link = PCIWADDR(next)|Tdvf;
}

static Qh*
qhalloc(Ctlr *ctlr, Qh *prev, Qio *io, char *tag)
{
	int i;
	Qh *qh;
	Qh *pool;

	lock(&qhpool);
	if(qhpool.free == nil){
		ddprint("uhci: qhalloc %d Qhs\n", Incr);
		pool = xspanalloc(Incr*sizeof(Qh), Align, 0);
		if(pool == nil)
			panic("qhalloc");
		for(i=Incr; --i>=0;){
			pool[i].next = qhpool.free;
			qhpool.free = &pool[i];
		}
		qhpool.nalloc += Incr;
		qhpool.nfree += Incr;
	}
	qh = qhpool.free;
	qhpool.free = qh->next;
	qh->next = nil;
	qh->link = QHterm;
	qhpool.ninuse++;
	qhpool.nfree--;
	unlock(&qhpool);

	qh->tds = nil;
	qh->elink = QHterm;
	qh->state = Qidle;
	qh->io = io;
	qh->tag = nil;
	kstrdup(&qh->tag, tag);

	if(prev != nil){
		coherence();
		ilock(ctlr);
		qhlinkqh(prev, qh);
		iunlock(ctlr);
	}

	assert(((ulong)qh & 0xF) == 0);
	return qh;
}

static void
qhfree(Ctlr *ctlr, Qh *qh)
{
	Td *td;
	Td *ltd;
	Qh *q;

	if(qh == nil)
		return;

	ilock(ctlr);
	for(q = ctlr->qhs; q != nil; q = q->next)
		if(q->next == qh)
			break;
	if(q == nil)
		panic("qhfree: nil q");
	q->next = qh->next;
	q->link = qh->link;
	iunlock(ctlr);

	for(td = qh->tds; td != nil; td = ltd){
		ltd = td->next;
		tdfree(td);
	}
	lock(&qhpool);
	qh->state = Qfree;	/* paranoia */
	qh->next = qhpool.free;
	qh->tag = nil;
	qh->io = nil;
	qhpool.free = qh;
	qhpool.ninuse--;
	qhpool.nfree++;
	unlock(&qhpool);
	ddprint("qhfree: qh %#p\n", qh);
}

static char*
errmsg(int err)
{
	if(err == 0)
		return "ok";
	if(err & Tdcrcto)
		return "crc/timeout error";
	if(err & Tdbabble)
		return "babble detected";
	if(err & Tddberr)
		return "db error";
	if(err & Tdbitstuff)
		return "bit stuffing error";
	if(err & Tdstalled)
		return Estalled;
	return Eio;
}

static int
isocanread(void *a)
{
	Isoio *iso;

	iso = a;
	return iso->state == Qclose ||
		(iso->state == Qrun &&
		iso->tok == Tdtokin && iso->tdi != iso->tdu);
}

static int
isocanwrite(void *a)
{
	Isoio *iso;

	iso = a;
	return iso->state == Qclose ||
		(iso->state == Qrun &&
		iso->tok == Tdtokout && iso->tdu->next != iso->tdi);
}

static void
tdisoinit(Isoio *iso, Td *td, long count)
{
	td->ndata = count;
	td->token = ((count-1)<<21)| ((iso->usbid & 0x7FF)<<8) | iso->tok;
	td->csw = Tderr1|Tdiso|Tdactive|Tdioc;
}

/*
 * Process Iso i/o on interrupt. For writes update just error status.
 * For reads update tds to reflect data and also error status.
 * When tdi aproaches tdu, advance tdu; data may be lost.
 * (If nframes is << Nframes tdu might be far away but this avoids
 * races regarding frno.)
 * If we suffer errors for more than half the frames we stall.
 */
static void
isointerrupt(Ctlr *ctlr, Isoio* iso)
{
	Td *tdi;
	int err;
	int i;
	int nframes;

	tdi = iso->tdi;
	if((tdi->csw & Tdactive) != 0)		/* nothing new done */
		return;
	ctlr->nisointr++;
	ddiprint("isointr: iso %#p: tdi %#p tdu %#p\n", iso, tdi, iso->tdu);
	if(iso->state != Qrun && iso->state != Qdone)
		panic("isointr: iso state");
	if(debug > 1 || iso->debug > 1)
		isodump(iso, 0);

	nframes = iso->nframes / 2;		/* limit how many we look */
	if(nframes > 64)
		nframes = 64;

	for(i = 0; i < nframes && (tdi->csw & Tdactive) == 0; i++){
		tdi->csw &= ~Tdioc;
		err = tdi->csw & Tderrors;
		if(err == 0)
			iso->nerrs = 0;
		else if(iso->nerrs++ > iso->nframes/2)
			tdi->csw |= Tdstalled;
		if((tdi->csw & Tdstalled) != 0){
			if(iso->err == nil){
				iso->err = errmsg(err);
				diprint("isointerrupt: tdi %#p error %#ux %s\n",
					tdi, err, iso->err);
				diprint("ctlr load %uld\n", ctlr->load);
			}
			tdi->ndata = 0;
		}else
			tdi->ndata = tdlen(tdi);

		if(tdi->next == iso->tdu || tdi->next->next == iso->tdu){
			memset(iso->tdu->data, 0, maxtdlen(iso->tdu));
			tdisoinit(iso, iso->tdu, maxtdlen(iso->tdu));
			iso->tdu = iso->tdu->next;
			iso->nleft = 0;
		}
		tdi = tdi->next;
	}
	ddiprint("isointr: %d frames processed\n", nframes);
	if(i == nframes)
		tdi->csw |= Tdioc;
	iso->tdi = tdi;
	if(isocanwrite(iso) || isocanread(iso)){
		diprint("wakeup iso %#p tdi %#p tdu %#p\n", iso,
			iso->tdi, iso->tdu);
		wakeup(iso);
	}

}

/*
 * Process a Qh upon interrupt. There's one per ongoing user I/O.
 * User process releases resources later, that is not done here.
 * We may find in this order one or more Tds:
 * - none/many non active and completed Tds
 * - none/one (usually(!) not active) and failed Td
 * - none/many active Tds.
 * Upon errors the entire transfer is aborted and error reported.
 * Otherwise, the transfer is complete only when all Tds are done or
 * when a read with less than maxpkt is found.
 * Use the software list and not qh->elink to avoid races.
 * We could use qh->elink to see if there's something new or not.
 */
static void
qhinterrupt(Ctlr *ctlr, Qh *qh)
{
	Td *td;
	int err;

	ctlr->nqhintr++;
	if(qh->state != Qrun)
		panic("qhinterrupt: qh state");
	if(qh->tds == nil)
		panic("qhinterrupt: no tds");
	if((qh->tds->csw & Tdactive) == 0)
		ddqprint("qhinterrupt port %#ux qh %#p p0 %#x p1 %#x\n",
			ctlr->port, qh, INS(PORT(0)), INS(PORT(1)));
	for(td = qh->tds; td != nil; td = td->next){
		if(td->csw & Tdactive)
			return;
		td->csw &= ~Tdioc;
		if((td->csw & Tdstalled) != 0){
			err = td->csw & Tderrors;
			/* just stalled is end of xfer but not an error */
			if(err != Tdstalled && qh->io->err == nil){
				qh->io->err = errmsg(td->csw & Tderrors);
				dqprint("qhinterrupt: td %#p error %#ux %s\n",
					td, err, qh->io->err);
				dqprint("ctlr load %uld\n", ctlr->load);
			}
			break;
		}
		if((td->csw & Tdnak) != 0){	/* retransmit; not serious */
			td->csw &= ~Tdnak;
			if(td->next == nil)
				td->csw |= Tdioc;
		}
		td->ndata = tdlen(td);
		if(td->ndata < maxtdlen(td)){	/* EOT */
			td = td->next;
			break;
		}
	}

	/*
	 * Done. Make void the Tds not used (errors or EOT) and wakeup epio.
	 */
	qh->elink = QHterm;
	for(; td != nil; td = td->next)
		td->ndata = 0;
	qh->state = Qdone;
	wakeup(qh->io);
}

static void
interrupt(Ureg*, void *a)
{
	Hci *hp;
	Ctlr *ctlr;
	int frptr;
	int frno;
	Qh *qh;
	Isoio *iso;
	int sts;
	int cmd;

	hp = a;
	ctlr = hp->aux;
	ilock(ctlr);
	ctlr->nintr++;
	sts = INS(Status);
	if((sts & Sall) == 0){		/* not for us; sharing irq */
		iunlock(ctlr);
		return;
	}
	OUTS(Status, sts & Sall);
	cmd = INS(Cmd);
	if(cmd & Crun == 0){
		print("uhci %#ux: not running: uhci bug?\n", ctlr->port);
		/* BUG: should abort everything in this case */
	}
	if(debug > 1){
		frptr = INL(Flbaseadd);
		frno = INL(Frnum);
		frno = TRUNC(frno, Nframes);
		print("cmd %#ux sts %#ux frptr %#ux frno %d\n",
			cmd, sts, frptr, frno);
	}
	ctlr->ntdintr++;
	/*
	 * Will we know in USB 3.0 who the interrupt was for?.
	 * Do they still teach indexing in CS?
	 * This is Intel's doing.
	 */
	for(iso = ctlr->iso; iso != nil; iso = iso->next)
		if(iso->state == Qrun || iso->state == Qdone)
			isointerrupt(ctlr, iso);
	for(qh = ctlr->qhs; qh != nil; qh = qh->next)
		if(qh->state == Qrun)
			qhinterrupt(ctlr, qh);
		else if(qh->state == Qclose)
			qhlinktd(qh, nil);
	iunlock(ctlr);
}

/*
 * iso->tdu is the next place to put data. When it gets full
 * it is activated and tdu advanced.
 */
static long
putsamples(Isoio *iso, uchar *b, long count)
{
	long tot;
	long n;

	for(tot = 0; isocanwrite(iso) && tot < count; tot += n){
		n = count-tot;
		if(n > maxtdlen(iso->tdu) - iso->nleft)
			n = maxtdlen(iso->tdu) - iso->nleft;
		memmove(iso->tdu->data+iso->nleft, b+tot, n);
		iso->nleft += n;
		if(iso->nleft == maxtdlen(iso->tdu)){
			tdisoinit(iso, iso->tdu, iso->nleft);
			iso->nleft = 0;
			iso->tdu = iso->tdu->next;
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
	int tot;
	int nw;
	char *err;

	iso->debug = ep->debug;
	diprint("uhci: episowrite: %#p ep%d.%d\n", iso, ep->dev->nb, ep->nb);

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
	b = a;
	for(tot = 0; tot < count; tot += nw){
		while(isocanwrite(iso) == 0){
			iunlock(ctlr);
			diprint("uhci: episowrite: %#p sleep\n", iso);
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
	diprint("uhci: episowrite: %#p %d bytes\n", iso, tot);
	return tot;
}

/*
 * Available data is kept at tdu and following tds, up to tdi (excluded).
 */
static long
episoread(Ep *ep, Isoio *iso, void *a, int count)
{
	Ctlr *ctlr;
	uchar *b;
	int nr;
	int tot;
	Td *tdu;

	iso->debug = ep->debug;
	diprint("uhci: episoread: %#p ep%d.%d\n", iso, ep->dev->nb, ep->nb);

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
	while(isocanread(iso) == 0){
		iunlock(ctlr);
		diprint("uhci: episoread: %#p sleep\n", iso);
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
	assert(iso->tdu != iso->tdi);

	for(tot = 0; iso->tdi != iso->tdu && tot < count; tot += nr){
		tdu = iso->tdu;
		if(tdu->csw & Tdactive){
			diprint("uhci: episoread: %#p tdu active\n", iso);
			break;
		}
		nr = tdu->ndata;
		if(tot + nr > count)
			nr = count - tot;
		if(nr == 0)
			print("uhci: ep%d.%d: too many polls\n",
				ep->dev->nb, ep->nb);
		else{
			iunlock(ctlr);		/* We could page fault here */
			memmove(b+tot, tdu->data, nr);
			ilock(ctlr);
			if(nr < tdu->ndata)
				memmove(tdu->data, tdu->data+nr, tdu->ndata - nr);
			tdu->ndata -= nr;
		}
		if(tdu->ndata == 0){
			tdisoinit(iso, tdu, ep->maxpkt);
			iso->tdu = tdu->next;
		}
	}
	iunlock(ctlr);
	qunlock(iso);
	poperror();
	diprint("uhci: episoread: %#p %d bytes err '%s'\n", iso, tot, iso->err);
	if(iso->err != nil)
		error(iso->err);
	return tot;
}

static int
nexttoggle(int tog)
{
	if(tog == Tddata0)
		return Tddata1;
	else
		return Tddata0;
}

static Td*
epgettd(Ep *ep, Qio *io, int flags, void *a, int count)
{
	Td *td;
	int tok;

	if(ep->maxpkt < count)
		error("maxpkt too short");
	td = tdalloc();
	if(count <= Tdndata)
		td->data = td->sbuff;
	else
		td->data = td->buff = smalloc(ep->maxpkt);
	td->buffer = PCIWADDR(td->data);
	td->ndata = count;
	if(a != nil && count > 0)
		memmove(td->data, a, count);
	td->csw = Tderr2|Tderr1|flags;
	if(ep->dev->speed == Lowspeed)
		td->csw |= Tdlow;
	tok = io->tok | io->toggle;
	io->toggle = nexttoggle(io->toggle);
	td->token = ((count-1)<<21) | ((io->usbid&0x7FF)<<8) | tok;

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
	qh->elink = QHterm;
	for(td = qh->tds; td != nil; td = td->next){
		if(td->csw & Tdactive)
			td->ndata = 0;
		td->csw &= ~(Tdactive|Tdioc);
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
epiowait(Ctlr *ctlr, Qio *io, int tmout, ulong load)
{
	Qh *qh;
	int timedout;

	qh = io->qh;
	ddqprint("uhci io %#p sleep on qh %#p state %uld\n", io, qh, qh->state);
	timedout = 0;
	if(waserror()){
		dqprint("uhci io %#p qh %#p timed out\n", io, qh);
		timedout++;
	}else{
		if(tmout == 0)
			sleep(io, epiodone, qh);
		else
			tsleep(io, epiodone, qh, tmout);
		poperror();
	}
	ilock(ctlr);
	if(qh->state == Qrun)
		timedout = 1;
	else if(qh->state != Qdone && qh->state != Qclose)
		panic("epio: queue not done and not closed");
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
	qhlinktd(qh, nil);
	ctlr->load -= load;
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
	Td *td, *ltd, *td0, *ntd;
	Ctlr *ctlr;
	Qh* qh;
	long n, tot;
	char buf[128];
	uchar *c;
	int saved, ntds, tmout;
	ulong load;
	char *err;

	qh = io->qh;
	ctlr = ep->hp->aux;
	io->debug = ep->debug;
	tmout = ep->tmout;
	ddeprint("epio: %s ep%d.%d io %#p count %ld load %uld\n",
		io->tok == Tdtokin ? "in" : "out",
		ep->dev->nb, ep->nb, io, count, ctlr->load);
	if((debug > 1 || ep->debug > 1) && io->tok != Tdtokin){
		seprintdata(buf, buf+sizeof(buf), a, count);
		print("uchi epio: user data: %s\n", buf);
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
		n = ep->maxpkt;
		if(count-tot < n)
			n = count-tot;
		if(c != nil && io->tok != Tdtokin)
			td = epgettd(ep, io, Tdactive, c+tot, n);
		else
			td = epgettd(ep, io, Tdactive|Tdspd, nil, n);
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

	ltd->csw |= Tdioc;	/* the last one interrupts */
	ddeprint("uhci: load %uld ctlr load %uld\n", load, ctlr->load);
	ilock(ctlr);
	if(qh->state != Qclose){
		io->iotime = TK2MS(MACHP(0)->ticks);
		qh->state = Qrun;
		coherence();
		qhlinktd(qh, td0);
		ctlr->load += load;
	}
	iunlock(ctlr);

	epiowait(ctlr, io, tmout, load);

	if(debug > 1 || ep->debug > 1)
		dumptd(td0, "epio: got tds: ");

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
		if(td->csw & (Tdstalled|Tdactive)){
			if(saved++ == 0)
				io->toggle = td->token & Tddata1;
		}else{
			tot += td->ndata;
			if(c != nil && tdtok(td) == Tdtokin && td->ndata > 0){
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
	Isoio *iso;
	char buf[160];
	ulong delta;

	ddeprint("uhci: epread\n");
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
		iso = ep->aux;
		return episoread(ep, iso, a, count);
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
		error("short usb comand");
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
		if(cio->usbid == 0)
			cio->usbid = ((ep->nb&Epmax)<<7)|(ep->dev->nb&Devmax);
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
			error("d2h data too large to fit in uhci");
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

static long
epwrite(Ep *ep, void *a, long count)
{
	Ctlio *cio;
	Isoio *iso;
	Qio *io;
	ulong delta;
	char *b;
	int tot;
	int nw;

	ddeprint("uhci: epwrite ep%d.%d\n", ep->dev->nb, ep->nb);
	if(ep->aux == nil)
		panic("uhci: epwrite: not open");
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
		iso = ep->aux;
		return episowrite(ep, iso, a, count);
	default:
		panic("uhci: epwrite: bad ep ttype %d", ep->ttype);
	}
	return -1;
}

static void
isoopen(Ep *ep)
{
	Ctlr *ctlr;
	Isoio *iso;
	int frno;
	int i;
	Td* td;
	Td* ltd;
	int size;
	int left;

	if(ep->mode == ORDWR)
		error("iso i/o is half-duplex");
	ctlr = ep->hp->aux;
	iso = ep->aux;
	iso->debug = ep->debug;
	iso->next = nil;			/* paranoia */
	if(ep->mode == OREAD)
		iso->tok = Tdtokin;
	else
		iso->tok = Tdtokout;
	iso->usbid = ((ep->nb & Epmax)<<7)|(ep->dev->nb & Devmax);
	iso->state = Qidle;
	iso->nframes = Nframes/ep->pollival;
	if(iso->nframes < 3)
		error("uhci isoopen bug");	/* we need at least 3 tds */

	ilock(ctlr);
	if(ctlr->load + ep->load > 800)
		print("usb: uhci: bandwidth may be exceeded\n");
	ctlr->load += ep->load;
	ctlr->isoload += ep->load;
	dprint("uhci: load %uld isoload %uld\n", ctlr->load, ctlr->isoload);
	iunlock(ctlr);

	/*
	 * From here on this cannot raise errors
	 * unless we catch them and release here all memory allocated.
	 */
	if(ep->maxpkt > Tdndata)
		iso->data = smalloc(iso->nframes*ep->maxpkt);
	ilock(ctlr);
	frno = INS(Frnum) + 10;			/* start 10ms ahead */
	frno = TRUNC(frno, Nframes);
	iunlock(ctlr);
	iso->td0frno = frno;
	ltd = nil;
	left = 0;
	for(i = 0; i < iso->nframes; i++){
		td = iso->tdps[frno] = tdalloc();
		if(ep->mode == OREAD)
			size = ep->maxpkt;
		else{
			size = (ep->hz+left) * ep->pollival / 1000;
			size *= ep->samplesz;
			left = (ep->hz+left) * ep->pollival % 1000;
			if(size > ep->maxpkt){
				print("uhci: ep%d.%d: size > maxpkt\n",
					ep->dev->nb, ep->nb);
				print("size = %d max = %ld\n", size, ep->maxpkt);
				size = ep->maxpkt;
			}
		}
		if(size > Tdndata)
			td->data = iso->data + i * ep->maxpkt;
		else
			td->data = td->sbuff;
		td->buffer = PCIWADDR(td->data);
		tdisoinit(iso, td, size);
		if(ltd != nil)
			ltd->next = td;
		ltd = td;
		frno = TRUNC(frno+ep->pollival, Nframes);
	}
	ltd->next = iso->tdps[iso->td0frno];
	iso->tdi = iso->tdps[iso->td0frno];
	iso->tdu = iso->tdi;	/* read: right now; write: 1s ahead */
	ilock(ctlr);
	frno = iso->td0frno;
	for(i = 0; i < iso->nframes; i++){
		iso->tdps[frno]->link = ctlr->frames[frno];
		frno = TRUNC(frno+ep->pollival, Nframes);
	}
	coherence();
	frno = iso->td0frno;
	for(i = 0; i < iso->nframes; i++){
		ctlr->frames[frno] = PCIWADDR(iso->tdps[frno]);
		frno = TRUNC(frno+ep->pollival, Nframes);
	}
	iso->next = ctlr->iso;
	ctlr->iso = iso;
	iso->state = Qdone;
	iunlock(ctlr);
	if(debug > 1 || iso->debug >1)
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
	Qh *cqh;
	Qio *io;
	Ctlio *cio;
	int usbid;

	ctlr = ep->hp->aux;
	deprint("uhci: epopen ep%d.%d\n", ep->dev->nb, ep->nb);
	if(ep->aux != nil)
		panic("uhci: epopen called with open ep");
	if(waserror()){
		free(ep->aux);
		ep->aux = nil;
		nexterror();
	}
	if(ep->maxpkt > Tdmaxpkt){
		print("uhci: maxkpkt too large: using %d\n", Tdmaxpkt);
		ep->maxpkt = Tdmaxpkt;
	}
	cqh = ctlr->qh[ep->ttype];
	switch(ep->ttype){
	case Tnone:
		error("endpoint not configured");
	case Tiso:
		ep->aux = smalloc(sizeof(Isoio));
		isoopen(ep);
		break;
	case Tctl:
		cio = ep->aux = smalloc(sizeof(Ctlio));
		cio->debug = ep->debug;
		cio->ndata = -1;
		cio->data = nil;
		if(ep->dev->isroot != 0 && ep->nb == 0)	/* root hub */
			break;
		cio->qh = qhalloc(ctlr, cqh, cio, "epc");
		break;
	case Tbulk:
	case Tintr:
		io = ep->aux = smalloc(sizeof(Qio)*2);
		io[OREAD].debug = io[OWRITE].debug = ep->debug;
		usbid = ((ep->nb&Epmax)<<7)|(ep->dev->nb &Devmax);
		if(ep->mode != OREAD){
			if(ep->toggle[OWRITE] != 0)
				io[OWRITE].toggle = Tddata1;
			else
				io[OWRITE].toggle = Tddata0;
			io[OWRITE].tok = Tdtokout;
			io[OWRITE].qh = qhalloc(ctlr, cqh, io+OWRITE, "epw");
			io[OWRITE].usbid = usbid;
		}
		if(ep->mode != OWRITE){
			if(ep->toggle[OREAD] != 0)
				io[OREAD].toggle = Tddata1;
			else
				io[OREAD].toggle = Tddata0;
			io[OREAD].tok = Tdtokin;
			io[OREAD].qh = qhalloc(ctlr, cqh, io+OREAD, "epr");
			io[OREAD].usbid = usbid;
		}
		break;
	}
	if(debug>1 || ep->debug)
		dump(ep->hp);
	deprint("uhci: epopen done\n");
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
	dqprint("uhci: cancelio for qh %#p state %s\n",
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
	Isoio **il;
	ulong *lp;
	int i;
	int frno;
	Td *td;

	ilock(ctlr);
	if(iso->state == Qclose){
		iunlock(ctlr);
		return;
	}
	if(iso->state != Qrun && iso->state != Qdone)
		panic("bad iso state");
	iso->state = Qclose;
	if(ctlr->isoload < load)
		panic("uhci: low isoload");
	ctlr->isoload -= load;
	ctlr->load -= load;
	for(il = &ctlr->iso; *il != nil; il = &(*il)->next)
		if(*il == iso)
			break;
	if(*il == nil)
		panic("isocancel: not found");
	*il = iso->next;
	frno = iso->td0frno;
	for(i = 0; i < iso->nframes; i++){
		td = iso->tdps[frno];
		td->csw &= ~(Tdioc|Tdactive);
		for(lp=&ctlr->frames[frno]; !(*lp & Tdterm);
					lp = &TPTR(*lp)->link)
			if(TPTR(*lp) == td)
				break;
		if(*lp & Tdterm)
			panic("cancelisoio: td not found");
		*lp = td->link;
		frno = TRUNC(frno+pollival, Nframes);
	}
	iunlock(ctlr);

	/*
	 * wakeup anyone waiting for I/O and
	 * wait to be sure no I/O is in progress in the controller.
	 * and then wait to be sure episo-io is no longer running.
	 */
	wakeup(iso);
	diprint("cancelisoio iso %#p waiting for I/O to cease\n", iso);
	tsleep(&up->sleep, return0, 0, 5);
	qlock(iso);
	qunlock(iso);
	diprint("cancelisoio iso %#p releasing iso\n", iso);

	frno = iso->td0frno;
	for(i = 0; i < iso->nframes; i++){
		tdfree(iso->tdps[frno]);
		iso->tdps[frno] = nil;
		frno = TRUNC(frno+pollival, Nframes);
	}
	free(iso->data);
	iso->data = nil;
}

static void
epclose(Ep *ep)
{
	Ctlr *ctlr;
	Ctlio *cio;
	Isoio *iso;
	Qio *io;

	ctlr = ep->hp->aux;
	deprint("uhci: epclose ep%d.%d\n", ep->dev->nb, ep->nb);

	if(ep->aux == nil)
		panic("uhci: epclose called with closed ep");
	switch(ep->ttype){
	case Tctl:
		cio = ep->aux;
		cancelio(ctlr, cio);
		free(cio->data);
		cio->data = nil;
		break;
	case Tbulk:
	case Tintr:
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
		break;
	case Tiso:
		iso = ep->aux;
		cancelisoio(ctlr, iso, ep->pollival, ep->load);
		break;
	default:
		panic("epclose: bad ttype %d", ep->ttype);
	}

	free(ep->aux);
	ep->aux = nil;

}

static char*
seprintep(char *s, char *e, Ep *ep)
{
	Ctlio *cio;
	Qio *io;
	Isoio *iso;
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
		s = seprint(s,e,"cio %#p qh %#p"
			" id %#x tog %#x tok %#x err %s\n",
			cio, cio->qh, cio->usbid, cio->toggle,
			cio->tok, cio->err);
		break;
	case Tbulk:
	case Tintr:
		io = ep->aux;
		if(ep->mode != OWRITE)
			s = seprint(s,e,"r: qh %#p id %#x tog %#x tok %#x err %s\n",
				io[OREAD].qh, io[OREAD].usbid, io[OREAD].toggle,
				io[OREAD].tok, io[OREAD].err);
		if(ep->mode != OREAD)
			s = seprint(s,e,"w: qh %#p id %#x tog %#x tok %#x err %s\n",
				io[OWRITE].qh, io[OWRITE].usbid, io[OWRITE].toggle,
				io[OWRITE].tok, io[OWRITE].err);
		break;
	case Tiso:
		iso = ep->aux;
		s = seprint(s,e,"iso %#p id %#x tok %#x tdu %#p tdi %#p err %s\n",
			iso, iso->usbid, iso->tok, iso->tdu, iso->tdi, iso->err);
		break;
	}
	iunlock(ctlr);
	return s;
}

static int
portenable(Hci *hp, int port, int on)
{
	int s;
	int ioport;
	Ctlr *ctlr;

	ctlr = hp->aux;
	dprint("uhci: %#x port %d enable=%d\n", ctlr->port, port, on);
	ioport = PORT(port-1);
	qlock(&ctlr->portlck);
	if(waserror()){
		qunlock(&ctlr->portlck);
		nexterror();
	}
	ilock(ctlr);
	s = INS(ioport);
	if(on)
		OUTS(ioport, s | PSenable);
	else
		OUTS(ioport, s & ~PSenable);
	microdelay(64);
	iunlock(ctlr);
	tsleep(&up->sleep, return0, 0, Enabledelay);
	dprint("uhci %#ux port %d enable=%d: sts %#x\n",
		ctlr->port, port, on, INS(ioport));
	qunlock(&ctlr->portlck);
	poperror();
	return 0;
}

static int
portreset(Hci *hp, int port, int on)
{
	int i, p;
	Ctlr *ctlr;

	if(on == 0)
		return 0;
	ctlr = hp->aux;
	dprint("uhci: %#ux port %d reset\n", ctlr->port, port);
	p = PORT(port-1);
	ilock(ctlr);
	OUTS(p, PSreset);
	delay(50);
	OUTS(p, INS(p) & ~PSreset);
	OUTS(p, INS(p) | PSenable);
	microdelay(64);
	for(i=0; i<1000 && (INS(p) & PSenable) == 0; i++)
		;
	OUTS(p, (INS(p) & ~PSreset)|PSenable);
	iunlock(ctlr);
	dprint("uhci %#ux after port %d reset: sts %#x\n",
		ctlr->port, port, INS(p));
	return 0;
}

static int
portstatus(Hci *hp, int port)
{
	int s;
	int r;
	int ioport;
	Ctlr *ctlr;

	ctlr = hp->aux;
	ioport = PORT(port-1);
	qlock(&ctlr->portlck);
	if(waserror()){
		iunlock(ctlr);
		qunlock(&ctlr->portlck);
		nexterror();
	}
	ilock(ctlr);
	s = INS(ioport);
	if(s & (PSstatuschg | PSchange)){
		OUTS(ioport, s);
		ddprint("uhci %#ux port %d status %#x\n", ctlr->port, port, s);
	}
	iunlock(ctlr);
	qunlock(&ctlr->portlck);
	poperror();

	/*
	 * We must return status bits as a
	 * get port status hub request would do.
	 */
	r = 0;
	if(s & PSpresent)
		r |= HPpresent;
	if(s & PSenable)
		r |= HPenable;
	if(s & PSsuspend)
		r |= HPsuspend;
	if(s & PSreset)
		r |= HPreset;
	if(s & PSslow)
		r |= HPslow;
	if(s & PSstatuschg)
		r |= HPstatuschg;
	if(s & PSchange)
		r |= HPchange;
	return r;
}

static void
scanpci(void)
{
	static int already = 0;
	int io;
	int i;
	Ctlr *ctlr;
	Pcidev *p;

	if(already)
		return;
	already = 1;
	p = nil;
	while(p = pcimatch(p, 0, 0)){
		/*
		 * Find UHCI controllers (Programming Interface = 0).
		 */
		if(p->ccrb != Pcibcserial || p->ccru != Pciscusb)
			continue;
		switch(p->ccrp){
		case 0:
			io = p->mem[4].bar & ~0x0F;
			break;
		default:
			continue;
		}
		if(io == 0){
			print("usbuhci: %#x %#x: failed to map registers\n",
				p->vid, p->did);
			continue;
		}
		if(ioalloc(io, p->mem[4].size, 0, "usbuhci") < 0){
			print("usbuhci: port %#ux in use\n", io);
			continue;
		}
		if(p->intl == 0xFF || p->intl == 0){
			print("usbuhci: no irq assigned for port %#ux\n", io);
			continue;
		}

		dprint("uhci: %#x %#x: port %#ux size %#x irq %d\n",
			p->vid, p->did, io, p->mem[4].size, p->intl);

		ctlr = malloc(sizeof(Ctlr));
		if (ctlr == nil)
			panic("uhci: out of memory");
		ctlr->pcidev = p;
		ctlr->port = io;
		for(i = 0; i < Nhcis; i++)
			if(ctlrs[i] == nil){
				ctlrs[i] = ctlr;
				break;
			}
		if(i == Nhcis)
			print("uhci: bug: no more controllers\n");
	}
}

static void
uhcimeminit(Ctlr *ctlr)
{
	Td* td;
	Qh *qh;
	int frsize;
	int i;

	ctlr->qhs = ctlr->qh[Tctl] = qhalloc(ctlr, nil, nil, "CTL");
	ctlr->qh[Tintr] = qhalloc(ctlr, ctlr->qh[Tctl], nil, "INT");
	ctlr->qh[Tbulk] = qhalloc(ctlr, ctlr->qh[Tintr], nil, "BLK");

	/* idle Td from dummy Qh at the end. looped back to itself */
	/* This is a workaround for PIIX4 errata 29773804.pdf */
	qh = qhalloc(ctlr, ctlr->qh[Tbulk], nil, "BWS");
	td = tdalloc();
	td->link = PCIWADDR(td);
	qhlinktd(qh, td);

	/* loop (hw only) from the last qh back to control xfers.
	 * this may be done only for some of them. Disable until ehci comes.
	 */
	if(0)
	qh->link = PCIWADDR(ctlr->qhs);

	frsize = Nframes*sizeof(ulong);
	ctlr->frames = xspanalloc(frsize, frsize, 0);
	if(ctlr->frames == nil)
		panic("uhci reset: no memory");

	ctlr->iso = nil;
	for(i = 0; i < Nframes; i++)
		ctlr->frames[i] = PCIWADDR(ctlr->qhs)|QHlinkqh;
	OUTL(Flbaseadd, PCIWADDR(ctlr->frames));
	OUTS(Frnum, 0);
	dprint("uhci %#ux flb %#ulx frno %#ux\n", ctlr->port,
		INL(Flbaseadd), INS(Frnum));
}

static void
init(Hci *hp)
{
	Ctlr *ctlr;
	int sts;
	int i;

	ctlr = hp->aux;
	dprint("uhci %#ux init\n", ctlr->port);
	coherence();
	ilock(ctlr);
	OUTS(Usbintr, Itmout|Iresume|Ioc|Ishort);
	uhcirun(ctlr, 1);
	dprint("uhci: init: cmd %#ux sts %#ux sof %#ux",
		INS(Cmd), INS(Status), INS(SOFmod));
	dprint(" flb %#ulx frno %#ux psc0 %#ux psc1 %#ux",
		INL(Flbaseadd), INS(Frnum), INS(PORT(0)), INS(PORT(1)));
	/* guess other ports */
	for(i = 2; i < 6; i++){
		sts = INS(PORT(i));
		if(sts != 0xFFFF && (sts & PSreserved1) == 1){
			dprint(" psc%d %#ux", i, sts);
			hp->nports++;
		}else
			break;
	}
	for(i = 0; i < hp->nports; i++)
		OUTS(PORT(i), 0);
	iunlock(ctlr);
}

static void
uhcireset(Ctlr *ctlr)
{
	int i;
	int sof;

	ilock(ctlr);
	dprint("uhci %#ux reset\n", ctlr->port);

	/*
	 * Turn off legacy mode. Some controllers won't
	 * interrupt us as expected otherwise.
	 */
	uhcirun(ctlr, 0);
	pcicfgw16(ctlr->pcidev, 0xc0, 0x2000);

	OUTS(Usbintr, 0);
	sof = INB(SOFmod);
	uhcicmd(ctlr, Cgreset);			/* global reset */
	delay(Resetdelay);
	uhcicmd(ctlr, 0);			/* all halt */
	uhcicmd(ctlr, Chcreset);			/* controller reset */
	for(i = 0; i < 100; i++){
		if((INS(Cmd) & Chcreset) == 0)
			break;
		delay(1);
	}
	if(i == 100)
		print("uhci %#x controller reset timed out\n", ctlr->port);
	OUTB(SOFmod, sof);
	iunlock(ctlr);
}

static void
setdebug(Hci*, int d)
{
	debug = d;
}

static void
shutdown(Hci *hp)
{
	Ctlr *ctlr;

	ctlr = hp->aux;

	ilock(ctlr);
	uhcirun(ctlr, 0);
	delay(100);
	iunlock(ctlr);
}

static int
reset(Hci *hp)
{
	static Lock resetlck;
	int i;
	Ctlr *ctlr;
	Pcidev *p;

	if(getconf("*nousbuhci"))
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
		if(hp->port == 0 || hp->port == ctlr->port){
			ctlr->active = 1;
			break;
		}
	}
	iunlock(&resetlck);
	if(ctlrs[i] == nil || i == Nhcis)
		return -1;

	p = ctlr->pcidev;
	hp->aux = ctlr;
	hp->port = ctlr->port;
	hp->irq = p->intl;
	hp->tbdf = p->tbdf;
	hp->nports = 2;			/* default */

	uhcireset(ctlr);
	uhcimeminit(ctlr);

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
	hp->debug = setdebug;
	hp->type = "uhci";
	return 0;
}

void
usbuhcilink(void)
{
	addhcitype("uhci", reset);
}
