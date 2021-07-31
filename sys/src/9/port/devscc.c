#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"

#include	"devtab.h"

/*
 *  Driver for the Z8530.
 */
enum
{
	/* wr 0 */
	ResExtPend=	2<<3,
	ResTxPend=	5<<3,
	ResErr=		6<<3,

	/* wr 1 */
	ExtIntEna=	1<<0,
	TxIntEna=	1<<1,
	RxIntDis=	0<<3,
	RxIntFirstEna=	1<<3,
	RxIntAllEna=	2<<3,

	/* wr 3 */
	RxEna=		1,
	Rx5bits=	0<<6,
	Rx7bits=	1<<6,
	Rx6bits=	2<<6,
	Rx8bits=	3<<6,
	Rxbitmask=	3<<6,

	/* wr 4 */
	ParEven=	3<<0,
	ParOdd=		1<<0,
	ParOff=		0<<0,
	ParMask=	3<<0,
	SyncMode=	0<<2,
	Rx1stop=	1<<2,
	Rx1hstop=	2<<2,
	Rx2stop=	3<<2,
	X16=		1<<6,

	/* wr 5 */
	TxRTS=		1<<1,
	TxEna=		1<<3,
	TxBreak=	1<<4,
	TxDTR=		1<<7,
	Tx5bits=	0<<5,
	Tx7bits=	1<<5,
	Tx6bits=	2<<5,
	Tx8bits=	3<<5,
	Txbitmask=	3<<5,

	/* wr 9 */
	IntEna=		1<<3,
	ResetB=		1<<6,
	ResetA=		2<<6,
	HardReset=	3<<6,

	/* wr 11 */
	TRxCOutBR=	2,
	TxClockBR=	2<<3,
	RxClockBR=	2<<5,
	TRxCOI=		1<<2,

	/* wr 14 */
	BREna=		1,
	BRSource=	2,

	/* rr 0 */
	RxReady=	1,
	TxReady=	1<<2,
	RxDCD=		1<<3,
	RxCTS=		1<<5,
	RxBreak=	1<<7,

	/* rr 3 */
	ExtPendB=	1,	
	TxPendB=	1<<1,
	RxPendB=	1<<2,
	ExtPendA=	1<<3,	
	TxPendA=	1<<4,
	RxPendA=	1<<5,
};

typedef struct SCC	SCC;
struct SCC
{
	QLock;
	ushort	sticky[16];	/* sticky write register values */
	uchar	*ptr;		/* command/pointer register in Z8530 */
	uchar	*data;		/* data register in Z8530 */
	int	printing;	/* true if printing */
	ulong	freq;		/* clock frequency */
	uchar	mask;		/* bits/char */

	/* console interface */
	int	special;	/* can't use the stream interface */
	IOQ	*iq;		/* input character queue */
	IOQ	*oq;		/* output character queue */

	/* stream interface */
	Queue	*wq;		/* write queue */
	Rendez	r;		/* kproc waiting for input */
 	int	kstarted;	/* kproc started */

	/* idiot flow control */
	int	xonoff;		/* true if we obey this tradition */
	int	blocked;	/* abstinence */
};

int	nscc;
SCC	*scc[8];	/* up to 4 8530's */
#define CTLS	023
#define CTLQ	021

#ifdef	Zduart
#define	SCCTYPE	'z'
#define	onepointseven()
#else
#define	SCCTYPE	't'
void
onepointseven(void)
{
	int i;
	for(i = 0; i < 20; i++)
		;
}
#endif

/*
 *  Access registers using the pointer in register 0.
 *  This is a bit stupid when accessing register 0.
 */
void
sccwrreg(SCC *sp, int addr, int value)
{
	onepointseven();
	*sp->ptr = addr;
	wbflush();
	onepointseven();
	*sp->ptr = sp->sticky[addr] | value;
	wbflush();
}
ushort
sccrdreg(SCC *sp, int addr)
{
	onepointseven();
	*sp->ptr = addr;
	wbflush();
	onepointseven();
	return *sp->ptr;
}

/*
 *  set the baud rate by calculating and setting the baudrate
 *  generator constant.  This will work with fairly non-standard
 *  baud rates.
 */
void
sccsetbaud(SCC *sp, int rate)
{
	int brconst;

	if(rate == 0)
		error(Ebadctl);

	brconst = (sp->freq+16*rate-1)/(2*16*rate) - 2;

	sccwrreg(sp, 12, brconst & 0xff);
	sccwrreg(sp, 13, (brconst>>8) & 0xff);
}

void
sccparity(SCC *sp, char type)
{
	int val;

	switch(type){
	case 'e':
		val = ParEven;
		break;
	case 'o':
		val = ParOdd;
		break;
	default:
		val = ParOff;
		break;
	}
	sp->sticky[4] = (sp->sticky[4] & ~ParMask) | val;
	sccwrreg(sp, 4, 0);
}

/*
 *  set bits/character, default 8
 */
void
sccbits(SCC *sp, int n)
{
	int rbits, tbits;

	switch(n){
	case 5:
		sp->mask = 0x1f;
		rbits = Rx5bits;
		tbits = Tx5bits;
		break;
	case 6:
		sp->mask = 0x3f;
		rbits = Rx6bits;
		tbits = Tx6bits;
		break;
	case 7:
		sp->mask = 0x7f;
		rbits = Rx7bits;
		tbits = Tx7bits;
		break;
	case 8:
	default:
		sp->mask = 0xff;
		rbits = Rx8bits;
		tbits = Tx8bits;
		break;
	}
	sp->sticky[3] = (sp->sticky[3]&~Rxbitmask) | rbits;
	sccwrreg(sp, 3, 0);
	sp->sticky[5] = (sp->sticky[5]&~Txbitmask) | tbits;
	sccwrreg(sp, 5, 0);
}

/*
 *  toggle DTR
 */
void
sccdtr(SCC *sp, int n)
{
	if(n)
		sp->sticky[5] |= TxDTR;
	else
		sp->sticky[5] &=~TxDTR;
	sccwrreg(sp, 5, 0);
}

/*
 *  toggle RTS
 */
void
sccrts(SCC *sp, int n)
{
	if(n)
		sp->sticky[5] |= TxRTS;
	else
		sp->sticky[5] &=~TxRTS;
	sccwrreg(sp, 5, 0);
}

/*
 *  send break
 */
void
sccbreak(SCC *sp, int ms)
{
	if(ms == 0)
		ms = 100;
	sp->sticky[1] &=~TxIntEna;
	sccwrreg(sp, 1, 0);
	sccwrreg(sp, 5, TxBreak|TxEna);
	tsleep(&u->p->sleep, return0, 0, ms);
	sccwrreg(sp, 5, 0);
	if(sp->oq){
		sp->sticky[1] |= TxIntEna;
		sccwrreg(sp, 1, 0);
	}
}

/*
 *  default is 9600 baud, 1 stop bit, 8 bit chars, no interrupts,
 *  transmit and receive enabled, interrupts disabled.
 */
static void
sccsetup0(SCC *sp, int brsource)
{
	memset(sp->sticky, 0, sizeof(sp->sticky));

	/*
	 *  turn on baud rate generator and set rate to 9600 baud.
	 *  use 1 stop bit.
	 */
	sp->sticky[14] = brsource ? BRSource : 0;
	sccwrreg(sp, 14, 0);
	sccsetbaud(sp, 9600);
	sp->sticky[4] = Rx1stop | X16;
	sccwrreg(sp, 4, 0);
	sp->sticky[11] = TxClockBR | RxClockBR | TRxCOutBR /*| TRxCOI*/;
	sccwrreg(sp, 11, 0);
	sp->sticky[14] = BREna;
	if(brsource)
		sp->sticky[14] |= BRSource;
	sccwrreg(sp, 14, 0);

	/*
	 *  enable I/O, 8 bits/character
	 */
	sp->sticky[3] = RxEna | Rx8bits;
	sccwrreg(sp, 3, 0);
	sp->sticky[5] = TxEna | Tx8bits;
	sccwrreg(sp, 5, 0);
	sp->mask = 0xff;
}

void
sccsetup(void *addr, ulong freq, int brsource)
{
	SCCdev *dev;
	SCC *sp;

	dev = addr;

	/*
	 *  allocate a structure, set port addresses, and setup the line
	 */
	sp = xalloc(sizeof(SCC));
	scc[nscc] = sp;
	sp->ptr = &dev->ptra;
	sp->data = &dev->dataa;
	sp->freq = freq;
	sccsetup0(sp, brsource);
	sp = xalloc(sizeof(SCC));
	scc[nscc+1] = sp;
	sp->ptr = &dev->ptrb;
	sp->data = &dev->datab;
	sp->freq = freq;
	sccsetup0(sp, brsource);
	nscc += 2;
}

/*
 *  Queue n characters for output; if queue is full, we lose characters.
 *  Get the output going if it isn't already.
 */
void
sccputs(IOQ *cq, char *s, int n)
{
	SCC *sp = cq->ptr;
	int ch, x;

	x = splhi();
	lock(cq);
	puts(cq, s, n);
	if(sp->printing == 0 && sp->blocked==0){
		ch = getc(cq);
		/*kprint("<start %2.2ux>", ch);*/
		if(ch >= 0){
			sp->printing = 1;
			while((*sp->ptr&TxReady)==0)
				;
			*sp->data = ch;
		}
	}
	unlock(cq);
	splx(x);
}

/*
 *  an scc interrupt (a damn lot of work for one character)
 */
void
sccintr0(SCC *sp, uchar x)
{
	int ch;
	IOQ *cq;

	if(x & ExtPendB){
		sccwrreg(sp, 0, ResExtPend);
	}
	if(x & RxPendB){
		cq = sp->iq;
		while(*sp->ptr&RxReady){
			onepointseven();
			ch = *sp->data & sp->mask;
			if (ch == CTLS && sp->xonoff)
				sp->blocked = 1;
			else if (ch == CTLQ && sp->xonoff) {
				sp->blocked = 0;
				sccputs(sp->oq, "", 0);
			}
			if(cq->putc)
				(*cq->putc)(cq, ch);
			else
				putc(cq, ch);
		}
	}
	if(x & TxPendB){
		if (sp->blocked) {
			sccwrreg(sp, 0, ResTxPend);
			sp->printing = 0;
			return;
		}
		cq = sp->oq;
		lock(cq);
		ch = getc(cq);
		onepointseven();
		if(ch < 0){
			sccwrreg(sp, 0, ResTxPend);
			sp->printing = 0;
			wakeup(&cq->r);
		}else
			*sp->data = ch;
		unlock(cq);
	}
}
int
sccintr(void)
{
	uchar x;
	int i, j;

	for(i = j = 0; i < nscc; i += 2){
		x = sccrdreg(scc[i], 3);
		if(x & (ExtPendB|RxPendB|TxPendB))
			++j, sccintr0(scc[i+1], x);
		x = x >> 3;
		if(x & (ExtPendB|RxPendB|TxPendB))
			++j, sccintr0(scc[i], x);
	}
	return j;
}

void
sccclock(void)
{
	SCC *sp;
	IOQ *cq;
	int i;

	for(i = 0; i < nscc; i++){
		sp = scc[i];
		cq = sp->iq;
		if(sp->wq && cangetc(cq))
			wakeup(&cq->r);
	}
}

/*
 *  turn on a port's interrupts.  set DTR and RTS
 */
void
sccenable(SCC *sp)
{
	/*
	 *  set up i/o routines
	 */
	if(sp->oq){
		sp->oq->puts = sccputs;
		sp->oq->ptr = sp;
		sp->sticky[1] |= TxIntEna | ExtIntEna;
	}
	if(sp->iq){
		sp->iq->ptr = sp;
		sp->sticky[1] |= RxIntAllEna | ExtIntEna;
	}

	/*
 	 *  turn on interrupts
	 */
	sccwrreg(sp, 1, 0);
	sp->sticky[9] |= IntEna;
	sccwrreg(sp, 9, 0);

	/*
	 *  turn on DTR and RTS
	 */
	sccdtr(sp, 1);
	sccrts(sp, 1);
}

/*
 *  set up an scc port as something other than a stream
 */
void
sccspecial(int port, IOQ *oq, IOQ *iq, int baud)
{
	SCC *sp = scc[port];

	sp->special = 1;
	sp->oq = oq;
	sp->iq = iq;
	sccenable(sp);
	if(baud)
		sccsetbaud(sp, baud);

	if(iq){
		/*
		 *  Stupid HACK to undo a stupid hack
		 */ 
		if(iq == &kbdq)
			kbdq.putc = kbdcr2nl;
	}
}

void
sccrawput(int port, int c)
{
	SCC *sp = scc[port];

	if(c == '\n') {
		sccrawput(port, '\r');
		delay(100);
	}

	while((*sp->ptr&TxReady)==0)
		;
	*sp->data = c;
}

static void	sccstopen(Queue*, Stream*);
static void	sccstclose(Queue*);
static void	sccoput(Queue*, Block*);
static void	scckproc(void *);
Qinfo sccinfo =
{
	nullput,
	sccoput,
	sccstopen,
	sccstclose,
	"scc"
};

static void
sccstopen(Queue *q, Stream *s)
{
	SCC *sp;
	char name[NAMELEN];

	kprint("sccstopen: q=0x%ux, inuse=%d, type=%d, dev=%d, id=%d\n",
		q, s->inuse, s->type, s->dev, s->id);
	sp = scc[s->id];
	qlock(sp);
	sp->wq = WR(q);
	WR(q)->ptr = sp;
	RD(q)->ptr = sp;
	qunlock(sp);

	if(sp->kstarted == 0){
		sp->kstarted = 1;
		sprint(name, "scc%d", s->id);
		kproc(name, scckproc, sp);
	}
}

static void
sccstclose(Queue *q)
{
	SCC *sp = q->ptr;

	if(sp->special)
		return;

	qlock(sp);
	sp->wq = 0;
	sp->iq->putc = 0;
	WR(q)->ptr = 0;
	RD(q)->ptr = 0;
	qunlock(sp);
}

static void
sccoput(Queue *q, Block *bp)
{
	SCC *sp = q->ptr;
	IOQ *cq;
	int n, m;

	if(sp == 0){
		freeb(bp);
		return;
	}
	cq = sp->oq;
	if(waserror()){
		freeb(bp);
		nexterror();
	}
	if(bp->type == M_CTL){
		while (cangetc(cq))	/* let output drain */
			sleep(&cq->r, cangetc, cq);
		n = strtoul((char *)(bp->rptr+1), 0, 0);
		switch(*bp->rptr){
		case 'B':
		case 'b':
			if(BLEN(bp)>4 && strncmp((char*)(bp->rptr+1), "reak", 4) == 0)
				sccbreak(sp, 0);
			else
				sccsetbaud(sp, n);
			break;
		case 'D':
		case 'd':
			sccdtr(sp, n);
			break;
		case 'L':
		case 'l':
			sccbits(sp, n);
			break;

		case 'P':
		case 'p':
			sccparity(sp, *(bp->rptr+1));
			break;
		case 'K':
		case 'k':
			sccbreak(sp, n);
			break;
		case 'R':
		case 'r':
			sccrts(sp, n);
			break;
		case 'W':
		case 'w':
			/* obsolete */
			break;
		case 'X':
		case 'x':
			sp->xonoff = n;
			break;
		}
	}else while((m = BLEN(bp)) > 0){
		while ((n = canputc(cq)) == 0){
			kprint(" sccoput: sleeping\n");
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
scckproc(void *a)
{
	SCC *sp = a;
	IOQ *cq = sp->iq;
	Block *bp;
	int n;

loop:
	while ((n = cangetc(cq)) == 0)
		sleep(&cq->r, cangetc, cq);
	/*kprint(" scckproc: %d\n", n);*/
	qlock(sp);
	if(sp->wq == 0){
		cq->out = cq->in;
	}else{
		bp = allocb(n);
		bp->flags |= S_DELIM;
		bp->wptr += gets(cq, bp->wptr, n);
		PUTNEXT(RD(sp->wq), bp);
	}
	qunlock(sp);
	goto loop;
}

Dirtab *sccdir;

/*
 *  create 2 directory entries for each 'sccsetup' ports.
 *  allocate the queues if no one else has.
 */
void
sccreset(void)
{
	SCC *sp;
	int i;
	Dirtab *dp;

	sccdir = xalloc(2 * nscc * sizeof(Dirtab));
	dp = sccdir;
	for(i = 0; i < nscc; i++){
		/* 2 directory entries per port */
		sprint(dp->name, "eia%d", i);
		dp->qid.path = STREAMQID(i, Sdataqid);
		dp->perm = 0666;
		dp++;
		sprint(dp->name, "eia%dctl", i);
		dp->qid.path = STREAMQID(i, Sctlqid);
		dp->perm = 0666;
		dp++;

		/* set up queues if a stream port */
		sp = scc[i];
		if(sp->special)
			continue;
		sp->iq = xalloc(sizeof(IOQ));
		initq(sp->iq);
		sp->oq = xalloc(sizeof(IOQ));
		initq(sp->oq);
		sccenable(sp);
	}
}

void
sccinit(void)
{
}

Chan*
sccattach(char *spec)
{
	return devattach(SCCTYPE, spec);
}

Chan*
sccclone(Chan *c, Chan *nc)
{
	return devclone(c, nc);
}

int
sccwalk(Chan *c, char *name)
{
	return devwalk(c, name, sccdir, 2*nscc, devgen);
}

void
sccstat(Chan *c, char *dp)
{
	int i;

	i = STREAMID(c->qid.path);
	switch(STREAMTYPE(c->qid.path)){
	case Sdataqid:
		streamstat(c, dp, sccdir[2*i].name, sccdir[2*i].perm);
		break;
	default:
		devstat(c, dp, sccdir, 2*nscc, devgen);
		break;
	}
}

Chan*
sccopen(Chan *c, int omode)
{
	SCC *sp;

	if(c->qid.path != CHDIR){
		sp = scc[STREAMID(c->qid.path)];
		if(sp->special)
			error(Einuse);
		streamopen(c, &sccinfo);
	}
	return devopen(c, omode, sccdir, 2*nscc, devgen);
}

void
scccreate(Chan *c, char *name, int omode, ulong perm)
{
	USED(c, name, omode, perm);
	error(Eperm);
}

void
sccclose(Chan *c)
{
	if(c->stream)
		streamclose(c);
}

long
sccread(Chan *c, void *buf, long n, ulong offset)
{
	char b[8];

	if(c->qid.path == CHDIR)
		return devdirread(c, buf, n, sccdir, 2*nscc, devgen);

	switch(STREAMTYPE(c->qid.path)){
	case Sdataqid:
		return streamread(c, buf, n);
	case Sctlqid:
		sprint(b, "%d", STREAMID(c->qid.path));
		return readstr(offset, buf, n, b);
	}
	error(Egreg);
	return 0;	/* not reached */
}

long
sccwrite(Chan *c, void *va, long n, ulong offset)
{
	USED(offset);
	return streamwrite(c, va, n, 0);
}

void
sccremove(Chan *c)
{
	USED(c);
	error(Eperm);
}

void
sccwstat(Chan *c, char *dp)
{
	USED(c, dp);
	error(Eperm);
}
