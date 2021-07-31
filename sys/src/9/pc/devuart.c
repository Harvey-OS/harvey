#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"

#include	"devtab.h"

/*
 *  Driver for an NS16450 serial port
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
	Istat=	2,		/* interrupt flag (read) */
	Tctl=	2,		/* test control (write) */
	Format=	3,		/* byte format */
	 Bits8=	(3<<0),		/*  8 bits/byte */
	 Stop2=	(1<<2),		/*  2 stop bits */
	 Pena=	(1<<3),		/*  generate parity */
	 Peven=	(1<<4),		/*  even parity */
	 Pforce=(1<<5),		/*  force parity */
	 Break=	(1<<6),		/*  generate a break */
	 Dra=	(1<<7),		/*  address the divisor */
	Mctl=	4,		/* modem control */
	 Dtr=	(1<<0),		/*  data terminal ready */
	 Rts=	(1<<1),		/*  request to send */
	 Ri=	(1<<2),		/*  ring */
	 Inton=	(1<<3),		/*  turn on interrupts */
	 Loop=	(1<<4),		/*  loop back */
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

	Serial=	0,
	Modem=	1,
};

typedef struct Uart	Uart;
struct Uart
{
	QLock;
	int	port;
	uchar	sticky[8];	/* sticky write register values */
	int	printing;	/* true if printing */
	int	enabled;
	int	cts;

	/* console interface */
	int	special;	/* can't use the stream interface */
	IOQ	*iq;		/* input character queue */
	IOQ	*oq;		/* output character queue */

	/* stream interface */
	Queue	*wq;		/* write queue */
	Rendez	r;		/* kproc waiting for input */
 	int	kstarted;	/* kproc started */

	/* error statistics */
	ulong	frame;
	ulong	overrun;
};

Uart uart[2];

#define UartFREQ 1846200

#define uartwrreg(u,r,v)	outb((u)->port + r, (u)->sticky[r] | (v))
#define uartrdreg(u,r)		inb((u)->port + r)

void	uartintr(Uart*);
void	uartintr0(Ureg*);
void	uartintr1(Ureg*);
void	uartsetup(void);

/*
 *  set the baud rate by calculating and setting the baudrate
 *  generator constant.  This will work with fairly non-standard
 *  baud rates.
 */
void
uartsetbaud(Uart *up, int rate)
{
	ulong brconst;

	brconst = (UartFREQ+8*rate-1)/(16*rate);

	uartwrreg(up, Format, Dra);
	outb(up->port+Dmsb, (brconst>>8) & 0xff);
	outb(up->port+Dlsb, brconst & 0xff);
	uartwrreg(up, Format, 0);
}

/*
 *  toggle DTR
 */
void
uartdtr(Uart *up, int n)
{
	if(n)
		up->sticky[Mctl] |= Dtr;
	else
		up->sticky[Mctl] &= ~Dtr;
	uartwrreg(up, Mctl, 0);
}

/*
 *  toggle RTS
 */
void
uartrts(Uart *up, int n)
{
	if(n)
		up->sticky[Mctl] |= Rts;
	else
		up->sticky[Mctl] &= ~Rts;
	uartwrreg(up, Mctl, 0);
}

/*
 *  send break
 */
void
uartbreak(Uart *up, int ms)
{
	if(ms == 0)
		ms = 200;
	uartwrreg(up, Format, Break);
	tsleep(&u->p->sleep, return0, 0, ms);
	uartwrreg(up, Format, 0);
}

/*
 *  set bits/char
 */
void
uartbits(Uart *up, int bits)
{
	if(bits < 5 || bits > 8)
		error(Ebadarg);
	up->sticky[Format] &= ~3;
	up->sticky[Format] |= bits-5;
	uartwrreg(up, Format, 0);
}

/*
 *  set parity
 */
void
uartparity(Uart *up, int c)
{
	switch(c&0xff){
	case 'e':
		up->sticky[Format] |= Pena|Peven;
		break;
	case 'o':
		up->sticky[Format] &= ~Peven;
		up->sticky[Format] |= Pena;
		break;
	default:
		up->sticky[Format] &= ~(Pena|Peven);
		break;
	}
	uartwrreg(up, Format, 0);
}

/*
 *  set stop bits
 */
void
uartstop(Uart *up, int n)
{
	switch(n){
	case 1:
		up->sticky[Format] &= ~Stop2;
		break;
	case 2:
	default:
		up->sticky[Format] |= Stop2;
		break;
	}
	uartwrreg(up, Format, 0);
}

/*
 *  modem flow control on/off (rts/cts)
 */
void
uartmflow(Uart *up, int n)
{
	if(n){
		up->sticky[Iena] |= Imstat;
		uartwrreg(up, Iena, 0);
		up->cts = uartrdreg(up, Mstat) & Cts;
	} else {
		up->sticky[Iena] &= ~Imstat;
		uartwrreg(up, Iena, 0);
		up->cts = 1;
	}
}


/*
 *  default is 9600 baud, 1 stop bit, 8 bit chars, no interrupts,
 *  transmit and receive enabled, interrupts disabled.
 */
void
uartsetup(void)
{
	Uart *up;
	static int already;

	if(already)
		return;
	already = 1;

	/*
	 *  set port addresses
	 */
	uart[0].port = 0x3F8;
	uart[1].port = 0x2F8;
	setvec(Uart0vec, uartintr0);
	setvec(Uart1vec, uartintr1);

	for(up = uart; up < &uart[2]; up++){
		memset(up->sticky, 0, sizeof(up->sticky));

		/*
		 *  set rate to 9600 baud.
		 *  8 bits/character.
		 *  1 stop bit.
		 *  interrupts enabled.
		 */
		uartsetbaud(up, 9600);
		up->sticky[Format] = Bits8;
		uartwrreg(up, Format, 0);
		up->sticky[Mctl] |= Inton;
		uartwrreg(up, Mctl, 0x0);
	}
}

/*
 *  Queue n characters for output; if queue is full, we lose characters.
 *  Get the output going if it isn't already.
 */
void
uartputs(IOQ *cq, char *s, int n)
{
	Uart *up = cq->ptr;
	int ch, x, multiprocessor;
	int tries;

	multiprocessor = active.machs > 1;
	x = splhi();
	if(multiprocessor)
		lock(cq);
	puts(cq, s, n);
	if(up->printing == 0){
		ch = getc(cq);
		if(ch >= 0){
			up->printing = 1;
			for(tries = 0; tries<10000 && !(uartrdreg(up, Lstat)&Outready);
				tries++)
				;
			outb(up->port + Data, ch);
		}
	}
	if(multiprocessor)
		unlock(cq);
	splx(x);
}

/*
 *  a uart interrupt (a damn lot of work for one character)
 */
void
uartintr(Uart *up)
{
	int ch;
	IOQ *cq;
	int s, l, multiprocessor;

	multiprocessor = active.machs > 1;
	for(;;){
		s = uartrdreg(up, Istat);
		switch(s){
		case 6:	/* receiver line status */
			l = uartrdreg(up, Lstat);
			if(l & Ferror)
				up->frame++;
			if(l & Oerror)
				up->overrun++;
			break;
	
		case 4:	/* received data available */
			ch = uartrdreg(up, Data) & 0xff;
			cq = up->iq;
			if(cq == 0)
				break;
			if(cq->putc)
				(*cq->putc)(cq, ch);
			else
				putc(cq, ch);
			break;
	
		case 2:	/* transmitter empty */
			cq = up->oq;
			if(cq == 0)
				break;
			if(multiprocessor)
				lock(cq);
			if(up->cts == 0)
				up->printing = 0;
			else {
				ch = getc(cq);
				if(ch < 0){
					up->printing = 0;
					wakeup(&cq->r);
				}else
					outb(up->port + Data, ch);
			}
			if(multiprocessor)
				unlock(cq);
			break;
	
		case 0:	/* modem status */
			ch = uartrdreg(up, Mstat);
			if(ch & Ctsc){
				up->cts = ch & Cts;
				cq = up->oq;
				if(cq == 0)
					break;
				if(multiprocessor)
					lock(cq);
				if(up->cts && up->printing == 0){
					ch = getc(cq);
					if(ch >= 0){
						up->printing = 1;
						outb(up->port + Data, getc(cq));
					} else
						wakeup(&cq->r);
				}
				if(multiprocessor)
					unlock(cq);
			}
			break;
	
		default:
			if(s&1)
				return;
			print("weird modem interrupt\n");
			break;
		}
	}
}
void
uartintr0(Ureg *ur)
{
	USED(ur);
	uartintr(&uart[0]);
}
void
uartintr1(Ureg *ur)
{
	USED(ur);
	uartintr(&uart[1]);
}

void
uartclock(void)
{
	Uart *up;
	IOQ *cq;

	for(up = uart; up < &uart[2]; up++){
		cq = up->iq;
		if(up->wq && cangetc(cq))
			wakeup(&cq->r);
	}
}


/*
 *  turn on a port's interrupts.  set DTR and RTS
 */
void
uartenable(Uart *up)
{
	/*
	 *  turn on power to the port
	 */
	if(up == &uart[Serial]){
		if(serial(1) < 0)
			print("can't turn on serial port power\n");
	} else {
		if(modem(1) < 0)
			print("can't turn on modem speaker\n");
	}

	/*
	 *  set up i/o routines
	 */
	if(up->oq){
		up->oq->puts = uartputs;
		up->oq->ptr = up;
	}
	if(up->iq){
		up->iq->ptr = up;
	}
	up->enabled = 1;

	/*
 	 *  turn on interrupts
	 */
	up->sticky[Iena] = Ircv | Ixmt | Irstat;
	uartwrreg(up, Iena, 0);

	/*
	 *  turn on DTR and RTS
	 */
	uartdtr(up, 1);
	uartrts(up, 1);

	/*
	 *  assume we can send
	 */
	up->cts = 1;
}

/*
 *  turn off the uart
 */
void
uartdisable(Uart *up)
{

	/*
 	 *  turn off interrupts
	 */
	up->sticky[Iena] = 0;
	uartwrreg(up, Iena, 0);

	/*
	 *  revert to default settings
	 */
	up->sticky[Format] = Bits8;
	uartwrreg(up, Format, 0);

	/*
	 *  turn off DTR and RTS
	 */
	uartdtr(up, 0);
	uartrts(up, 0);
	up->enabled = 0;

	/*
	 *  turn off power
	 */
	if(up == &uart[Serial]){
		if(serial(0) < 0)
			print("can't turn off serial power\n");
	} else {
		if(modem(0) < 0)
			print("can't turn off modem speaker\n");
	}
}

/*
 *  set up an uart port as something other than a stream
 */
void
uartspecial(int port, IOQ *oq, IOQ *iq, int baud)
{
	Uart *up = &uart[port];

	uartsetup();
	up->special = 1;
	up->oq = oq;
	up->iq = iq;
	uartenable(up);
	if(baud)
		uartsetbaud(up, baud);

	if(iq){
		/*
		 *  Stupid HACK to undo a stupid hack
		 */ 
		if(iq == &kbdq)
			kbdq.putc = kbdcr2nl;
	}
}

static int	uartputc(IOQ *, int);
static void	uartstopen(Queue*, Stream*);
static void	uartstclose(Queue*);
static void	uartoput(Queue*, Block*);
static void	uartkproc(void *);
Qinfo uartinfo =
{
	nullput,
	uartoput,
	uartstopen,
	uartstclose,
	"uart"
};

static void
uartstopen(Queue *q, Stream *s)
{
	Uart *up;
	char name[NAMELEN];

	up = &uart[s->id];
	up->iq->putc = 0;
	uartenable(up);

	qlock(up);
	up->wq = WR(q);
	WR(q)->ptr = up;
	RD(q)->ptr = up;
	qunlock(up);

	if(up->kstarted == 0){
		up->kstarted = 1;
		sprint(name, "uart%d", s->id);
		kproc(name, uartkproc, up);
	}
}

static void
uartstclose(Queue *q)
{
	Uart *up = q->ptr;

	if(up->special)
		return;

	uartdisable(up);

	qlock(up);
	kprint("uartstclose: q=0x%ux, id=%d\n", q, up-uart);
	up->wq = 0;
	up->iq->putc = 0;
	WR(q)->ptr = 0;
	RD(q)->ptr = 0;
	qunlock(up);
}

static void
uartoput(Queue *q, Block *bp)
{
	Uart *up = q->ptr;
	IOQ *cq;
	int n, m;

	if(up == 0){
		freeb(bp);
		return;
	}
	cq = up->oq;
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
			uartsetbaud(up, n);
			break;
		case 'D':
		case 'd':
			uartdtr(up, n);
			break;
		case 'K':
		case 'k':
			uartbreak(up, n);
			break;
		case 'L':
		case 'l':
			uartbits(up, n);
			break;
		case 'm':
		case 'M':
			uartmflow(up, n);
			break;
		case 'P':
		case 'p':
			uartparity(up, *(bp->rptr+1));
			break;
		case 'R':
		case 'r':
			uartrts(up, n);
			break;
		case 'S':
		case 's':
			uartstop(up, n);
			break;
		}
	}else while((m = BLEN(bp)) > 0){
		while ((n = canputc(cq)) == 0){
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
uartkproc(void *a)
{
	Uart *up = a;
	IOQ *cq = up->iq;
	Block *bp;
	int n;
	ulong frame, overrun;
	static ulong ints;

	frame = 0;
	overrun = 0;

	if(waserror())
		print("uartkproc got an error\n");

	for(;;){
		sleep(&cq->r, cangetc, cq);
		if((ints++ & 0x1f) == 0)
			lights((ints>>5)&1);
		qlock(up);
		if(up->wq == 0){
			cq->out = cq->in;
		}else{
			n = cangetc(cq);
			bp = allocb(n);
			bp->flags |= S_DELIM;
			bp->wptr += gets(cq, bp->wptr, n);
			PUTNEXT(RD(up->wq), bp);
		}
		qunlock(up);
		if(up->frame != frame){
			kprint("uart%d: %d framing\n", up-uart, up->frame);
			frame = up->frame;
		}
		if(up->overrun != overrun){
			kprint("uart%d: %d overruns\n", up-uart, up->overrun);
			overrun = up->overrun;
		}
	}
}

enum{
	Qdir=		0,
	Qeia0=		STREAMQID(0, Sdataqid),
	Qeia0ctl=	STREAMQID(0, Sctlqid),
	Qeia1=		STREAMQID(1, Sdataqid),
	Qeia1ctl=	STREAMQID(1, Sctlqid),
};

Dirtab uartdir[]={
	"eia0",		{Qeia0},	0,		0666,
	"eia0ctl",	{Qeia0ctl},	0,		0666,
	"eia1",		{Qeia1},	0,		0666,
	"eia1ctl",	{Qeia1ctl},	0,		0666,
};

#define	NUart	(sizeof uartdir/sizeof(Dirtab))

/*
 *  allocate the queues if no one else has
 */
void
uartreset(void)
{
	Uart *up;

	uartsetup();
	for(up = uart; up < &uart[2]; up++){
		if(up->special)
			continue;
		up->iq = xalloc(sizeof(IOQ));
		initq(up->iq);
		up->oq = xalloc(sizeof(IOQ));
		initq(up->oq);
	}
}

void
uartinit(void)
{
}

Chan*
uartattach(char *upec)
{
	return devattach('t', upec);
}

Chan*
uartclone(Chan *c, Chan *nc)
{
	return devclone(c, nc);
}

int
uartwalk(Chan *c, char *name)
{
	return devwalk(c, name, uartdir, NUart, devgen);
}

void
uartstat(Chan *c, char *dp)
{
	switch(c->qid.path){
	case Qeia0:
		streamstat(c, dp, uartdir[0].name, uartdir[0].perm);
		break;
	case Qeia1:
		streamstat(c, dp, uartdir[2].name, uartdir[2].perm);
		break;
	default:
		devstat(c, dp, uartdir, NUart, devgen);
		break;
	}
}

Chan*
uartopen(Chan *c, int omode)
{
	Uart *up;

	switch(c->qid.path){
	case Qeia0:
	case Qeia0ctl:
		up = &uart[0];
		break;
	case Qeia1:
	case Qeia1ctl:
		up = &uart[1];
		break;
	default:
		up = 0;
		break;
	}

	if(up && up->special)
		error(Einuse);

	if((c->qid.path & CHDIR) == 0)
		streamopen(c, &uartinfo);
	return devopen(c, omode, uartdir, NUart, devgen);
}

void
uartcreate(Chan *c, char *name, int omode, ulong perm)
{
	USED(c, name, omode, perm);
	error(Eperm);
}

void
uartclose(Chan *c)
{
	if(c->stream)
		streamclose(c);
}

long
uartread(Chan *c, void *buf, long n, ulong offset)
{
	USED(offset);
	switch(c->qid.path&~CHDIR){
	case Qdir:
		return devdirread(c, buf, n, uartdir, NUart, devgen);
	case Qeia1ctl:
	case Qeia0ctl:
		return 0;
	}
	return streamread(c, buf, n);
}

long
uartwrite(Chan *c, void *va, long n, ulong offset)
{
	USED(offset);
	return streamwrite(c, va, n, 0);
}

void
uartremove(Chan *c)
{
	USED(c);
	error(Eperm);
}

void
uartwstat(Chan *c, char *dp)
{
	USED(c, dp);
	error(Eperm);
}
