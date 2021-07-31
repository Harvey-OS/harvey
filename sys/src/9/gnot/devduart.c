#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"ureg.h"
#include	"../port/error.h"

#include	"devtab.h"

int	duartacr;
int	duartimr;
void	(*kprofp)(ulong);

/*
 * Register set for half the duart.  There are really two sets.
 */
struct Duart{
	uchar	mr1_2;		/* Mode Register Channels 1 & 2 */
	uchar	sr_csr;		/* Status Register/Clock Select Register */
	uchar	cmnd;		/* Command Register */
	uchar	data;		/* RX Holding / TX Holding Register */
	uchar	ipc_acr;	/* Input Port Change/Aux. Control Register */
#define	ivr	ivr		/* Interrupt Vector Register */
	uchar	is_imr;		/* Interrupt Status/Interrupt Mask Register */
#define	ip_opcr	is_imr		/* Input Port/Output Port Configuration Register */
	uchar	ctur;		/* Counter/Timer Upper Register */
#define	scc_sopbc ctur		/* Start Counter Command/Set Output Port Bits Command */
	uchar	ctlr;		/* Counter/Timer Lower Register */
#define	scc_ropbc ctlr		/* Stop Counter Command/Reset Output Port Bits Command */
};

enum{
	CHAR_ERR	=0x00,	/* MR1x - Mode Register 1 */
	PAR_ENB		=0x00,
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
	BD38400		=0xCC|0x0000,
	BD19200		=0xCC|0x0100,
	BD9600		=0xBB|0x0000,
	BD4800		=0x99|0x0000,
	BD2400		=0x88|0x0000,
	BD1200		=0x66|0x0000,
	BD300		=0x44|0x0000,
	IM_IPC		=0x80,	/* IMRx/ISRx - Interrupt Mask/Interrupt Status */
	IM_DBB		=0x40,
	IM_RRDYB	=0x20,
	IM_XRDYB	=0x10,
	IM_CRDY		=0x08,
	IM_DBA		=0x04,
	IM_RRDYA	=0x02,
	IM_XRDYA	=0x01,
};

/*
 *  software info for a serial duart interface
 */
typedef struct Duartport	Duartport;
struct Duartport
{
	QLock;
	int	printing;	/* true if printing */

	/* console interface */
	int	nostream;	/* can't use the stream interface */
	IOQ	*iq;		/* input character queue */
	IOQ	*oq;		/* output character queue */

	/* stream interface */
	Queue	*wq;		/* write queue */
	Rendez	r;		/* kproc waiting for input */
 	int	kstarted;	/* kproc started */
};
Duartport	duartport[1];

uchar keymap[]={
/*80*/	0x58,	0x58,	0x58,	0x58,	0x58,	0x58,	0x58,	0x58,
	0x58,	0x58,	0x58,	0x58,	0x58,	0x58,	0x8e,	0x58,
/*90*/	0x90,	0x91,	0x92,	0x93,	0x94,	0x95,	0x96,	0x97,
	0x98,	0x99,	0x9a,	0x9b,	0x58,	0x58,	0x58,	0x58,
/*A0*/	0x58,	0xa1,	0xa2,	0xa3,	0xa4,	0xa5,	0xa6,	0xa7,
	0x58,	0x58,	0x58,	0x58,	0x58,	0x58,	0xae,	0xaf,
/*B0*/	0xb0,	0xb1,	0xb2,	0xb3,	0xb4,	0xb5,	0xb6,	0xb7,
	0xb8,	0xb9,	0x00,	0xbb,	0x1e,	0xbd,	0x60,	0x1f,
/*C0*/	0xc0,	0xc1,	0xc2,	0xc3,	0xc4,	0x58,	0xc6,	0x0a,
	0xc8,	0xc9,	0xca,	0xcb,	0xcc,	0xcd,	0xce,	0xcf,
/*D0*/	0x09,	0x08,	0xd2,	0xd3,	0xd4,	0xd5,	0xd6,	0xd7,
	0x58,	0x58,	0x58,	0x58,	0x58,	0x58,	0x7f,	0x58,
/*E0*/	0x58,	0x58,	0xe2,	0x1b,	0x0d,	0xe5,	0x58,	0x0a,
	0xe8,	0xe9,	0xea,	0xeb,	0xec,	0xed,	0xee,	0xef,
/*F0*/	0x09,	0x08,	0xb2,	0x1b,	0x0d,	0xf5,	0x81,	0x58,
	0x58,	0x58,	0x58,	0x58,	0x58,	0x58,	0x7f,	0x80,
};

void
duartinit(void)
{
	Duart *duart;
	static int already;

	if(already)
		return;
	already = 1;

	duart  =  DUARTREG;

	/*
	 * Keyboard
	 */
	duart[0].cmnd = RESET_RCV|DIS_TX|DIS_RX;
	duart[0].cmnd = RESET_TRANS;
	duart[0].cmnd = RESET_ERR;
	duart[0].cmnd = RESET_MR;
	duart[0].mr1_2 = CHAR_ERR|PAR_ENB|EVEN_PAR|CBITS8;
	duart[0].mr1_2 = NORM_OP|ONESTOPB;
	duart[0].sr_csr = BD4800;

	/*
	 * RS232
	 */
	duart[1].cmnd = RESET_RCV|DIS_TX|DIS_RX;
	duart[1].cmnd = RESET_TRANS;
	duart[1].cmnd = RESET_ERR;
	duart[1].cmnd = RESET_MR;
	duart[1].mr1_2 = CHAR_ERR|NO_PAR|CBITS8;
	duart[1].mr1_2 = NORM_OP|ONESTOPB;
	duart[1].sr_csr = BD9600;

	/*
	 * Output port
	 */
	duart[0].ipc_acr = duartacr = 0xB7;	/* allow change	of state interrupt */
	duart[1].ip_opcr = 0x00;
	duart[1].scc_ropbc = 0xFF;	/* make sure the port is reset first */
	duart[1].scc_sopbc = 0x04;	/* dtr = 1, pp = 01 */
	duart[0].is_imr = duartimr = IM_IPC|IM_RRDYB|IM_XRDYB|IM_RRDYA|IM_XRDYA;
	duart[0].cmnd = ENB_TX|ENB_RX;	/* enable TX and RX last */
	duart[1].cmnd = ENB_TX|ENB_RX;

	/*
	 * Initialize keyboard
	 */
	while (!(duart[0].sr_csr & (XMT_EMT|XMT_RDY)))
		;
	duart[0].data = 0x02;
}


void
duartbaud(int b)
{
	int x = 0;
	Duart *duart = DUARTREG;

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
		error(Ebadarg);
	}
	if(x & 0x0100)
		duart[0].ipc_acr = duartacr |= 0x80;
	else
		duart[0].ipc_acr = duartacr &= ~0x80;
	duart[1].sr_csr = x;
}

void
duartdtr(int val)
{
	Duart *duart = DUARTREG;
	if (val)
		duart[1].scc_ropbc=0x01;
	else
		duart[1].scc_sopbc=0x01;
}

void
duartbreak(int ms)
{
	static QLock brk;
	Duart *duart = DUARTREG;
	if (ms<=0 || ms >20000)
		error(Ebadarg);
	qlock(&brk);
	duart[0].is_imr = duartimr &= ~IM_XRDYB;
	duart[1].cmnd = STRT_BRK|ENB_TX;
	tsleep(&u->p->sleep, return0, 0, ms);
	duart[1].cmnd = STOP_BRK|ENB_TX;
	duart[0].is_imr = duartimr |= IM_XRDYB;
	qunlock(&brk);
}

enum{
	Kptime=200
};
void
duartstarttimer(void)
{
	Duart *duart;
	char x;

	duart = DUARTREG;
	duart[0].ctur = (Kptime)>>8;
	duart[0].ctlr = (Kptime)&255;
	duart[0].is_imr = duartimr |= IM_CRDY;
	x = duart[1].scc_sopbc;
	USED(x);
}

void
duartstoptimer(void)
{
	Duart *duart;
	char x;

	duart = DUARTREG;
	x = duart[1].scc_ropbc;
	USED(x);
	duart[0].is_imr = duartimr &= ~IM_CRDY;
}


/*
 *  a serial line input interrupt
 */
void
duartrintr(char ch)
{
	IOQ *cq;
	Duartport *dp = duartport;

	cq = dp->iq;
	if(cq->putc)
		(*cq->putc)(cq, ch);
	else {
		putc(cq, ch);
	}
}
void
duartclock(void)
{
	Duartport *dp = duartport;
	IOQ *cq;

	cq = dp->iq;
	if(cangetc(cq))
		wakeup(&cq->r);
}

/*
 *  a serial line output interrupt
 */
void
duartxintr(void)
{
	int ch;
	IOQ *cq;
	Duartport *dp = duartport;
	Duart *duart;

	cq = dp->oq;
	lock(cq);
	ch = getc(cq);
	duart = DUARTREG;
	if(ch < 0){
		dp->printing = 0;
		wakeup(&cq->r);
		duart[1].cmnd = DIS_TX;
	} else
		duart[1].data = ch;
	unlock(cq);
}


void
duartintr(Ureg *ur)
{
	int cause, status, ch, i, nk;
	Duart *duart;
	static int lstate;
	static uchar kc[5];

	duart = DUARTREG;
	cause = duart->is_imr;
	/*
	 * I can guess your interrupt.
	 */
	/*
	 * Is it 0?
	 */
	if(cause & IM_CRDY){
		if(kprofp)
			(*kprofp)(ur->pc);
		ch = duart[1].scc_ropbc;
		USED(ch);
		duart[0].ctur = (Kptime)>>8;
		duart[0].ctlr = (Kptime)&255;
		ch = duart[1].scc_sopbc;
		USED(ch);
		return;
	}
	/*
	 * Is it 1?
	 */
	if(cause & IM_RRDYA){		/* keyboard input */
		status = duart->sr_csr;
		ch = duart->data;
		if(status & (FRM_ERR|OVR_ERR|PAR_ERR))
			duart->cmnd = RESET_ERR;
		if(status & PAR_ERR) /* control word: caps lock (0x4) or repeat (0x10) */
			kbdrepeat((ch&0x10) == 0);
		else{
			if(ch == 0x7F)	/* VIEW key (bizarre) */
				ch = 0xFF;
			if(ch == 0xB6)	/* NUM PAD */
				lstate = 1;
			else{
				if(ch & 0x80)
					ch = keymap[ch&0x7F];
				switch(lstate){
				case 1:
					kc[0] = ch;
					lstate = 2;
					if(ch == 'X')
						lstate = 3;
					break;
				case 2:
					kc[1] = ch;
					ch = latin1(kc);
					nk = 2;
				putit:
					lstate = 0;
					if(ch != -1)
						kbdputc(&kbdq, ch);
					else for(i=0; i<nk; i++)
						kbdputc(&kbdq, kc[i]);
					break;
				case 3:
				case 4:
				case 5:
					kc[lstate-2] = ch;
					lstate++;
					break;
				case 6:
					kc[4] = ch;
					ch = unicode(kc);
					nk = 5;
					goto putit;
				default:
					kbdputc(&kbdq, ch);
					break;
				}
			}
		}
	}
	/*
	 * Is it 2?
	 */
	while(cause & IM_RRDYB){	/* duart input */
		status = duart[1].sr_csr;
		ch = duart[1].data;
		if(status & (FRM_ERR|OVR_ERR|PAR_ERR))
			duart[1].cmnd = RESET_ERR;
		else
			duartrintr(ch);
		cause = duart->is_imr;
	}
	/*
	 * Is it 3?
	 */
	if(cause & IM_XRDYB)		/* duart output */
		duartxintr();
	/*
	 * Is it 4?
	 */
	if(cause & IM_XRDYA)
		duart[0].cmnd = DIS_TX;
	/*
	 * Is it 5?
	 */
	if(cause & IM_IPC)
		mousebuttons((~duart[0].ipc_acr) & 7);
}


/*
 *  Queue n characters for output; if queue is full, we lose characters.
 *  Get the output going if it isn't already.
 */
void
duartputs(IOQ *cq, char *s, int n)
{
	int ch, x;
	Duartport *dp = duartport;
	Duart *duart;

	x = splduart();
	lock(cq);
	puts(cq, s, n);
	if(dp->printing == 0){
		ch = getc(cq);
		if(ch >= 0){
			dp->printing = 1;
			duart = DUARTREG;
			duart[1].cmnd = ENB_TX;
			while(!(duart[1].sr_csr & (XMT_RDY|XMT_EMT)))
				;
			duart[1].data = ch;
		}
	}
	unlock(cq);
	splx(x);
}

void
duartenable(Duartport *dp)
{
	/*
	 *  set up i/o routines
	 */
	if(dp->oq){
		dp->oq->puts = duartputs;
		dp->oq->ptr = dp;
	}
	if(dp->iq)
		dp->iq->ptr = dp;
}

/*
 *  set up an duart port as something other than a stream
 */
void
duartspecial(int port, IOQ *oq, IOQ *iq, int baud)
{
	Duartport *dp = &duartport[port];

	dp->nostream = 1;
	dp->oq = oq;
	dp->iq = iq;
	duartenable(dp);
	duartbaud(baud);
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

static void
duartstopen(Queue *q, Stream *s)
{
	Duartport *dp;
	char name[NAMELEN];

	if(s->id > 0)
		panic("duartstopen");
	dp = &duartport[s->id];

	qlock(dp);
	dp->wq = WR(q);
	WR(q)->ptr = dp;
	RD(q)->ptr = dp;
	qunlock(dp);

	if(dp->kstarted == 0){
		dp->kstarted = 1;
		sprint(name, "duart%d", s->id);
		kproc(name, duartkproc, dp);
	}
}

static void
duartstclose(Queue *q)
{
	Duartport *dp = q->ptr;

	qlock(dp);
	dp->wq = 0;
	dp->iq->putc = 0;
	WR(q)->ptr = 0;
	RD(q)->ptr = 0;
	qunlock(dp);
}

static void
duartoput(Queue *q, Block *bp)
{
	Duartport *dp = q->ptr;
	IOQ *cq;
	int n, m;

	if(dp == 0){
		freeb(bp);
		return;
	}
	cq = dp->oq;
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
			duartbaud(n);
			break;
		case 'D':
		case 'd':
			duartdtr(n);
			break;
		case 'K':
		case 'k':
			duartbreak(n);
			break;
		case 'R':
		case 'r':
			/* can't control? */
			break;
		case 'W':
		case 'w':
			/* obsolete */
			break;
		}
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
	poperror();
	freeb(bp);
}

/*
 *  process to send bytes upstream for a port
 */
static void
duartkproc(void *a)
{
	Duartport *dp = a;
	IOQ *cq = dp->iq;
	Block *bp;
	int n;

loop:
	while ((n = cangetc(cq)) == 0)
		sleep(&cq->r, cangetc, cq);
	qlock(dp);
	if(dp->wq == 0){
		cq->out = cq->in;
	}else{
		bp = allocb(n);
		bp->flags |= S_DELIM;
		bp->wptr += gets(cq, bp->wptr, n);
		PUTNEXT(RD(dp->wq), bp);
	}
	qunlock(dp);
	goto loop;
}

enum{
	Qdir=		0,
	Qeia0=		STREAMQID(0, Sdataqid),
	Qeia0ctl=	STREAMQID(0, Sctlqid),
};

Dirtab duartdir[]={
	"eia0",		{Qeia0},	0,		0666,
	"eia0ctl",	{Qeia0ctl},	0,		0666,
};

#define	NDuartport	(sizeof duartdir/sizeof(Dirtab))

/*
 *  allocate the queues if no one else has
 */
void
duartreset(void)
{
	Duartport *dp = duartport;

	if(dp->nostream)
		return;
	dp->iq = xalloc(sizeof(IOQ));
	initq(dp->iq);
	dp->oq = xalloc(sizeof(IOQ));
	initq(dp->oq);
	duartenable(dp);
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
	return devwalk(c, name, duartdir, NDuartport, devgen);
}

void
duartstat(Chan *c, char *dp)
{
	switch(c->qid.path){
	case Qeia0:
		streamstat(c, dp, duartdir[0].name, duartdir[0].perm);
		break;
	default:
		devstat(c, dp, duartdir, NDuartport, devgen);
		break;
	}
}

Chan*
duartopen(Chan *c, int omode)
{
	Duartport *dp;

	switch(c->qid.path){
	case Qeia0:
	case Qeia0ctl:
		dp = &duartport[0];
		break;
	default:
		dp = 0;
		break;
	}

	if(dp && dp->nostream)
		error(Einuse);

	if((c->qid.path & CHDIR) == 0)
		streamopen(c, &duartinfo);
	return devopen(c, omode, duartdir, NDuartport, devgen);
}

void
duartcreate(Chan *c, char *name, int omode, ulong perm)
{
	USED(c, name, omode, perm);
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
	int s;
	Duart *duart = DUARTREG;

	switch(c->qid.path&~CHDIR){
	case Qdir:
		return devdirread(c, buf, n, duartdir, NDuartport, devgen);
	case Qeia0ctl:
		if(offset)
			return 0;
		s = splhi();
		*(uchar *)buf = duart[1].ip_opcr;
		splx(s);
		return 1;
	}
	return streamread(c, buf, n);
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
duartwstat(Chan *c, char *dp)
{
	USED(c, dp);
	error(Eperm);
}
