#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"

#include	"../port/netif.h"

/*
 *  Driver for the ns16552.
 */
enum
{
	/*
	 *  register numbers
	 */
	Data=	0,		/* xmit/rcv buffer */
	Iena=	1,		/* interrupt enable */
	 Ircv=	(1<<0),		/*  for char rcv'd */
	 Ixmt=	(1<<1),		/*  for xmit buffer empty */
	 Irstat=(1<<2),		/*  for change in rcv'er status */
	 Imstat=(1<<3),		/*  for change in modem status */
	 Isleep=(1<<4),		/*  put in sleep mode; 16C650 */
	 Ixoff=	(1<<5),		/*  for xoff received; 16C650 */
	 Irts=	(1<<6),		/*  for !rts asserted in auto mode; 16C650 */
	 Icts=	(1<<7),		/*  for !cts asserted in auto mode; 16C650 */
	Istat=	2,		/* interrupt flag (read) */
	 Fenabd=(3<<6),		/*  on if fifo's enabled */
	 Irctsd=(1<<4),		/*  auto rts or cts changed state */
	Fifoctl=2,		/* fifo control (write) */
	 Fena=	(1<<0),		/*  enable xmit/rcv fifos */
	 Ftrig4=	(1<<6),		/*  trigger after 4 input characters */
	 Ftrig24=	(2<<6),		/*  trigger after 24 input characters */
	 Fclear=(3<<1),		/*  clear xmit & rcv fifos */
	Format=	3,		/* byte format */
	 Bits8=	(3<<0),		/*  8 bits/byte */
	 Stop2=	(1<<2),		/*  2 stop bits */
	 Pena=	(1<<3),		/*  generate parity */
	 Peven=	(1<<4),		/*  even parity */
	 Pforce=(1<<5),		/*  force parity */
	 Break=	(1<<6),		/*  generate a break */
	 Dra=	(1<<7),		/*  address the divisor */
	 Ers=	0xbf,		/*  enable enhaced register set on 16C650 */
	Mctl=	4,		/* modem control */
	 Dtr=	(1<<0),		/*  data terminal ready */
	 Rts=	(1<<1),		/*  request to send */
	 Ri=	(1<<2),		/*  ring */
	 Inton=	(1<<3),		/*  turn on interrupts */
	 Loop=	(1<<4),		/*  loop back */
	 Intsel=(1<<5),		/*  NO! open source interrupts; 16C650 */
	 Irda=	(1<<6),		/*  infrared interface; 16C650 */
	 Cdiv=	(1<<7),		/*  divide clock by four; 16C650 */
	Lstat=	5,		/* line status */
	 Inready=(1<<0),	/*  receive buffer full */
	 Oerror=(1<<1),		/*  receiver overrun */
	 Perror=(1<<2),		/*  receiver parity error */
	 Ferror=(1<<3),		/*  rcv framing error */
	 Outready=(1<<5),	/*  output buffer full */
	Mstat=	6,		/* modem status */
	 Ctsc=	(1<<0),		/*  clear to send changed */
	 Dsrc=	(1<<1),		/*  data set ready changed */
	 Rire=	(1<<2),		/*  rising edge of ring indicator */
	 Dcdc=	(1<<3),		/*  data carrier detect changed */
	 Cts=	(1<<4),		/*  complement of clear to send line */
	 Dsr=	(1<<5),		/*  complement of data set ready line */
	 Ring=	(1<<6),		/*  complement of ring indicator line */
	 Dcd=	(1<<7),		/*  complement of data carrier detect line */
	Scratch=7,		/* scratchpad */
	Dlsb=	0,		/* divisor lsb */
	Dmsb=	1,		/* divisor msb */
	Efr=	2,		/* enhanced features for 16C650 */
	 Eena=	(1<<4),		/* enable enhanced bits in normal registers */
	 Arts=	(1<<6),		/* auto rts */
	 Acts=	(1<<7),		/* auto cts */

	CTLS= 023,
	CTLQ= 021,

	Stagesize= 1024,
	Nuart=	32,		/* max per machine */

	Ns450=	0,
	Ns550,
	Ns650,
};

typedef struct Uart Uart;
struct Uart
{
	QLock;
	int	opens;

	int	enabled;
	Uart	*elist;			/* next enabled interface */
	char	name[NAMELEN];

	uchar	sticky[8];		/* sticky write register values */
	uchar	osticky[8];		/* kernel saved sticky write register values */
	ulong	port;			/* io ports */
	ulong	freq;			/* clock frequency */
	uchar	mask;			/* bits/char */
	int	dev;
	int	baud;			/* baud rate */

	uchar	istat;			/* last istat read */
	int	frame;			/* framing errors */
	int	overrun;		/* rcvr overruns */

	/* buffers */
	int	(*putc)(Queue*, int);
	Queue	*iq;
	Queue	*oq;

	Lock	flock;			/* fifo */
	uchar	fifoon;			/* fifo's enabled */
	uchar	type;			/* chip version */

	Lock	rlock;			/* receive */
	uchar	istage[Stagesize];
	uchar	*ip;
	uchar	*ie;

	int	haveinput;

	Lock	tlock;			/* transmit */
	uchar	ostage[Stagesize];
	uchar	*op;
	uchar	*oe;

	int	modem;			/* hardware flow control on */
	int	xonoff;			/* software flow control on */
	int	blocked;
	int	cts, dsr, dcd, dcdts;		/* keep track of modem status */ 
	int	ctsbackoff;
	int	hup_dsr, hup_dcd;	/* send hangup upstream? */
	int	dohup;

	int	kinuse;		/* device in use by kernel */

	Rendez	r;
};

static Uart *uart[Nuart];
static int nuart;

struct Uartalloc {
	Lock;
	Uart *elist;	/* list of enabled interfaces */
} uartalloc;

void ns16552intr(int);

/*
 * means the kernel is using this for debugging output
 */
static char	Ekinuse[] = "device in use by kernel";

/*
 *  pick up architecture specific routines and definitions
 */
#include "ns16552.h"

/*
 *  set the baud rate by calculating and setting the baudrate
 *  generator constant.  This will work with fairly non-standard
 *  baud rates.
 */
static void
ns16552setbaud(Uart *p, int rate)
{
	ulong brconst;

	if(rate <= 0)
		return;

	brconst = (p->freq+8*rate-1)/(16*rate);

	uartwrreg(p, Format, Dra);
	outb(p->port + Dmsb, (brconst>>8) & 0xff);
	outb(p->port + Dlsb, brconst & 0xff);
	uartwrreg(p, Format, 0);

	p->baud = rate;
}

/*
 * decide if we should hangup when dsr or dcd drops.
 */
static void
ns16552dsrhup(Uart *p, int n)
{
	p->hup_dsr = n;
}

static void
ns16552dcdhup(Uart *p, int n)
{
	p->hup_dcd = n;
}

static void
ns16552parity(Uart *p, char type)
{
	switch(type){
	case 'e':
		p->sticky[Format] |= Pena|Peven;
		break;
	case 'o':
		p->sticky[Format] &= ~Peven;
		p->sticky[Format] |= Pena;
		break;
	default:
		p->sticky[Format] &= ~(Pena|Peven);
		break;
	}
	uartwrreg(p, Format, 0);
}

/*
 *  set bits/character, default 8
 */
void
ns16552bits(Uart *p, int bits)
{
	if(bits < 5 || bits > 8)
		error(Ebadarg);

	p->sticky[Format] &= ~3;
	p->sticky[Format] |= bits-5;

	uartwrreg(p, Format, 0);
}

/*
 *  toggle DTR
 */
void
ns16552dtr(Uart *p, int n)
{
	if(n)
		p->sticky[Mctl] |= Dtr;
	else
		p->sticky[Mctl] &= ~Dtr;

	uartwrreg(p, Mctl, 0);
}

/*
 *  toggle RTS
 */
void
ns16552rts(Uart *p, int n)
{
	if(n)
		p->sticky[Mctl] |= Rts;
	else
		p->sticky[Mctl] &= ~Rts;

	uartwrreg(p, Mctl, 0);
}

/*
 *  save dcd timestamps for gps clock
 */
static void
ns16552dcdts(Uart *p, int n)
{
	p->dcdts = n;
}

/*
 *  send break
 */
static void
ns16552break(Uart *p, int ms)
{
	if(ms == 0)
		ms = 200;

	uartwrreg(p, Format, Break);
	tsleep(&up->sleep, return0, 0, ms);
	uartwrreg(p, Format, 0);
}

/*
 *  always called under lock(&p->flock)
 */
static void
ns16552fifo(Uart *p, int n)
{
	ulong i;
	int x;

	if(p->type < Ns550)
		return;

	x = splhi();

	if(n == 0){
		/* turn off fifo's */
		p->fifoon = 0;
		uartwrreg(p, Fifoctl, 0);
		splx(x);
		return;
	}

	/* reset fifos */
	uartwrreg(p, Fifoctl, Fclear);

	/* empty buffer and interrupt conditions */
	for(i = 0; i < 16; i++){
		if(uartrdreg(p, Istat))
			;
		if(uartrdreg(p, Data))
			;
	}

	/* turn on fifo */
	p->fifoon = 1;
	if(p->type == Ns650)
		uartwrreg(p, Fifoctl, Fena|Ftrig24);
	else
		uartwrreg(p, Fifoctl, Fena|Ftrig4);

	/* fix from Scott Schwartz <schwartz@bio.cse.psu.edu> */
 	p->istat = uartrdreg(p, Istat); 
	if((p->istat & Fenabd) == 0){
		/* didn't work, must be an earlier chip type */
		p->type = Ns450;
	}
	splx(x);
}

/*
 *  modem flow control on/off (rts/cts)
 */
static void
ns16552mflow(Uart *p, int n)
{

	ilock(&p->tlock);
	if(n){
		if(p->type == Ns650){
			outb(p->port + Format, 0xbf);
			outb(p->port + Efr, Eena|Arts|Acts);
			uartwrreg(p, Format, 0);
			p->cts = 1;
		} else {
			p->sticky[Iena] |= Imstat;
			uartwrreg(p, Iena, 0);
			p->modem = 1;
			p->cts = uartrdreg(p, Mstat) & Cts;
		}
	} else {
		if(p->type == Ns650){
			outb(p->port + Format, 0xbf);
			outb(p->port + Efr, Eena|Arts|Acts);
			uartwrreg(p, Format, 0);
		} else {
			p->sticky[Iena] &= ~Imstat;
			uartwrreg(p, Iena, 0);
			p->modem = 0;
		}
		p->cts = 1;
	}
	iunlock(&p->tlock);

	/* modem needs fifo */
	lock(&p->flock);
	ns16552fifo(p, n);
	unlock(&p->flock);
}

/*
 *  turn on a port's interrupts.  set DTR and RTS
 */
static void
ns16552enable(Uart *p)
{
	Uart **l;

	if(p->enabled)
		return;

	uartpower(p->dev, 1);

	p->hup_dsr = p->hup_dcd = 0;
	p->cts = p->dsr = p->dcd = 0;

	/*
 	 *  turn on interrupts
	 */
	p->sticky[Iena] = Ircv | Ixmt | Irstat;
	uartwrreg(p, Iena, 0);

	/*
	 *  turn on DTR and RTS
	 */
	ns16552dtr(p, 1);
	ns16552rts(p, 1);

	/*
	 *  assume we can send
	 */
	ilock(&p->tlock);
	p->cts = 1;
	p->blocked = 0;
	iunlock(&p->tlock);

	/*
	 *  set baud rate to the last used
	 */
	ns16552setbaud(p, p->baud);

	lock(&uartalloc);
	for(l = &uartalloc.elist; *l; l = &(*l)->elist){
		if(*l == p)
			break;
	}
	if(*l == 0){
		p->elist = uartalloc.elist;
		uartalloc.elist = p;
	}
	p->enabled = 1;
	unlock(&uartalloc);
}

/*
 *  turn off a port's interrupts.  reset DTR and RTS
 */
static void
ns16552disable(Uart *p)
{
	Uart **l;

	/*
 	 *  turn off interrupts
	 */
	p->sticky[Iena] = 0;
	uartwrreg(p, Iena, 0);

	/*
	 *  revert to default settings
	 */
	p->sticky[Format] = Bits8;
	uartwrreg(p, Format, 0);

	/*
	 *  turn off DTR, RTS, hardware flow control & fifo's
	 */
	ns16552dtr(p, 0);
	ns16552rts(p, 0);
	ns16552mflow(p, 0);
	ilock(&p->tlock);
	p->xonoff = p->blocked = 0;
	iunlock(&p->tlock);

	uartpower(p->dev, 0);

	lock(&uartalloc);
	for(l = &uartalloc.elist; *l; l = &(*l)->elist){
		if(*l == p){
			*l = p->elist;
			break;
		}
	}
	p->enabled = 0;
	unlock(&uartalloc);
}

/*
 *  put some bytes into the local queue to avoid calling
 *  qconsume for every character
 */
static int
stageoutput(Uart *p)
{
	int n;

	n = qconsume(p->oq, p->ostage, Stagesize);
	if(n <= 0)
		return 0;
	p->op = p->ostage;
	p->oe = p->ostage + n;
	return n;
}

/*
 *  (re)start output
 */
static void
ns16552kick0(void *v)
{
	int i;
	Uart *p;

	p = v;
	if(p->cts == 0 || p->blocked)
		return;

	/*
	 *  128 here is an arbitrary limit to make sure
	 *  we don't stay in this loop too long.  If the
	 *  chips output queue is longer than 128, too
	 *  bad -- presotto
	 */
	for(i = 0; i < 128; i++){
		if(!(uartrdreg(p, Lstat) & Outready))
			break;
		if(p->op >= p->oe && stageoutput(p) == 0)
			break;
		outb(p->port + Data, *(p->op++));
	}
}

static void
ns16552kick(void *v)
{
	Uart *p;

	p = v;
	ilock(&p->tlock);
	ns16552kick0(p);
	iunlock(&p->tlock);
}

/*
 *  restart input if it's off
 */
static void
ns16552flow(void *v)
{
	Uart *p;

	p = v;
	if(p->modem){
		ns16552rts(p, 1);
		ilock(&p->rlock);
		p->haveinput = 1;
		iunlock(&p->rlock);
	}
}

/*
 *  default is 9600 baud, 1 stop bit, 8 bit chars, no interrupts,
 *  transmit and receive enabled, interrupts disabled.
 */
static void
ns16552setup0(Uart *p)
{
	memset(p->sticky, 0, sizeof(p->sticky));
	/*
	 *  set rate to 9600 baud.
	 *  8 bits/character.
	 *  1 stop bit.
	 *  interrupts enabled.
	 */
	p->sticky[Format] = Bits8;
	uartwrreg(p, Format, 0);
	p->sticky[Mctl] |= Inton;
	uartwrreg(p, Mctl, 0x0);

	ns16552setbaud(p, 9600);

	p->iq = qopen(4*1024, 0, ns16552flow, p);
	p->oq = qopen(4*1024, 0, ns16552kick, p);
	if(p->iq == nil || p->oq == nil)
		panic("ns16552setup0");

	p->ip = p->istage;
	p->ie = &p->istage[Stagesize];
	p->op = p->ostage;
	p->oe = p->ostage;
}

/*
 *  called by main() to create a new duart
 */
void
ns16552setup(ulong port, ulong freq, char *name, int type)
{
	Uart *p;

	if(nuart >= Nuart)
		return;

	p = xalloc(sizeof(Uart));
	uart[nuart] = p;
	strcpy(p->name, name);
	p->dev = nuart;
	nuart++;
	p->port = port;
	p->freq = freq;
	p->type = type;
	ns16552setup0(p);
}

/*
 *  called by main() to configure a duart port as a console or a mouse
 */
void
ns16552special(int port, int baud, Queue **in, Queue **out, int (*putc)(Queue*, int))
{
	Uart *p = uart[port];

	ns16552enable(p);
	if(baud)
		ns16552setbaud(p, baud);
	p->putc = putc;
	if(in)
		*in = p->iq;
	if(out)
		*out = p->oq;
	p->opens++;
}

/*
 *  handle an interrupt to a single uart
 */
void
ns16552intr(int dev)
{
	uchar ch;
	int s, l, loops;
	Uart *p = uart[dev];

	for(loops = 0;; loops++){
		p->istat = s = uartrdreg(p, Istat);
		if(loops > 10000000){
			print("lstat %ux mstat %ux istat %ux iena %ux ferr %d oerr %d",
				uartrdreg(p, Lstat), uartrdreg(p, Mstat),
				s, uartrdreg(p, Iena), p->frame, p->overrun);
			panic("ns16552intr");
		}
		switch(s&0x3f){
		case 6:	/* receiver line status */
			l = uartrdreg(p, Lstat);
			if(l & Ferror)
				p->frame++;
			if(l & Oerror)
				p->overrun++;
			break;

		case 4:	/* received data available */
		case 12:
			ch = uartrdreg(p, Data) & 0xff;
			if(p->xonoff){
				if(ch == CTLS){
					p->blocked = 1;
				}else if (ch == CTLQ){
					p->blocked = 0;
					p->ctsbackoff = 2; /* clock gets output going again */
				}
			}
			if(p->putc)
				p->putc(p->iq, ch);
			else {
				ilock(&p->rlock);
				if(p->ip < p->ie)
					*p->ip++ = ch;
				p->haveinput = 1;
				iunlock(&p->rlock);
			}
			break;

		case 2:	/* transmitter not full */
			ns16552kick(p);
			break;

		case 0:	/* modem status */
			ch = uartrdreg(p, Mstat);
			if(ch & Ctsc){
				l = p->cts;
				p->cts = ch & Cts;
				if(l == 0 && p->cts)
					p->ctsbackoff = 2; /* clock gets output going again */
			}
	 		if (ch & Dsrc) {
				l = ch & Dsr;
				if(p->hup_dsr && p->dsr && !l)
					p->dohup = 1;	 /* clock peforms hangup */
				p->dsr = l;
			}
	 		if (ch & Dcdc) {
				l = ch & Dcd;
				if(l == 0 && p->dcd != 0 && p->dcdts && saveintrts != nil)
					(*saveintrts)();
				if(p->hup_dcd && p->dcd && !l)
					p->dohup = 1;	 /* clock peforms hangup */
				p->dcd = l;
			}
			break;

		default:
			if(s&1)
				return;
			print("weird modem interrupt #%2.2ux\n", s);
			break;
		}
	}
}

/*
 *  we save up input characters till clock time
 *
 *  There's also a bit of code to get a stalled print going.
 *  It shouldn't happen, but it does.  Obviously I don't
 *  understand something.  Since it was there, I bundled a
 *  restart after flow control with it to give some hysteresis
 *  to the hardware flow control.  This makes compressing
 *  modems happier but will probably bother something else.
 *	 -- presotto
 */
void
uartclock(void)
{
	int n;
	Uart *p;

	for(p = uartalloc.elist; p; p = p->elist){

		/* this amortizes cost of qproduce to many chars */
		if(p->haveinput){
			ilock(&p->rlock);
			if(p->haveinput){
				n = p->ip - p->istage;
				if(n > 0 && p->iq){
					if(n > Stagesize)
						panic("uartclock");
					if(qproduce(p->iq, p->istage, n) < 0)
						ns16552rts(p, 0);
					else
						p->ip = p->istage;
				}
				p->haveinput = 0;
			}
			iunlock(&p->rlock);
		}
		if(p->dohup){
			ilock(&p->rlock);
			if(p->dohup){
				qhangup(p->iq, 0);
				qhangup(p->oq, 0);
			}
			p->dohup = 0;
			iunlock(&p->rlock);
		}

		/* this adds hysteresis to hardware/software flow control */
		if(p->ctsbackoff){
			ilock(&p->tlock);
			if(p->ctsbackoff){
				if(--(p->ctsbackoff) == 0)
					ns16552kick0(p);
			}
			iunlock(&p->tlock);
		}
	}
}

Dirtab *ns16552dir;
int ndir;

static void
setlength(int i)
{
	Uart *p;

	if(i > 0){
		p = uart[i];
		if(p && p->opens && p->iq)
			ns16552dir[3*i].length = qlen(p->iq);
	} else for(i = 0; i < nuart; i++){
		p = uart[i];
		if(p && p->opens && p->iq)
			ns16552dir[3*i].length = qlen(p->iq);
	}
}

/*
 *  all uarts must be ns16552setup() by this point or inside of ns16552install()
 */
static void
ns16552reset(void)
{
	int i;
	Dirtab *dp;

	ns16552install();	/* architecture specific */

	ndir = 3*nuart;
	ns16552dir = xalloc(ndir * sizeof(Dirtab));
	dp = ns16552dir;
	for(i = 0; i < nuart; i++){
		/* 3 directory entries per port */
		strcpy(dp->name, uart[i]->name);
		dp->qid.path = NETQID(i, Ndataqid);
		dp->perm = 0660;
		dp++;
		sprint(dp->name, "%sctl", uart[i]->name);
		dp->qid.path = NETQID(i, Nctlqid);
		dp->perm = 0660;
		dp++;
		sprint(dp->name, "%sstat", uart[i]->name);
		dp->qid.path = NETQID(i, Nstatqid);
		dp->perm = 0444;
		dp++;
	}
}

static Chan*
ns16552attach(char *spec)
{
	return devattach('t', spec);
}

static int
ns16552walk(Chan *c, char *name)
{
	return devwalk(c, name, ns16552dir, ndir, devgen);
}

static void
ns16552stat(Chan *c, char *dp)
{
	if(NETTYPE(c->qid.path) == Ndataqid)
		setlength(NETID(c->qid.path));
	devstat(c, dp, ns16552dir, ndir, devgen);
}

static Chan*
ns16552open(Chan *c, int omode)
{
	Uart *p;

	c = devopen(c, omode, ns16552dir, ndir, devgen);

	switch(NETTYPE(c->qid.path)){
	case Nctlqid:
	case Ndataqid:
		p = uart[NETID(c->qid.path)];
		if(p->kinuse)
			error(Ekinuse);
		qlock(p);
		if(p->opens++ == 0){
			ns16552enable(p);
			qreopen(p->iq);
			qreopen(p->oq);
		}
		qunlock(p);
		break;
	}

	return c;
}

static void
ns16552close(Chan *c)
{
	Uart *p;

	if(c->qid.path & CHDIR)
		return;
	if((c->flag & COPEN) == 0)
		return;
	switch(NETTYPE(c->qid.path)){
	case Ndataqid:
	case Nctlqid:
		p = uart[NETID(c->qid.path)];
		if(p->kinuse)
			error(Ekinuse);
		qlock(p);
		if(--(p->opens) == 0){
			ns16552disable(p);
			qclose(p->iq);
			qclose(p->oq);
			p->ip = p->istage;
			p->dcd = p->dsr = p->dohup = 0;
		}
		qunlock(p);
		break;
	}
}

static long
uartstatus(Chan*, Uart *p, void *buf, long n, long offset)
{
	uchar mstat, fstat, istat, tstat;
	char str[256];

	str[0] = 0;
	tstat = p->sticky[Mctl];
	mstat = uartrdreg(p, Mstat);
	istat = p->sticky[Iena];
	fstat = p->sticky[Format];
	snprint(str, sizeof str,
		"b%d c%d d%d e%d l%d m%d p%c r%d s%d i%d\n"
		"dev(%d) type(%d) framing(%d) overruns(%d)%s%s%s%s\n",

		p->baud,
		p->hup_dcd, 
		(tstat & Dtr) != 0,
		p->hup_dsr,
		(fstat & Bits8) + 5,
		(istat & Imstat) != 0, 
		(fstat & Pena) ? ((fstat & Peven) ? 'e' : 'o') : 'n',
		(tstat & Rts) != 0,
		(fstat & Stop2) ? 2 : 1,
		p->fifoon,

		p->dev,
		p->type,
		p->frame,
		p->overrun, 
		(mstat & Cts)    ? " cts"  : "",
		(mstat & Dsr)    ? " dsr"  : "",
		(mstat & Dcd)    ? " dcd"  : "",
		(mstat & Ring)   ? " ring" : ""
	);
	return readstr(offset, buf, n, str);
}

static long
ns16552read(Chan *c, void *buf, long n, vlong off)
{
	Uart *p;
	ulong offset = off;

	if(c->qid.path & CHDIR){
		setlength(-1);
		return devdirread(c, buf, n, ns16552dir, ndir, devgen);
	}

	p = uart[NETID(c->qid.path)];
	if(p->kinuse)
		error(Ekinuse);
	switch(NETTYPE(c->qid.path)){
	case Ndataqid:
		return qread(p->iq, buf, n);
	case Nctlqid:
		return readnum(offset, buf, n, NETID(c->qid.path), NUMSIZE);
	case Nstatqid:
		return uartstatus(c, p, buf, n, offset);
	}

	return 0;
}

static void
ns16552ctl(Uart *p, char *cmd)
{
	int i, n;
	char *f[32];
	int nf;

	/* let output drain for a while */
	for(i = 0; i < 16 && qlen(p->oq); i++)
		tsleep(&p->r, (int(*)(void*))qlen, p->oq, 125);

	nf = getfields(cmd, f, nelem(f), 1, " \t\n");

	for(i = 0; i < nf; i++){

		if(strncmp(f[i], "break", 5) == 0){
			ns16552break(p, 0);
			continue;
		}

		n = atoi(f[i]+1);
		switch(*f[i]){
		case 'B':
		case 'b':
			ns16552setbaud(p, n);
			break;
		case 'C':
		case 'c':
			ns16552dcdhup(p, n);
			break;
		case 'D':
		case 'd':
			ns16552dtr(p, n);
			break;
		case 'E':
		case 'e':
			ns16552dsrhup(p, n);
			break;
		case 'f':
		case 'F':
			qflush(p->oq);
			break;
		case 'H':
		case 'h':
			qhangup(p->iq, 0);
			qhangup(p->oq, 0);
			break;
		case 'i':
		case 'I':
			lock(&p->flock);
			ns16552fifo(p, n);
			unlock(&p->flock);
			break;
		case 'L':
		case 'l':
			ns16552bits(p, n);
			break;
		case 'm':
		case 'M':
			ns16552mflow(p, n);
			break;
		case 'n':
		case 'N':
			qnoblock(p->oq, n);
			break;
		case 'P':
		case 'p':
			ns16552parity(p, *(cmd+1));
			break;
		case 'K':
		case 'k':
			ns16552break(p, n);
			break;
		case 'R':
		case 'r':
			ns16552rts(p, n);
			break;
		case 'Q':
		case 'q':
			qsetlimit(p->iq, n);
			qsetlimit(p->oq, n);
			break;
		case 'T':
		case 't':
			ns16552dcdts(p, n);
			break;
		case 'W':
		case 'w':
			/* obsolete */
			break;
		case 'X':
		case 'x':
			ilock(&p->tlock);
			p->xonoff = n;
			iunlock(&p->tlock);
			break;
		}
	}
}

static long
ns16552write(Chan *c, void *buf, long n, vlong)
{
	Uart *p;
	char cmd[32];

	if(c->qid.path & CHDIR)
		error(Eperm);

	p = uart[NETID(c->qid.path)];
	if(p->kinuse)
		error(Ekinuse);

	/*
	 *  The fifo's turn themselves off sometimes.
	 *  It must be something I don't understand. -- presotto
	 */
	if((p->istat & Fenabd) == 0 && p->fifoon && p->type < Ns550){
		lock(&p->flock);
		if((p->istat & Fenabd) == 0 && p->fifoon && p->type < Ns550)
			ns16552fifo(p, 1);
		unlock(&p->flock);
	}

	switch(NETTYPE(c->qid.path)){
	case Ndataqid:
		return qwrite(p->oq, buf, n);
	case Nctlqid:
		if(n >= sizeof(cmd))
			n = sizeof(cmd)-1;
		memmove(cmd, buf, n);
		cmd[n] = 0;
		ns16552ctl(p, cmd);
		return n;
	}
}

static void
ns16552wstat(Chan *c, char *dp)
{
	Dir d;
	Dirtab *dt;

	if(!iseve())
		error(Eperm);
	if(CHDIR & c->qid.path)
		error(Eperm);
	if(NETTYPE(c->qid.path) == Nstatqid)
		error(Eperm);

	dt = &ns16552dir[3 * NETID(c->qid.path)];
	convM2D(dp, &d);
	d.mode &= 0666;
	dt[0].perm = dt[1].perm = d.mode;
}

Dev ns16552devtab = {
	't',
	"ns16552",

	ns16552reset,
	devinit,
	ns16552attach,
	devclone,
	ns16552walk,
	ns16552stat,
	ns16552open,
	devcreate,
	ns16552close,
	ns16552read,
	devbread,
	ns16552write,
	devbwrite,
	devremove,
	ns16552wstat,
};

/*
 * Polling I/O routines for kernel debugging helps.
 */
static void
ns16552setuppoll(Uart *p, int rate)
{
	ulong brconst;

	/*
	 * 8 bits/character, 1 stop bit;
	 * set rate to rate baud;
	 * turn on Rts and Dtr.
	 */
	memmove(p->osticky, p->sticky, sizeof p->osticky);
	
	p->sticky[Format] = Bits8;
	uartwrreg(p, Format, 0);

	brconst = (UartFREQ+8*rate-1)/(16*rate);
	uartwrreg(p, Format, Dra);
	outb(p->port+Dmsb, (brconst>>8) & 0xff);
	outb(p->port+Dlsb, brconst & 0xff);
	uartwrreg(p, Format, 0);

	p->sticky[Mctl] = Rts|Dtr;
	uartwrreg(p, Mctl, 0);

	p->kinuse = 1;
}

/*
 * Restore serial state from before kernel took over.
 */
static void
ns16552restore(Uart *p)
{
	ulong brconst;

	memmove(p->sticky, p->osticky, sizeof p->sticky);
	uartwrreg(p, Format, 0);

	brconst = (UartFREQ+8*p->baud-1)/(16*p->baud);
	uartwrreg(p, Format, Dra);
	outb(p->port+Dmsb, (brconst>>8) & 0xff);
	outb(p->port+Dlsb, brconst & 0xff);
	uartwrreg(p, Format, 0);

	uartwrreg(p, Mctl, 0);
	p->kinuse = 0;
}

/* BUG should be configurable */
enum {
	WHICH = 0,
	RATE = 9600,
};

int
serialgetc(void)
{
	Uart *p;
	int c;

	if((p=uart[WHICH]) == nil)
		return -1;
	if(!p->kinuse)
		ns16552setuppoll(p, RATE);

	while((uartrdreg(p, Lstat)&Inready) == 0)
		;
	c = inb(p->port+Data) & 0xFF;
	return c;
	// there should be a restore here but i think it will slow things down too much.
}

static void
serialputc(Uart *p, int c)
{
	while((uartrdreg(p, Lstat)&Outready) == 0)
		;
	outb(p->port+Data, c);
	while((uartrdreg(p, Lstat)&Outready) == 0)
		;
}

void
serialputs(char *s, int n)
{
	Uart *p;

	if((p=uart[WHICH]) == nil)
		return;
	if(!p->kinuse)
		ns16552setuppoll(p, RATE);

	while(n-- > 0){
		serialputc(p, *s);
		if(*s == '\n')
			serialputc(p, '\r');
		s++;
	}
	ns16552restore(p);
}
