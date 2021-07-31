#include "all.h"
#include "mem.h"

#define	DEBUG	if(cons.flags&Fip)print
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
	Chan*	ilp;
} il;

static	void	ilout(void);
static	void	ilprocess(Chan*, Msgbuf*);
static	Chan*	getchan(Ifc*, Ilpkt*);
static	void	ilhangup(Chan*, char*);
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
getchan(Ifc *ifc, Ilpkt *p)
{
	Chan *cp, *xcp;
	int srcp, dstp;

	srcp = nhgets(p->ilsrc);
	dstp = nhgets(p->ildst);

	xcp = 0;
	for(cp = il.ilp; cp; cp = cp->ilp.link) {
		if(cp->ilp.alloc == 0) {
			xcp = cp;
			continue;
		}
		if(srcp == cp->ilp.srcp)
		if(dstp == cp->ilp.dstp)
		if(memcmp(p->src, cp->ilp.iphis, Pasize) == 0)
		if(memcmp(p->dst, ifc->ipa, Pasize) == 0)
			return cp;
	}

	lock(&il);
	if(il.reply == 0) {
		il.reply = newqueue(Nqueue);
		userinit(ilout, &il, "ilo");
		userinit(iltimer, &il, "ilt");
	}

	if(xcp && xcp->ilp.alloc != 0)
		xcp = 0;
	cp = xcp;
	if(cp == 0) {
		cp = chaninit(Devil, 1);
		cp->ilp.link = il.ilp;
		il.ilp = cp;
	}

	cp->ilp.alloc = 1;
	cp->ifc = ifc;
	memmove(cp->ilp.iphis, p->src, Pasize);
	memmove(ifc->ipa, p->dst, Pasize);
	cp->ilp.srcp = srcp;
	cp->ilp.dstp = dstp;
	cp->ilp.state = Ilopening;

	ilpinit(&cp->ilp);

	memmove(cp->ilp.ipgate, cp->ilp.iphis, Pasize);
	if((nhgetl(ifc->ipa)&ifc->mask) != (nhgetl(p->src)&ifc->mask))
		iproute(cp->ilp.ipgate, p->src, ifc->netgate);

	cp->send = serveq;
	cp->reply = il.reply;
	cp->ilp.reply = ifc->reply;
	cp->whotime = 0;
	sprint(cp->whochan, "il!%I!%d", p->src, srcp);

	unlock(&il);
	print("il: allocating %s\n", cp->whochan);
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
		print("il: cksum error\n");
		ifc->sumerr++;
		goto drop;
	}

	cp = getchan(ifc, ih);
	mb->chan = cp;
	ilp = &cp->ilp;

	if(ilp->state == Ilopening) {
		if(ilp->dstp != Ilfsport) {
			if(ih->iltype != Ilclose)
				ilsendctl(cp, ih, Ilclose, 0, 0, 0);
			goto drop;
		}
		if(ih->iltype != Ilsync) {
/*			print("il: open not sync %I\n", ih->src);	/**/
			if(ih->iltype != Ilclose)
				ilsendctl(cp, ih, Ilclose, 0, 0, 0);
			goto drop;
		}
		ilp->state = Ilsyncee;
		ilpinit(ilp);
		ilp->rstart = nhgetl(ih->ilid);
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
	ilp = &cp->ilp;

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
	memmove(ih->src, cp->ifc->ipa, Pasize);
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

	ilp = &cp->ilp;
	lock(&il);
	if(ilp->unacked)
		ilp->unackedtail->next = nmb;
	else 
		ilp->unacked = nmb;
	ilp->unackedtail = nmb;
	ilp->unackedbytes += nmb->count;
	unlock(&il);
}

static
void
ilprocess(Chan *cp, Msgbuf *mb)
{
	ulong id, ack;
	Ilp* ilp;
	Ilpkt *h;

	ilp = &cp->ilp;
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
				ilhangup(cp, "connection rejected");
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
				ilhangup(cp, "remote close");
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
				ilhangup(cp, "remote close");
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
				ilhangup(cp, "remote close");
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
				ilhangup(cp, 0);
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
	Ilpkt *ih;
	Msgbuf *mb;
	Ilp *ilp;

	ilp = &cp->ilp;
	mb = mballoc(Ensize+Ipsize+Ilsize, cp, Mbil3);

	ih = (Ilpkt*)mb->data;

	ih->proto = Ilproto;
	memmove(ih->src, cp->ifc->ipa, Pasize);
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
ilhangup(Chan *cp, char *msg)
{
	Ilp *ilp;
	int s;

	ilp = &cp->ilp;
	s = ilp->state;
	ilp->state = Ilclosed;
	if(s == Ilsyncer)
		wakeup(&ilp->syn);

	print("hangup! %s %d/%I.%d\n", msg? msg: "??", ilp->srcp,
		ilp->iphis, ilp->dstp);
	ilfreeq(cp);

	fileinit(cp);
	cp->whotime = 0;
	strcpy(cp->whoname, "<none>");

	lock(&il);
	ilp->alloc = 0;
	ilp->srcp = 0;
	ilp->dstp = 0;
	memset(ilp->iphis, 0, sizeof(ilp->iphis));
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

	ilp = &cp->ilp;
	lock(&il);

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
	unlock(&il);
}

static
void
iloutoforder(Chan *cp, Ilpkt *h, Msgbuf *mb)
{
	Msgbuf **l, *f;
	Ilp *ilp;
	ulong id, ilid;
	uchar *lid;

	ilp = &cp->ilp;
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
	lock(&il);
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
					unlock(&il);
					return;
				}
				mb->next = f;
				break;
			}
			l = &f->next;
		}
		*l = mb;
	}
	unlock(&il);
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

	ilp = &cp->ilp;
	if(ilp->rttack == ackto)
		ilrttcalc(ilp, mb);

	/* Cancel if we lost the packet we were interested in */
	if(ilp->rttack <= ackto)
		ilp->rttack = 0;

	lock(&il);
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
	unlock(&il);
}

static
void
ilrexmit(Ilp *ilp)
{
	Msgbuf *omb, *mb;
	Ilpkt *h;

	lock(&il);
	omb = ilp->unacked;
	if(omb == 0) {
		unlock(&il);
		return;
	}

/* botch -- a reference count will save this copy */
	mb = mballoc(omb->count, omb->chan, Mbil4);
	memmove(mb->data, omb->data, omb->count);
	unlock(&il);
	
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

	ilp = &cp->ilp;
	lock(&il);
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
	unlock(&il);
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
	for(cp = il.ilp; cp; cp = cp->ilp.link) {
		ilp = &cp->ilp;
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
					ilhangup(cp, 0);
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
					ilhangup(cp, "connection timed out-1");
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
					ilhangup(cp, "connection timed out-2");
					break;
				}
				ilsendctl(cp, 0, Ilquery, ilp->next, ilp->recvd, ilnextqt(ilp));
				ilp->querytime = msec + QueryTime;
			}
			if(ilp->unacked != nil)
			if(later(msec, ilp->timeout, "timeout")) {
				if(ilp->rexmit > MaxRexmit) {
					ilp->state = Ilclosed;
					ilhangup(cp, "connection timed out-3");
					break;
				}
				ilsendctl(cp, 0, Ilquery, ilp->next, ilp->recvd, ilnextqt(ilp));
				ilbackoff(ilp);
			}
			break;
		}
	}
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

	lock(&il);
	x = ilp->qtx;
	if(++x > Nqt)
		x = 1;
	ilp->qtx = x;
	ilp->qt[x] = ilp->next-1;	/* highest xmitted packet */
	ilp->qt[0] = ilp->qt[x];	/* compatibility with old implementations */
	unlock(&il);

	return x;
}
