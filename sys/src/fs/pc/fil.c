#include "all.h"
#include "mem.h"

#define DBG if(0) print

typedef struct Fchan Fchan;
typedef struct Ilhdr Ilhdr;

enum
{

	Ilsync		= 0,		/* Packet type */
	Ildata,
	Ildataquery,
	Ilack,
	Ilquerey,
	Ilstate,
	Ilclose,

	Ilclosed	= 0,		/* Connection state */
	Ilsyncer,
	Ilsyncee,
	Ilestablished,
	Illistening,
	Ilclosing,
	Ilopening,

	Iltickms 	= 100,
	Slowtime 	= 200*Iltickms,
	Fasttime 	= 4*Iltickms,
	Acktime		= 2*Iltickms,
	Ackkeepalive	= 6000*Iltickms,
	Querytime	= 60*Iltickms,		/* time between queries */
	Keepalivetime	= 10*Querytime,		/* keep alive time */
	Defaultwin	= 300,
	ILgain		= 8,

	QSIZE		= 1024*1024,		/* Maximum readq */
	QWINSIZE	= 128*1024,		/* Initial window size */
};

struct Ilhdr
{
	uchar	dst[4];		/* Ip destination */
	uchar	src[4];		/* Ip source */
	uchar	illen[2];	/* Packet length */
	uchar	iltype;		/* Packet type */
	uchar	ilspec;		/* Special */
	uchar	ilsrc[2];	/* Src port */
	uchar	ildst[2];	/* Dst port */
	uchar	ilid[4];	/* Sequence id */
	uchar	ilack[4];	/* Acked sequence */
	uchar	ilwin[4];	/* Flow control window */
};

enum
{
	FIL_LISTEN	= 1,
	FIL_CONNECT	= 2,
	FIL_HDRSIZE	= 32,
};

char*	iltype[] = 
{	
	"sync",
	"data",
	"dataquerey",
	"ack",
	"querey",
	"state",
	"close" 
};

char*	ilstates[] = 
{ 
	"Closed",
	"Syncer",
	"Syncee",
	"Established",
	"Listening",
	"Closing" 
};

static	int	nextport = 5000;
static	Lock	fillock;
static	int	initseq;
static	Rendez	filrend;

#define IP(v)		(v>>24), (v>>16)&0xff, (v>>8)&0xff, v&0xff
#define nil		((void*)0)

static	void	filsendctl(Chan*, Ilhdr*, int, ulong, ulong);
static	void	filfreeq(Fchan*);
static	void	filhangup(Chan*, char*);
static	void	filprocess(Chan*, Ilhdr*, Msgbuf*);

struct
{
	Lock;
	Chan*	ilp;
}fil;

int
qwindow(Queue *q)
{
	USED(q);
	return QSIZE /* - (q->count*MAXMSG) */;
}

void
filtimers(Fchan *s)
{
	s->timeout = 0;
	s->fasttime = (Fasttime*s->rtt)/Iltickms;
	s->slowtime = (Slowtime*s->rtt)/Iltickms;
}

static
void
filpinit(Fchan *ilp)
{
	ilp->state = Ilopening;
	ilp->unacked = 0;
	ilp->unackedtail = 0;
	ilp->outoforder = 0;
	ilp->recvd = 0;
	ilp->next = 0;
	ilp->start = 0;
	ilp->rstart = 0;
	ilp->slowtime = 0;
	ilp->acktime = 0;
	ilp->rtt = 0;
	ilp->rttack = 0;
	ilp->ackms = 0;
	ilp->window = 0;
	ilp->querytime = Keepalivetime;
	ilp->deathtime = Keepalivetime;

	filtimers(ilp);
}

Chan *
getchan(Ifc *ic, Ilhdr *h)
{
	Chan *cp, *xcp;
	int lport, rport;
	ulong raddr, laddr;

	rport = nhgets(h->ilsrc);
	lport = nhgets(h->ildst);

	raddr = nhgetl(h->src);
	laddr = nhgetl(h->dst);

	xcp = 0;
	for(cp = fil.ilp; cp; cp = cp->filp.link) {
		if(cp->filp.alloc == 0) {
			xcp = cp;
			continue;
		}
		if(rport == cp->filp.rport)
		if(lport == cp->filp.lport)
		if(laddr == cp->filp.laddress)
		if(raddr == cp->filp.raddress)
			return cp;
	}

	if(xcp && xcp->filp.alloc != 0)
		xcp = 0;
	cp = xcp;
	if(cp == 0) {
		cp = chaninit(Devfil, 1);
		cp->filp.link = fil.ilp;
		fil.ilp = cp;
	}

	cp->filp.lport = lport;
	cp->filp.rport = rport;
	cp->filp.raddress = raddr;
	cp->filp.laddress = laddr;
	cp->filp.ifaddr = ifroute(raddr);

	filpinit(&cp->filp);

	cp->send = serveq;
	cp->reply = ic->reply;
	cp->whotime = 0;
	sprint(cp->whochan, "fil!%I!%d", h->src, rport);

	print("fil: allocating %s\n", cp->whochan);
	cp->filp.alloc = 1;
	return cp;
}

void
filackq(Fchan *ic, Msgbuf *bp)
{
	lock(&ic->ackq);
	if(ic->unacked)
		ic->unackedtail->next = bp;
	else {
		/* Start timer since we may have been idle for some time */
		filtimers(ic);
		ic->unacked = bp;
	}
	ic->unackedtail = bp;
	ic->unackedbytes += bp->count;
	bp->next = nil;
	unlock(&ic->ackq);
}

static int
txflowctl(void *a)
{
	Fchan *ic;
	ic = a;
	return ic->sndwindow > ic->need;
}

void
filoput(Msgbuf *bp)
{
	int n;
	Fchan *ic;
	ulong id;
	Ilhdr *ih;
	Chan *cp;

	cp = bp->chan;
 	ic = &cp->filp;
	n = bp->count;

	qlock(&ic->txq);
	if(ic->sndwindow < n) {
		ic->need = n;
		sleep(&ic->flowr, txflowctl, ic);
	}

	switch(ic->state) {
	case Ilclosed:
	case Illistening:
	case Ilclosing:
		mbfree(bp);
		print("fil: send on closed channel\n");
		return;
	}

	id = ic->next++;
	ic->sndwindow -= n;
	qunlock(&ic->txq);

	bp->data -= FIL_HDRSIZE;		/* make room for header */
	bp->count += FIL_HDRSIZE;
	if(bp->data < bp->xdata)
		panic("filoput: no room for header");

	ih = (Ilhdr*)bp->data;

	hnputl(ih->dst, ic->raddress);
	hnputl(ih->src, ic->laddress);
	hnputs(ih->illen, n+FIL_HDRSIZE);
	hnputs(ih->ilsrc, ic->lport);
	hnputs(ih->ildst, ic->rport);
	hnputl(ih->ilwin, qwindow(cp->send));
	hnputl(ih->ilid, id);
	hnputl(ih->ilack, ic->recvd);
	ih->iltype = Ildata;
	ih->ilspec = 0;

	filackq(ic, bp);

	/*
	 * Start the round trip timer for this packet if the timer is free
	 */
	if(ic->rttack == 0) {
		ic->rttack = id;
		ic->ackms = MACHP(0)->ticks;
	}
	ic->acktime = Ackkeepalive;

	ifwrite(ic->ifaddr, bp, 0);
}

static void
filsendctl(Chan *cp, Ilhdr *inih, int type, ulong id, ulong ack)
{
	void *ifa;
	Ilhdr *ih;
	Msgbuf *bp;
	Fchan *ic;

	ic = &cp->filp;

	bp = mballoc(LARGEBUF, 0, Mbfil);
	if(bp == nil)
		return;

	bp->count = FIL_HDRSIZE;

	ih = (Ilhdr*)(bp->data);

	hnputs(ih->illen, FIL_HDRSIZE);
	if(inih != nil) {
		hnputl(ih->dst, nhgetl(inih->src));
		hnputs(ih->ilsrc, nhgets(inih->ildst));
		hnputs(ih->ildst, nhgets(inih->ilsrc));
		hnputl(ih->ilid, nhgetl(inih->ilack));
		hnputl(ih->ilack, nhgetl(inih->ilid));
		hnputl(ih->ilwin, QSIZE);
	}
	else {
		hnputl(ih->dst, ic->raddress);
		hnputs(ih->ilsrc, ic->lport);
		hnputs(ih->ildst, ic->rport);
		hnputl(ih->ilid, id);
		hnputl(ih->ilack, ack);
		hnputl(ih->ilwin, qwindow(cp->send));

		ic->acktime = Ackkeepalive;
	}

	if(ic != nil) {
		ifa = ic->ifaddr;
		if(ic->laddress == 0)
			ic->laddress = ifaddr(ifa);

		hnputl(ih->src, ic->laddress);
	}
	else {
		ifa = ifroute(nhgetl(inih->dst));
		hnputl(ih->src, ifaddr(ifa));
	}
	
	ih->iltype = type;
	ih->ilspec = 0;

	DBG("ctl(%s id %d ack %d %d.%d.%d.%d %d->%d.%d.%d.%d %d)\n",
		iltype[ih->iltype],
		nhgetl(ih->ilid),
		nhgetl(ih->ilack),
		ih->src[0], ih->src[1], ih->src[2], ih->src[3],
		nhgets(ih->ilsrc),
		ih->dst[0], ih->dst[1], ih->dst[2], ih->dst[3],
		nhgets(ih->ildst));

	ifwrite(ifa, bp, 1);
}

static void
filpullup(Chan *c)
{
	Ilhdr *oh;
	ulong oid;
	Fchan *ic;
	Msgbuf *bp;

	ic = &c->filp;

	if(ic->state != Ilestablished)
		return;

	lock(&ic->outo);
	while(ic->outoforder) {
		bp = ic->outoforder;
		oh = (Ilhdr*)(bp->data);
		oid = nhgetl(oh->ilid);
		if(oid <= ic->recvd) {
			ic->outoforder = bp->next;
			mbfree(bp);
			continue;
		}
		if(oid != ic->recvd+1)
			break;

		ic->recvd = oid;
		ic->outoforder = bp->next;

		/*
		 * strip off the header
		 */
		bp->count = nhgets(oh->illen)-FIL_HDRSIZE;
		bp->data += FIL_HDRSIZE;
		send(c->send, bp);
	}
	unlock(&ic->outo);
}

static void
filoutoforder(Fchan *ic, Ilhdr *h, Msgbuf *bp)
{
	ulong id;
	uchar *lid;
	Msgbuf *f, **l;

	bp->next = nil;

	id = nhgetl(h->ilid);
	if(id <= ic->recvd || id > ic->recvd+ic->window) {
		mbfree(bp);
		return;
	}

	/* Packet is acceptable so sort onto receive queue for pullup */
	lock(&ic->outo);
	if(ic->outoforder == nil)
		ic->outoforder = bp;
	else {
		l = &ic->outoforder;
		for(f = *l; f; f = f->next) {
			lid = ((Ilhdr*)(f->data))->ilid;
			if(id < nhgetl(lid)) {
				bp->next = f;
				*l = bp;
				unlock(&ic->outo);
				return;
			}
			l = &f->next;
		}
		*l = bp;
	}
	unlock(&ic->outo);
}

static void
filackto(Fchan *ic, ulong ackto)
{
	Ilhdr *h;
	Msgbuf *bp;
	ulong id, t;

	if(ic->rttack == ackto) {
		t = MACHP(0)->ticks - ic->ackms;
		/* Guard against the ulong zero wrap of MACP->ticks */
		if(t < 100*ic->rtt)
			ic->rtt = (ic->rtt*(ILgain-1)+t)/ILgain;
		if(ic->rtt < Iltickms)
			ic->rtt = Iltickms;
	}

	/* Cancel if we lost the packet we were interested in */
	if(ic->rttack <= ackto)
		ic->rttack = 0;

	lock(&ic->ackq);
	while(ic->unacked) {
		h = (Ilhdr *)ic->unacked->data;
		id = nhgetl(h->ilid);
		if(ackto < id)
			break;

		bp = ic->unacked;
		ic->unackedbytes -= bp->count;
		ic->unacked = bp->next;
		mbfree(bp);
	}
	unlock(&ic->ackq);
}

static void
filrexmit(Fchan *ic)
{
	int len;
	Ilhdr *h;
	Msgbuf *nb, *bp;

	nb = nil;
	lock(&ic->ackq);
	bp = ic->unacked;
	if(bp != nil) {
		len = bp->count;
		nb = mballoc(LARGEBUF, 0, Mbfil);
		if(nb != nil) {
			memmove(nb->data, bp->data, len);
			nb->count = len;
		}
	}
	unlock(&ic->ackq);

	if(nb == nil)
		return;

	h = (Ilhdr*)nb->data;
	DBG("rxmit %d.", nhgetl(h->ilid));

	h->iltype = Ildataquery;
	hnputl(h->ilack, ic->recvd);

	ifwrite(ic->ifaddr, nb, 1);
}

static void
filfreeq(Fchan *ic)
{
	Msgbuf *bp, *next;

	lock(&ic->ackq);
	for(bp = ic->unacked; bp; bp = next) {
		next = bp->next;
		mbfree(bp);
	}
	ic->unacked = nil;
	ic->unackedbytes = 0;
	unlock(&ic->ackq);

	lock(&ic->outo);
	for(bp = ic->outoforder; bp; bp = next) {
		next = bp->next;
		mbfree(bp);
	}
	ic->outoforder = nil;
	unlock(&ic->outo);
}

static void
filhangup(Chan *cp, char *msg)
{
	Fchan *ic;
	Rendez *r;
	int callout;

	ic = &cp->filp;

	if(msg == nil)
		msg = "close";

	print("fil: hangup '%s' %d.%d.%d.%d!%d\n",
			msg, IP(ic->raddress), ic->rport);

	callout = ic->state == Ilsyncer;
	ic->state = Ilclosed;

	wakeup(&ic->flowr);

	ic->msg = msg;
	filfreeq(ic);

	fileinit(cp);
	cp->whotime = 0;
	strcpy(cp->whoname, "<none>");

	if(callout) {
		if(msg == nil)
			msg = "hungup";
		ic->msg = msg;
		r = ic->cr;
		if(r != nil)
			wakeup(r);
	}
	ic->lport = 0;
	ic->rport = 0;
	ic->alloc = 0;
}

static void
_filprocess(Chan *c, Ilhdr *h, Msgbuf *bp)
{
	int s;
	Fchan *ic;
	ulong id, ack;

	ic = &c->filp;

	id = nhgetl(h->ilid);
	ack = nhgetl(h->ilack);

	ic->querytime = Keepalivetime;
	ic->deathtime = Keepalivetime;

	switch(ic->state) {
	default:
		print("fil: unknown state");
	case Ilclosed:
	case Illistening:
		break;
	case Ilsyncer:
		switch(h->iltype) {
		default:
			break;
		case Ilsync:
			if(ack != ic->start) {
				filhangup(c, "connection rejected");
				break;
			}
			ic->recvd = id;
			ic->rstart = id;
			filsendctl(c, nil, Ilack, ic->next, ic->recvd);
			ic->state = Ilestablished;
			wakeup(ic->cr);
			filpullup(c);
			filtimers(ic);
			break;
		case Ilclose:
			if(ack == ic->start)
				filhangup(c, "connection rejected");
			break;
		}
		break;
	case Ilsyncee:
		switch(h->iltype) {
		default:
			break;
		case Ilsync:
			if(id != ic->rstart || ack != 0) {
				ic->state = Ilclosed;
				break;
			}
			ic->recvd = id;
			filsendctl(c, nil, Ilsync, ic->start, ic->recvd);
			filtimers(ic);
			break;
		case Ilack:
			if(ack != ic->start)
				break;
			ic->state = Ilestablished;
			filpullup(c);
			filtimers(ic);
			filsendctl(c, nil, Ilack, ic->start, ic->recvd);
			break;
		case Ilclose:
			if(id == ic->next)
				filhangup(c, "remote close");
			break;
		}
		break;
	case Ilclosing:
	case Ilestablished:
		switch(h->iltype) {
		case Ilsync:
			if(ic->state == Ilclosing)
				break;
			if(id != ic->rstart) {
				filhangup(c, "remote close");
				break;
			}
			filsendctl(c, nil, Ilack, ic->next, ic->rstart);
			filtimers(ic);
			break;
		case Ildata:
			filtimers(ic);
			filackto(ic, ack);
			ic->acktime = Acktime;
			filoutoforder(ic, h, bp);
			bp = 0;
			filpullup(c);
			break;
		case Ildataquery:
			filtimers(ic);
			filackto(ic, ack);
			filoutoforder(ic, h, bp);
			bp = 0;
			filpullup(c);
			filsendctl(c, nil, Ilstate, ic->next, ic->recvd);
			break;
		case Ilack:
			filackto(ic, ack);
			filtimers(ic);
			break;
		case Ilquerey:
			filackto(ic, ack);
			filsendctl(c, nil, Ilstate, ic->next, ic->recvd);
			filtimers(ic);
			break;
		case Ilstate:
			filackto(ic, ack);
			filrexmit(ic);
			filtimers(ic);
			ic->acktime = Acktime;
			break;
		case Ilclose:
			if(ic->state == Ilclosing) {
				ic->recvd = id;
				filsendctl(c, nil, Ilclose, ic->next, ic->recvd);
				if(ack == ic->next)
					filhangup(c, nil);
				filtimers(ic);
				break;
			}
			if(ack < ic->start || ack > ic->next) 
				break;
			filsendctl(c, nil, Ilclose, ic->next, ic->recvd);
			ic->state = Ilclosing;
			filfreeq(ic);
			filtimers(ic);
			break;
		}
		break;
	}

	s = nhgetl(h->ilwin) - ic->unackedbytes;
	if(s < 0)
		s = 0;
	ic->sndwindow = s;
	if(ic->need && s >= ic->need) {
		ic->need = 0;
		wakeup(&ic->flowr);
	}

	if(bp != 0)
		mbfree(bp);
}

void
filprocess(Chan *c, Ilhdr *h, Msgbuf *bp)
{
	DBG("%11s rcv %d/%d snt %d/%d pkt(%s id %d ack %d %d->%d) ",
	ilstates[c->filp.state],  c->filp.rstart, c->filp.recvd,
	c->filp.start, c->filp.next, iltype[h->iltype], nhgetl(h->ilid), 
		nhgetl(h->ilack), nhgets(h->ilsrc), nhgets(h->ildst));

	_filprocess(c, h, bp);

	DBG("* %s rcv %d snt %d\n", ilstates[c->filp.state],
		c->filp.recvd, c->filp.next);
}

void
filrecv(Msgbuf *mb, Ifc *ifc)
{
	Chan *cp;
	int plen;
	Ilhdr *ih;
	Fchan *ilp;

	ih = (Ilhdr*)mb->data;
	plen = mb->count;
	if(plen < FIL_HDRSIZE) 
		goto drop;

	if(nhgets(ih->illen) > plen)
		goto drop;

	cp = getchan(ifc, ih);
	mb->chan = cp;
	ilp = &cp->filp;

	if(ilp->state != Ilopening) {
		filprocess(cp, ih, mb);
		return;
	}

	if(ilp->lport != Ilfsport) {
		print("fil: not service port %I!%d\n", ih->src, ilp->lport);
		if(ih->iltype != Ilclose)
			filsendctl(cp, ih, Ilclose, 0, 0);
		goto drop;
	}
	if(ih->iltype != Ilsync) {
		print("fil: open not sync %I!%d\n", ih->src, ilp->rport);
		if(ih->iltype != Ilclose)
			filsendctl(cp, ih, Ilclose, 0, 0);
		goto drop;
	}
	ilp->state = Ilsyncee;
	ilp->start = (toytime() * 80021) & 0x3fffffffUL;
	ilp->next = ilp->start+1;
	ilp->recvd = 0;
	ilp->rstart = nhgetl(ih->ilid);
	ilp->slowtime = Slowtime;
	ilp->rtt = Iltickms;
	ilp->window = Defaultwin;
	ilp->querytime = Keepalivetime;
	ilp->deathtime = Keepalivetime;
	filprocess(cp, ih, mb);
	return;
drop:
	mbfree(mb);
}	

void
filbackoff(Fchan *ic)
{
	if(ic->fasttime < Slowtime/2)
		ic->fasttime += Fasttime;
	else
		ic->fasttime = (ic->fasttime)*3/2;
}

static void
ftimecon(Chan *c)
{
	Fchan *ic;
	static char etime[] = "connection timed out";

	ic = &c->filp;

	ic->timeout += Iltickms;
	switch(ic->state) {
	case Ilclosing:
		if(ic->timeout >= ic->slowtime)
			filhangup(c, nil);
		if(ic->timeout >= ic->fasttime) {
			filbackoff(ic);
			if(ic->unacked != nil) {
				filrexmit(ic);
				break;
			}
			filsendctl(c, nil, Ilclose, ic->next, ic->recvd);
		}
		break;
	case Ilsyncee:
	case Ilsyncer:
		if(ic->timeout >= ic->fasttime) {
			filsendctl(c, nil, Ilsync, ic->start, ic->recvd);
			filbackoff(ic);
		}
		if(ic->timeout >= ic->slowtime)
			filhangup(c, etime);
		break;
	case Ilestablished:
		ic->acktime -= Iltickms;
		if(ic->acktime <= 0)
			filsendctl(c, nil, Ilack, ic->next, ic->recvd);

		ic->querytime -= Iltickms;
		if(ic->querytime <= 0) {
			ic->deathtime -= Querytime;
			if(ic->deathtime < 0) {
				filhangup(c, etime);
				break;
			}
			filsendctl(c, nil, Ilquerey, ic->next, ic->recvd);
			ic->querytime = Querytime;
		}
		if(ic->unacked == nil) {
			ic->timeout = 0;
			break;
		}
		if(ic->timeout >= ic->fasttime) {
			filrexmit(ic);
			filbackoff(ic);
		}
		if(ic->timeout >= ic->slowtime) {
			filhangup(c, etime);
			break;
		}
		break;
	case Illistening:
		break;
	}
}

void
filproc(void)
{
	Chan *c;
	static	Rendez	filt;

	for(;;) {
		tsleep(&filt, no, 0, Iltickms);

		for(c = fil.ilp; c; c = c->filp.link)
			if(c->filp.alloc && c->filp.state != Ilclosed)
				ftimecon(c);
	}
}
