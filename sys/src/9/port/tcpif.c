#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include 	"arp.h"
#include 	"../port/ipdat.h"

extern int tcpdbg;
#define DPRINT	if(tcpdbg) print

void
tcpxstate(Ipconv *s, char oldstate, char newstate)
{
	int len;
	Block *bp;
	Tcpctl *tcb;

	if(oldstate == newstate)
		return;

	tcb = &s->tcpctl;
	switch(newstate) {
	case Closed:
		s->psrc = 0;		/* This connection is toast */
		s->pdst = 0;
		s->dst = 0;
		/* fall through */

	case Close_wait:		/* Remote closes */
		if(s->err) {
			len = strlen(s->err);
			bp = allocb(len);
			strcpy((char *)bp->wptr, s->err);
			bp->wptr += len;
		}
		else
			bp = allocb(0);

		bp->flags |= S_DELIM;
		bp->type = M_HANGUP;
		qlock(s);
		if(waserror()) {
			qunlock(s);
			nexterror();
		}
		if(s->readq == 0) {
			if(newstate == Close_wait)
				putb(&tcb->rcvq, bp);
			else
				freeb(bp);
		} else
			PUTNEXT(s->readq, bp);
		if(tcb){
			tcb->sndfull = 0;
			wakeup(&tcb->sndr);
		}
		poperror();
		qunlock(s);
		break;
	}

	if(oldstate == Syn_sent)
		wakeup(&tcb->syner);
}

static int
notsyner(void *ic)
{
	return ((Tcpctl*)ic)->state != Syn_sent;
}

void
tcpstart(Ipconv *s, int mode, ushort window, char tos)
{
	Tcpctl *tcb;

	tcb = &s->tcpctl;
	if(tcb->state != Closed || tcb->sndq != 0){
		print("tcpstart: %lux %d sndq %lux dest %d.%d.%d.%d %d to %d\n",
			s, tcb->state, tcb->sndq, fmtaddr(s->dst), s->pdst, s->psrc);
		error(Einuse);
	}

	init_tcpctl(s);

	tcb->window = window;
	tcb->rcv.wnd = window;
	tcb->tos = tos;

	switch(mode){
	case TCP_PASSIVE:
		tcb->flags |= CLONE;
		tcpsetstate(s, Listen);
		break;

	case TCP_ACTIVE:
		/* Send SYN, go into SYN_SENT state */
		tcb->flags |= ACTIVE;
		qlock(tcb);
		if(waserror()) {
			qunlock(tcb);
			nexterror();
		}
		tcpsndsyn(tcb);
		tcpsetstate(s, Syn_sent);
		tcpoutput(s);
		poperror();
		qunlock(tcb);
		tsleep(&tcb->syner, notsyner, tcb, 120*1000);
		if(tcb->state != Established && tcb->state != Syn_received){
			if(s->err)
				error(s->err);
			error(Etimedout);
		}
		break;
	}
}
