#include	"u.h"
#include	"lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"

#define DPRINT if(q->flag&QDEBUG)kprint

typedef struct Lapd	Lapd;

#define NOW		TK2MS(MACHP(0)->ticks)
#define	T200		(NOW+2500)
#define	SAPI(s, cr)	(((s)<<2)|cr)
#define	TEI(t)		(((t)<<1)|1)

/*
 *  Commands and responses
 */
enum
{
	RR	= 0x01,	/* Receiver Ready */
	RNR	= 0x05,	/* Receiver Not Ready */
	REJ	= 0x09,	/* Reject */
	SABME	= 0x6f,	/* Set Asynchronous Balanced Mode Extended */
	DM	= 0x0f,	/* Disconnect Mode */
	UI	= 0x03,	/* Unnumbered Information */
	DISC	= 0x43,	/* Disconnect */
	UA	= 0x63,	/* Unnumbered Acknowledgement */
	FRMR	= 0x87,	/* Frame Reject */
	XID	= 0xaf,	/* Exchange Identification */

	/* C/R bits */
	UCmd	= 0,	/* command, user to network */
	UResp	= 2,	/* response, user to network */
	NCmd	= 2,	/* command, network to user */
	NResp	= 0	/* response, network to user */
};

enum States
{
	Unused = 0,
	NoTEI,
	WaitTEI,
	TEIassigned,
	WaitEst,
	Multiframe,
	WaitRel,
};

enum Flags
{
	Ownbusy 	= 0x01,
	Peerbusy 	= 0x02,
	Rejecting	= 0x04,
	Timeout		= 0x08,
	Hungup		= 0x80
};

#define	SMASK		127
#define	NSEQ		(SMASK+1)
#define	THISS(i)	((i)&SMASK)
#define	NEXTS(i)	(((i)+1)&SMASK)

#define	OMASK		7
#define	NOBUF		(OMASK+1)
#define	THISO(i)	((i)&OMASK)
#define	NEXTO(i)	(((i)+1)&OMASK)

#define	RSTATE(lp)	((lp->flags&Ownbusy) ? RNR : \
			(lp->flags&Rejecting) ? REJ : RR)

struct Lapd
{
	QLock;		/* access, state change */
	Rendez;		/* waiting for state change */
	int	klocked;/* locked by kernel proc */
	int	state;
	int	flags;
	Queue *	rq;
	Queue *	wq;
	ushort	refnum;
	int	tei;
	int	kout;	/* maximum allowed number of outstanding I frames */
	int	vs;	/* send state variable (next seq. no. to send) */
	int	va;	/* acknowledge state variable (next expected ack) */
	int	vr;	/* receive state variable (next expected input) */
	Rendez	tr;	/* so transmit writer can wait */
	Block *	outb[NOBUF]; /* transmitter circular buffer */
	int	vt;	/* input pointer for outb */
	int	t200;
	Rendez	timer;
	int	kstarted;
	int	error;
};

enum {
	LAPD_OK,
	Address,
	BadUA,
	Badassign,
	Badmanage,
	Tooshort,
	Unimpmanage,
	Unkdlci,
	Egates,
};

char *lapderrors[] = {
	"LAPD OK",
	"Address",
	"BadUA",
	"Badassign",
	"Badmanage",
	"Tooshort",
	"Unimpmanage",
	"Egates",
};

Lapd	*lapd;

/*
 *  predeclared
 */
static void	assigntei(Lapd *);
static void	dl_establish(Lapd *);
static void	dl_release(Lapd *);
static void	lapdreset(void);
static void	lapdclose(Queue *);
static void	lapdiput(Queue*, Block*);
static void	lapdkproc(void *);
static void	lapdopen(Queue*, Stream*);
static void	lapdoput(Queue*, Block*);
static int	lapdxmit(Lapd *);
static void	mdl_assign(Lapd *);
static void	mdl_iput(Queue *, Block *);
static int	recvack(Lapd *, int);
static void	sendctl(Lapd *, int, int, int);

Qinfo lapdinfo = { lapdiput, lapdoput, lapdopen, lapdclose, "lapd", lapdreset };

#define	Predicate(name, expr)	static int name(void *arg) { return expr; }
#define	lp	((Lapd *)arg)

Predicate(pTEIassigned, lp->state == TEIassigned);
Predicate(pMultiframe, lp->state == Multiframe);

Predicate(tEmpty, lp->vs == lp->va)
Predicate(tFull, NEXTO(lp->vt) == THISO(lp->va))
Predicate(tNotFull, NEXTO(lp->vt) != THISO(lp->va))

#undef	lp

static void
lapderror(Lapd *lp, int code)
{
	lp->error = code;
	nexterror();
}

static void
lapdreset(void)
{
	lapd = xalloc(conf.nlapd*sizeof(Lapd));
}

static void
lapdopen(Queue *q, Stream *s)
{
	Lapd *lp;
	char name[32];

	USED(s);
	for(lp=lapd; lp<&lapd[conf.nlapd]; lp++){
		qlock(lp);
		if(lp->state == 0)
			break;
		qunlock(lp);
	}
	if(lp>=&lapd[conf.nlapd])
		error(Enoifc);

	/*q->flag |= QDEBUG;
	q->other->flag |= QDEBUG;*/
	DPRINT("lapdopen: lapd %d\n", lp - lapd);
	q->ptr = q->other->ptr = lp;
	lp->rq = q;
	lp->wq = q->other;
	lp->state = NoTEI;
	lp->flags = 0;
	lp->kout = 1;
	qunlock(lp);
	sprint(name, "lapd%d", lp - lapd);
	kproc(name, lapdkproc, lp);
	DPRINT("lapdopen: mdl_assign\n");
	mdl_assign(lp);
	DPRINT("lapdopen: dl_establish\n");
	dl_establish(lp);
}

static void
mdl_assign(Lapd *lp)
{
	char buf[ERRLEN];

	qlock(lp);
	if(lp->state != NoTEI){
		qunlock(lp);
		sprint(buf, "mdl_assign: state=%d", lp->state);
		kprint("%s\n", buf);
		error(buf);
	}
	lp->state = WaitTEI;
	assigntei(lp);
	lp->t200 = T200;
	qunlock(lp);
	sleep(lp, pTEIassigned, lp);
	if(lp->state != TEIassigned){
		/*lapdclose(lp->rq);*/
		sprint(buf, "mdl_assign failed: state=%d", lp->state);
		kprint("%s\n", buf);
		error(buf);
	}
}

static void
assigntei(Lapd *lp)
{
	Queue *q = lp->rq;
	Block *b = allocb(16);
	uchar *p = b->wptr;

	lp->refnum = NOW;
	*p++ = SAPI(63, UCmd);
	*p++ = TEI(127);
	*p++ = UI;
	*p++ = 0x0f;
	*p++ = lp->refnum>>8;
	*p++ = lp->refnum;
	*p++ = 0x01;
	*p++ = 0xff;
	b->wptr = p;
	b->flags |= S_DELIM;
	DPRINT("assigntei: refnum=0x%4.4ux\n", lp->refnum);
	PUTNEXT(lp->wq, b);
}

static void
dl_establish(Lapd *lp)
{
	char buf[ERRLEN];

	qlock(lp);
	if(lp->state != TEIassigned){
		qunlock(lp);
		sprint(buf, "dl_establish: state=%d", lp->state);
		kprint("%s\n", buf);
		error(buf);
	}
	lp->state = WaitEst;
	sendctl(lp, UCmd, SABME, 0);
	lp->t200 = T200;
	qunlock(lp);
	sleep(lp, pMultiframe, lp);
	if(lp->state != Multiframe){
		/*lapdclose(lp->rq);*/
		sprint(buf, "dl_establish failed: state=%d", lp->state);
		kprint("%s\n", buf);
		error(buf);
	}
}

static void
dl_release(Lapd *lp)
{
	char buf[ERRLEN];

	qlock(lp);
	if(lp->state != Multiframe){
		qunlock(lp);
		sprint(buf, "dl_release: state=%d", lp->state);
		kprint("%s\n", buf);
		error(buf);
	}
	lp->state = WaitRel;
	sendctl(lp, UCmd, DISC, 0);
	lp->t200 = T200;
	qunlock(lp);
	tsleep(lp, pTEIassigned, lp, 30*1000);
	qlock(lp);
	if(lp->state > TEIassigned)
		lp->state = TEIassigned;
	qunlock(lp);
}

static void
lapdclose(Queue *q)
{
	Lapd *lp = (Lapd *)q->ptr;
	int i;

	if(lp == 0)
		return;

	if(lp->state == Multiframe){
		for(i=0; i<NOBUF && !tEmpty(lp); i++)
			tsleep(&lp->tr, tEmpty, lp, 2500);	/* drain */
		dl_release(lp);					/* DISC to NT */
	}
	for(i=0; i<NOBUF; i++)
		if(lp->outb[i]){
			freeb(lp->outb[i]);
			lp->outb[i] = 0;
		}
	lp->flags |= Hungup;			/* tell kernel proc */
	lp->t200 = NOW;
	wakeup(&lp->timer);
	for(i=0; i<10 && lp->kstarted; i++)
		tsleep(lp, return0, 0, 500);

	lp->state = 0;
}

/*
 *  upstream control messages
 */
static void
lapdctliput(Lapd *lp, Queue *q, Block *bp)
{
	switch(bp->type){
	case M_HANGUP:
		lp->flags |= Hungup;
		wakeup(lp);
		break;
	}
	PUTNEXT(q, bp);
}

void
lapdiput(Queue *q, Block *bp)
{
	Lapd *lp = (Lapd *)q->ptr;
	int sapi, cr, tei, ctl, ns, nr = 0, pf;
	uchar *start, *p;

	DPRINT("lapdiput: %d byte(s)\n", bp->wptr-bp->rptr);
	qlock(lp);
	lp->error = 0;
	p = start = bp->rptr;
	if (waserror()) {
		qunlock(lp);
		kprint("lapdiput: %s:", lapderrors[lp->error]);
		for(p=start; p<bp->wptr; p++)
			kprint(" %2.2ux", *p);
		kprint("\n");
		freeb(bp);
		return;
	}

	if(bp->type != M_DATA){
		lapdctliput(lp, q, bp);
		bp = 0;
		goto out;
	}
	if (bp->wptr-p < 3)
		lapderror(lp, Tooshort);
	if ((p[0] & 0x01) || !(p[1] & 0x01))
		lapderror(lp, Address);
	sapi = p[0] >> 2;
	cr = p[0] & 0x02;
	tei = p[1] >> 1;
	ctl = p[2];
	if ((ctl&0x03) != 0x03) {
		if (bp->wptr-p < 4)
			lapderror(lp, Tooshort);
		pf = p[3]&0x01;
		nr = p[3]>>1;
		p += 4;
	} else {
		pf = ctl&0x10;
		ctl &= ~0x10;
		p += 3;
	}
	bp->rptr = p;
	DPRINT("lapdiput: cr=%d sapi=%d tei=%d ctl=0x%2.2x pf=%d\n",
		cr, sapi, tei, ctl, pf);
	if (cr==NCmd && tei==127 && ctl==UI) {
		switch (sapi) {
		case 0:
			PUTNEXT(q, bp);
			bp = 0;
			break;
		case 63:
			mdl_iput(q, bp);
			bp = 0;
			break;
		default:
			lapderror(lp, Unkdlci);
			break;
		}
		goto out;
	}
	if (sapi != 0 || lp->state < TEIassigned || tei != lp->tei)
		lapderror(lp, Unkdlci);
	if (!(ctl&0x01)) {	/* Information */
		if (recvack(lp, nr)>0 && !(lp->flags&(Peerbusy|Timeout)))
			lp->t200 = 0;
		if (lp->flags&Ownbusy) {
			if (pf)
				sendctl(lp, cr, RNR, pf);
			lapdxmit(lp);
			goto out;
		}
		ns = ctl>>1;
		if (ns != lp->vr) {
			lp->flags |= Rejecting;
			sendctl(lp, cr, REJ, pf);
			lapdxmit(lp);
			goto out;
		}
		PUTNEXT(q, bp);
		bp = 0;
		lp->flags &= ~Rejecting;
		lp->vr = NEXTS(lp->vr);
		if (QFULL(q->next))
			lp->flags |= Ownbusy;
		if (pf || (lp->flags&Ownbusy)) {
			sendctl(lp, cr, RSTATE(lp), pf);
			lapdxmit(lp);
		} else if (!lapdxmit(lp))
			sendctl(lp, cr, RR, pf);
		goto out;
	}
	switch (ctl) {
	case RR:
		if (recvack(lp, nr)<0)
			goto out;
		lp->flags &= ~Peerbusy;
		if (cr==NCmd && pf)
			sendctl(lp, cr, RSTATE(lp), pf);
		if (!(lp->flags&Timeout)) {
			lp->t200 = 0;
			if (cr==NResp && pf)
				DPRINT("lapdiput: RR response with F==1\n");
			lapdxmit(lp);
		} else if (cr==NResp && pf) {
			lp->flags &= ~Timeout;
			lp->vs = nr;
			lp->t200 = 0;
			lapdxmit(lp);
		}
		break;
	case RNR:
		if (recvack(lp, nr)<0)
			goto out;
		lp->flags |= Peerbusy;
		if (cr==NCmd && pf)
			sendctl(lp, cr, RSTATE(lp), pf);
		if (!(lp->flags&Timeout)) {
			lp->t200 = T200;
			if (cr==NResp && pf)
				DPRINT("lapdiput: RNR response with F==1\n");
		} else if (cr==NResp && pf) {
			lp->flags &= ~Timeout;
			lp->vs = nr;
			lp->t200 = T200;
		}
		break;
	case REJ:
		if (recvack(lp, nr)<0)
			goto out;
		lp->flags &= ~Peerbusy;
		if (cr==NCmd && pf)
			sendctl(lp, cr, RSTATE(lp), pf);
		if (!(lp->flags&Timeout)) {
			lp->vs = nr;
			lp->t200 = 0;
			if (cr==NResp && pf)
				DPRINT("lapdiput: REJ response with F==1\n");
			lapdxmit(lp);
		} else if (cr==NResp && pf) {
			lp->flags &= ~Timeout;
			lp->vs = nr;
			lp->t200 = 0;
			lapdxmit(lp);
		}
		break;
	case DM:
		lp->state = TEIassigned;
		lp->flags = 0;
		lp->t200 = 0;
		wakeup(lp);
		break;
	case UA:
		if (lp->state == WaitEst) {
			lp->state = Multiframe;
			lp->flags = 0;
			lp->vs = 0;
			lp->va = 0;
			lp->vr = 0;
			lp->vt = 0;
			lp->t200 = 0;
			wakeup(lp);
		} else if (lp->state == WaitRel) {
			lp->state = TEIassigned;
			lp->flags = 0;
			lp->t200 = 0;
			wakeup(lp);
		} else
			lapderror(lp, BadUA);
		break;
	case SABME:
	case UI:
	case DISC:
	case FRMR:
	case XID:
		lapderror(lp, Egates);
		break;
	}
out:
	if (bp)
		freeb(bp);
	poperror();
	qunlock(lp);
}

static void
mdl_iput(Queue *q, Block *bp)
{
	Lapd *lp = (Lapd *)q->ptr;
	uchar *p = bp->rptr;
	ushort refnum;
	int ai;

	if (bp->wptr-p != 5 || p[0] != 0x0f || !(p[4]&0x01))
		lapderror(lp, Badmanage);
	refnum = (p[1]<<8)|p[2];
	ai = p[4]>>1;
	switch (p[3]) {
	case 2:		/* identity assigned */
		if (lp->state == WaitTEI && lp->refnum == refnum) {
			lp->state = TEIassigned;
			lp->tei = ai;
			lp->t200 = 0;
			DPRINT("assigned tei = %d\n", lp->tei);
			wakeup(lp);
		} else
			lapderror(lp, Badassign);
		break;
	case 3:		/* identity denied */
		if (lp->state == WaitTEI && lp->refnum == refnum) {
			lp->t200 = 0;
			DPRINT("denied tei = %d\n", ai);
			wakeup(lp);
		} else
			lapderror(lp, Badassign);
		break;
	case 4:		/* identity check request */
		if (lp->state >= TEIassigned && (ai==127||ai==lp->tei)) {
			Block *b = allocb(16);
			uchar *p = b->wptr;
		
			*p++ = SAPI(63, UCmd);
			*p++ = TEI(127);
			*p++ = UI;
			*p++ = 0x0f;
			*p++ = NOW>>8;
			*p++ = NOW;
			*p++ = 0x05;
			*p++ = (lp->tei<<1)|1;
			b->wptr = p;
			b->flags |= S_DELIM;
			DPRINT("id check response: tei=%d\n", lp->tei);
			PUTNEXT(lp->wq, b);
		} else
			lapderror(lp, Badassign);
		break;
	case 6:		/* identity removal */
		DPRINT("id removal: ai=%d\n", ai);
		if(lp->state >= TEIassigned && (ai == lp->tei || ai == 127)){
			Block *b = allocb(0);
			lp->state = NoTEI;
			DPRINT("lapd hangup\n");
			b->type = M_HANGUP;
			PUTNEXT(lp->rq, b);
		}
		break;
	default:
		{
			Block *b = allocb(0);
			b->type = M_HANGUP;
			PUTNEXT(lp->rq, b);
		}
		lapderror(lp, Unimpmanage);
		break;
	}
	freeb(bp);
}

static int
recvack(Lapd *lp, int nr)
{
	Queue *q = lp->wq;
	int i;

	for(i=lp->va; i!=lp->vs; i=NEXTS(i)){
		if(i==nr)
			break;
	}
	if(i!=nr) {
		DPRINT("lapd recvack: va,nr,vs = %d,%d,%d\n",
			lp->va, nr, lp->vs);
		return -1;
	}
	i = 0;
	while (lp->va != nr) {
		freeb(lp->outb[THISO(lp->va)]);
		lp->outb[THISO(lp->va)] = 0;
		lp->va = NEXTS(lp->va);
		i++;
	}
	if (i > 0)
		wakeup(&lp->tr);
	return i;
}

/*
 *  downstream control
 */
static void
lapdctloput(Lapd *lp, Queue *q, Block *bp)
{
	char *fields[2];

	USED(lp);
	switch(bp->type){
	case M_CTL:
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
lapdoput(Queue *q, Block *bp)
{
	Lapd *lp = (Lapd *)q->ptr;

	if(bp->type != M_DATA){
		lapdctloput(lp, q, bp);
		return;
	}
	if(!putq(q, bp))	/* send only whole messages */
		return;
	DPRINT("lapdoput: len=%d\n", q->len);
	if (tFull(lp))
		sleep(&lp->tr, tNotFull, lp);
	qlock(lp);
	if (waserror()) {
		qunlock(lp);
		nexterror();
	}
	lp->outb[lp->vt] = grabq(q);
	lp->vt = NEXTO(lp->vt);

	lapdxmit(lp);
	poperror();
	qunlock(lp);
}

static int
lapdxmit(Lapd *lp)
{
	Block *bp, *db;
	int i; uchar *p;

	for (i=0;;i++) {
		if (THISO(lp->vs) == lp->vt)		/* buffer empty */
			break;
		if (THISS(lp->va+lp->kout) == lp->vs)	/* window full */
			break;
		if (lp->flags & Peerbusy)		/* flow-controlled */
			break;
		bp = allocb(16);
		p = bp->wptr;
		*p++ = SAPI(0, UCmd);
		*p++ = TEI(lp->tei);
		*p++ = lp->vs<<1;
		*p++ = lp->vr<<1;
		bp->wptr = p;
		PUTNEXT(lp->wq, bp);
		for(bp=lp->outb[THISO(lp->vs)]; bp; bp=bp->next){
			db = allocb(0);
			db->rptr = bp->rptr;
			db->wptr = bp->wptr;
			db->flags |= (bp->flags&S_DELIM);
			PUTNEXT(lp->wq, db);
		}
		lp->vs = NEXTS(lp->vs);
		lp->t200 = T200;
	}
	return i;
}

/*
 *  send a control frame.
 */
static void
sendctl(Lapd *lp, int cmd, int ctl, int pf)
{
	Block *bp = allocb(16);
	uchar *p = bp->wptr;

	*p++ = SAPI(0, cmd);
	*p++ = TEI(lp->tei);
	if (ctl & 0x02) {
		*p++ = ctl|(pf?0x10:0);
	} else {
		*p++ = ctl;
		*p++ = (lp->vr<<1)|(pf?0x01:0);
	}
	bp->wptr = p;
	bp->flags |= S_DELIM;
	PUTNEXT(lp->wq, bp);
}

static void
lapdkproc(void *arg)
{
	Lapd *lp = (Lapd *)arg;

	lp->klocked = 0;
	lp->kstarted = 1;

	if(waserror()){
		if(lp->klocked)
			qunlock(lp);
		lp->klocked = 0;
		print("lapdkproc %d error\n", lp-lapd);
		lp->kstarted = 0;
		wakeup(lp);
		return;
	}
	for(;;){
		tsleep(&lp->timer, return0, 0, 500);
		if(!lp->t200 || NOW < lp->t200)
			continue;
		qlock(lp);
		lp->klocked = 1;
		if(lp->flags & Hungup){
			qunlock(lp);
			lp->klocked = 0;
			break;
		}
		switch(lp->state){
		case WaitTEI:
			assigntei(lp);
			lp->t200 = T200;
			break;
		case WaitEst:
			sendctl(lp, UCmd, SABME, 0);
			break;
		case Multiframe:
			sendctl(lp, UCmd, RSTATE(lp), 1);
			lp->flags |= Timeout;
			lp->t200 = T200;
			break;
		}
		qunlock(lp);
		lp->klocked = 0;
	}
	lp->kstarted = 0;
	wakeup(lp);
	poperror();
}
