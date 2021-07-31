#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"

#include	"usb.h"

#define XPRINT if(debug)print

static int Chatty = 0;
static int debug = 0;

static	char	Estalled[] = "usb endpoint stalled";

/*
 * UHCI interface registers and bits
 */
enum
{
	/* i/o space */
	Cmd = 0,
	Status = 2,
	Usbintr = 4,
	Frnum = 6,
	Flbaseadd = 8,
	SOFMod = 0xC,
	Portsc0 = 0x10,
	Portsc1 = 0x12,

	/* port status */
	Suspend =		1<<12,
	PortReset =		1<<9,
	SlowDevice =		1<<8,
	ResumeDetect =	1<<6,
	PortChange =		1<<3,	/* write 1 to clear */
	PortEnable =		1<<2,
	StatusChange =	1<<1,	/* write 1 to clear */
	DevicePresent =	1<<0,

	NFRAME = 	1024,
	FRAMESIZE=	NFRAME*sizeof(ulong),	/* fixed by hardware; aligned to same */

	Vf =			1<<2,	/* TD only */
	IsQH =		1<<1,
	Terminate =	1<<0,

	/* TD.status */
	SPD =		1<<29,
	ErrLimit0 =	0<<27,
	ErrLimit1 =	1<<27,
	ErrLimit2 =	2<<27,
	ErrLimit3 =	3<<27,
	LowSpeed =	1<<26,
	IsoSelect =	1<<25,
	IOC =		1<<24,
	Active =		1<<23,
	Stalled =		1<<22,
	DataBufferErr =	1<<21,
	Babbling =	1<<20,
	NAKed =		1<<19,
	CRCorTimeout = 1<<18,
	BitstuffErr =	1<<17,
	AnyError = (Stalled | DataBufferErr | Babbling | NAKed | CRCorTimeout | BitstuffErr),

	/* TD.dev */
	IsDATA1 =	1<<19,

	/* TD.flags (software) */
	CancelTD=	1<<0,
	IsoClean=		1<<2,
};

static struct
{
	int	bit;
	char	*name;
}
portstatus[] =
{
	{ Suspend,		"suspend", },
	{ PortReset,		"reset", },
	{ SlowDevice,		"lowspeed", },
	{ ResumeDetect,	"resume", },
	{ PortChange,		"portchange", },
	{ PortEnable,		"enable", },
	{ StatusChange,	"statuschange", },
	{ DevicePresent,	"present", },
};

typedef struct Ctlr Ctlr;
typedef struct Endptx Endptx;
typedef struct QH QH;
typedef struct TD TD;

/*
 * software structures
 */
struct Ctlr
{
	Lock;	/* protects state shared with interrupt (eg, free list) */
	Ctlr*	next;
	Pcidev*	pcidev;
	int	active;

	int	io;
	ulong*	frames;	/* frame list */
	ulong*	frameld;	/* real time load on each of the frame list entries */
	QLock	resetl;	/* lock controller during USB reset */

	TD*	tdpool;
	TD*	freetd;
	QH*	qhpool;
	QH*	freeqh;

	QH*	ctlq;	/* queue for control i/o */
	QH*	bwsop;	/* empty bandwidth sop (to PIIX4 errata specifications) */
	QH*	bulkq;	/* queue for bulk i/o (points back to bandwidth sop) */
	QH*	recvq;	/* receive queues for bulk i/o */

	Udev*	ports[2];

	struct {
		Lock;
		Endpt*	f;
	} activends;

	long		usbints;	/* debugging */
	long		framenumber;
	long		frameptr;
	long		usbbogus;
};

#define	IN(x)	ins(ctlr->io+(x))
#define	OUT(x, v)	outs(ctlr->io+(x), (v))

static Ctlr* ctlrhead;
static Ctlr* ctlrtail;

struct Endptx
{
	QH*		epq;	/* queue of TDs for this endpoint */

	/* ISO related: */
	void*	tdalloc;
	void*	bpalloc;
	uchar*	bp0;		/* first block in array */
	TD	*	td0;		/* first td in array */
	TD	*	etd;		/* pointer into circular list of TDs for isochronous ept */
	TD	*	xtd;		/* next td to be cleaned */
};

/*
 * UHCI hardware structures, aligned on 16-byte boundary
 */
struct TD
{
	ulong	link;
	ulong	status;	/* controller r/w */
	ulong	dev;
	ulong	buffer;

	/* software */
	ulong	flags;
	union{
		Block*	bp;		/* non-iso */
		ulong	offset;	/* iso */
	};
	Endpt*	ep;
	TD*		next;
};
#define	TFOL(p)	((TD*)KADDR((ulong)(p) & ~(0xF|PCIWINDOW)))

struct QH
{
	ulong	head;
	ulong	entries;	/* address of next TD or QH to process (updated by controller) */

	/* software */
	QH*		hlink;
	TD*		first;
	QH*		next;		/* free list */
	TD*		last;
	ulong	_d1;		/* fillers */
	ulong	_d2;
};
#define	QFOL(p)	((QH*)KADDR((ulong)(p) & ~(0xF|PCIWINDOW)))

static TD *
alloctd(Ctlr *ctlr)
{
	TD *t;

	ilock(ctlr);
	t = ctlr->freetd;
	if(t == nil)
		panic("alloctd");	/* TO DO */
	ctlr->freetd = t->next;
	t->next = nil;
	iunlock(ctlr);
	t->ep = nil;
	t->bp = nil;
	t->status = 0;
	t->link = Terminate;
	t->buffer = 0;
	t->flags = 0;
	return t;
}

static void
freetd(Ctlr *ctlr, TD *t)
{
	t->ep = nil;
	if(t->bp)
		freeb(t->bp);
	t->bp = nil;
	ilock(ctlr);
	t->buffer = 0xdeadbeef;
	t->next = ctlr->freetd;
	ctlr->freetd = t;
	iunlock(ctlr);
}

static void
dumpdata(Block *b, int n)
{
	int i;

	XPRINT("\tb %8.8lux[%d]: ", (ulong)b->rp, n);
	if(n > 16)
		n = 16;
	for(i=0; i<n; i++)
		XPRINT(" %2.2ux", b->rp[i]);
	XPRINT("\n");
}

static void
dumptd(TD *t, int follow)
{
	int i, n;
	char buf[20], *s;
	TD *t0;

	t0 = t;
	while(t){
		i = t->dev & 0xFF;
		if(i == TokOUT || i == TokSETUP)
			n = ((t->dev>>21) + 1) & 0x7FF;
		else if((t->status & Active) == 0)
			n = (t->status + 1) & 0x7FF;
		else
			n = 0;
		s = buf;
		if(t->status & Active)
			*s++ = 'A';
		if(t->status & Stalled)
			*s++ = 'S';
		if(t->status & DataBufferErr)
			*s++ = 'D';
		if(t->status & Babbling)
			*s++ = 'B';
		if(t->status & NAKed)
			*s++ = 'N';
		if(t->status & CRCorTimeout)
			*s++ = 'T';
		if(t->status & BitstuffErr)
			*s++ = 'b';
		if(t->status & LowSpeed)
			*s++ = 'L';
		*s = 0;
		XPRINT("td %8.8lux: ", t);
		XPRINT("l=%8.8lux s=%8.8lux d=%8.8lux b=%8.8lux %8.8lux f=%8.8lux\n",
			t->link, t->status, t->dev, t->buffer, t->bp?(ulong)t->bp->rp:0, t->flags);
		XPRINT("\ts=%s,ep=%ld,d=%ld,D=%ld\n",
			buf, (t->dev>>15)&0xF, (t->dev>>8)&0xFF, (t->dev>>19)&1);
		if(debug && t->bp && (t->flags & CancelTD) == 0)
			dumpdata(t->bp, n);
		if(!follow || t->link & Terminate || t->link & IsQH)
			break;
		t = TFOL(t->link);
		if(t == t0)
			break;	/* looped */
	}
}

static TD *
alloctde(Ctlr *ctlr, Endpt *e, int pid, int n)
{
	TD *t;
	int tog, id;

	t = alloctd(ctlr);
	id = (e->x<<7)|(e->dev->x&0x7F);
	tog = 0;
	if(e->data01 && pid != TokSETUP)
		tog = IsDATA1;
	t->ep = e;
	t->status = ErrLimit3 | Active | IOC;	/* or put IOC only on last? */
	if(e->dev->ls)
		t->status |= LowSpeed;
	t->dev = ((n-1)<<21) | ((id&0x7FF)<<8) | pid | tog;
	return t;
}

static QH *
allocqh(Ctlr *ctlr)
{
	QH *qh;

	ilock(ctlr);
	qh = ctlr->freeqh;
	if(qh == nil)
		panic("allocqh");	/* TO DO */
	ctlr->freeqh = qh->next;
	qh->next = nil;
	iunlock(ctlr);
	qh->head = Terminate;
	qh->entries = Terminate;
	qh->hlink = nil;
	qh->first = nil;
	qh->last = nil;
	return qh;
}

static void
freeqh(Ctlr *ctlr, QH *qh)
{
	ilock(ctlr);
	qh->next = ctlr->freeqh;
	ctlr->freeqh = qh;
	iunlock(ctlr);
}

static void
dumpqh(QH *q)
{
	int i;
	QH *q0;

	q0 = q;
	for(i = 0; q != nil && i < 10; i++){
		XPRINT("qh %8.8lux: %8.8lux %8.8lux\n", q, q->head, q->entries);
		if((q->entries & Terminate) == 0)
			dumptd(TFOL(q->entries), 1);
		if(q->head & Terminate)
			break;
		if((q->head & IsQH) == 0){
			XPRINT("head:");
			dumptd(TFOL(q->head), 1);
			break;
		}
		q = QFOL(q->head);
		if(q == q0)
			break;	/* looped */
	}
}

static void
queuetd(Ctlr *ctlr, QH *q, TD *t, int vf, char *why)
{
	TD *lt;

	for(lt = t; lt->next != nil; lt = lt->next)
		lt->link = PCIWADDR(lt->next) | vf;
	lt->link = Terminate;
	ilock(ctlr);
	XPRINT("queuetd %s: t=%p lt=%p q=%p first=%p last=%p entries=%.8lux\n",
		why, t, lt, q, q->first, q->last, q->entries);
	if(q->first != nil){
		q->last->link = PCIWADDR(t) | vf;
		q->last->next = t;
	}else{
		q->first = t;
		q->entries = PCIWADDR(t);
	}
	q->last = lt;
	XPRINT("	t=%p q=%p first=%p last=%p entries=%.8lux\n",
		t, q, q->first, q->last, q->entries);
	dumpqh(q);
	iunlock(ctlr);
}

static void
cleantd(Ctlr *ctlr, TD *t, int discard)
{
	Block *b;
	int n, err;

	XPRINT("cleanTD: %8.8lux %8.8lux %8.8lux %8.8lux\n", t->link, t->status, t->dev, t->buffer);
	if(t->ep != nil && t->ep->debug)
		dumptd(t, 0);
	if(t->status & Active)
		panic("cleantd Active");
	err = t->status & (AnyError&~NAKed);
	/* TO DO: on t->status&AnyError, q->entries will not have advanced */
	if (err) {
		XPRINT("cleanTD: Error %8.8lux %8.8lux %8.8lux %8.8lux\n", t->link, t->status, t->dev, t->buffer);
//		print("cleanTD: Error %8.8lux %8.8lux %8.8lux %8.8lux\n", t->link, t->status, t->dev, t->buffer);
	}
	switch(t->dev&0xFF){
	case TokIN:
		if(discard || (t->flags & CancelTD) || t->ep == nil || t->ep->x!=0&&err){
			if(t->ep != nil){
				if(err != 0)
					t->ep->err = err==Stalled? Estalled: Eio;
				wakeup(&t->ep->rr);	/* in case anyone cares */
			}
			break;
		}
		b = t->bp;
		n = (t->status + 1) & 0x7FF;
		if(n > b->lim - b->wp)
			n = 0;
		b->wp += n;
		if(Chatty)
			dumpdata(b, n);
		t->bp = nil;
		t->ep->nbytes += n;
		t->ep->nblocks++;
		qpass(t->ep->rq, b);	/* TO DO: flow control */
		wakeup(&t->ep->rr);	/* TO DO */
		break;
	case TokSETUP:
		XPRINT("cleanTD: TokSETUP %lux\n", &t->ep);
		/* don't really need to wakeup: subsequent IN or OUT gives status */
		if(t->ep != nil) {
			wakeup(&t->ep->wr);	/* TO DO */
			XPRINT("cleanTD: wakeup %lux\n", &t->ep->wr);
		}
		break;
	case TokOUT:
		/* TO DO: mark it done somewhere */
		XPRINT("cleanTD: TokOut %lux\n", &t->ep);
		if(t->ep != nil){
			if(t->bp){
				n = BLEN(t->bp);
				t->ep->nbytes += n;
				t->ep->nblocks++;
			}
			if(t->ep->x!=0 && err != 0)
				t->ep->err = err==Stalled? Estalled: Eio;
			if(--t->ep->ntd < 0)
				panic("cleantd ntd");
			wakeup(&t->ep->wr);	/* TO DO */
			XPRINT("cleanTD: wakeup %lux\n", &t->ep->wr);
		}
		break;
	}
	freetd(ctlr, t);
}

static void
cleanq(Ctlr *ctlr, QH *q, int discard, int vf)
{
	TD *t, *tp;

	ilock(ctlr);
	tp = nil;
	for(t = q->first; t != nil;){
		XPRINT("cleanq: %8.8lux %8.8lux %8.8lux %8.8lux %8.8lux %8.8lux\n", t->link, t->status, t->dev, t->buffer, t->flags, t->next);
		if(t->status & Active){
			if(t->status & NAKed){
				t->status = (t->status & ~NAKed) | IOC;	/* ensure interrupt next frame */
				tp = t;
				t = t->next;
				continue;
			}
			if(t->flags & CancelTD){
				XPRINT("cancelTD: %8.8lux\n", (ulong)t);
				t->status = (t->status & ~Active) | IOC;	/* ensure interrupt next frame */
				tp = t;
				t = t->next;
				continue;
			}
			tp = t;
			t = t->next;
			continue;
		}
		t->status &= ~IOC;
		if (tp == nil) {
			q->first = t->next;
			if(q->first != nil)
				q->entries = PCIWADDR(q->first);
			else
				q->entries = Terminate;
		} else {
			tp->next = t->next;
			if (t->next != nil)
				tp->link = PCIWADDR(t->next) | vf;
			else
				tp->link = Terminate;
		}
		if (q->last == t)
			q->last = tp;
		iunlock(ctlr);
		cleantd(ctlr, t, discard);
		ilock(ctlr);
		if (tp)
			t = tp->next;
		else
			t = q->first;
		XPRINT("t = %8.8lux\n", t);
		dumpqh(q);
	}
	if(q->first && q->entries != PCIWADDR(q->first)){
		ctlr->usbbogus++;
		q->entries = PCIWADDR(q->first);
	}
	iunlock(ctlr);
}

static void
canceltds(Ctlr *ctlr, QH *q, Endpt *e)
{
	TD *t;

	if(q != nil){
		ilock(ctlr);
		for(t = q->first; t != nil; t = t->next)
			if(t->ep == e)
				t->flags |= CancelTD;
		iunlock(ctlr);
		XPRINT("cancel:\n");
		dumpqh(q);
	}
}

static void
eptcancel(Ctlr *ctlr, Endpt *e)
{
	Endptx *x;

	if(e == nil)
		return;
	x = e->private;
	canceltds(ctlr, x->epq, e);
	canceltds(ctlr, ctlr->ctlq, e);
	canceltds(ctlr, ctlr->bulkq, e);
}

static void
eptactivate(Ctlr *ctlr, Endpt *e)
{
	ilock(&ctlr->activends);
	if(e->active == 0){
		XPRINT("activate 0x%p\n", e);
		e->active = 1;
		e->activef = ctlr->activends.f;
		ctlr->activends.f = e;
	}
	iunlock(&ctlr->activends);
}

static void
eptdeactivate(Ctlr *ctlr, Endpt *e)
{
	Endpt **l;

	/* could be O(1) but not worth it yet */
	ilock(&ctlr->activends);
	if(e->active){
		e->active = 0;
		XPRINT("deactivate 0x%p\n", e);
		for(l = &ctlr->activends.f; *l != e; l = &(*l)->activef)
			if(*l == nil){
				iunlock(&ctlr->activends);
				panic("usb eptdeactivate");
			}
		*l = e->activef;
	}
	iunlock(&ctlr->activends);
}

static void
queueqh(Ctlr *ctlr, QH *qh)
{
	QH *q;

	// See if it's already queued
	for (q = ctlr->recvq->next; q; q = q->hlink)
		if (q == qh)
			return;
	if ((qh->hlink = ctlr->recvq->next) == nil)
		qh->head = Terminate;
	else
		qh->head = PCIWADDR(ctlr->recvq->next) | IsQH;
	ctlr->recvq->next = qh;
	ctlr->recvq->entries = PCIWADDR(qh) | IsQH;
}

static QH*
qxmit(Ctlr *ctlr, Endpt *e, Block *b, int pid)
{
	TD *t;
	int n, vf;
	QH *qh;
	Endptx *x;

	x = e->private;
	if(b != nil){
		n = BLEN(b);
		t = alloctde(ctlr, e, pid, n);
		t->bp = b;
		t->buffer = PCIWADDR(b->rp);
	}else
		t = alloctde(ctlr, e, pid, 0);
	ilock(ctlr);
	e->ntd++;
	iunlock(ctlr);
	if(e->debug) pprint("QTD: %8.8lux n=%ld\n", t, b?BLEN(b): 0);
	vf = 0;
	if(e->x == 0){
		qh = ctlr->ctlq;
		vf = 0;
	}else if((qh = x->epq) == nil || e->mode != OWRITE){
		qh = ctlr->bulkq;
		vf = Vf;
	}
	queuetd(ctlr, qh, t, vf, "qxmit");
	return qh;
}

static QH*
qrcv(Ctlr *ctlr, Endpt *e)
{
	TD *t;
	Block *b;
	QH *qh;
	int vf;
	Endptx *x;

	x = e->private;
	t = alloctde(ctlr, e, TokIN, e->maxpkt);
	b = allocb(e->maxpkt);
	t->bp = b;
	t->buffer = PCIWADDR(b->wp);
	vf = 0;
	if(e->x == 0){
		qh = ctlr->ctlq;
	}else if((qh = x->epq) == nil || e->mode != OREAD){
		qh = ctlr->bulkq;
		vf = Vf;
	}
	queuetd(ctlr, qh, t, vf, "qrcv");
	return qh;
}

static int
usbsched(Ctlr *ctlr, int pollms, ulong load)
{
	int i, d, q;
	ulong best, worst;

	best = 1000000;
	q = -1;
	for (d = 0; d < pollms; d++){
		worst = 0;
		for (i = d; i < NFRAME; i++){
			if (ctlr->frameld[i] + load > worst)
				worst = ctlr->frameld[i] + load;
		}
		if (worst < best){
			best = worst;
			q = d;
		}
	}
	return q;
}

static int
schedendpt(Ctlr *ctlr, Endpt *e)
{
	TD *td;
	Endptx *x;
	uchar *bp;
	int i, id, ix, size, frnum;

	if(!e->iso || e->sched >= 0)
		return 0;

	if (e->active){
		return -1;
	}
	e->off = 0;
	e->sched = usbsched(ctlr, e->pollms, e->maxpkt);
	if(e->sched < 0)
		return -1;

	x = e->private;
	if (x->tdalloc || x->bpalloc)
		panic("usb: tdalloc/bpalloc");
	x->tdalloc = mallocz(0x10 + NFRAME*sizeof(TD), 1);
	x->bpalloc = mallocz(0x10 + e->maxpkt*NFRAME/e->pollms, 1);
	x->td0 = (TD*)(((ulong)x->tdalloc + 0xf) & ~0xf);
	x->bp0 = (uchar *)(((ulong)x->bpalloc + 0xf) & ~0xf);
	frnum = (IN(Frnum) + 1) & 0x3ff;
	frnum = (frnum & ~(e->pollms - 1)) + e->sched;
	x->xtd = &x->td0[(frnum+8)&0x3ff];	/* Next td to finish */
	x->etd = nil;
	e->remain = 0;
	e->nbytes = 0;
	td = x->td0;
	for(i = e->sched; i < NFRAME; i += e->pollms){
		bp = x->bp0 + e->maxpkt*i/e->pollms;
		td->buffer = PCIWADDR(bp);
		td->ep = e;
		td->next = &td[1];
		ctlr->frameld[i] += e->maxpkt;
		td++;
	}
	td[-1].next = x->td0;
	for(i = e->sched; i < NFRAME; i += e->pollms){
		ix = (frnum+i) & 0x3ff;
		td = &x->td0[ix];

		id = (e->x<<7)|(e->dev->x&0x7F);
		if (e->mode == OREAD)
			/* enable receive on this entry */
			td->dev = ((e->maxpkt-1)<<21) | ((id&0x7FF)<<8) | TokIN;
		else{
			size = (e->hz + e->remain)*e->pollms/1000;
			e->remain = (e->hz + e->remain)*e->pollms%1000;
			size *= e->samplesz;
			td->dev = ((size-1)<<21) | ((id&0x7FF)<<8) | TokOUT;
		}
		td->status = ErrLimit1 | Active | IsoSelect | IOC;
		td->link = ctlr->frames[ix];
		td->flags |= IsoClean;
		ctlr->frames[ix] = PCIWADDR(td);
	}
	return 0;
}

static void
unschedendpt(Ctlr *ctlr, Endpt *e)
{
	int q;
	TD *td;
	Endptx *x;
	ulong *addr;

	if(!e->iso || e->sched < 0)
		return;

	x = e->private;
	if (x->tdalloc == nil)
		panic("tdalloc");
	for (q = e->sched; q < NFRAME; q += e->pollms){
		td = x->td0++;
		addr = &ctlr->frames[q];
		while(*addr != PADDR(td)) {
			if(*addr & IsQH)
				panic("usb: TD expected");
			addr = &TFOL(*addr)->link;
		}
		*addr = td->link;
		ctlr->frameld[q] -= e->maxpkt;
	}
	free(x->tdalloc);
	free(x->bpalloc);
	x->tdalloc = nil;
	x->bpalloc = nil;
	x->etd = nil;
	x->td0 = nil;
	e->sched = -1;
}

static void
epalloc(Usbhost *uh, Endpt *e)
{
	Endptx *x;

	x = malloc(sizeof(Endptx));
	e->private = x;
	x->epq = allocqh(uh->ctlr);
	if(x->epq == nil)
		panic("devendptx");
}

static void
epfree(Usbhost *uh, Endpt *e)
{
	Ctlr *ctlr;
	Endptx *x;

	ctlr = uh->ctlr;
	x = e->private;
	if(x->epq != nil)
		freeqh(ctlr, x->epq);
}

static void
epopen(Usbhost *uh, Endpt *e)
{
	Ctlr *ctlr;

	ctlr = uh->ctlr;
	if(e->iso && e->active)
		error("already open");
	if(schedendpt(ctlr, e) < 0){
		if(e->active)
			error("cannot schedule USB endpoint, active");
		else
			error("cannot schedule USB endpoint");
	}
	eptactivate(ctlr, e);
}

static void
epclose(Usbhost *uh, Endpt *e)
{
	Ctlr *ctlr;

	ctlr = uh->ctlr;
	eptdeactivate(ctlr, e);
	unschedendpt(ctlr, e);
}

static void
epmode(Usbhost *uh, Endpt *e)
{
	Ctlr *ctlr;
	Endptx *x;

	ctlr = uh->ctlr;
	x = e->private;
	if(e->iso) {
		if(x->epq != nil) {
			freeqh(ctlr, x->epq);
			x->epq = nil;
		}
	}
	else {
		/* Each bulk device gets a queue head hanging off the
		 * bulk queue head
		 */
		if(x->epq == nil) {
			x->epq = allocqh(ctlr);
			if(x->epq == nil)
				panic("epbulk: allocqh");
		}
		queueqh(ctlr, x->epq);
	}
}

static	int	ioport[] = {-1, Portsc0, Portsc1};

static void
portreset(Usbhost *uh, int port)
{
	int i, p;
	Ctlr *ctlr;

	ctlr = uh->ctlr;
	if(port != 1 && port != 2)
		error(Ebadarg);

	/* should check that device not being configured on other port? */
	p = ioport[port];
	qlock(&ctlr->resetl);
	if(waserror()){
		qunlock(&ctlr->resetl);
		nexterror();
	}
	XPRINT("r: %x\n", IN(p));
	ilock(ctlr);
	OUT(p, PortReset);
	delay(12);	/* BUG */
	XPRINT("r2: %x\n", IN(p));
	OUT(p, IN(p) & ~PortReset);
	XPRINT("r3: %x\n", IN(p));
	OUT(p, IN(p) | PortEnable);
	microdelay(64);
	for(i=0; i<1000 && (IN(p) & PortEnable) == 0; i++)
		;
	XPRINT("r': %x %d\n", IN(p), i);
	OUT(p, (IN(p) & ~PortReset)|PortEnable);
	iunlock(ctlr);
	poperror();
	qunlock(&ctlr->resetl);
}

static void
portenable(Usbhost *uh, int port, int on)
{
	int w, p;
	Ctlr *ctlr;

	ctlr = uh->ctlr;
	if(port != 1 && port != 2)
		error(Ebadarg);

	/* should check that device not being configured on other port? */
	p = ioport[port];
	qlock(&ctlr->resetl);
	if(waserror()){
		qunlock(&ctlr->resetl);
		nexterror();
	}
	ilock(ctlr);
	w = IN(p);
	if(on)
		w |= PortEnable;
	else
		w &= ~PortEnable;
	OUT(p, w);
	microdelay(64);
	iunlock(ctlr);
	XPRINT("e: %x\n", IN(p));
	poperror();
	qunlock(&ctlr->resetl);
}

static void
portinfo(Usbhost *uh, char *s, char *se)
{
	int x, i, j;
	Ctlr *ctlr;

	ctlr = uh->ctlr;
	for(i = 1; i <= 2; i++) {
		ilock(ctlr);
		x = IN(ioport[i]);
		if((x & (PortChange|StatusChange)) != 0)
			OUT(ioport[i], x);
		iunlock(ctlr);
		s = seprint(s, se, "%d %ux", i, x);
		for(j = 0; j < nelem(portstatus); j++) {
			if((x & portstatus[j].bit) != 0)
				s = seprint(s, se, " %s", portstatus[j].name);
		}
		s = seprint(s, se, "\n");
	}
}

static void
cleaniso(Endpt *e, int frnum)
{
	TD *td;
	int id, n, i;
	Endptx *x;
	uchar *bp;

	x = e->private;
	td = x->xtd;
	if (td->status & Active)
		return;
	id = (e->x<<7)|(e->dev->x&0x7F);
	do {
		if (td->status & AnyError)
			XPRINT("usbisoerror 0x%lux\n", td->status);
		n = (td->status + 1) & 0x3ff;
		e->nbytes += n;
		if ((td->flags & IsoClean) == 0)
			e->nblocks++;
		if (e->mode == OREAD){
			e->buffered += n;
			e->poffset += (td->status + 1) & 0x3ff;
			td->offset = e->poffset;
			td->dev = ((e->maxpkt -1)<<21) | ((id&0x7FF)<<8) | TokIN;
			e->toffset = td->offset;
		}else{
			if ((td->flags & IsoClean) == 0){
				e->buffered -= n;
				if (e->buffered < 0){
//					print("e->buffered %d?\n", e->buffered);
					e->buffered = 0;
				}
			}
			e->toffset = td->offset;
			n = (e->hz + e->remain)*e->pollms/1000;
			e->remain = (e->hz + e->remain)*e->pollms%1000;
			n *= e->samplesz;
			td->dev = ((n -1)<<21) | ((id&0x7FF)<<8) | TokOUT;
			td->offset = e->poffset;
			e->poffset += n;
		}
		td = td->next;
		if (x->xtd == td){
			XPRINT("@");
			break;
		}
	} while ((td->status & Active) == 0);
	e->time = todget(nil);
	x->xtd = td;
	for (n = 2; n < 4; n++){
		i = ((frnum + n)&0x3ff);
		td = x->td0 + i;
		bp = x->bp0 + e->maxpkt*i/e->pollms;
		if (td->status & Active)
			continue;

		if (e->mode == OWRITE){
			if (td == x->etd) {
				XPRINT("*");
				memset(bp+e->off, 0, e->maxpkt-e->off);
				if (e->off == 0)
					td->flags |= IsoClean;
				else
					e->buffered += (((td->dev>>21) +1) & 0x3ff) - e->off;
				x->etd = nil;
			}else if ((td->flags & IsoClean) == 0){
				XPRINT("-");
				memset(bp, 0, e->maxpkt);
				td->flags |= IsoClean;
			}
		} else {
			/* Unread bytes are now lost */
			e->buffered -= (td->status + 1) & 0x3ff;
		}
		td->status = ErrLimit1 | Active | IsoSelect | IOC;
	}
	wakeup(&e->wr);
}

static void
interrupt(Ureg*, void *a)
{
	QH *q;
	Ctlr *ctlr;
	Endpt *e;
	Endptx *x;
	int s, frnum;
	Usbhost *uh;

	uh = a;
	ctlr = uh->ctlr;
	s = IN(Status);
	ctlr->frameptr = inl(ctlr->io+Flbaseadd);
	ctlr->framenumber = IN(Frnum) & 0x3ff;
	OUT(Status, s);
	if ((s & 0x1f) == 0)
		return;
	ctlr->usbints++;
	frnum = IN(Frnum) & 0x3ff;
	if (s & 0x1a) {
		XPRINT("cmd #%x sofmod #%x\n", IN(Cmd), inb(ctlr->io+SOFMod));
		XPRINT("sc0 #%x sc1 #%x\n", IN(Portsc0), IN(Portsc1));
	}

	ilock(&ctlr->activends);
	for(e = ctlr->activends.f; e != nil; e = e->activef) {
		x = e->private;
		if(!e->iso && x->epq != nil) {
			XPRINT("cleanq(ctlr, x->epq, 0, 0)\n");
			cleanq(ctlr, x->epq, 0, 0);
		}
		if(e->iso) {
			XPRINT("cleaniso(e)\n");
			cleaniso(e, frnum);
		}
	}
	iunlock(&ctlr->activends);
	XPRINT("cleanq(ctlr, ctlr->ctlq, 0, 0)\n");
	cleanq(ctlr, ctlr->ctlq, 0, 0);
	XPRINT("cleanq(ctlr, ctlr->bulkq, 0, Vf)\n");
	cleanq(ctlr, ctlr->bulkq, 0, Vf);
	XPRINT("clean recvq\n");
	for (q = ctlr->recvq->next; q; q = q->hlink) {
		XPRINT("cleanq(ctlr, q, 0, Vf)\n");
		cleanq(ctlr, q, 0, Vf);
	}
}

static int
eptinput(void *arg)
{
	Endpt *e;

	e = arg;
	return e->eof || e->err || qcanread(e->rq);
}

static int
isoreadyx(Endptx *x)
{
	return x->etd == nil || (x->etd != x->xtd && (x->etd->status & Active) == 0);
}

static int
isoready(void *arg)
{
	int ret;
	Ctlr *ctlr;
	Endpt *e;
	Endptx *x;

	e = arg;
	ctlr = e->dev->uh->ctlr;
	x = e->private;
	ilock(&ctlr->activends);
	ret = isoreadyx(x);
	iunlock(&ctlr->activends);
	return ret;
}

static long
isoio(Ctlr *ctlr, Endpt *e, void *a, long n, ulong offset, int w)
{
	TD *td;
	Endptx *x;
	int i, frnum;
	uchar *p, *q, *bp;
	volatile int isolock;

	x = e->private;
	qlock(&e->rlock);
	isolock = 0;
	if(waserror()){
		if (isolock){
			isolock = 0;
			iunlock(&ctlr->activends);
		}
		qunlock(&e->rlock);
		eptcancel(ctlr, e);
		nexterror();
	}
	p = a;
	if (offset != 0 && offset != e->foffset){
		iprint("offset %lud, foffset %lud\n", offset, e->foffset);
		/* Seek to a specific position */
		frnum = (IN(Frnum) + 8) & 0x3ff;
		td = x->td0 +frnum;
		if (offset < td->offset)
			error("ancient history");
		while (offset > e->toffset){
			tsleep(&e->wr, return0, 0, 500);
		}
		while (offset >= td->offset + ((w?(td->dev >> 21):td->status) + 1) & 0x7ff){
			td = td->next;
			if (td == x->xtd)
				iprint("trouble\n");
		}
		ilock(&ctlr->activends);
		isolock = 1;
		e->off = td->offset - offset;
		if (e->off >= e->maxpkt){
			iprint("I can't program: %d\n", e->off);
			e->off = 0;
		}
		x->etd = td;
		e->foffset = offset;
	}
	do {
		if (isolock == 0){
			ilock(&ctlr->activends);
			isolock = 1;
		}
		td = x->etd;
		if (td == nil || e->off == 0){
			if (td == nil){
				XPRINT("0");
				if (w){
					frnum = (IN(Frnum) + 1) & 0x3ff;
					td = x->td0 + frnum;
					while(td->status & Active)
						td = td->next;
				}else{
					frnum = (IN(Frnum) - 4) & 0x3ff;
					td = x->td0 + frnum;
					while(td->next != x->xtd)
						td = td->next;
				}
				x->etd = td;
				e->off = 0;
			}else{
				/* New td, make sure it's ready */
				while (isoreadyx(x) == 0){
					isolock = 0;
					iunlock(&ctlr->activends);
					sleep(&e->wr, isoready, e);
					ilock(&ctlr->activends);
					isolock = 1;
				}
				if (x->etd == nil){
					XPRINT("!");
					continue;
				}
			}
			if (w)
				e->psize = ((td->dev >> 21) + 1) & 0x7ff;
			else
				e->psize = (x->etd->status + 1) & 0x7ff;
			if(e->psize > e->maxpkt)
				panic("packet size > maximum");
		}
		if((i = n) >= e->psize)
			i = e->psize;
		if (w)
			e->buffered += i;
		else{
			e->buffered -= i;
			if (e->buffered < 0)
				e->buffered = 0;
		}
		isolock = 0;
		iunlock(&ctlr->activends);
		td->flags &= ~IsoClean;
		bp = x->bp0 + (td - x->td0) * e->maxpkt / e->pollms;
		q = bp + e->off;
		if (w){
			memmove(q, p, i);
		}else{
			memmove(p, q, i);
		}
		p += i;
		n -= i;
		e->off += i;
		e->psize -= i;
		if (e->psize){
			if (n != 0)
				panic("usb iso: can't happen");
			break;
		}
		if(w)
			td->offset = offset + (p-(uchar*)a) - (((td->dev >> 21) + 1) & 0x7ff);
		td->status = ErrLimit3 | Active | IsoSelect | IOC;
		x->etd = td->next;
		e->off = 0;
	} while(n > 0);
	n = p-(uchar*)a;
	e->foffset += n;
	poperror();
	if (isolock)
		iunlock(&ctlr->activends);
	qunlock(&e->rlock);
	return n;
}

static long
read(Usbhost *uh, Endpt *e, void *a, long n, vlong offset)
{
	long l, i;
	Block *b;
	Ctlr *ctlr;
	uchar *p;

	ctlr = uh->ctlr;
	if(e->iso)
		return isoio(ctlr, e, a, n, (ulong)offset, 0);

	XPRINT("qlock(%p)\n", &e->rlock);
	qlock(&e->rlock);
	XPRINT("got qlock(%p)\n", &e->rlock);
	if(waserror()){
		qunlock(&e->rlock);
		eptcancel(ctlr, e);
		nexterror();
	}
	p = a;
	do {
		if(e->eof) {
			XPRINT("e->eof\n");
			break;
		}
		if(e->err)
			error(e->err);
		qrcv(ctlr, e);
		if(!e->iso)
			e->data01 ^= 1;
		sleep(&e->rr, eptinput, e);
		if(e->err)
			error(e->err);
		b = qget(e->rq);	/* TO DO */
		if(b == nil) {
			XPRINT("b == nil\n");
			break;
		}
		if(waserror()){
			freeb(b);
			nexterror();
		}
		l = BLEN(b);
		if((i = l) > n)
			i = n;
		if(i > 0){
			memmove(p, b->rp, i);
			p += i;
		}
		poperror();
		freeb(b);
		n -= i;
		if (l != e->maxpkt)
			break;
	} while (n > 0);
	poperror();
	qunlock(&e->rlock);
	return p-(uchar*)a;
}

static int
qisempty(void *arg)
{
	return ((QH*)arg)->entries & Terminate;
}

static long
write(Usbhost *uh, Endpt *e, void *a, long n, vlong offset, int tok)
{
	int i, j;
	QH *qh;
	Block *b;
	Ctlr *ctlr;
	uchar *p;

	ctlr = uh->ctlr;
	if(e->iso)
		return isoio(ctlr, e, a, n, (ulong)offset, 1);

	p = a;
	qlock(&e->wlock);
	if(waserror()){
		qunlock(&e->wlock);
		eptcancel(ctlr, e);
		nexterror();
	}
	do {
		if(e->err)
			error(e->err);
		if((i = n) >= e->maxpkt)
			i = e->maxpkt;
		b = allocb(i);
		if(waserror()){
			freeb(b);
			nexterror();
		}
		XPRINT("out [%d]", i);
		for (j = 0; j < i; j++) XPRINT(" %.2x", p[j]);
		XPRINT("\n");
		memmove(b->wp, p, i);
		b->wp += i;
		p += i;
		n -= i;
		poperror();
		qh = qxmit(ctlr, e, b, tok);
		tok = TokOUT;
		e->data01 ^= 1;
		if(e->ntd >= e->nbuf) {
XPRINT("qh %s: q=%p first=%p last=%p entries=%.8lux\n",
 "writeusb sleep", qh, qh->first, qh->last, qh->entries);
			XPRINT("write: sleep %lux\n", &e->wr);
			sleep(&e->wr, qisempty, qh);
			XPRINT("write: awake\n");
		}
	} while(n > 0);
	poperror();
	qunlock(&e->wlock);
	return p-(uchar*)a;
}

static void
init(Usbhost* uh)
{
	Ctlr *ctlr;

	ctlr = uh->ctlr;
	ilock(ctlr);
	outl(ctlr->io+Flbaseadd, PCIWADDR(ctlr->frames));
	OUT(Frnum, 0);
	OUT(Usbintr, 0xF);	/* enable all interrupts */
	XPRINT("cmd 0x%x sofmod 0x%x\n", IN(Cmd), inb(ctlr->io+SOFMod));
	XPRINT("sc0 0x%x sc1 0x%x\n", IN(Portsc0), IN(Portsc1));
	if((IN(Cmd)&1)==0)
		OUT(Cmd, 1);	/* run */
//	pprint("at: c=%x s=%x c0=%x\n", IN(Cmd), IN(Status), IN(Portsc0));
	iunlock(ctlr);
}

static void
scanpci(void)
{
	int io;
	Ctlr *ctlr;
	Pcidev *p;
	static int already = 0;

	if(already)
		return;
	already = 1;
	p = nil;
	while(p = pcimatch(p, 0, 0)) {
		/*
		 * Find UHCI controllers.  Class = 12 (serial controller),
		 * Sub-class = 3 (USB) and Programming Interface = 0.
		 */
		if(p->ccrb != 0x0C || p->ccru != 0x03 || p->ccrp != 0x00)
			continue;
		io = p->mem[4].bar & ~0x0F;
		if(io == 0) {
			print("usbuhci: failed to map registers\n");
			continue;
		}
		if(ioalloc(io, p->mem[4].size, 0, "usbuhci") < 0){
			print("usbuhci: port %d in use\n", io);
			continue;
		}
		if(p->intl == 0xFF || p->intl == 0) {
			print("usbuhci: no irq assigned for port %d\n", io);
			continue;
		}

		XPRINT("usbuhci: %x/%x port 0x%ux size 0x%x irq %d\n",
			p->vid, p->did, io, p->mem[4].size, p->intl);

		ctlr = malloc(sizeof(Ctlr));
		ctlr->pcidev = p;
		ctlr->io = io;
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
	int i;
	TD *t;
	ulong io;
	Ctlr *ctlr;
	Pcidev *p;

	scanpci();

	/*
	 * Any adapter matches if no uh->port is supplied,
	 * otherwise the ports must match.
	 */
	for(ctlr = ctlrhead; ctlr != nil; ctlr = ctlr->next){
		if(ctlr->active)
			continue;
		if(uh->port == 0 || uh->port == ctlr->io){
			ctlr->active = 1;
			break;
		}
	}
	if(ctlr == nil)
		return -1;

	io = ctlr->io;
	p = ctlr->pcidev;

	uh->ctlr = ctlr;
	uh->port = io;
	uh->irq = p->intl;
	uh->tbdf = p->tbdf;

	XPRINT("usbcmd\t0x%.4x\nusbsts\t0x%.4x\nusbintr\t0x%.4x\nfrnum\t0x%.2x\n",
		IN(Cmd), IN(Status), IN(Usbintr), inb(io+Frnum));
	XPRINT("frbaseadd\t0x%.4x\nsofmod\t0x%x\nportsc1\t0x%.4x\nportsc2\t0x%.4x\n",
		IN(Flbaseadd), inb(io+SOFMod), IN(Portsc0), IN(Portsc1));

	OUT(Cmd, 0);					/* stop */
	while((IN(Status) & (1<<5)) == 0)	/* wait for halt */
		;
	OUT(Status, 0xFF);				/* clear pending interrupts */
	pcicfgw16(p, 0xc0, 0x2000);		/* legacy support register: turn off lunacy mode */

	if(0){
		i = inb(io+SOFMod);
		OUT(Cmd, 4);	/* global reset */
		delay(15);
		OUT(Cmd, 0);	/* end reset */
		delay(4);
		outb(io+SOFMod, i);
	}

	ctlr->tdpool = xspanalloc(128*sizeof(TD), 16, 0);
	for(i=128; --i>=0;){
		ctlr->tdpool[i].next = ctlr->freetd;
		ctlr->freetd = &ctlr->tdpool[i];
	}
	ctlr->qhpool = xspanalloc(64*sizeof(QH), 16, 0);
	for(i=64; --i>=0;){
		ctlr->qhpool[i].next = ctlr->freeqh;
		ctlr->freeqh = &ctlr->qhpool[i];
	}

	/*
	 * the last entries of the periodic (interrupt & isochronous) scheduling TD entries
	 * points to the control queue and the bandwidth sop for bulk traffic.
	 * this is looped following the instructions in PIIX4 errata 29773804.pdf:
	 * a QH links to a looped but inactive TD as its sole entry,
	 * with its head entry leading on to the bulk traffic, the last QH of which
	 * links back to the empty QH.
	 */
	ctlr->ctlq = allocqh(ctlr);
	ctlr->bwsop = allocqh(ctlr);
	ctlr->bulkq = allocqh(ctlr);
	ctlr->recvq = allocqh(ctlr);
	t = alloctd(ctlr);	/* inactive TD, looped */
	t->link = PCIWADDR(t);
	ctlr->bwsop->entries = PCIWADDR(t);

	ctlr->ctlq->head = PCIWADDR(ctlr->bulkq) | IsQH;
	ctlr->bulkq->head = PCIWADDR(ctlr->recvq) | IsQH;
	ctlr->recvq->head = PCIWADDR(ctlr->bwsop) | IsQH;
	if (1)	/* don't use loop back */
 		ctlr->bwsop->head = Terminate;
	else	/* set up loop back */
		ctlr->bwsop->head = PCIWADDR(ctlr->bwsop) | IsQH;

	ctlr->frames = xspanalloc(FRAMESIZE, FRAMESIZE, 0);
	ctlr->frameld = xallocz(FRAMESIZE, 1);
	for (i = 0; i < NFRAME; i++)
		ctlr->frames[i] = PCIWADDR(ctlr->ctlq) | IsQH;

	/*
	 * Linkage to the generic USB driver.
	 */
	uh->init = init;
	uh->interrupt = interrupt;

	uh->portinfo = portinfo;
	uh->portreset = portreset;
	uh->portenable = portenable;

	uh->epalloc = epalloc;
	uh->epfree = epfree;
	uh->epopen = epopen;
	uh->epclose = epclose;
	uh->epmode = epmode;

	uh->read = read;
	uh->write = write;

	return 0;
}

void
usbuhcilink(void)
{
	addusbtype("uhci", reset);
}
