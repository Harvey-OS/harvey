#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"

#include	"../port/netif.h"

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
	TxClockTRxC=	1<<3,
	TxClockBR=	2<<3,
	RxClockTRxC=	1<<5,
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

	Cbufsize=	512,
	CTLS=		023,
	CTLQ=		021,
};

typedef struct
{
	QLock;

	ushort	sticky[16];	/* sticky write register values */
	uchar	*ptr;		/* command/pointer register in Uart */
	uchar	*data;		/* data register in Uart */
	Lock	plock;		/* for printing variable */
	int	printing;	/* true if printing */
	ulong	freq;		/* clock frequency */
	uchar	mask;		/* bits/char */
	int	opens;
	int	baud;		/* current rate */

	/* hardware flow control */
	int	xonoff;		/* true if we obey this tradition */
	int	blocked;	/* abstinence */
	Rendez	r;

	/* stats */
	ulong	frame;
	ulong	overrun;

	/* buffers */
	int	(*putc)(Queue*, int);
	Queue	*iq;
	Queue	*oq;
} Uart;

int	invrtsdtr;	/* set to 1 on indigo's */
static	Uart*	uart[Nuart];
static	int	nuart;
static	int	iprintdev;

static void
uartdelay(void)
{
	microdelay(2);
}

/*
 *  access the ptr and data with the appropriate delays
 */
static void
wrptr(Uart *p, uchar c)
{
	uartdelay();
	*p->ptr = c;
	wbflush();
}
static uchar
rdptr(Uart *p)
{
	uartdelay();
	return *p->ptr;
}
static void
wrdata(Uart *p, uchar c)
{
	uartdelay();
	*p->data = c;
	wbflush();
}
static uchar
rddata(Uart *p)
{
	uartdelay();
	return *p->data;
}

/*
 *  Access registers using the pointer in register 0.
 *  This is a bit stupid when accessing register 0.
 */
static void
wrreg(Uart *p, int addr, int value)
{
	wrptr(p, addr);
	wrptr(p, p->sticky[addr] | value);
}
static ushort
rdreg(Uart *p, int addr)
{
	wrptr(p, addr);
	return rdptr(p);
}

/*
 *  set the baud rate by calculating and setting the baudrate
 *  generator constant.  This will work with fairly non-standard
 *  baud rates.
 */
static int
z8530setbaud(Uart *p, int rate)
{
	int brconst;

	if(rate <= 0)
		return -1;

	brconst = (p->freq+16*rate-1)/(2*16*rate) - 2;
	wrreg(p, 12, brconst & 0xff);
	wrreg(p, 13, (brconst>>8) & 0xff);
	p->baud = rate;

	return 0;
}

static void
z8530parity(Uart *p, char type)
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
	p->sticky[4] = (p->sticky[4] & ~ParMask) | val;
	wrreg(p, 4, 0);
}

/*
 *  set bits/character, default 8
 */
static void
z8530bits(Uart *p, int n)
{
	int rbits, tbits;

	switch(n){
	case 5:
		p->mask = 0x1f;
		rbits = Rx5bits;
		tbits = Tx5bits;
		break;
	case 6:
		p->mask = 0x3f;
		rbits = Rx6bits;
		tbits = Tx6bits;
		break;
	case 7:
		p->mask = 0x7f;
		rbits = Rx7bits;
		tbits = Tx7bits;
		break;
	case 8:
	default:
		p->mask = 0xff;
		rbits = Rx8bits;
		tbits = Tx8bits;
		break;
	}
	p->sticky[3] = (p->sticky[3]&~Rxbitmask) | rbits;
	wrreg(p, 3, 0);
	p->sticky[5] = (p->sticky[5]&~Txbitmask) | tbits;
	wrreg(p, 5, 0);
}


/*
 *  toggle DTR
 */
static void
z8530dtr(Uart *p, int n)
{
	if((n!=0)^invrtsdtr)
		p->sticky[5] |= TxDTR;
	else
		p->sticky[5] &=~TxDTR;
	wrreg(p, 5, 0);
}

/*
 *  toggle RTS
 */
static void
z8530rts(Uart *p, int n)
{
	if((n!=0)^invrtsdtr)
		p->sticky[5] |= TxRTS;
	else
		p->sticky[5] &=~TxRTS;
	wrreg(p, 5, 0);
}

/*
 *  send break
 */
static void
z8530break(Uart*, int)
{
}

/*
 *  turn on a port's interrupts.  set DTR and RTS
 */
static void
z8530enable(Uart *p)
{
	/*
 	 *  turn on interrupts
	 */
	p->sticky[1] |= TxIntEna | ExtIntEna;
	p->sticky[1] |= RxIntAllEna | ExtIntEna;
	wrreg(p, 1, 0);
	p->sticky[9] |= IntEna;
	wrreg(p, 9, 0);

	/*
	 *  turn on DTR and RTS
	 */
	z8530dtr(p, 1);
	z8530rts(p, 1);
}

static void
z8530disable(Uart *p)
{
	/*
	 *  turn off DTR and RTS
	 */
	z8530dtr(p, 0);
	z8530rts(p, 0);

	/*
 	 *  turn off interrupts
	p->sticky[1] &= ~(TxIntEna|ExtIntEna);
	p->sticky[1] &= ~(RxIntAllEna|ExtIntEna);
	wrreg(p, 1, 0);
	 */
}

/*
 *  (re)start output
 */
static void
z8530kick(Uart *p)
{
	char ch;
	int x, n;

	x = splhi();
	lock(&p->plock);
	if(p->printing)
		goto out;
	n = qconsume(p->oq, &ch, 1);
	if(n <= 0)
		goto out;

	n = 0;
	while((rdreg(p, 0)&TxReady)==0){
		if(n++ > 10000)
			break;
	}
	*p->data = ch;
	p->printing = 1;
out:
	unlock(&p->plock);
	splx(x);
}

/*
 *  default is 9600 baud, 1 stop bit, 8 bit chars, no interrupts,
 *  transmit and receive enabled, interrupts disabled.
 */
static void
z8530setup0(Uart *p, int brsource)
{
	memset(p->sticky, 0, sizeof(p->sticky));

	/*
	 *  turn on baud rate generator and set rate to 9600 baud.
	 *  use 1 stop bit.
	 */
	p->sticky[14] = brsource ? BRSource : 0;
	wrreg(p, 14, 0);
	z8530setbaud(p, 9600);
	p->sticky[4] = Rx1stop | X16;
	wrreg(p, 4, 0);
	p->sticky[11] = TxClockBR | RxClockBR | TRxCOutBR /*| TRxCOI*/;
	wrreg(p, 11, 0);
	p->sticky[14] = BREna;
	if(brsource)
		p->sticky[14] |= BRSource;
	wrreg(p, 14, 0);

	/*
	 *  enable I/O, 8 bits/character
	 */
	p->sticky[3] = RxEna | Rx8bits;
	wrreg(p, 3, 0);
	p->sticky[5] = TxEna | Tx8bits;
	wrreg(p, 5, 0);
	p->mask = 0xff;

	p->iq = qopen(4*1024, 0, 0, 0);
	p->oq = qopen(4*1024, 0, (void(*)(void*))z8530kick, p);
}

/*
 *  called by main() to create a new duart
 */
void
z8530setup(uchar *dataa, uchar *ctla, uchar *datab, uchar *ctlb, ulong freq, int brsource)
{
	Uart *p;

	if(nuart >= Nuart)
		return;

	p = uart[nuart] = xalloc(sizeof(Uart));
	nuart++;
	p->data = dataa;
	p->ptr = ctla;
	p->freq = freq;
	z8530setup0(p, brsource);

	p = uart[nuart] = xalloc(sizeof(Uart));
	nuart++;
	p->data = datab;
	p->ptr = ctlb;
	p->freq = freq;
	z8530setup0(p, brsource);
}

/*
 *  called by main() to configure a duart port as a console or a mouse
 */
void
z8530special(int port, int baud, Queue **in, Queue **out, int (*putc)(Queue*, int))
{
	Uart *p = uart[port];

	z8530enable(p);
	if(baud)
		z8530setbaud(p, baud);
	p->putc = putc;
	if(in)
		*in = p->iq;
	if(out)
		*out = p->oq;
	p->opens++;

	iprintdev = port;
}

/*
 *  called to reconfigure a duart port
 */
void
z8530config(int port, int baud, int parity, int dtr, int rts)
{
	Uart *p = uart[port];

	z8530enable(p);
	if(baud > 0)
		z8530setbaud(p, baud);
	if(parity >= 0)
		z8530parity(p, parity);
	if(dtr >= 0)
		z8530dtr(p, dtr);
	if(rts >= 0)
		z8530rts(p, rts);
}

/*
 *  an z8530 interrupt (a damn lot of work for one character)
 */
static void
z8530intr0(Uart *p, uchar x)
{
	char ch;

	if(x & ExtPendB){
		wrreg(p, 0, ResExtPend);
	}
	if(x & RxPendB){
		while(rdptr(p)&RxReady){
			ch = rddata(p) & p->mask;
			if (ch == CTLS && p->xonoff)
				p->blocked = 1;
			else if (ch == CTLQ && p->xonoff){
				p->blocked = 0;
				z8530kick(p);
			}
			if(p->putc)
				p->putc(p->iq, ch);
			else
				qproduce(p->iq, &ch, 1);
		}
	}
	if(x & TxPendB){
		lock(&p->plock);
		if (p->blocked || qconsume(p->oq, &ch, 1) <= 0){
			wrreg(p, 0, ResTxPend);
			p->printing = 0;
		} else
			wrdata(p, ch);
		unlock(&p->plock);
	}
}

void
z8530intr(int dev)
{
	uchar x;
	Uart *p;

	p = uart[dev];
	x = rdreg(p, 3);
	if(x & (ExtPendB|RxPendB|TxPendB))
		z8530intr0(uart[dev+1], x);
	x = x >> 3;
	if(x & (ExtPendB|RxPendB|TxPendB))
		z8530intr0(p, x);
}

Dirtab *z8530dir;

/*
 *  create 2 directory entries for each 'z8530setup' ports.
 *  allocate the queues if no one else has.
 */
static void
z8530reset(void)
{
	int i;
	Dirtab *dp;

	z8530dir = xalloc(3 * nuart * sizeof(Dirtab));
	dp = z8530dir;
	for(i = 0; i < nuart; i++){
		/* 2 directory entries per port */
		sprint(dp->name, "eia%d", i);
		dp->qid.path = NETQID(i, Ndataqid);
		dp->perm = 0666;
		dp++;
		sprint(dp->name, "eia%dctl", i);
		dp->qid.path = NETQID(i, Nctlqid);
		dp->perm = 0666;
		dp++;
		sprint(dp->name, "eia%dstat", i);
		dp->qid.path = NETQID(i, Nstatqid);
		dp->perm = 0444;
		dp++;
	}
}

static Chan*
z8530attach(char *spec)
{
	return devattach('t', spec);
}

static int
z8530walk(Chan *c, char *name)
{
	return devwalk(c, name, z8530dir, 3*nuart, devgen);
}

static void
z8530stat(Chan *c, char *dp)
{
	int i;
	Uart *p;
	Dir dir;

	i = NETID(c->qid.path);
	switch(NETTYPE(c->qid.path)){
	case Ndataqid:
		p = uart[i];
		devdir(c, c->qid, z8530dir[3*i].name, qlen(p->iq), eve, 0660, &dir);
		convD2M(&dir, dp);
		break;
	default:
		devstat(c, dp, z8530dir, 3*nuart, devgen);
		break;
	}
}

static Chan*
z8530open(Chan *c, int omode)
{
	Uart *p;

	if(c->qid.path & CHDIR){
		if(omode != OREAD)
			error(Ebadarg);
	} else {
		p = uart[NETID(c->qid.path)];
		qlock(p);
		p->opens++;
		qreopen(p->iq);
		qreopen(p->oq);
		if(p->opens == 1)
			z8530enable(p);
		qunlock(p);
	}

	c->mode = omode&~OTRUNC;
	c->flag |= COPEN;
	c->offset = 0;
	return c;
}

static void
z8530close(Chan *c)
{
	Uart *p;

	if(c->qid.path & CHDIR)
		return;

	p = uart[NETID(c->qid.path)];
	qlock(p);
	p->opens--;
	if(p->opens == 0){
		z8530disable(p);
		qclose(p->iq);
		qclose(p->oq);
	}
	qunlock(p);
}

static long
statread(Uart *p, long offset, void *buf, long n)
{
	uchar tstat, mstat;
	char str[256];
	int x;

	x = splhi();
	mstat = rdreg(p, 0);
	tstat = rdreg(p, 5);
	splx(x);

	str[0] = 0;
	sprint(str, "opens %d ferr %lud oerr %lud baud %d", p->opens,
		p->frame, p->overrun, p->baud);
	if(mstat & RxCTS)
		strcat(str, " cts");
	if(mstat & RxDCD)
		strcat(str, " dcd");
	if(tstat & TxDTR)
		strcat(str, " dtr");
	if(tstat & TxRTS)
		strcat(str, " rts");
	strcat(str, "\n");
	return readstr(offset, buf, n, str);
}

static long
z8530read(Chan *c, void *buf, long n, vlong off)
{
	Uart *p;
	ulong offset = off;

	if(c->qid.path & CHDIR)
		return devdirread(c, buf, n, z8530dir, 3*nuart, devgen);

	p = uart[NETID(c->qid.path)];
	switch(NETTYPE(c->qid.path)){
	case Ndataqid:
		return qread(p->iq, buf, n);
	case Nctlqid:
		return readnum(offset, buf, n, NETID(c->qid.path), NUMSIZE);
	case Nstatqid:
		return statread(p, offset, buf, n);
	}

	return 0;
}

static void
z8530ctl(Uart *p, char *cmd)
{
	int i, n;

	/* let output drain for a while */
	for(i = 0; i < 16 && qlen(p->oq); i++)
		tsleep(&p->r, (int(*)(void*))qlen, p->oq, 125);

	n = atoi(cmd+1);
	switch(*cmd){
	case 'B':
	case 'b':
		if(strncmp(cmd+1, "reak", 4) == 0)
			z8530break(p, 0);
		else
			z8530setbaud(p, n);
		break;
	case 'D':
	case 'd':
		z8530dtr(p, n);
		break;
	case 'L':
	case 'l':
		z8530bits(p, n);
		break;
	case 'P':
	case 'p':
		z8530parity(p, *(cmd+1));
		break;
	case 'K':
	case 'k':
		z8530break(p, n);
		break;
	case 'R':
	case 'r':
		z8530rts(p, n);
		break;
	case 'W':
	case 'w':
		/* obsolete */
		break;
	case 'X':
	case 'x':
		p->xonoff = n;
		break;
	}
}

static long
z8530write(Chan *c, void *buf, long n, vlong)
{
	Uart *p;
	char cmd[32];

	if(c->qid.path & CHDIR)
		error(Eperm);

	p = uart[NETID(c->qid.path)];
	switch(NETTYPE(c->qid.path)){
	case Ndataqid:
		return qwrite(p->oq, buf, n);
	case Nctlqid:
		if(n >= sizeof(cmd))
			n = sizeof(cmd)-1;
		memmove(cmd, buf, n);
		cmd[n] = 0;
		z8530ctl(p, cmd);
		return n;
	}
}

Dev z8530devtab = {
	't',
	"z8530",

	z8530reset,
	devinit,
	z8530attach,
	devclone,
	z8530walk,
	z8530stat,
	z8530open,
	devcreate,
	z8530close,
	z8530read,
	devbread,
	z8530write,
	devbwrite,
	devremove,
	devwstat,
};

/*
 * polling routines for rdb()
 */
int
serialgetc(void)
{
	return -1;	/* BUG no data available */
}

void
serialputs(char *s, int n)
{
	int i, tries;
	Uart *p = uart[iprintdev];

	for(i = 0; i < n; i++){
		tries = 0;
		while((rdreg(p, 0) & TxReady)==0)
			if(tries++ > 10000)
				break;
		wrdata(p, s[i]);
	}
	return;
}

