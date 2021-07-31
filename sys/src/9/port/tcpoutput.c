#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include 	"arp.h"
#include 	"ipdat.h"

#define DPRINT if(tcpdbg) print

extern	int tcpdbg;
extern	ushort tcp_mss;
	int tcptimertype;

void
tcpoutput(Ipconv *s)
{
	Tcp seg;
	int qlen;
	Tcphdr ph;
	Tcpctl *tcb;
	Block *hbp,*dbp, *sndq;
	ushort ssize, dsize, usable, sent;

	tcb = &s->tcpctl;

	switch(tcb->state) {
	case Listen:
	case Closed:
		return;
	}

	for(;;) {
		qlen = tcb->sndcnt;
		sent = tcb->snd.ptr - tcb->snd.una;
		sndq = tcb->sndq;

		/* Don't send anything else until our SYN has been acked */
		if(sent != 0)
		if((tcb->flags & SYNACK) == 0)
			break;

		/* Compute usable segment based on offered window and limit
		 * window probes to one
		 */
		if(tcb->snd.wnd == 0){
			if(sent != 0) {
				if (tcb->flags&FORCE)
						tcb->snd.ptr = tcb->snd.una;
				else
					break;
			}
			usable = 1;
		}
		else {
			usable = MIN(tcb->snd.wnd,tcb->cwind) - sent;
			if(sent != 0)
			if(qlen - sent < tcb->mss) 
				usable = 0;
		}

		ssize = MIN(qlen - sent, usable);
		ssize = MIN(ssize, tcb->mss);
		dsize = ssize;
		seg.up = 0;

		if(ssize == 0)
		if((tcb->flags&FORCE) == 0)
			break;

		tcphalt(&tcb->acktimer);

		tcb->flags &= ~FORCE;
		tcprcvwin(s);

		/* By default we will generate an ack */
		seg.source = s->psrc;
		seg.dest = s->pdst;
		seg.flags = ACK; 	
		seg.mss = 0;

		switch(tcb->state){
		case Syn_sent:
			seg.flags = 0;
			/* No break */
		case Syn_received:
			if(tcb->snd.ptr == tcb->iss){
				seg.flags |= SYN;
				dsize--;
				seg.mss = tcp_mss;
			}
			break;
		}
		seg.seq = tcb->snd.ptr;
		seg.ack = tcb->last_ack = tcb->rcv.nxt;
		seg.wnd = tcb->rcv.wnd;

		/* Pull out data to send */
		dbp = 0;
		if(dsize != 0){
			if(dupb(&dbp, sndq, sent, dsize) != dsize) {
				seg.flags |= FIN;
				dsize--;
			}
			DPRINT("dupb: %d\n", dbp->rptr[0]);
		}

		if(sent+dsize == qlen)
			seg.flags |= PSH;

		/*
		 * keep track of balance of resent data */
		if(tcb->snd.ptr < tcb->snd.nxt)
			tcb->resent += MIN((int)tcb->snd.nxt - (int)tcb->snd.ptr,(int)ssize);

		tcb->snd.ptr += ssize;

		/* Pull up the send pointer so we can accept acks for this window */
		if(seq_gt(tcb->snd.ptr,tcb->snd.nxt))
			tcb->snd.nxt = tcb->snd.ptr;

		/* Fill in fields of pseudo IP header */
		hnputl(ph.tcpdst, s->dst);
		hnputl(ph.tcpsrc, Myip[Myself]);
		hnputs(ph.tcpsport, s->psrc);
		hnputs(ph.tcpdport, s->pdst);

		/* Build header, link data and compute cksum */
		if((hbp = htontcp(&seg, dbp, &ph)) == 0) {
			freeb(dbp);
			return;
		}

		/* Start the transmission timers if there is new data and we
		 * expect acknowledges
		 */
		if(ssize != 0){
			tcb->timer.start = backoff(tcb->backoff) *
			 (2 * tcb->mdev + tcb->srtt + MSPTICK) / MSPTICK;
			if(!run_timer(&tcb->timer))
				tcpgo(&tcb->timer);

			/* If round trip timer isn't running, start it */
			if(!run_timer(&tcb->rtt_timer)){
				tcpgo(&tcb->rtt_timer);
				tcb->rttseq = tcb->snd.ptr;
			}
		}
		PUTNEXT(Ipoutput, hbp);
	}
}

void
tcprxmit(Ipconv *s)
{
	Tcpctl *tcb;

	tcb = &s->tcpctl;
	qlock(tcb);
	tcb->flags |= RETRAN|FORCE;
	tcb->snd.ptr = tcb->snd.una;

	/* Pull window down to a single packet and halve the slow
	 * start threshold
	 */
	tcb->ssthresh = tcb->cwind / 2;
	tcb->ssthresh = MAX(tcb->ssthresh, tcb->mss);

	tcb->cwind = tcb->mss;
	tcpoutput(s);
	qunlock(tcb);
}

void
tcptimeout(void *arg)
{
	Tcpctl *tcb;
	Ipconv *s;

	s = (Ipconv *)arg;
	tcb = &s->tcpctl;
	switch(tcb->state){
	default:
		tcb->backoff++;
		if (tcb->backoff >= MAXBACKOFF && tcb->snd.wnd > 0) {
			localclose(s, Etimedout);
			break;
		}
		tcprxmit(s);
		break;

	case Time_wait:
		localclose(s, 0);
		break;
	}
}

int
backoff(int n)
{
	if(tcptimertype == 1) 
		return n+1;

	if(n <= 4)
		return 1 << n;

	return n*n;
}

void
tcpacktimer(Ipconv *s)
{
	Tcpctl *tcb = &s->tcpctl;

	qlock(tcb);
	tcb->flags |= FORCE;
	tcprcvwin(s);
	tcpoutput(s);
	qunlock(tcb);
}

void
tcprcvwin(Ipconv *s)				/* Call with tcb locked */
{
	int w;
	Tcpctl *tcb;

	tcb = &s->tcpctl;
	qlock(s);
	if(s->readq) {
		w = Streamhi - s->readq->next->len;
		if(w < 0)
			tcb->rcv.wnd = 0;
		else
			tcb->rcv.wnd = w;
	}
	else
		tcb->rcv.wnd = Streamhi;
	qunlock(s);
}

/*
 * Network byte order functions
 */

void
hnputs(uchar *ptr, ushort val)
{
	ptr[0] = val>>8;
	ptr[1] = val;
}

void
hnputl(uchar *ptr, ulong val)
{
	ptr[0] = val>>24;
	ptr[1] = val>>16;
	ptr[2] = val>>8;
	ptr[3] = val;
}

ulong
nhgetl(uchar *ptr)
{
	return ((ptr[0]<<24) | (ptr[1]<<16) | (ptr[2]<<8) | ptr[3]);
}

ushort
nhgets(uchar *ptr)
{
	return ((ptr[0]<<8) | ptr[1]);
}
