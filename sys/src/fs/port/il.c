#include "all.h"
#include "mem.h"

#define	DEBUG	if(cons.flags&Fip)print
#define Starttimer(s)	{(s)->timeout = 0; \
			 (s)->fasttime = (Fasttime*(s)->rtt)/Iltickms; \
			 (s)->slowtime = (Slowtime*(s)->rtt)/Iltickms; }

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
static	void	ilsendctl(Chan*, Ilpkt*, int, ulong, ulong);
static	void	ilpullup(Chan*);
static	void	ilackto(Chan*, ulong);
static	void	ilrexmit(Ilp*);
static	void	iloutoforder(Chan*, Ilpkt*, Msgbuf*);
static	void	ilfreeq(Chan*);
static	void	ilackq(Chan*, Msgbuf*);
static	void	ilbackoff(Ilp*);
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
	"dataquerey",
	"ack",
	"querey",
	"state",
	"close",
};
static
char*	etime	= "connection timed out";

static	Rendez	ild;

static
void
ilpinit(Ilp *ilp)
{
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
	Starttimer(ilp);
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
		if(memcmp(p->dst, cp->ilp.ipmy, Pasize) == 0)
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
	memmove(cp->ilp.iphis, p->src, Pasize);
	memmove(cp->ilp.ipmy, p->dst, Pasize);
	cp->ilp.srcp = srcp;
	cp->ilp.dstp = dstp;
	cp->ilp.state = Ilopening;

	ilpinit(&cp->ilp);

	memmove(cp->ilp.ipgate, ifc->netgate, Pasize);
	memmove(cp->ilp.ea, ifc->ea, Easize);

	cp->ilp.usegate = 0;
	if((nhgetl(ifc->ipa)&ifc->mask) != (nhgetl(p->src)&ifc->mask))
		cp->ilp.usegate = 1;

	cp->send = serveq;
	cp->reply = il.reply;
	cp->ilp.reply = ifc->reply;
	cp->whotime = 0;
	sprint(cp->whochan, "%I-%d", p->src, srcp);

	unlock(&il);
	print("allocating %s\n", cp->whochan);
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
				ilsendctl(cp, ih, Ilclose, 0, 0);
			goto drop;
		}
		if(ih->iltype != Ilsync) {
			print("il: open not sync %I\n", ih->src);
			if(ih->iltype != Ilclose)
				ilsendctl(cp, ih, Ilclose, 0, 0);
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
	}

	ilprocess(cp, mb);
	return;

drop:
	mbfree(mb);
}

/*
 * process to convert p9 to il/ip
 */
void
ilout(void)
{
	Msgbuf *mb;
	Ilp *ilp;
	Ilpkt *ih;
	Chan *cp;
	int dlen;
	ulong id;

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
	memmove(ih->src, ilp->ipmy, Pasize);
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
	hnputl(ih->ilack, ilp->recvd);
	ih->iltype = Ildata;
	ih->ilspec = 0;
	ih->ilsum[0] = 0;
	ih->ilsum[1] = 0;

	/*
	 * checksum
	 */
	hnputs(ih->ilsum, ptclcsum((uchar*)ih+(Ensize+Ipsize), dlen+Ilsize));

	ilackq(cp, mb);
	ilp->acktime = Ackkeepalive;

	/* Start the round trip timer for this packet if the timer is free */
	if(ilp->rttack == 0) {
		ilp->rttack = id;
		ilp->ackms = MACHP(0)->ticks;
	}

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
	else {
		/*
		 * Start timer since we may have been idle for some time
		 */
		Starttimer(ilp);
		ilp->unacked = nmb;
	}
	ilp->unackedtail = nmb;
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

	ilp->querytime = Keepalivetime;
	ilp->deathtime = Keepalivetime;
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
				ilsendctl(cp, 0, Ilack, ilp->next, ilp->recvd);
				ilp->state = Ilestablished;
				wakeup(&ilp->syn);
				ilpullup(cp);
				Starttimer(ilp);
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
				ilsendctl(cp, 0, Ilsync, ilp->start, ilp->recvd);
				Starttimer(ilp);
			}
			break;
		case Ilack:
			if(ack == ilp->start) {
				ilp->state = Ilestablished;
				ilpullup(cp);
				Starttimer(ilp);
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
		switch(h->iltype) {
		default:
			mbfree(mb);
			break;
		case Ilsync:
			if(id != ilp->start) {
				ilp->state = Ilclosed;
				ilhangup(cp, "remote close");
			} else {
				ilsendctl(cp, 0, Ilack, ilp->next, ilp->rstart);
				Starttimer(ilp);
			}
			mbfree(mb);
			break;
		case Ildata:
			Starttimer(ilp);
			ilackto(cp, ack);
			ilp->acktime = Acktime;
			iloutoforder(cp, h, mb);
			ilpullup(cp);
			break;
		case Ildataquery:
			Starttimer(ilp);
			ilackto(cp, ack);
			ilp->acktime = Acktime;
			iloutoforder(cp, h, mb);
			ilpullup(cp);
			ilsendctl(cp, 0, Ilstate, ilp->next, ilp->recvd);
			break;
		case Ilack:
			ilackto(cp, ack);
			Starttimer(ilp);
			mbfree(mb);
			break;
		case Ilquerey:
			ilackto(cp, ack);
			ilsendctl(cp, 0, Ilstate, ilp->next, ilp->recvd);
			Starttimer(ilp);
			mbfree(mb);
			break;
		case Ilstate:
			ilackto(cp, ack);
			ilrexmit(ilp);
			Starttimer(ilp);
			mbfree(mb);
			break;
		case Ilclose:
			mbfree(mb);
			if(ack < ilp->start || ack > ilp->next)
				break;
			ilsendctl(cp, 0, Ilclose, ilp->next, ilp->recvd);
			ilp->state = Ilclosing;
			ilfreeq(cp);
			Starttimer(ilp);
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
			ilsendctl(cp, 0, Ilclose, ilp->next, ilp->recvd);
			if(ack == ilp->next) {
				ilp->state = Ilclosed;
				ilhangup(cp, 0);
			}
			Starttimer(ilp);
			break;
		}
		mbfree(mb);
		break;
	}
}

static
void
ilsendctl(Chan *cp, Ilpkt *inih, int type, ulong id, ulong ack)
{
	Ilpkt *ih;
	Msgbuf *mb;
	Ilp *ilp;

	ilp = &cp->ilp;
	mb = mballoc(Ensize+Ipsize+Ilsize, cp, Mbil3);

	ih = (Ilpkt*)mb->data;

	ih->proto = Ilproto;
	memmove(ih->src, ilp->ipmy, Pasize);
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
		ilp->acktime = Ackkeepalive;
	}
	ih->iltype = type;
	ih->ilspec = 0;
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

	print("hangup! %s %d/%d\n", msg? msg: "??", ilp->srcp, ilp->dstp);
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
	ulong id;
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
	if(ilp->outoforder == 0)
		ilp->outoforder = mb;
	else {
		l = &ilp->outoforder;
		for(f = *l; f; f = f->next) {
			lid = ((Ilpkt*)(f->data))->ilid;
			if(id < nhgetl(lid)) {
				mb->next = f;
				*l = mb;
				unlock(&il);
				return;
			}
			l = &f->next;
		}
		*l = mb;
	}
	unlock(&il);
}

static
void
ilackto(Chan *cp, ulong ackto)
{
	Ilpkt *h;
	Ilp *ilp;
	Msgbuf *mb;
	ulong id, t;

	ilp = &cp->ilp;
	if(ilp->rttack == ackto) {
		t = TK2MS(MACHP(0)->ticks - ilp->ackms);
		/* Guard against the ulong zero wrap */
		if(t < 100*ilp->rtt)
			ilp->rtt = (ilp->rtt*(ILgain-1)+t)/ILgain;
		if(ilp->rtt < Iltickms)
			ilp->rtt = Iltickms;
	}

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
		mbfree(mb);
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
	h->ilsum[0] = 0;
	h->ilsum[1] = 0;
	hnputs(h->ilsum, ptclcsum((uchar*)mb->data+(Ensize+Ipsize), nhgets(h->illen)));

	ipsend(mb);
}

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
ilbackoff(Ilp *ilp)
{
	if(ilp->fasttime < Slowtime/2)
		ilp->fasttime += Fasttime;
	else
		ilp->fasttime = (ilp->fasttime)*3/2;
}

/*
 * il timer
 * every 100ms
 */
static	Rendez	ilt;

static
void
callil(Alarm *a, void *arg)
{

	USED(arg);
	cancel(a);
	wakeup(&ilt);
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

		ilp->timeout += Iltickms;
		switch(ilp->state) {
		case Ilclosed:
		case Illistening:
			break;

		case Ilclosing:
			if(ilp->timeout >= ilp->fasttime) {
				ilsendctl(cp, 0, Ilclose, ilp->next, ilp->recvd);
				ilbackoff(ilp);
			}
			if(ilp->timeout >= ilp->slowtime) {
				ilp->state = Ilclosed;
				ilhangup(cp, 0);
			}
			break;

		case Ilsyncee:
		case Ilsyncer:
			if(ilp->timeout >= ilp->fasttime) {
				ilsendctl(cp, 0, Ilsync, ilp->start, ilp->recvd);
				ilbackoff(ilp);
			}
			if(ilp->timeout >= ilp->slowtime) {
				ilp->state = Ilclosed;
				ilhangup(cp, etime);
			}
			break;

		case Ilestablished:
			ilp->acktime -= Iltickms;
			if(ilp->acktime <= 0)
				ilsendctl(cp, 0, Ilack, ilp->next, ilp->recvd);

			ilp->querytime -= Iltickms;
			if(ilp->querytime <= 0){
				ilp->deathtime -= Querytime;
				if(ilp->deathtime < 0){
					ilhangup(cp, etime);
					break;
				}
				ilsendctl(cp, 0, Ilquerey, ilp->next, ilp->recvd);
				ilp->querytime = Querytime;
			}
			if(ilp->unacked == 0) {
				ilp->timeout = 0;
				break;
			}
			if(ilp->timeout >= ilp->fasttime) {
				ilrexmit(ilp);
				ilbackoff(ilp);
			}
			if(ilp->timeout >= ilp->slowtime) {
				ilp->state = Ilclosed;
				ilhangup(cp, etime);
				break;
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
callildial(Alarm *a, void *arg)
{

	USED(arg);
	cancel(a);
	wakeup(&ild);
}

Chan*
ildial(uchar *ip, Queue *rcp)
{
	Ifc *i;
	Ilp *ilp;
	Chan *cp;
	static ulong initseq;

	for(i = enets; i; i = i->next)
		if((nhgetl(i->ipa)&i->mask) == (nhgetl(ip)&i->mask))
			goto found;

	print("ilauth: no interface for %I\n", ip);
	return 0;

found:
	for(cp = il.ilp; cp; cp = cp->ilp.link)
		if(cp->ilp.alloc == 0) 
			break;
			
	if(cp == 0) {
		cp = chaninit(Devil, 1);
		cp->ilp.link = il.ilp;
		il.ilp = cp;
	}
	ilp = &cp->ilp;
	ilp->alloc = 1;

	lock(&il);
	if(il.reply == 0) {
		il.reply = newqueue(Nqueue);
		userinit(ilout, &il, "ilo");
		userinit(iltimer, &il, "ilt");
	}

	memmove(ilp->iphis, ip, Pasize);
	memmove(ilp->ipmy, i->ipa, Pasize);
	ilp->srcp = Ilauthport;
	ilp->dstp = Ilfsout;

	ilpinit(ilp);

	memmove(ilp->ipgate, i->netgate, Pasize);
	memmove(ilp->ea, i->ea, Easize);
	ilp->usegate = 0;

	cp->send = rcp;
	cp->reply = il.reply;
	ilp->reply = i->reply;
	cp->whotime = 0;
	unlock(&il);

	sprint(cp->whochan, "%I-%d", ip, Ilauthport);

	initseq += toytime();
	ilp->start = initseq & 0xffffff;
	ilp->next = ilp->start+1;
	ilp->window = Defaultwin;
	ilp->state = Ilsyncer;
	ilp->slowtime = Slowtime;
	ilp->rtt = Iltickms;
	ilsendctl(cp, 0, Ilsync, ilp->start, ilp->recvd);
	sleep(&ilp->syn, notsyncer, ilp);

	if(ilp->state == Ilclosed) {
		print("ilauth: connection failed\n");
		return 0;
	}
	print("allocating %s\n", cp->whochan);
	print("ilauth: connected\n");	
	return cp;
}

void
ilauth(void)
{
	int i;

	if(authchan)
		ilhangup(authchan, "ilauth close");

		print("ilauth: dialing %I\n", authip);

	for(i = 0; i < 5; i++) {
		authchan = ildial(authip, authreply);
		if(authchan)
			return;
		alarm(10000, callildial, 0);
		sleep(&ild, no, 0);
	}
	print("ilauth: dial giving up\n");
}
