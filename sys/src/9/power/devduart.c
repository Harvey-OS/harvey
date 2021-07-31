#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"

void	duartputs(IOQ*, char*, int);
void	iprint(char*, ...);

#define	PAD	15	/* registers are well-spaced */

/*
 *  Register set for half the duart.  There are really two sets in adjacent
 *  memory locations.
 */
struct Duartreg
{
	uchar	mr1_2,		/* Mode Register Channels 1 & 2 */
		pad0[PAD];
	uchar	sr_csr,		/* Status Register/Clock Select Register */
		pad1[PAD];
	uchar	cmnd,		/* Command Register */
		pad2[PAD];
	uchar	data,		/* RX Holding / TX Holding Register */
		pad3[PAD];
	uchar	ipc_acr,	/* Input Port Change/Aux. Control Register */
		pad4[PAD];
	uchar	is_imr,		/* Interrupt Status/Interrupt Mask Register */
		pad5[PAD];
	uchar	ctur,		/* Counter/Timer Upper Register */
		pad6[PAD];
	uchar	ctlr,		/* Counter/Timer Lower Register */
		pad7[PAD];
};
#define	ppcr	is_imr		/* in the second register set */

#define DBD75		0
#define DBD110		1
#define DBD38400	2
#define DBD150		3
#define DBD300		4
#define DBD600		5
#define DBD1200		6
#define DBD2000		7
#define DBD2400		8
#define DBD4800		9
#define DBD1800		10
#define DBD9600		11
#define DBD19200	12

enum{
	CHAR_ERR	=0x00,	/* MR1x - Mode Register 1 */
	EVEN_PAR	=0x00,
	ODD_PAR		=0x04,
	NO_PAR		=0x10,
	CBITS8		=0x03,
	CBITS7		=0x02,
	CBITS6		=0x01,
	CBITS5		=0x00,
	NORM_OP		=0x00,	/* MR2x - Mode Register 2 */
	TWOSTOPB	=0x0F,
	ONESTOPB	=0x07,
	ENB_RX		=0x01,	/* CRx - Command Register */
	DIS_RX		=0x02,
	ENB_TX		=0x04,
	DIS_TX		=0x08,
	RESET_MR 	=0x10,
	RESET_RCV  	=0x20,
	RESET_TRANS  	=0x30,
	RESET_ERR  	=0x40,
	RESET_BCH	=0x50,
	STRT_BRK	=0x60,
	STOP_BRK	=0x70,
	RCV_RDY		=0x01,	/* SRx - Channel Status Register */
	FIFOFULL	=0x02,
	XMT_RDY		=0x04,
	XMT_EMT		=0x08,
	OVR_ERR		=0x10,
	PAR_ERR		=0x20,
	FRM_ERR		=0x40,
	RCVD_BRK	=0x80,
	IM_IPC		=0x80,	/* IMRx/ISRx - Interrupt Mask/Interrupt Status */
	IM_DBB		=0x40,
	IM_RRDYB	=0x20,
	IM_XRDYB	=0x10,
	IM_CRDY		=0x08,
	IM_DBA		=0x04,
	IM_RRDYA	=0x02,
	IM_XRDYA	=0x01,
	BD38400		=0xCC|0x0000,
	BD19200		=0xCC|0x0100,
	BD9600		=0xBB|0x0000,
	BD4800		=0x99|0x0000,
	BD2400		=0x88|0x0000,
	BD1200		=0x66|0x0000,
	BD300		=0x44|0x0000,

	Maxduart	=8,
};

/*
 *  requests to perform on a duart
 */
enum {
	Dnone=	0,
	Dbaud,
	Dbreak,
	Ddtr,
	Dprint,
	Dena,
	Dstate,
};

/*
 *  a duart
 */
typedef struct Duart	Duart;
struct Duart
{
	QLock;
	Duartreg	*reg;		/* duart registers */
	uchar		imr;		/* sticky interrupt mask reg bits */
	uchar		acr;		/* sticky auxiliary reg bits */
	int		inited;
};
Duart duart[Maxduart];

/*
 *  values specific to a single duart port
 */
typedef struct Port	Port;
struct Port
{
	QLock;
	int		printing;	/* true if printing */
	Duart		*d;		/* device */
	Duartreg	*reg;		/* duart registers (for this port) */
	int		c;		/* character to restart output */
	int		op;		/* operation requested */
	int		val;		/* value of operation */
	Rendez		opr;		/* waiot here for op to complete */

	/* console interface */
	int		nostream;	/* can't use the stream interface */
	IOQ		*iq;		/* input character queue */
	IOQ		*oq;		/* output character queue */

	/* stream interface */
	Queue		*wq;		/* write queue */
	Rendez		r;		/* kproc waiting for input */
 	int		kstarted;	/* kproc started */
};
Port duartport[2*Maxduart];

/*
 *  configure a duart port, default is 9600 baud, 8 bits/char, 1 stop bit,
 *  no parity
 */
void
duartsetup(Port *p, Duart *d, int devno)
{
	Duartreg *reg;

	p->d = d;
	reg = &d->reg[devno];
	p->reg = reg;

	reg->cmnd = RESET_RCV|DIS_TX|DIS_RX;
	reg->cmnd = RESET_TRANS;
	reg->cmnd = RESET_ERR;
	reg->cmnd = STOP_BRK;

	reg->cmnd = RESET_MR;
	reg->mr1_2 = NO_PAR|CBITS8;
	reg->mr1_2 = ONESTOPB;
	reg->sr_csr = (DBD9600<<4)|DBD9600;
	reg->cmnd = ENB_TX|ENB_RX;
}

/*
 *  init the duart on the current processor
 */
void
duartinit(void)
{
	Port *p;
	Duart *d;

	d = &duart[m->machno];
	if(d->inited)
		return;
	d->reg = DUARTREG;
	d->imr = IM_RRDYA|IM_XRDYA|IM_RRDYB|IM_XRDYB;
	d->reg->is_imr = d->imr;
	d->acr = 0x80;			/* baud rate set 2 */
	d->reg->ipc_acr = d->acr;

	p = &duartport[2*m->machno];
	duartsetup(p, d, 0);
	p++;
	duartsetup(p, d, 1);
	d->inited = 1;
}

/*
 *  enable a duart port
 */
void
duartenable(Port *p)
{
	p->reg->cmnd = ENB_TX|ENB_RX;
}

void
duartenable0(void)
{
	DUARTREG->cmnd = ENB_TX|ENB_RX;
}

void
duartbaud(Port *p, int b)
{
	int x;

	switch(b){
	case 38400:
		x = BD38400;
		break;
	case 19200:
		x = BD19200;
		break;
	case 9600:
		x = BD9600;
		break;
	case 4800:
		x = BD4800;
		break;
	case 2400:
		x = BD2400;
		break;
	case 1200:
		x = BD1200;
		break;
	case 300:
		x = BD300;
		break;
	default:
		return;
	}
	if(x & 0x0100)
		p->d->acr |= 0x80;
	else
		p->d->acr &= ~0x80;
	p->d->reg->ipc_acr = p->d->acr;
	p->reg->sr_csr = x;
}

void
duartdtr(Port *p, int val)
{
	if (val)
		p->reg->ctlr = 0x01;
	else
		p->reg->ctur = 0x01;
}

void
duartbreak(Port *p, int val)
{
	Duartreg *reg;

	reg = p->reg;
	if (val){
		p->d->imr &= ~IM_XRDYB;
		p->d->reg->is_imr = p->d->imr;
		reg->cmnd = STRT_BRK|ENB_TX;
	} else {
		reg->cmnd = STOP_BRK|ENB_TX;
		p->d->imr |= IM_XRDYB;
		p->d->reg->is_imr = p->d->imr;
	}
}

/*
 *  do anything requested for this CPU's duarts
 */
void
duartslave0(Port *p)
{
	switch(p->op){
	case Ddtr:
		duartdtr(p, p->val);
		break;
	case Dbaud:
		duartbaud(p, p->val);
		break;
	case Dbreak:
		duartbreak(p, p->val);
		break;
	case Dprint:
		p->reg->cmnd = ENB_TX;
		p->reg->data = p->val;
		break;
	case Dena:
		duartenable(p);
		break;
	case Dstate:
		p->val = p->reg->ppcr;
		break;
	}
	p->op = Dnone;
	wakeup(&p->opr);
}
void
duartslave(void)
{
	IOQ *cq;
	Port *p;

	p = &duartport[2*m->machno];
	cq = p->iq;
	if(p->wq && cangetc(cq))
		wakeup(&cq->r);
	if(p->op != Dnone)
		duartslave0(p);
	p++;
	cq = p->iq;
	if(p->wq && cangetc(cq))
		wakeup(&cq->r);
	if(p->op != Dnone)
		duartslave0(p);
}

void
duartrintr(Port *p)
{
	Duartreg *reg;
	IOQ *cq;
	int status;
	char ch;

	reg = p->reg;
	status = reg->sr_csr;
	ch = reg->data;
	if(status & (FRM_ERR|OVR_ERR|PAR_ERR))
		reg->cmnd = RESET_ERR;

	cq = p->iq;
	if(cq->putc)
		(*cq->putc)(cq, ch);
	else
		putc(cq, ch);
}

void
duartxintr(Port *p)
{
	Duartreg *reg;
	IOQ *cq;
	int ch;

	cq = p->oq;
	lock(cq);
	ch = getc(cq);
	reg = p->reg;
	if(ch < 0){
		p->printing = 0;
		wakeup(&cq->r);
		reg->cmnd = DIS_TX;
	} else
		reg->data = ch;
	unlock(cq);
}

void
duartintr(void)
{
	int cause;
	Duartreg *reg;
	Port *p;

	p = &duartport[2*m->machno];
	reg = p->reg;
	cause = reg->is_imr;

	/*
	 * I can guess your interrupt.
	 */
	/*
	 * Is it 1?
	 */
	if(cause & IM_RRDYA)
		duartrintr(p);
	/*
	 * Is it 2?
	 */
	if(cause & IM_XRDYA)
		duartxintr(p);
	/*
	 * Is it 3?
	 */
	if(cause & IM_RRDYB)
		duartrintr(p+1);
	/*
	 * Is it 4?
	 */
	if(cause & IM_XRDYB)
		duartxintr(p+1);
}

/*
 *  processor 0 only
 */
int
duartrawputc(int c)
{
	Duartreg *reg;
	int i;

	reg = DUARTREG;
	if(c == '\n')
		duartrawputc('\r');
	reg->cmnd = ENB_TX;
	i = 0;
	while((reg->sr_csr&XMT_RDY) == 0)
		if(++i >= 1000000){
			duartsetup(&duartport[0], &duart[0], 0);
			for(i=0; i<100000; i++)
				;
			break;
		}
	reg->data = c;
	if(c == '\n')
		for(i=0; i<100000; i++)
			;
	return c;
}
void
duartrawputs(char *s)
{
	int i;
	while(*s){
		duartrawputc(*s++);
	}
	for(i=0; i < 1000000; i++)
		;
}
void
iprint(char *fmt, ...)
{
	char buf[1024];
	long *arg;

	arg = (long*)(&fmt+1);
	sprint(buf, fmt, *arg, *(arg+1), *(arg+2), *(arg+3));
	duartrawputs(buf);
}

/*
 *  Queue n characters for output; if queue is full, we lose characters.
 *  Get the output going if it isn't already.
 */
void
duartputs(IOQ *cq, char *s, int n)
{
	int ch, x;
	Port *p;

	x = splhi();
	lock(cq);
	puts(cq, s, n);
	p = cq->ptr;
	if(p->printing == 0){
		ch = getc(cq);
		if(ch >= 0){
			p->printing = 1;
			p->val = ch;
			p->op = Dprint;
		}
	}
	unlock(cq);
	splx(x);
}

/*
 *  set up an duart port as something other than a stream
 */
void
duartspecial(int port, IOQ *oq, IOQ *iq, int baud)
{
	Port *p = &duartport[port];
	IOQ *zq;

	p->nostream = 1;
	if(oq){
		p->oq = oq;
		p->oq->puts = duartputs;
		p->oq->ptr = p;
	}
	if(iq){
		p->iq = iq;
		p->iq->ptr = p;

		/*
		 *  Stupid HACK to undo a stupid hack
		 */ 
		zq = &kbdq;
		if(iq == zq)
			kbdq.putc = kbdcr2nl;
	}
	duartenable(p);
	duartbaud(p, baud);
}

static int	duartputc(IOQ *, int);
static void	duartstopen(Queue*, Stream*);
static void	duartstclose(Queue*);
static void	duartoput(Queue*, Block*);
static void	duartkproc(void *);
Qinfo duartinfo =
{
	nullput,
	duartoput,
	duartstopen,
	duartstclose,
	"duart"
};

static int
opdone(void *x)
{
	Port *p = x;

	return p->op == Dnone;
}

static void
duartstopen(Queue *q, Stream *s)
{
	Port *p;
	char name[NAMELEN];

	p = &duartport[s->id];

	qlock(p);
	p->wq = WR(q);
	WR(q)->ptr = p;
	RD(q)->ptr = p;
	qunlock(p);

	if(p->kstarted == 0){
		p->kstarted = 1;
		sprint(name, "duart%d", s->id+1);
		kproc(name, duartkproc, p);
	}

	/* enable the port */
	qlock(p);
	p->op = Dena;
	sleep(&p->opr, opdone, p);
	qunlock(p);
}

static void
duartstclose(Queue *q)
{
	Port *p = q->ptr;

	qlock(p);
	p->wq = 0;
	p->iq->putc = 0;
	WR(q)->ptr = 0;
	RD(q)->ptr = 0;
	qunlock(p);
}

static void
duartoput(Queue *q, Block *bp)
{
	Port *p = q->ptr;
	IOQ *cq;
	int n, m;

	if(p == 0){
		freeb(bp);
		return;
	}
	cq = p->oq;
	if(waserror()){
		freeb(bp);
		nexterror();
	}
	if(bp->type == M_CTL){
		if(waserror()){
			qunlock(p);
			qunlock(p->d);
			nexterror();
		}
		qlock(p);
		qlock(p->d);
		while (cangetc(cq))	/* let output drain */
			sleep(&cq->r, cangetc, cq);
		n = strtoul((char *)(bp->rptr+1), 0, 0);
		switch(*bp->rptr){
		case 'B':
		case 'b':
			p->val = n;
			p->op = Dbaud;
			sleep(&p->opr, opdone, p);
			break;
		case 'D':
		case 'd':
			p->val = n;
			p->op = Ddtr;
			sleep(&p->opr, opdone, p);
			break;
		case 'K':
		case 'k':
			p->val = 1;
			p->op = Dbreak;
			if(!waserror()){
				sleep(&p->opr, opdone, p);
				tsleep(&p->opr, return0, 0, n);
				poperror();
			}
			p->val = 0;
			p->op = Dbreak;
			sleep(&p->opr, opdone, p);
			break;
		case 'R':
		case 'r':
			/* can't control? */
			break;
		}
		qunlock(p->d);
		qunlock(p);
		poperror();
	}else while((m = BLEN(bp)) > 0){
		while ((n = canputc(cq)) == 0){
			kprint(" duartoput: sleeping\n");
			sleep(&cq->r, canputc, cq);
		}
		if(n > m)
			n = m;
		(*cq->puts)(cq, bp->rptr, n);
		bp->rptr += n;
	}
	freeb(bp);
	poperror();
}

/*
 *  process to send bytes upstream for a port
 */
static void
duartkproc(void *a)
{
	Port *p = a;
	Queue *q;
	IOQ *cq = p->iq;
	Block *bp;
	int n;

loop:
	while ((n = cangetc(cq)) == 0)
		sleep(&cq->r, cangetc, cq);
	qlock(p);
	if(p->wq == 0){
		cq->out = cq->in;
	}else{
		q = RD(p->wq);
		if(!QFULL(q->next)){
			bp = allocb(n);
			bp->flags |= S_DELIM;
			bp->wptr += gets(cq, bp->wptr, n);
			PUTNEXT(q, bp);
		} else
			tsleep(&cq->r, return0, 0, 250);
	}
	qunlock(p);
	goto loop;
}

Dirtab *duartdir;
int nduartport;

/*
 *  allocate the queues if no one else has
 */
void
duartreset(void)
{
	Port *p;
	int i;

	/*
 	 *  allocate the directory and fill it in
	 */
	nduartport = 2*conf.nmach;
	duartdir = xalloc(nduartport*2*sizeof(Dirtab));
	for(i = 0; i < nduartport; i++){
		sprint(duartdir[2*i].name, "eia%d", i+1);
		sprint(duartdir[2*i+1].name, "eia%dctl", i+1);
		duartdir[2*i].length = 0;
		duartdir[2*i+1].length = 0;
		duartdir[2*i].perm = 0666;
		duartdir[2*i+1].perm = 0666;
		duartdir[2*i].qid.path = STREAMQID(i, Sdataqid);
		duartdir[2*i+1].qid.path = STREAMQID(i, Sctlqid);
	}

	/*
	 *  allocate queues for any stream interfaces
	 */
	for(p = duartport; p < &duartport[nduartport]; p++){
		if(p->nostream)
			continue;

		p->iq = xalloc(sizeof(IOQ));
		initq(p->iq);
		p->iq->ptr = p;

		p->oq = xalloc(sizeof(IOQ));
		initq(p->oq);
		p->oq->ptr = p;
		p->oq->puts = duartputs;
	}
}

Chan*
duartattach(char *spec)
{
	return devattach('t', spec);
}

Chan*
duartclone(Chan *c, Chan *nc)
{
	return devclone(c, nc);
}

int
duartwalk(Chan *c, char *name)
{
	return devwalk(c, name, duartdir, 2*nduartport, devgen);
}

void
duartstat(Chan *c, char *p)
{
	switch(STREAMTYPE(c->qid.path)){
	case Sdataqid:
		streamstat(c, p, duartdir[2*STREAMID(c->qid.path)].name,
			duartdir[2*STREAMID(c->qid.path)].perm);
		break;
	default:
		devstat(c, p, duartdir, 2*nduartport, devgen);
		break;
	}
}

Chan*
duartopen(Chan *c, int omode)
{
	Port *p;

	switch(STREAMTYPE(c->qid.path)){
	case Sdataqid:
	case Sctlqid:
		p = &duartport[STREAMID(c->qid.path)];
		break;
	default:
		p = 0;
		break;
	}

	if(p && p->nostream)
		error(Einuse);

	if((c->qid.path & CHDIR) == 0)
		streamopen(c, &duartinfo);
	return devopen(c, omode, duartdir, 2*nduartport, devgen);
}

void
duartcreate(Chan *c, char *name, int omode, ulong perm)
{
	USED(c);
	USED(name);
	USED(omode);
	USED(perm);
	error(Eperm);
}

void
duartclose(Chan *c)
{
	if(c->stream)
		streamclose(c);
}

long
duartread(Chan *c, void *buf, long n, ulong offset)
{
	Port *p;

	if(c->qid.path&CHDIR)
		return devdirread(c, buf, n, duartdir, 2*nduartport, devgen);

	switch(STREAMTYPE(c->qid.path)){
	case Sdataqid:
		return streamread(c, buf, n);
	case Sctlqid:
		if(offset)
			return 0;
		p = &duartport[STREAMID(c->qid.path)];
		qlock(p);
		p->op = Dstate;
		sleep(&p->opr, opdone, p);
		*(uchar *)buf = p->val;
		qunlock(p);
		return 1;
	}

	error(Egreg);
	return 0;	/* not reached */
}

long
duartwrite(Chan *c, void *va, long n, ulong offset)
{
	USED(offset);
	return streamwrite(c, va, n, 0);
}

void
duartremove(Chan *c)
{
	USED(c);
	error(Eperm);
}

void
duartwstat(Chan *c, char *p)
{
	USED(c);
	USED(p);
	error(Eperm);
}

int
duartactive(void)
{
	int i;

	for(i = 0; i < nduartport; i++)
		if(duartport[i].printing)
			return 1;
	return 0;
}
