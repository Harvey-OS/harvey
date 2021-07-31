#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"

enum {
	MSrexmit=	1000,
	Nmask=		0x7,
};

#define DPRINT if(q->flag&QDEBUG)kprint

typedef struct Urp	Urp;

#define NOW (MACHP(0)->ticks*MS2HZ)

/*
 * URP status
 */
struct urpstat {
	ulong	input;		/* bytes read from urp */
	ulong	output;		/* bytes output to urp */
	ulong	rexmit;		/* retransmit rejected urp msg */
	ulong	rjtrs;		/* reject, trailer size */
	ulong	rjpks;		/* reject, packet size */
	ulong	rjseq;		/* reject, sequence number */
	ulong	levelb;		/* unknown level b */
	ulong	enqsx;		/* enqs sent */
	ulong	enqsr;		/* enqs rcved */
} urpstat;

struct Urp {
	QLock;
	Urp	*list;		/* list of all urp structures */
	short	state;		/* flags */
	Rendez	r;		/* process waiting for output to finish */

	/* input */
	QLock	ack;		/* ack lock */
	Queue	*rq;		/* input queue */
	uchar	iseq;		/* last good input sequence number */
	uchar	lastecho;	/* last echo/rej sent */
	uchar	trbuf[3];	/* trailer being collected */
	short	trx;		/* # bytes in trailer being collected */
	int	blocks;

	/* output */
	QLock	xmit;		/* output lock, only one process at a time */
	Queue	*wq;		/* output queue */
	int	maxout;		/* maximum outstanding unacked blocks */
	int	maxblock;	/* max block size */
	int	next;		/* next block to send */
	int	unechoed;	/* first unechoed block */
	int	unacked;	/* first unacked block */
	int	nxb;		/* next xb to use */
	Block	*xb[8];		/* the xmit window buffer */
	QLock	xl[8];
	ulong	timer;		/* timeout for xmit */
	int	rexmit;
};

/* list of allocated urp structures (never freed) */
struct
{
	Lock;
	Urp	*urp;
} urpalloc;

Rendez	urpkr;
QLock	urpkl;
int	urpkstarted;

#define WINDOW(u) ((u)->unechoed>(u)->next ? (u)->unechoed+(u)->maxout-(u)->next-8 :\
			(u)->unechoed+(u)->maxout-(u)->next)
#define IN(x, f, n) (f<=n ? (x>=f && x<n) : (x<n || x>=f))
#define NEXT(x) (((x)+1)&Nmask)

/*
 *  Protocol control bytes
 */
#define	SEQ	0010		/* sequence number, ends trailers */
#undef	ECHO
#define	ECHO	0020		/* echos, data given to next queue */
#define	REJ	0030		/* rejections, transmission error */
#define	ACK	0040		/* acknowledgments */
#define	BOT	0050		/* beginning of trailer */
#define	BOTM	0051		/* beginning of trailer, more data follows */
#define	BOTS	0052		/* seq update algorithm on this trailer */
#define	SOU	0053		/* start of unsequenced trailer */
#define	EOU	0054		/* end of unsequenced trailer */
#define	ENQ	0055		/* xmitter requests flow/error status */
#define	CHECK	0056		/* xmitter requests error status */
#define	INITREQ 0057		/* request initialization */
#define	INIT0	0060		/* disable trailer processing */
#define	INIT1	0061		/* enable trailer procesing */
#define	AINIT	0062		/* response to INIT0/INIT1 */
#undef	DELAY
#define	DELAY	0100		/* real-time printing delay */
#define	BREAK	0110		/* Send/receive break (new style) */

#define	REJECTING	0x1
#define	INITING 	0x2
#define HUNGUP		0x4
#define	OPEN		0x8
#define CLOSING		0x10

/*
 *  predeclared
 */
static void	urpreset(void);
static void	urpciput(Queue*, Block*);
static void	urpiput(Queue*, Block*);
static void	urpoput(Queue*, Block*);
static void	urpopen(Queue*, Stream*);
static void	urpclose(Queue *);
static void	output(Urp*);
static void	sendblock(Urp*, int);
static void	rcvack(Urp*, int);
static void	flushinput(Urp*);
static void	sendctl(Urp*, int);
static void	sendack(Urp*);
static void	sendrej(Urp*);
static void	initoutput(Urp*, int);
static void	initinput(Urp*);
static void	urpkproc(void *arg);
static void	urpvomit(char*, Urp*);
static void	tryoutput(Urp*);

Qinfo urpinfo =
{
	urpciput,
	urpoput,
	urpopen,
	urpclose,
	"urp",
	urpreset
};

static void
urpreset(void)
{
}

static void
urpopen(Queue *q, Stream *s)
{
	Urp *up;

	USED(s);
	if(!urpkstarted){
		qlock(&urpkl);
		if(!urpkstarted){
			urpkstarted = 1;
			kproc("urpkproc", urpkproc, 0);
		}
		qunlock(&urpkl);
	}

	/*
	 *  find an unused urp structure
	 */
	for(up = urpalloc.urp; up; up = up->list){
		if(up->state == 0){
			qlock(up);
			if(up->state == 0)
				break;
			qunlock(up);
		}
	}
	if(up == 0){
		/*
		 *  none available, create a new one, they are never freed
		 */
		up = smalloc(sizeof(Urp));
		qlock(up);
		lock(&urpalloc);
		up->list = urpalloc.urp;
		urpalloc.urp = up;
		unlock(&urpalloc);
	}
	q->ptr = q->other->ptr = up;
	q->rp = &urpkr;
	up->rq = q;
	up->wq = q->other;
	up->state = OPEN;
	qunlock(up);
	initinput(up);
	initoutput(up, 0);
}

/*
 *  Shut down the connection and kill off the kernel process
 */
static int
isflushed(void *a)
{
	Urp *up;

	up = (Urp *)a;
	return (up->state&HUNGUP) || (up->unechoed==up->nxb && up->wq->len==0);
}
static void
urpclose(Queue *q)
{
	Urp *up;
	int i;

	up = (Urp *)q->ptr;
	if(up == 0)
		return;

	/*
	 *  wait for all outstanding messages to drain, tell kernel
	 *  process we're closing.
	 *
	 *  if 2 minutes elapse, give it up
	 */
	up->state |= CLOSING;
	if(!waserror()){
		tsleep(&up->r, isflushed, up, 2*60*1000);
		poperror();
	}
	up->state |= HUNGUP;

	qlock(&up->xmit);
	/*
	 *  ack all outstanding messages
	 */
	i = up->next - 1;
	if(i < 0)
		i = 7;
	rcvack(up, ECHO+i);

	/*
	 *  free all staged but unsent messages
	 */
	for(i = 0; i < 7; i++)
		if(up->xb[i]){
			freeb(up->xb[i]);
			up->xb[i] = 0;
		}
	qunlock(&up->xmit);

	qlock(up);
	up->state = 0;
	qunlock(up);
}

/*
 *  upstream control messages
 */
static void
urpctliput(Urp *up, Queue *q, Block *bp)
{
	switch(bp->type){
	case M_HANGUP:
		up->state |= HUNGUP;
		wakeup(&up->r);
		break;
	}
	PUTNEXT(q, bp);
}

/*
 *  character mode input.
 *
 *  the first byte in every message is a ctl byte (which belongs at the end).
 */
void
urpciput(Queue *q, Block *bp)
{
	Urp *up;
	int i;
	int ctl;

	up = (Urp *)q->ptr;
	if(up == 0)
		return;
	if(bp->type != M_DATA){
		urpctliput(up, q, bp);
		return;
	}

	/*
	 *  get the control character
	 */
	ctl = *bp->rptr++;
	if(ctl < 0)
		return;

	/*
	 *  take care of any data
	 */
	if(BLEN(bp)>0  && q->next->len<2*Streamhi && q->next->nb<2*Streambhi){
		bp->flags |= S_DELIM;
		urpstat.input += BLEN(bp);
		PUTNEXT(q, bp);
	} else
		freeb(bp);

	/*
	 *  handle the control character
	 */
	switch(ctl){
	case 0:
		break;
	case ENQ:
		DPRINT("rENQ(c)\n");
		urpstat.enqsr++;
		sendctl(up, up->lastecho);
		sendctl(up, ACK+up->iseq);
		break;

	case CHECK:
		DPRINT("rCHECK(c)\n");
		sendctl(up, ACK+up->iseq);
		break;

	case AINIT:
		DPRINT("rAINIT(c)\n");
		up->state &= ~INITING;
		flushinput(up);
		tryoutput(up);
		break;

	case INIT0:
	case INIT1:
		DPRINT("rINIT%d(c)\n", ctl-INIT0);
		sendctl(up, AINIT);
		if(ctl == INIT1)
			q->put = urpiput;
		initinput(up);
		break;

	case INITREQ:
		DPRINT("rINITREQ(c)\n");
		initoutput(up, 0);
		break;

	case BREAK:
		break;

	case REJ+0: case REJ+1: case REJ+2: case REJ+3:
	case REJ+4: case REJ+5: case REJ+6: case REJ+7:
		DPRINT("rREJ%d(c)\n", ctl-REJ);
		rcvack(up, ctl);
		break;
	
	case ACK+0: case ACK+1: case ACK+2: case ACK+3:
	case ACK+4: case ACK+5: case ACK+6: case ACK+7:
	case ECHO+0: case ECHO+1: case ECHO+2: case ECHO+3:
	case ECHO+4: case ECHO+5: case ECHO+6: case ECHO+7:
		DPRINT("%s%d(c)\n", (ctl&ECHO)?"rECHO":"rACK", ctl&7);
		rcvack(up, ctl);
		break;

	case SEQ+0: case SEQ+1: case SEQ+2: case SEQ+3:
	case SEQ+4: case SEQ+5: case SEQ+6: case SEQ+7:
		DPRINT("rSEQ%d(c)\n", ctl-SEQ);
		qlock(&up->ack);
		i = ctl & Nmask;
		if(!QFULL(q->next))
			sendctl(up, up->lastecho = ECHO+i);
		up->iseq = i;
		qunlock(&up->ack);
		break;
	}
}

/*
 *  block mode input.
 *
 *  the first byte in every message is a ctl byte (which belongs at the end).
 *
 *  Simplifying assumption:  one put == one message && the control byte
 *	is in the first block.  If this isn't true, strange bytes will be
 *	used as control bytes.
 *
 *	There's no input lock.  The channel could be closed while we're
 *	processing a message.
 */
void
urpiput(Queue *q, Block *bp)
{
	Urp *up;
	int i, len;
	int ctl;

	up = (Urp *)q->ptr;
	if(up == 0)
		return;
	if(bp->type != M_DATA){
		urpctliput(up, q, bp);
		return;
	}

	/*
	 *  get the control character
	 */
	ctl = *bp->rptr++;

	/*
	 *  take care of any block count(trx)
	 */
	while(up->trx){
		if(BLEN(bp)<=0)
			break;
		switch (up->trx) {
		case 1:
		case 2:
			up->trbuf[up->trx++] = *bp->rptr++;
			continue;
		default:
			up->trx = 0;
			break;
		}
	}

	/*
	 *  queue the block(s)
	 */
	if(BLEN(bp) > 0){
		bp->flags &= ~S_DELIM;
		putq(q, bp);
		if(q->len > 4*1024){
			flushinput(up);
			return;
		}
	} else
		freeb(bp);

	/*
	 *  handle the control character
	 */
	switch(ctl){
	case 0:
		break;
	case ENQ:
		DPRINT("rENQ %d %uo %uo\n", up->blocks, up->lastecho, ACK+up->iseq);
		up->blocks = 0;
		urpstat.enqsr++;
		sendctl(up, up->lastecho);
		sendctl(up, ACK+up->iseq);
		flushinput(up);
		break;

	case CHECK:
		DPRINT("rCHECK\n");
		sendctl(up, ACK+up->iseq);
		break;

	case AINIT:
		DPRINT("rAINIT\n");
		up->state &= ~INITING;
		flushinput(up);
		tryoutput(up);
		break;

	case INIT0:
	case INIT1:
		DPRINT("rINIT%d\n", ctl-INIT0);
		sendctl(up, AINIT);
		if(ctl == INIT0)
			q->put = urpciput;
		initinput(up);
		break;

	case INITREQ:
		DPRINT("rINITREQ\n");
		initoutput(up, 0);
		break;

	case BREAK:
		break;

	case BOT:
	case BOTM:
	case BOTS:
		DPRINT("rBOT%c...", " MS"[ctl-BOT]);
		up->trx = 1;
		up->trbuf[0] = ctl;
		break;

	case REJ+0: case REJ+1: case REJ+2: case REJ+3:
	case REJ+4: case REJ+5: case REJ+6: case REJ+7:
		DPRINT("rREJ%d\n", ctl-REJ);
		rcvack(up, ctl);
		break;
	
	case ACK+0: case ACK+1: case ACK+2: case ACK+3:
	case ACK+4: case ACK+5: case ACK+6: case ACK+7:
	case ECHO+0: case ECHO+1: case ECHO+2: case ECHO+3:
	case ECHO+4: case ECHO+5: case ECHO+6: case ECHO+7:
		DPRINT("%s%d\n", (ctl&ECHO)?"rECHO":"rACK", ctl&7);
		rcvack(up, ctl);
		break;

	/*
	 *  if the sequence number is the next expected
	 *	and the trailer length == 3
	 *	and the block count matches the bytes received
	 *  then send the bytes upstream.
	 */
	case SEQ+0: case SEQ+1: case SEQ+2: case SEQ+3:
	case SEQ+4: case SEQ+5: case SEQ+6: case SEQ+7:
		len = up->trbuf[1] + (up->trbuf[2]<<8);
		DPRINT("rSEQ%d(%d,%d,%d)...", ctl-SEQ, up->trx, len, q->len);
		i = ctl & Nmask;
		if(up->trx != 3){
			urpstat.rjtrs++;
			sendrej(up);
			break;
		}else if(q->len != len){
			urpstat.rjpks++;
			sendrej(up);
			break;
		}else if(i != ((up->iseq+1)&Nmask)){
			urpstat.rjseq++;
			sendrej(up);
			break;
		}else if(q->next->len > (3*Streamhi)/2
			|| q->next->nb > (3*Streambhi)/2){
			DPRINT("next->len=%d, next->nb=%d\n",
				q->next->len, q->next->nb);
			flushinput(up);
			break;
		}
		DPRINT("accept %d\n", q->len);

		/*
		 *  send data upstream
		 */
		if(q->first) {
			if(up->trbuf[0] != BOTM)
				q->last->flags |= S_DELIM;
			while(bp = getq(q)){
				urpstat.input += BLEN(bp);
				PUTNEXT(q, bp);
			}
		} else {
			bp = allocb(0);
			if(up->trbuf[0] != BOTM)
				bp->flags |= S_DELIM;
			PUTNEXT(q, bp);
		}
		up->trx = 0;

		/*
		 *  acknowledge receipt
		 */
		qlock(&up->ack);
		up->iseq = i;
		if(!QFULL(q->next))
			sendctl(up, up->lastecho = ECHO|i);
		qunlock(&up->ack);
		break;
	}
}

/*
 *  downstream control
 */
static void
urpctloput(Urp *up, Queue *q, Block *bp)
{
	char *fields[2];
	int outwin;

	switch(bp->type){
	case M_CTL:
		if(streamparse("break", bp)){
			/*
			 *  send a break as part of the data stream
			 */
			urpstat.output++;
			bp->wptr = bp->lim;
			bp->rptr = bp->wptr - 1;
			*bp->rptr = BREAK;
			putq(q, bp);
			output(up);
			return;
		}
		if(streamparse("init", bp)){
			outwin = strtoul((char*)bp->rptr, 0, 0);
			initoutput(up, outwin);
			freeb(bp);
			return;
		}
		if(streamparse("debug", bp)){
			switch(getfields((char *)bp->rptr, fields, 2, ' ')){
			case 1:
				if (strcmp(fields[0], "on") == 0) {
					q->flag |= QDEBUG;
					q->other->flag |= QDEBUG;
				}
				if (strcmp(fields[0], "off") == 0) {
					q->flag &= ~QDEBUG;
					q->other->flag &= ~QDEBUG;
				}
			}
			freeb(bp);
			return;
		}
	}
	PUTNEXT(q, bp);
}

/*
 *  accept data from a writer
 */
static void
urpoput(Queue *q, Block *bp)
{
	Urp *up;

	up = (Urp *)q->ptr;

	if(bp->type != M_DATA){
		urpctloput(up, q, bp);
		return;
	}

	urpstat.output += BLEN(bp);
	putq(q, bp);
	output(up);
}

/*
 *  start output
 */
static void
output(Urp *up)
{
	Block *bp, *nbp;
	ulong now;
	Queue *q;
	int i;

	if(!canqlock(&up->xmit))
		return;

	if(waserror()){
		print("urp output error\n");
		qunlock(&up->xmit);
		nexterror();
	}

	/*
	 *  if still initing and it's time to rexmit, send an INIT1
	 */
	now = NOW;
	if(up->state & INITING){
		if(now > up->timer){
			q = up->wq;
			DPRINT("INITING timer (%d, %d): ", now, up->timer);
			sendctl(up, INIT1);
			up->timer = now + MSrexmit;
		}
		goto out;
	}

	/*
	 *  fill the transmit buffers, `nxb' can never overtake `unechoed'
	 */
	q = up->wq;
	i = NEXT(up->nxb);
	if(i != up->unechoed) {
		for(bp = getq(q); bp && i!=up->unechoed; i = NEXT(i)){
			if(up->xb[up->nxb] != 0)
				urpvomit("output", up);
			if(BLEN(bp) > up->maxblock){
				nbp = up->xb[up->nxb] = allocb(0);
				nbp->rptr = bp->rptr;
				nbp->wptr = bp->rptr = bp->rptr + up->maxblock;
			} else {
				up->xb[up->nxb] = bp;
				bp = getq(q);
			}
			up->nxb = i;
		}
		if(bp)
			putbq(q, bp);
	}

	/*
	 *  retransmit cruft
	 */
	if(up->rexmit){
		/*
		 *  if a retransmit is requested, move next back to
		 *  the unacked blocks
		 */
		urpstat.rexmit++;
		up->rexmit = 0;
		up->next = up->unacked;
	} else if(up->unechoed!=up->next && NOW>up->timer){
		/*
		 *  if a retransmit time has elapsed since a transmit,
		 *  send an ENQ
		 */
		DPRINT("OUTPUT timer (%d, %d): ", NOW, up->timer);
		up->timer = NOW + MSrexmit;
		up->state &= ~REJECTING;
		urpstat.enqsx++;
		sendctl(up, ENQ);
		goto out;
	}

	/*
	 *  if there's a window open, push some blocks out
	 *
	 *  the lock is to synchronize with acknowledges that free
	 *  blocks.
	 */
	while(WINDOW(up)>0 && up->next!=up->nxb){
		i = up->next;
		qlock(&up->xl[i]);
		if(waserror()){
			qunlock(&up->xl[i]);
			nexterror();
		}
		sendblock(up, i);
		qunlock(&up->xl[i]);
		up->next = NEXT(up->next);
		poperror();
	}
out:
	qunlock(&up->xmit);
	poperror();
}

/*
 *  try output, this is called by an input process
 */
void
tryoutput(Urp *up)
{
	if(!waserror()){
		output(up);
		poperror();
	}
}

/*
 *  send a control byte, put the byte at the end of the allocated
 *  space in case a lower layer needs header room.
 */
static void
sendctl(Urp *up, int ctl)
{
	Block *bp;
	Queue *q;

	q = up->wq;
	if(QFULL(q->next))
		return;
	bp = allocb(1);
	bp->wptr = bp->lim;
	bp->rptr = bp->lim-1;
	*bp->rptr = ctl;
	bp->flags |= S_DELIM;
	DPRINT("sCTL %ulx\n", ctl);
	PUTNEXT(q, bp);
}

/*
 *  send a reject
 */
static void
sendrej(Urp *up)
{
	Queue *q = up->wq;
	flushinput(up);
	qlock(&up->ack);
	if((up->lastecho&~Nmask) == ECHO){
		DPRINT("REJ %d\n", up->iseq);
		sendctl(up, up->lastecho = REJ|up->iseq);
	}
	qunlock(&up->ack);
}

/*
 *  send an acknowledge
 */
static void
sendack(Urp *up)
{
	/*
	 *  check the precondition for acking
	 */
	if(QFULL(up->rq->next) || (up->lastecho&Nmask)==up->iseq)
		return;

	if(!canqlock(&up->ack))
		return;

	/*
	 *  check again now that we've locked
	 */
	if(QFULL(up->rq->next) || (up->lastecho&Nmask)==up->iseq){
		qunlock(&up->ack);
		return;
	}

	/*
	 *  send the ack
	 */
	{ Queue *q = up->wq; DPRINT("sendack: "); }
	sendctl(up, up->lastecho = ECHO|up->iseq);
	qunlock(&up->ack);
}

/*
 *  send a block.
 */
static void
sendblock(Urp *up, int bn)
{
	Block *bp, *m, *nbp;
	int n;
	Queue *q;

	q = up->wq;
	up->timer = NOW + MSrexmit;
	if(QFULL(q->next))
		return;

	/*
	 *  message 1, the BOT and the data
	 */
	bp = up->xb[bn];
	if(bp == 0)
		return;
	m = allocb(1);
	m->rptr = m->lim - 1;
	m->wptr = m->lim;
	*m->rptr = (bp->flags & S_DELIM) ? BOT : BOTM;
	nbp = allocb(0);
	nbp->rptr = bp->rptr;
	nbp->wptr = bp->wptr;
	nbp->base = bp->base;
	nbp->lim = bp->lim;
	nbp->flags |= S_DELIM;
	if(bp->type == M_CTL){
		PUTNEXT(q, nbp);
		m->flags |= S_DELIM;
		PUTNEXT(q, m);
	} else {
		m->next = nbp;
		PUTNEXT(q, m);
	}

	/*
	 *  message 2, the block length and the SEQ
	 */
	m = allocb(3);
	m->rptr = m->lim - 3;
	m->wptr = m->lim;
	n = BLEN(bp);
	m->rptr[0] = SEQ | bn;
	m->rptr[1] = n;
	m->rptr[2] = n<<8;
	m->flags |= S_DELIM;
	PUTNEXT(q, m);
	DPRINT("sb %d (%d)\n", bn, up->timer);
}

/*
 *  receive an acknowledgement
 */
static void
rcvack(Urp *up, int msg)
{
	int seqno;
	int next;
	int i;

	seqno = msg&Nmask;
	next = NEXT(seqno);

	/*
	 *  release any acknowledged blocks
	 */
	if(IN(seqno, up->unacked, up->next)){
		for(; up->unacked != next; up->unacked = NEXT(up->unacked)){
			i = up->unacked;
			qlock(&up->xl[i]);
			if(up->xb[i])
				freeb(up->xb[i]);
			else
				urpvomit("rcvack", up);
			up->xb[i] = 0;
			qunlock(&up->xl[i]);
		}
	}

	switch(msg & 0370){
	case ECHO:
		if(IN(seqno, up->unechoed, up->next)) {
			up->unechoed = next;
		}
		/*
		 *  the next reject at the start of a window starts a 
		 *  retransmission.
		 */
		up->state &= ~REJECTING;
		break;
	case REJ:
		if(IN(seqno, up->unechoed, up->next))
			up->unechoed = next;
		/*
		 *  ... FALL THROUGH ...
		 */
	case ACK:
		/*
		 *  start a retransmission if we aren't retransmitting
		 *  and this is the start of a window.
		 */
		if(up->unechoed==next && !(up->state & REJECTING)){
			up->state |= REJECTING;
			up->rexmit = 1;
		}
		break;
	}

	tryoutput(up);
	if(up->state & CLOSING)
		wakeup(&up->r);
}

/*
 * throw away any partially collected input
 */
static void
flushinput(Urp *up)
{
	Block *bp;

	while (bp = getq(up->rq))
		freeb(bp);
	up->trx = 0;
}

/*
 *  initialize output
 */
static void
initoutput(Urp *up, int window)
{
	int i;

	/*
	 *  set output window
	 */
	up->maxblock = window/4;
	if(up->maxblock < 64)
		up->maxblock = 64;
	up->maxblock -= 4;
	up->maxout = 4;

	/*
	 *  set sequence varialbles
	 */
	up->unechoed = 1;
	up->unacked = 1;
	up->next = 1;
	up->nxb = 1;
	up->rexmit = 0;

	/*
	 *  free any outstanding blocks
	 */
	for(i = 0; i < 8; i++){
		qlock(&up->xl[i]);
		if(up->xb[i])
			freeb(up->xb[i]);
		up->xb[i] = 0;
		qunlock(&up->xl[i]);
	}

	/*
	 *  tell the other side we've inited
	 */
	up->state |= INITING;
	up->timer = NOW + MSrexmit;
	{ Queue *q = up->wq; DPRINT("initoutput (%d): ", up->timer); }
	sendctl(up, INIT1);
}

/*
 *  initialize input
 */
static void
initinput(Urp *up)
{
	/*
	 *  restart all sequence parameters
	 */
	up->blocks = 0;
	up->trx = 0;
	up->iseq = 0;
	up->lastecho = ECHO+0;
	flushinput(up);
}

static void
urpkproc(void *arg)
{
	Urp *up;

	USED(arg);

	if(waserror())
		;

	for(;;){
		for(up = urpalloc.urp; up; up = up->list){
			if(up->state==0 || (up->state&HUNGUP))
				continue;
			if(!canqlock(up))
				continue;
			if(waserror()){
				qunlock(up);
				continue;
			}
			if(up->state==0 || (up->state&HUNGUP)){
				qunlock(up);
				poperror();
				continue;
			}
			if(up->iseq!=(up->lastecho&7) && !QFULL(up->rq->next))
				sendack(up);
			output(up);
			qunlock(up);
			poperror();
		}
		tsleep(&urpkr, return0, 0, 500);
	}
}

/*
 *  urp got very confused, complain
 */
static void
urpvomit(char *msg, Urp* up)
{
	print("urpvomit: %s %ux next %d unechoed %d unacked %d nxb %d\n",
		msg, up, up->next, up->unechoed, up->unacked, up->nxb);
	print("\txb: %ux %ux %ux %ux %ux %ux %ux %ux\n",
		up->xb[0], up->xb[1], up->xb[2], up->xb[3], up->xb[4], 
		up->xb[5], up->xb[6], up->xb[7]);
	print("\tiseq: %uo lastecho: %uo trx: %d trbuf: %uo %uo %uo\n",
		up->iseq, up->lastecho, up->trx, up->trbuf[0], up->trbuf[1],
		up->trbuf[2]);
	print("\tupq: %ux %d %d\n", &up->rq->next->r,  up->rq->next->nb,
		up->rq->next->len);
}

void
urpfillstats(Chan *c, char *buf, int len)
{
	char b[256];

	USED(c);
	sprint(b, "in: %d\nout: %d\nrexmit: %d\nrjtrs: %d\nrjpks: %d\nrjseq: %d\nenqsx: %d\nenqsr: %d\n",
		urpstat.input, urpstat.output, urpstat.rexmit, urpstat.rjtrs,
		urpstat.rjpks, urpstat.rjseq, urpstat.enqsx, urpstat.enqsr);
	strncpy(buf, b, len);
}
