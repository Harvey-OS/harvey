#include "all.h"
#include "mem.h"

#include "../ip/ip.h"

#define	DEBUG	if(cons.flags&ilflag)print
#define	msec	(MACHP(0)->ticks * (1000/HZ))

enum
{
	Ilsync		= 0,		/* Packet types */
	Ildata,
	Ildataquery,
	Ilack,
	Ilquery,
	Ilstate,
	Ilclose,

	Ilclosed	= 0,		/* Connection state */
	Ilsyncer,
	Ilsyncee,
	Ilestablished,
	Illistening,
	Ilclosing,
	Ilopening,

	Seconds		= 1000,
	Iltickms 	= 50,			/* time base */
	AckDelay	= (ulong)(2*Iltickms),	/* max time twixt message rcvd & ack sent */
	MaxTimeout 	= (ulong)(4*Seconds),	/* max time between rexmit */
	QueryTime	= (ulong)(10*Seconds),	/* time between subsequent queries */
	DeathTime	= (ulong)(30*QueryTime),

	MaxRexmit 	= 16,		/* max retransmissions before hangup */
	DefWin		= 20,

	LogAGain	= 3,
	AGain		= 1<<LogAGain,
	LogDGain	= 2,
	DGain		= 1<<LogDGain,

	DefByteRate	= 100,		/* assume a megabit link */
	DefRtt		= 50,		/* cross country on a great day */
};

static
struct
{
	Lock;
	Queue*	reply;
	Chan*	chan;
} il;

static	void	ilout(void);
static	void	ilprocess(Chan*, Msgbuf*);
static	Chan*	getchan(Ifc*, Ilpkt*, Msgbuf*);
static	void	ilhangup(Chan*, char*, int);
static	void	ilsendctl(Chan*, Ilpkt*, int, ulong, ulong, int);
static	void	ilpullup(Chan*);
static	void	ilackto(Chan*, ulong, Msgbuf*);
static	void	ilrexmit(Ilp*);
static	void	iloutoforder(Chan*, Ilpkt*, Msgbuf*);
static	void	ilfreeq(Chan*);
static	void	ilackq(Chan*, Msgbuf*);
static	void	ilbackoff(Ilp*);
static	void	ilsettimeout(Ilp*);
static	int	ilnextqt(Ilp*);
static	void	iltimer(void);
static void	ilgoaway(Msgbuf*, Ifc*);

static
char*	ilstate[] =
{
	"Closed",
	"Syncer",
	"Syncee",
	"Established",
	"Listening",
	"Closing",
	"Opening",
};
static
char*	iltype[] =
{
	"sync",
	"data",
	"dataquery",
	"ack",
	"query",
	"state",
	"close",
};
static	Rendez	ild;
static	int	ilflag;

static void
ilwhoprint(Chan* cp)
{
	Ilp *ilp;
	ulong t;

	if(cp->type != Devil)
		return;

	ilp = cp->pdata;
	t = MACHP(0)->ticks * (1000/HZ);
	print(" (%d,%d)", ilp->alloc, ilp->state);
	print(" (%ld,%ld,%ld)",
		ilp->timeout-t, ilp->querytime-t,
		ilp->lastrecv-t);
	print(" (%ld,%ld,%ld,%ld)", ilp->rate, ilp->delay,
		ilp->mdev, ilp->unackedbytes);
}

static
void
ilpinit(Ilp *ilp)
{
	ilp->start = (toytime() * 80021) & 0x3fffffffUL;
	ilp->next = ilp->start + 1;
	ilp->rstart = 0;
	ilp->recvd = 0;
	ilp->window = DefWin;
	ilp->unackedbytes = 0;
	ilp->unacked = nil;
	ilp->unackedtail = nil;
	ilp->outoforder = nil;
	ilp->rexmit = 0;

	/* timers */
	ilp->delay = DefRtt<<LogAGain;
	ilp->mdev = DefRtt<<LogDGain;
	ilp->rate = DefByteRate<<LogAGain;
	ilp->querytime = msec + QueryTime;
	ilp->lastrecv = msec;			/* to avoid immediate timeout */
	ilsettimeout(ilp);
}

static
Chan*
getchan(Ifc *ifc, Ilpkt *p, Msgbuf *mb)
{
	Ilp *ilp;
	Chan *cp, *xcp;
	int srcp, dstp;

	srcp = nhgets(p->ilsrc);
	dstp = nhgets(p->ildst);

	lock(&il);
	xcp = 0;
	for(cp = il.chan; cp; cp = ilp->chan) {
		ilp = cp->pdata;
		if(ilp->alloc == 0) {
			xcp = cp;
			continue;
		}
		if(srcp == ilp->srcp)
		if(dstp == ilp->dstp)
		if(memcmp(p->src, ilp->iphis, Pasize) == 0)
		if(memcmp(p->dst, ifc->ipa, Pasize) == 0){
			unlock(&il);
			return cp;
		}
	}

	if(il.reply == 0) {
		il.reply = newqueue(Nqueue);
		userinit(ilout, &il, "ilo");
		userinit(iltimer, &il, "ilt");
		ilflag = flag_install("il", "-- on errors");
	}

	if(dstp != Ilfsport) {
		ilgoaway(mb, ifc);
		unlock(&il);
		DEBUG("open not fsport %I.%d -> %I.%d\n", p->src, srcp, p->dst, dstp);
		return nil;
	}

	if(p->iltype != Ilsync) {
		ilgoaway(mb, ifc);
		unlock(&il);
		DEBUG("open not sync %I.%d -> %I.%d\n", p->src, srcp, p->dst, dstp);
		return nil;
	}

	cp = xcp;
	if(cp == 0) {
		cp = chaninit(Devil, 1, sizeof(Ilp));
		ilp = cp->pdata;
		ilp->chan = il.chan;
		il.chan = cp;
	}
	

	cp->ifc = ifc;
	ilp = cp->pdata;
	memmove(ilp->iphis, p->src, Pasize);
	memmove(ifc->ipa, p->dst, Pasize);
	ilp->srcp = srcp;
	ilp->dstp = dstp;
	ilp->state = Ilopening;

	ilpinit(ilp);

	memmove(ilp->ipgate, ilp->iphis, Pasize);
	if((nhgetl(ifc->ipa)&ifc->mask) != (nhgetl(p->src)&ifc->mask))
		iproute(ilp->ipgate, p->src, ifc->netgate);

	cp->send = serveq;
	cp->reply = il.reply;
	ilp->reply = ifc->reply;
	cp->protocol = nil;
	cp->msize = 0;
	cp->whotime = 0;
	sprint(cp->whochan, "il!%I!%d", p->src, srcp);
	cp->whoprint = ilwhoprint;
	ilp->alloc = 1;

	unlock(&il);
	return cp;
}

void
ilrecv(Msgbuf *mb, Ifc *ifc)
{
	Ilpkt *ih;
	Chan *cp;
	Ilp *ilp;
	int illen, plen;

	ih = (Ilpkt*)mb->data;

	plen = mb->count;
	if(plen < Ensize+Ipsize+Ilsize)
		goto drop;

	illen = nhgets(ih->illen);
	if(illen+Ilsize > plen)
		goto drop;

	if(ptclcsum((uchar*)ih+(Ensize+Ipsize), illen) != 0) {
		print("il: cksum error %E %I\n", ih->s, ih->src);
		ifc->sumerr++;
		goto drop;
	}

	cp = getchan(ifc, ih, mb);
	if(cp == nil)
		goto drop;
	mb->chan = cp;
	ilp = cp->pdata;

	if(ilp->state == Ilopening) {
		ilp->state = Ilsyncee;
		ilpinit(ilp);
		ilp->rstart = nhgetl(ih->ilid);
		print("il: allocating %s\n", cp->whochan);
	}

	ilprocess(cp, mb);
	return;

drop:
	mbfree(mb);
}

/*
 * process to convert p9 to il/ip
 */
static
void
ilout(void)
{
	Ifc *ifc;
	Msgbuf *mb;
	Ilp *ilp;
	Ilpkt *ih;
	Chan *cp;
	int dlen;
	ulong id, ack;

loop:
	mb = recv(il.reply, 0);
	if(mb == 0)
		goto loop;

	cp = mb->chan;
	ilp = cp->pdata;

	switch(ilp->state) {
	case Ilclosed:
	case Illistening:
	case Ilclosing:
		goto err;
	}

	dlen = mb->count;
	mb->data -= Ensize+Ipsize+Ilsize;	/* make room for header */
	mb->count += Ensize+Ipsize+Ilsize;
	if(mb->data < mb->xdata)
		panic("ilout: no room for header");
	ih = (Ilpkt*)mb->data;

	/*
	 * Ip fields
	 */
	ifc = cp->ifc;
	memmove(ih->src, ifc->ipa, Pasize);
	memmove(ih->dst, ilp->iphis, Pasize);
	ih->proto = Ilproto;

	/*
	 * Il fields
	 */
	hnputs(ih->illen, Ilsize+dlen);
	hnputs(ih->ilsrc, ilp->dstp);
	hnputs(ih->ildst, ilp->srcp);
	id = ilp->next++;
	hnputl(ih->ilid, id);
	ack = ilp->recvd;
	hnputl(ih->ilack, ack);
	ilp->acksent = ack;
	ilp->acktime = msec + AckDelay;
	ih->iltype = Ildata;
	ih->ilspec = 0;
	ih->ilsum[0] = 0;
	ih->ilsum[1] = 0;

	/*
	 * checksum
	 */
	hnputs(ih->ilsum, ptclcsum((uchar*)ih+(Ensize+Ipsize), dlen+Ilsize));

	ilackq(cp, mb);

	/* Start the round trip timer for this packet if the timer is free */
	if(ilp->rttack == 0) {
		ilp->rttack = id;
		ilp->rttstart = msec;
		ilp->rttlen = dlen+Ipsize+Ilsize;
	}

	if(ilp->timeout <= msec)
		ilsettimeout(ilp);
	ipsend(mb);
	goto loop;

err:
	print("ilout: error\n");
	mbfree(mb);
	goto loop;

}

static
void
ilackq(Chan *cp, Msgbuf *mb)
{
	Msgbuf *nmb;
	Ilp *ilp;

	/*
	 * Enqueue a copy on the unacked queue in case this one gets lost
	 */
/* botch -- a reference count will save this copy */
	nmb = mballoc(mb->count, cp, Mbil2);
	memmove(nmb->data, mb->data, mb->count);
	nmb->next = 0;

	ilp = cp->pdata;
	lock(ilp);
	if(ilp->unacked)
		ilp->unackedtail->next = nmb;
	else 
		ilp->unacked = nmb;
	ilp->unackedtail = nmb;
	ilp->unackedbytes += nmb->count;
	unlock(ilp);
}

static
void
ilprocess(Chan *cp, Msgbuf *mb)
{
	ulong id, ack;
	Ilp* ilp;
	Ilpkt *h;

	ilp = cp->pdata;
	h = (Ilpkt*)mb->data;

	id = nhgetl(h->ilid);
	ack = nhgetl(h->ilack);

	ilp->lastrecv = msec;

	switch(ilp->state) {
	default:
		print("il unknown state\n");
	case Ilclosed:
		mbfree(mb);
		break;
	case Ilsyncer:
		switch(h->iltype) {
		default:
			break;
		case Ilsync:
			if(ack != ilp->start) {
				ilp->state = Ilclosed;
				ilhangup(cp, "connection rejected", 1);
			} else {
				ilp->recvd = id;
				ilp->rstart = id;
				ilsendctl(cp, 0, Ilack, ilp->next, ilp->recvd, 0);
				ilp->state = Ilestablished;
				wakeup(&ilp->syn);
				ilpullup(cp);
			}
			break;
		case Ilclose:
			if(ack == ilp->start) {
				ilp->state = Ilclosed;
				ilhangup(cp, "remote close-1", 1);
			}
			break;
		}
		mbfree(mb);
		break;

	case Ilsyncee:
		switch(h->iltype) {
		default:
			break;
		case Ilsync:
			if(id != ilp->rstart || ack != 0)
				ilp->state = Ilclosed;
			else {
				ilp->recvd = id;
				ilsendctl(cp, 0, Ilsync, ilp->start, ilp->recvd, 0);
			}
			break;
		case Ilack:
			if(ack == ilp->start) {
				ilp->state = Ilestablished;
				ilpullup(cp);
			}
			break;
		case Ildata:
			if(ack == ilp->start) {
				ilp->state = Ilestablished;
				goto established;
			}
			break;
		case Ilclose:
			if(id == ilp->next) {
				ilp->state = Ilclosed;
				ilhangup(cp, "remote close-2", 1);
			}
			break;
		}
		mbfree(mb);
		break;

	case Ilestablished:
	established:
		switch(h->iltype) {
		default:
			mbfree(mb);
			break;
		case Ilsync:
			if(id != ilp->rstart) {
				ilp->state = Ilclosed;
				ilhangup(cp, "remote close-3", 1);
			} else
				ilsendctl(cp, 0, Ilack, ilp->next, ilp->rstart, 0);
			mbfree(mb);
			break;
		case Ildata:
			ilackto(cp, ack, mb);
			iloutoforder(cp, h, mb);
			ilpullup(cp);
			break;
		case Ildataquery:
			ilackto(cp, ack, mb);
			iloutoforder(cp, h, mb);
			ilpullup(cp);
			ilsendctl(cp, 0, Ilstate, ilp->next, ilp->recvd, h->ilspec);
			break;
		case Ilack:
			ilackto(cp, ack, mb);
			mbfree(mb);
			break;
		case Ilquery:
			ilackto(cp, ack, mb);
			ilsendctl(cp, 0, Ilstate, ilp->next, ilp->recvd, h->ilspec);
			mbfree(mb);
			break;
		case Ilstate:
			if(ack >= ilp->rttack)
				ilp->rttack = 0;
			ilackto(cp, ack, mb);
			if(h->ilspec > Nqt)
				h->ilspec = 0;
			if(ilp->qt[h->ilspec] > ack){
				ilrexmit(ilp);
				ilsettimeout(ilp);
			}
			mbfree(mb);
			break;
		case Ilclose:
			mbfree(mb);
			if(ack < ilp->start || ack > ilp->next)
				break;
			ilp->recvd = id;
			ilsendctl(cp, 0, Ilclose, ilp->next, ilp->recvd, 0);
			ilp->state = Ilclosing;
			ilfreeq(cp);
			break;
		}
		break;

	case Illistening:
		mbfree(mb);
		break;

	case Ilclosing:
		switch(h->iltype) {
		case Ilclose:
			ilp->recvd = id;
			ilsendctl(cp, 0, Ilclose, ilp->next, ilp->recvd, 0);
			if(ack == ilp->next) {
				ilp->state = Ilclosed;
				ilhangup(cp, "closed", 1);
			}
			break;
		}
		mbfree(mb);
		break;
	}
}

static
void
ilsendctl(Chan *cp, Ilpkt *inih, int type, ulong id, ulong ack, int ilspec)
{
	Ifc *ifc;
	Ilpkt *ih;
	Msgbuf *mb;
	Ilp *ilp;

	ilp = cp->pdata;
	mb = mballoc(Ensize+Ipsize+Ilsize, cp, Mbil3);

	ih = (Ilpkt*)mb->data;

	ih->proto = Ilproto;
	ifc = cp->ifc;
	memmove(ih->src, ifc->ipa, Pasize);
	hnputs(ih->illen, Ilsize);
	if(inih) {
		memmove(ih->dst, inih->src, Pasize);
		memmove(ih->ilsrc, inih->ildst, sizeof(ih->ilsrc));
		memmove(ih->ildst, inih->ilsrc, sizeof(ih->ildst));
		memmove(ih->ilid, inih->ilack, sizeof(ih->ilid));
		memmove(ih->ilack, inih->ilid, sizeof(ih->ilack));
	} else {
		memmove(ih->dst, ilp->iphis, Pasize);
		hnputs(ih->ilsrc, ilp->dstp);
		hnputs(ih->ildst, ilp->srcp);
		hnputl(ih->ilid, id);
		hnputl(ih->ilack, ack);
		ilp->acksent = ack;
		ilp->acktime = msec;
	}
	ih->iltype = type;
	ih->ilspec = ilspec;
	ih->ilsum[0] = 0;
	ih->ilsum[1] = 0;

	hnputs(ih->ilsum, ptclcsum((uchar*)mb->data+(Ensize+Ipsize), Ilsize));

	ipsend(mb);
}

static
void
ilhangup(Chan *cp, char *msg, int dolock)
{
	Ilp *ilp;
	int s;

	ilp = cp->pdata;
	s = ilp->state;
	ilp->state = Ilclosed;
	if(s == Ilsyncer)
		wakeup(&ilp->syn);

	if(msg != nil)
		print("hangup! %s %d/%I.%d\n", msg, ilp->srcp,
			ilp->iphis, ilp->dstp);
	ilfreeq(cp);

	fileinit(cp);
	cp->whotime = 0;
	strcpy(cp->whoname, "<none>");

	if(dolock)
		lock(&il);
	ilp->alloc = 0;
	ilp->srcp = 0;
	ilp->dstp = 0;
	memset(ilp->iphis, 0, sizeof(ilp->iphis));
	if(dolock)
		unlock(&il);
}

static
void
ilpullup(Chan *cp)
{
	Ilpkt *oh;
	Msgbuf *mb;
	Ilp *ilp;
	ulong oid, dlen;

	ilp = cp->pdata;
	lock(ilp);

	while(ilp->outoforder) {
		mb = ilp->outoforder;
		oh = (Ilpkt*)mb->data;
		oid = nhgetl(oh->ilid);
		if(oid <= ilp->recvd) {
			ilp->outoforder = mb->next;
			mbfree(mb);
			continue;
		}
		if(oid != ilp->recvd+1)
			break;

		ilp->recvd = oid;
		ilp->outoforder = mb->next;

		/*
		 * strip off the header
		 */
		dlen = nhgets(oh->illen)-Ilsize;
		mb->data += Ensize+Ipsize+Ilsize;
		mb->count = dlen;
		send(cp->send, mb);
	}
	unlock(ilp);
}

static
void
iloutoforder(Chan *cp, Ilpkt *h, Msgbuf *mb)
{
	Msgbuf **l, *f;
	Ilp *ilp;
	ulong id, ilid;
	uchar *lid;

	ilp = cp->pdata;
	id = nhgetl(h->ilid);

	/*
	 * Window checks
	 */
	if(id <= ilp->recvd || id > ilp->recvd+ilp->window) {
		mbfree(mb);
		return;
	}

	/*
	 * Packet is acceptable so
	 * sort onto receive queue for pullup
	 */
	mb->next = 0;
	lock(ilp);
	if(ilp->outoforder == 0) {
		ilp->outoforder = mb;
	} else {
		l = &ilp->outoforder;
		for(f = *l; f; f = f->next) {
			lid = ((Ilpkt*)(f->data))->ilid;
			ilid = nhgetl(lid);
			if(id <= ilid) {
				if(id == ilid) {
					mbfree(mb);
					unlock(ilp);
					return;
				}
				mb->next = f;
				break;
			}
			l = &f->next;
		}
		*l = mb;
	}
	unlock(ilp);
}

static
void
ilrttcalc(Ilp *ilp, Msgbuf *mb)
{
	int rtt, tt, pt, delay, rate;

	rtt = msec - ilp->rttstart + TK2MS(1) - 1;
	delay = ilp->delay;
	rate = ilp->rate;

	/* Guard against zero wrap */
	if(rtt > 120000 || rtt < 0)
		return;

	/* this block had to be transmitted after the one acked so count its size */
	ilp->rttlen += mb->count+Ipsize+Ilsize;

	if(ilp->rttlen < 256){
		/* guess fixed delay as rtt of small packets */
		delay += rtt - (delay>>LogAGain);
		if(delay < AGain)
			delay = AGain;
		ilp->delay = delay;
	} else {
		/* if packet took longer than avg rtt delay, recalc rate */
		tt = rtt - (delay>>LogAGain);
		if(tt > 0){
			rate += ilp->rttlen/tt - (rate>>LogAGain);
			if(rate < AGain)
				rate = AGain;
			ilp->rate = rate;
		}
	}

	/* mdev */
	pt = ilp->rttlen/(rate>>LogAGain) + (delay>>LogAGain);
	ilp->mdev += abs(rtt-pt) - (ilp->mdev>>LogDGain);

	if(rtt > ilp->maxrtt)
		ilp->maxrtt = rtt;
}

static
void
ilackto(Chan *cp, ulong ackto, Msgbuf *mb)
{
	Ilpkt *h;
	Ilp *ilp;
	ulong id;

	ilp = cp->pdata;
	if(ilp->rttack == ackto)
		ilrttcalc(ilp, mb);

	/* Cancel if we lost the packet we were interested in */
	if(ilp->rttack <= ackto)
		ilp->rttack = 0;

	lock(ilp);
	while(ilp->unacked) {
		h = (Ilpkt*)ilp->unacked->data;
		id = nhgetl(h->ilid);
		if(ackto < id)
			break;

		mb = ilp->unacked;
		ilp->unacked = mb->next;
		mb->next = 0;
		ilp->unackedbytes -= mb->count;
		mbfree(mb);
		ilp->rexmit = 0;
		ilsettimeout(ilp);
	}
	unlock(ilp);
}

static
void
ilrexmit(Ilp *ilp)
{
	Msgbuf *omb, *mb;
	Ilpkt *h;

	lock(ilp);
	omb = ilp->unacked;
	if(omb == 0) {
		unlock(ilp);
		return;
	}

/* botch -- a reference count will save this copy */
	mb = mballoc(omb->count, omb->chan, Mbil4);
	memmove(mb->data, omb->data, omb->count);
	unlock(ilp);
	
	h = (Ilpkt*)mb->data;

	h->iltype = Ildataquery;
	hnputl(h->ilack, ilp->recvd);
	h->ilspec = ilnextqt(ilp);
	h->ilsum[0] = 0;
	h->ilsum[1] = 0;
	hnputs(h->ilsum, ptclcsum((uchar*)mb->data+(Ensize+Ipsize), nhgets(h->illen)));

	ilbackoff(ilp);

	ipsend(mb);
}

static
void
ilfreeq(Chan *cp)
{
	Ilp *ilp;
	Msgbuf *mb, *next;

	ilp = cp->pdata;
	lock(ilp);
	for(mb = ilp->unacked; mb; mb = next) {
		next = mb->next;
		mbfree(mb);
	}
	ilp->unacked = 0;

	for(mb = ilp->outoforder; mb; mb = next) {
		next = mb->next;
		mbfree(mb);
	}
	ilp->outoforder = 0;
	unlock(ilp);
}

static
void
ilsettimeout(Ilp *ilp)
{
	ulong pt;

	pt = (ilp->delay>>LogAGain)
		+ ilp->unackedbytes/(ilp->rate>>LogAGain)
		+ (ilp->mdev>>(LogDGain-1))
		+ AckDelay;
	if(pt > MaxTimeout)
		pt = MaxTimeout;
	ilp->timeout = msec + pt;
}

static
void
ilbackoff(Ilp *ilp)
{
	ulong pt;
	int i;

	pt = (ilp->delay>>LogAGain)
		+ ilp->unackedbytes/(ilp->rate>>LogAGain)
		+ (ilp->mdev>>(LogDGain-1))
		+ AckDelay;
	for(i = 0; i < ilp->rexmit; i++)
		pt = pt + (pt>>1);
	if(pt > MaxTimeout)
		pt = MaxTimeout;
	ilp->timeout = msec + pt;

	ilp->rexmit++;
}

/*
 * il timer
 * every 100ms
 */
static	Rendez	ilt;

static
void
callil(Alarm *a, void *)
{

	cancel(a);
	wakeup(&ilt);
}

// complain if two numbers not within an hour of each other
#define Tfuture (1000*60*60)
int
later(ulong t1, ulong t2, char *x)
{
	int dt;

	dt = t1 - t2;
	if(dt > 0) {
		if(dt > Tfuture)
			print("%s: way future %d\n", x, dt);
		return 1;
	}
	if(dt < -Tfuture) {
		print("%s: way past %d\n", x, -dt);
		return 1;
	}
	return 0;
}


static
void
iltimer(void)
{
	Chan *cp;
	Ilp *ilp;

loop:
	lock(&il);
	for(cp = il.chan; cp; cp = ilp->chan) {
		ilp = cp->pdata;
		if(ilp->alloc == 0)
			continue;

		switch(ilp->state) {
		case Ilclosed:
		case Illistening:
			break;

		case Ilclosing:
			if(later(msec, ilp->timeout, "timeout")){
				if(ilp->rexmit > MaxRexmit){ 
					ilp->state = Ilclosed;
					ilhangup(cp, "connection timed out-0", 0);
					break;
				}
				ilsendctl(cp, 0, Ilclose, ilp->next, ilp->recvd, 0);
				ilbackoff(ilp);
			}
			break;

		case Ilsyncee:
		case Ilsyncer:
			if(later(msec, ilp->timeout, "timeout")){
				if(ilp->rexmit > MaxRexmit){ 
					ilp->state = Ilclosed;
					ilhangup(cp, "connection timed out-1", 0);
					break;
				}
				ilsendctl(cp, 0, Ilsync, ilp->start, ilp->recvd, 0);
				ilbackoff(ilp);
			}
			break;

		case Ilestablished:
			if(ilp->recvd != ilp->acksent)
			if(later(msec, ilp->acktime, "acktime"))
				ilsendctl(cp, 0, Ilack, ilp->next, ilp->recvd, 0);

			if(later(msec, ilp->querytime, "querytime")){
				if(later(msec, ilp->lastrecv+DeathTime, "deathtime")){
					ilhangup(cp, "connection timed out-2", 0);
					break;
				}
				ilsendctl(cp, 0, Ilquery, ilp->next, ilp->recvd, ilnextqt(ilp));
				ilp->querytime = msec + QueryTime;
			}
			if(ilp->unacked != nil)
			if(later(msec, ilp->timeout, "timeout")) {
				if(ilp->rexmit > MaxRexmit) {
					ilp->state = Ilclosed;
					ilhangup(cp, "connection timed out-3", 0);
					break;
				}
				ilsendctl(cp, 0, Ilquery, ilp->next, ilp->recvd, ilnextqt(ilp));
				ilbackoff(ilp);
			}
			break;
		}
	}
	unlock(&il);
	alarm(Iltickms, callil, 0);
	sleep(&ilt, no, 0);
	goto loop;
}

static
int
notsyncer(void *ic)
{
	return ((Ilp*)ic)->state != Ilsyncer;
}

static
void
callildial(Alarm *a, void*)
{

	cancel(a);
	wakeup(&ild);
}

static
int
ilnextqt(Ilp *ilp)
{
	int x;

	lock(ilp);
	x = ilp->qtx;
	if(++x > Nqt)
		x = 1;
	ilp->qtx = x;
	ilp->qt[x] = ilp->next-1;	/* highest xmitted packet */
	ilp->qt[0] = ilp->qt[x];	/* compatibility with old implementations */
	unlock(ilp);

	return x;
}

#define	HOWMANY(x, y)	(((x)+((y)-1))/(y))
#define ROUNDUP(x, y)	(HOWMANY((x), (y))*(y))

static void
ilgoaway(Msgbuf *inmb, Ifc *ifc)
{
	Chan *cp;
	int size;
	Ilpkt *ih, *inih;
	Msgbuf *mb;
	Ilp *ilp;
	uchar *p;

	inih = (Ilpkt*)inmb->data;

	/* allocate a temporary message, channel, and Ilp structure */
	size = ROUNDUP(Ensize+Ipsize+Ilsize, BY2WD)+sizeof(Chan)+sizeof(Ilp);
	mb = mballoc(size, nil, Mbil3);
	p = mb->data;
	p += ROUNDUP(Ensize+Ipsize+Ilsize, BY2WD);
	cp = (Chan*)p;
	p += sizeof(Chan);
	ilp = (Ilp*)p;

	/* link them together */
	cp->ifc = ifc;
	mb->chan = cp;
	cp->pdata = ilp;

	/* figure out next hop */
	memmove(ilp->ipgate, inih->src, Pasize);
	if((nhgetl(ifc->ipa)&ifc->mask) != (nhgetl(inih->src)&ifc->mask))
		iproute(ilp->ipgate, inih->src, ifc->netgate);

	/* create a close message */
	ih = (Ilpkt*)mb->data;
	ih->proto = Ilproto;
	hnputs(ih->illen, Ilsize);
	memmove(ih->src, ifc->ipa, Pasize);
	memmove(ih->dst, inih->src, Pasize);
	memmove(ih->ilsrc, inih->ildst, sizeof(ih->ilsrc));
	memmove(ih->ildst, inih->ilsrc, sizeof(ih->ildst));
	memmove(ih->ilid, inih->ilack, sizeof(ih->ilid));
	memmove(ih->ilack, inih->ilid, sizeof(ih->ilack));
	ih->iltype = Ilclose;
	ih->ilspec = 0;
	ih->ilsum[0] = 0;
	ih->ilsum[1] = 0;
	hnputs(ih->ilsum, ptclcsum((uchar*)mb->data+(Ensize+Ipsize), Ilsize));

	ipsend(mb);
}
