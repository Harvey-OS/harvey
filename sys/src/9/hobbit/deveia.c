#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/error.h"

enum {
	NCtlr		= 1,		/* number of controllers */

	NPort		= 4,		/* number of ports per controller */
	NDirent		= 2,		/* number of directory entries per port -
					 *  data + ctl
					 */
};

enum {
	Bits5		= 0x00,		/* mr1 */
	Bits6		= 0x01,
	Bits7		= 0x02,
	Bits8		= 0x03,
	BitsMask	= 0x03,
	ParityEven	= 0x00,
	ParityOdd	= 0x04,
	ParityOn	= 0x00,
	ParityOff	= 0x10,
	ParityMask	= 0x18,

	StopBits1	= 0x07,		/* mr2 */
	StopBits2	= 0x0F,

	RxEnable	= 0x01,		/* cr */
	RxDisable	= 0x02,
	TxEnable	= 0x04,
	TxDisable	= 0x08,

	MrReset		= 0x10,
	RxReset		= 0x20,
	TxReset		= 0x30,
	ErrReset	= 0x40,
	BrkReset	= 0x50,
	BrkStart	= 0x60,
	BrkStop		= 0x70,

	BD110		= 0x11,		/* csr */
	BD38400		= 0x22,
	BD300		= 0x44,
	BD600		= 0x55,
	BD1200		= 0x66,
	BD2400		= 0x88,
	BD4800		= 0x99,
	BD9600		= 0xBB,
	BD19200		= 0xCC,

	RxRDY		= 0x01,		/* sr */
	FFULL 		= 0x02,
	TxRDY		= 0x04,
	TxEMT		= 0x08,
	RxERR		= 0x70,
	RxBrk		= 0x80,

	TimerD16	= 0x70,		/* acr */
	BRGSet1		= 0x00,
	BRGSet2		= 0x80,

	TxRDYa		= 0x01,		/* isr/imr */
	RxRDYa		= 0x02,
	CtrRDY		= 0x08,
	TxRDYb		= 0x10,
	RxRDYb		= 0x20,

	TxAvail0	= 0x08,		/* cir */
	RxAvail		= 0x0C,
	CtAvail		= 0x14,
	TxAvail1	= 0x18,
	RxAvailErr	= 0x1C,
	TypeMask	= 0x1C,

	Crystal		= 3686400,
};

typedef struct {
	QLock;
	int	printing;		/* true if printing */

	/*
	 * console interface
	 */
	int	nostream;		/* can't use the stream interface */
	IOQ	*iq;			/* input character queue */
	IOQ	*oq;			/* output character queue */

	/*
	 * stream interface
	 */
	Queue	*wq;			/* write queue */
	Rendez	r;			/* kproc waiting for input */
 	int	kstarted;		/* kproc started */

	Uart	*uart;
	Duart	*duart;
	ulong	mr1;			/* sticky mr1 bits */
	ulong	mr2;			/* sticky mr2 bits */
	ulong	*imr;			/* sticky imr bits */
	ulong	realimr;
	ulong	iopcr;			/* sticky iopcr bits */
} Port;

typedef struct {
	Quart	*quart;
	Port	*port;
} Ctlr;
static Ctlr eiactlr[NCtlr];
static neiactlr;

#ifdef notdef
int
eiarawputc(int c)
{
	Uart *uart = &EIAADDR->duart[0].a;

	if(c == '\n')
		eiarawputc('\r');
	uart->cr = TxEnable;
	while((uart->sr & TxRDY) == 0)
		delay(4);
	uart->txfifo = c;
	while((uart->sr & TxEMT) == 0)
		delay(4);
	if(c == '\n')
		delay(100);
	return c;
}

void
eiarawputs(char *s)
{
	while(*s)
		eiarawputc(*s++);
}

void
eiarawprint(char *fmt, ...)
{
	char buf[PRINTSIZE];
	int s;

	s = splhi();
	doprint(buf, buf+sizeof(buf), fmt, (&fmt+1));
	eiarawputs(buf);
	splx(s);
}
#endif

static void
uartsetbaud(Port *port, int rate)
{
	switch(rate){

	case 38400:
		rate = BD38400;
		break;

	case 19200:
		rate = BD19200;
		break;

	case 9600:
		rate = BD9600;
		break;

	case 4800:
		rate = BD4800;
		break;

	case 2400:
		rate = BD2400;
		break;

	case 1200:
		rate = BD1200;
		break;

	case 600:
		rate = BD600;
		break;

	case 300:
		rate = BD300;
		break;

	default:
		return;
	}
	port->uart->csr = rate;
}

static void
uartsetbreak(Port *port, int ms)
{
	Uart *uart = port->uart;
	Duart *duart = port->duart;
	ulong imr;

	if(ms == 0)
		ms = 100;
	imr = (uart == (Uart*)duart) ? TxRDYa: TxRDYb;
	*port->imr &= ~imr;
	duart->imr = *port->imr;
	uart->cr = BrkStart|TxEnable;
	tsleep(&u->p->sleep, return0, 0, ms);
	uart->cr = BrkStop|TxDisable;
	if(port->oq){
		*port->imr |= imr;
		duart->imr = *port->imr;
		uart->cr = TxEnable;
	}
}

static void
uartsetlength(Port *port, int bits)
{
	Uart *uart = port->uart;

	switch(bits){

	case 5:
		bits = Bits5;
		break;

	case 6:
		bits = Bits6;
		break;

	case 7:
		bits = Bits7;
		break;

	case 8:
		bits = Bits8;
		break;

	default:
		return;
	}
	port->mr1 = (port->mr1 & ~BitsMask)|bits;
	uart->cr = MrReset;
	delay(1);
	uart->mr = port->mr1;
}

static void
uartsetdtr(Port *port, int set)
{
	ulong opr = port->duart->opr;

	if(set)
		port->iopcr |= 0x40;
	else
		port->iopcr &= ~0xC0;
	if(port->uart == (Uart*)port->duart){
		port->duart->iopcra = port->iopcr;
		if(set)
			opr |= 0x20;
		else
			opr &= ~0x20;
	}
	else {
		port->duart->iopcrb = port->iopcr;
		if(set)
			opr |= 0x80;
		else
			opr &= ~0x80;
	}
	port->duart->opr = opr;
}

static void
uartsetparity(Port *port, uchar type)
{
	Uart *uart = port->uart;
	ulong parity;

	switch(type){

	case 'e':
		parity = ParityEven;
		break;

	case 'o':
		parity = ParityOdd;
		break;

	default:
		parity = ParityOff;
		break;
	}
	port->mr1 = (port->mr1 & ~ParityMask)|parity;
	uart->cr = MrReset;
	delay(1);
	uart->mr = port->mr1;
}

static void
uartsetrts(Port *port, int set)
{
	ulong opr = port->duart->opr;

	if(set)
		port->iopcr |= 0x10;
	else
		port->iopcr &= ~0x30;
	if(port->uart == (Uart*)port->duart){
		port->duart->iopcra = port->iopcr;
		if(set)
			opr |= 0x10;
		else
			opr &= ~0x10;
	}
	else {
		port->duart->iopcrb = port->iopcr;
		if(set)
			opr |= 0x40;
		else
			opr &= ~0x40;
	}
	port->duart->opr = opr;
}

static void
uartsetstop(Port *port, int bits)
{
	switch(bits){

	case 1:
		bits = StopBits1;
		break;

	case 2:
		bits = StopBits2;
		break;

	default:
		return;
	}
	port->mr2 = StopBits1;
	port->uart->mr = port->mr2;
}

static void
uartputs(IOQ *ioq, char *s, int n)
{
	Port *port = ioq->ptr;
	Uart *uart = port->uart;
	int c, x;

	x = splhi();
	lock(ioq);
	puts(ioq, s, n);
	if(port->printing == 0 && (c = getc(ioq)) >= 0){
		port->printing = 1;
		uart->cr = TxEnable;
		while((uart->sr & TxRDY) == 0)
			;
		do
			uart->txfifo = c;
		while((uart->sr & TxRDY) && (c = getc(ioq)) >= 0);
	}
	unlock(ioq);
	splx(x);
}

static void
uartenable(Port *port)
{
	Uart *uart = port->uart;
	Duart *duart = port->duart;
	ulong imr = 0;

	if(port->oq){
		port->oq->puts = uartputs;
		port->oq->ptr = port;
		uart->cr = TxEnable;
		imr |= (uart == (Uart*)duart) ? TxRDYa: TxRDYb;
	}

	if(port->iq){
		port->iq->ptr = port;
		uart->cr = RxEnable;
		imr |= (uart == (Uart*)duart) ? RxRDYa: RxRDYb;
	}

	*port->imr |= imr;
	duart->imr = *port->imr;
	
	uartsetdtr(port, 1);
	uartsetrts(port, 1);
}

static void
uartsetup(Port *port)
{
	Uart *uart = port->uart;

	uart->cr = RxReset|TxDisable|RxDisable;
	delay(1);
	uart->cr = TxReset;
	delay(1);
	uart->cr = ErrReset;
	delay(1);
	uart->cr = BrkStop;
	delay(1);

	uart->cr = MrReset;
	delay(1);
	port->mr1 = ParityOff|Bits8;
	uart->mr = port->mr1;
	port->mr2 = StopBits1;
	uart->mr = port->mr2;
	uart->csr = BD19200;
}

static void
eiaintr(int ctlrno, Ureg *ur)
{
	Quart *quart;
	Port *port;
	IOQ *ioq;
	int type, count, c;

	quart = eiactlr[ctlrno].quart;
	for(quart->update = 0; type = (quart->cir & TypeMask); quart->update = 0){
		port = eiactlr[ctlrno].port+(quart->gicr & 0x03);
		switch(type){

		case TxAvail0:
		case TxAvail1:
			ioq = port->oq;
			lock(ioq);
			for(count = quart->gibcr & 0x07; count; count--){
				if((c = getc(ioq)) < 0){
					port->uart->cr = TxDisable;
					port->printing = 0;
					wakeup(&ioq->r);
					break;
				}
				quart->gtxfifo = c;
			}
			unlock(ioq);
			break;

		case RxAvail:
			ioq = port->iq;
			for(count = quart->gibcr & 0x07; count; count--){
				c = quart->grxfifo & 0xFF;
				if(ioq->putc)
					(*ioq->putc)(ioq, c);
				else
					putc(ioq, c);
			}
			break;

		case CtAvail:
			c = port->duart->stop;
			clockintr(ctlrno, ur);
			break;

		case RxAvailErr:
			c = quart->grxfifo & 0xFF;
			port->uart->cr = ErrReset;
			break;

		default:
			panic("eiaintr: type #%2.2ux\n", type);
			break;
		}
	}
}

static Vector vector[NCtlr] = {
	{ IRQQUART, 0, eiaintr, "eia0", },
};

void
eiasetup(int ctlrno, void *addr)
{
	Ctlr *ctlr;
	Quart *quart;
	Port *port;

	if(ctlrno >= NCtlr)
		return;
	ctlr = &eiactlr[ctlrno];
	ctlr->quart = quart = addr;
	ctlr->port = port = xalloc(sizeof(Port)*NPort);

	quart->ack = 0;
	quart->icr = 0;

	port->uart = &quart->duart[0].a;
	port->duart = &quart->duart[0];
	port->imr = &port->realimr;
	port->duart->acr = BRGSet2;
	uartsetup(port++);

	port->uart = &quart->duart[0].b;
	port->duart = &quart->duart[0];
	port->imr = &(port-1)->realimr;
	uartsetup(port++);

	port->uart = &quart->duart[1].a;
	port->duart = &quart->duart[1];
	port->imr = &port->realimr;
	port->duart->acr = BRGSet2;
	uartsetup(port++);

	port->uart = &quart->duart[1].b;
	port->duart = &quart->duart[1];
	port->imr = &(port-1)->realimr;
	uartsetup(port);

	setvector(&vector[ctlrno]);
	neiactlr++;
}

void
clockinit(void)
{
	Port *port = eiactlr[0].port;
	Duart *duart = port->duart;
	ulong x;

	duart->acr = BRGSet2|TimerD16;
	x = (Crystal/(2*16*HZ));
	duart->ctu = x>>8;
	duart->ctl = x & 0xFF;

	*port->imr |= CtrRDY;
	duart->imr = *port->imr;
	x = duart->start;
	USED(x);
}

void
eiaspecial(int portno, IOQ *oq, IOQ *iq, int baud)
{
	Port *port;
	int ctlrno;

	ctlrno = portno/NPort;
	if(ctlrno >= neiactlr)
		return;
	port = eiactlr[ctlrno].port+(portno%NPort);

	port->nostream = 1;
	port->oq = oq;
	port->iq = iq;
	uartsetbaud(port, baud);
	uartenable(port);
	if(iq == &kbdq)
		kbdq.putc = kbdcr2nl;
}

/*
 * Hacks for the serial keyboard:
 *	eiasetrts
 *	eiasetparity
 *	eiarawput
 */
static Port*
eiagetport(int portno)
{
	int ctlrno;

	ctlrno = portno/NPort;
	if(ctlrno >= neiactlr)
		return 0;
	return eiactlr[ctlrno].port+(portno%NPort);
}

void
eiasetrts(int portno, int set)
{
	Port *port;

	if(port = eiagetport(portno))
		uartsetrts(port, set);
}

void
eiasetparity(int portno, uchar type)
{
	Port *port;

	if(port = eiagetport(portno))
		uartsetparity(port, type);
}

void
eiarawput(int portno, int c)
{
	Port *port;
	Uart *uart;

	if(port = eiagetport(portno)){
		uart = port->uart;
		uart->cr = TxEnable;
		while((uart->sr & TxRDY) == 0)
			delay(4);
		uart->txfifo = c;
		while((uart->sr & TxEMT) == 0)
			delay(4);
	}
}

void
eiaclock(void)
{
	Port *port;
	int ctlrno, i;
	IOQ *ioq;

	for(ctlrno = 0; ctlrno < neiactlr; ctlrno++){
		for(port = eiactlr[ctlrno].port, i = 0; i < NPort; i++, port++){
			ioq = port->iq;
			if(port->wq && cangetc(ioq))
				wakeup(&ioq->r);
		}
	}
}

static void
eiakproc(void *a)
{
	Port *port = a;
	IOQ *ioq = port->iq;
	Block *bp;
	int n;

	for(;;){
		while((n = cangetc(ioq)) == 0)
			sleep(&ioq->r, cangetc, ioq);
		qlock(port);
		if(port->wq == 0)
			ioq->out = ioq->in;
		else {
			bp = allocb(n);
			bp->flags |= S_DELIM;
			bp->wptr += gets(ioq, bp->wptr, n);
			PUTNEXT(RD(port->wq), bp);
		}
		qunlock(port);
	}
}

static void
eiastopen(Queue *q, Stream *s)
{
	Port *port;
	char name[NAMELEN];

	port = eiactlr[s->id/NPort].port+(s->id%NPort);
	qlock(port);
	port->wq = WR(q);
	WR(q)->ptr = port;
	RD(q)->ptr = port;
	qunlock(port);
	if(port->kstarted == 0){
		port->kstarted = 1;
		sprint(name, "eia%dkproc", s->id);
		kproc(name, eiakproc, port);
	}
}

static void
eiastclose(Queue *q)
{
	Port *port = q->ptr;

	qlock(port);
	port->wq = 0;
	port->iq->putc = 0;
	WR(q)->ptr = 0;
	RD(q)->ptr = 0;
	qunlock(port);
}

static void
eiaoput(Queue *q, Block *bp)
{
	Port *port = q->ptr;
	IOQ *ioq;
	int n, m;

	if(port == 0){
		freeb(bp);
		return;
	}
	ioq = port->oq;
	if(waserror()){
		freeb(bp);
		nexterror();
	}
	if(bp->type == M_CTL){
		while(cangetc(ioq))
			sleep(&ioq->r, cangetc, ioq);
		n = strtoul((char*)(bp->rptr+1), 0, 0);
		switch(*bp->rptr){

		case 'B':
		case 'b':
			uartsetbaud(port, n);
			break;

		case 'D':
		case 'd':
			uartsetdtr(port, n);
			break;

		case 'K':
		case 'k':
			uartsetbreak(port, n);
			break;

		case 'L':
		case 'l':
			uartsetlength(port, n);
			break;

		case 'P':
		case 'p':
			uartsetparity(port, *(bp->rptr+1));
			break;

		case 'R':
		case 'r':
			uartsetrts(port, n);
			break;

		case 'S':
		case 's':
			uartsetstop(port, n);
			break;

		case 'W':
		case 'w':
			/* obsolete */
			break;
		}
	}
	else while((m = BLEN(bp)) > 0){
		while((n = canputc(ioq)) == 0){
			kprint("eiaoput: sleeping\n");
			sleep(&ioq->r, canputc, ioq);
		}
		if(n > m)
			n = m;
		(*ioq->puts)(ioq, bp->rptr, n);
		bp->rptr += n;
	}
	freeb(bp);
	poperror();
}

Qinfo eiainfo = {
	nullput, eiaoput, eiastopen, eiastclose, "eia"
};

static Dirtab *eiadir;

void
eiareset(void)
{
	Port *port;
	int ctlrno, i, portno;
	Dirtab *dp;

	eiadir = xalloc(NDirent * NPort*neiactlr * sizeof(Dirtab));
	dp = eiadir;
	for(ctlrno = 0, portno = 0; ctlrno < neiactlr; ctlrno++){
		for(port = eiactlr[ctlrno].port, i = 0; i < NPort; i++, portno++, port++){
			sprint(dp->name, "eia%d", portno);
			dp->length = 0;
			dp->qid.path = STREAMQID(portno, Sdataqid);
			dp->perm = 0666;
			dp++;
			sprint(dp->name, "eia%dctl", portno);
			dp->length = 0;
			dp->qid.path = STREAMQID(portno, Sctlqid);
			dp->perm = 0666;
			dp++;
	
			if(port->nostream)
				continue;
			port->iq = xalloc(sizeof(IOQ));
			initq(port->iq);
			port->oq = xalloc(sizeof(IOQ));
			initq(port->oq);
			uartenable(port);
		}
	}
}

void
eiainit(void)
{
}

Chan *
eiaattach(char *spec)
{
	return devattach('t', spec);
}

Chan *
eiaclone(Chan *c, Chan *nc)
{
	return devclone(c, nc);
}

int
eiawalk(Chan *c, char *name)
{
	return devwalk(c, name, eiadir, NDirent * NPort*neiactlr, devgen);
}

void
eiastat(Chan *c, char *dp)
{
	int i;

	i = STREAMID(c->qid.path);
	switch(STREAMTYPE(c->qid.path)){
	case Sdataqid:
		streamstat(c, dp, eiadir[NDirent*i].name, eiadir[NDirent*i].perm);
		break;
	default:
		devstat(c, dp, eiadir, NDirent * NPort*neiactlr, devgen);
		break;
	}
}

Chan *
eiaopen(Chan *c, int omode)
{
	Port *port;

	if(c->qid.path != CHDIR){
		port = eiactlr[STREAMID(c->qid.path)/NPort].port+(STREAMID(c->qid.path)%NPort);
		if(port->nostream)
			error(Einuse);
		streamopen(c, &eiainfo);
	}
	return devopen(c, omode, eiadir, NDirent * NPort*neiactlr, devgen);
}

void
eiacreate(Chan *c, char *name, int omode, ulong perm)
{
	USED(c, name, omode, perm);
	error(Eperm);
}

void
eiaclose(Chan *c)
{
	if(c->stream)
		streamclose(c);
}

long
eiaread(Chan *c, void *va, long n, ulong offset)
{
	char id[8];

	if(c->qid.path == CHDIR)
		return devdirread(c, va, n, eiadir, NDirent * NPort*neiactlr, devgen);

	switch(STREAMTYPE(c->qid.path)){

	case Sdataqid:
		return streamread(c, va, n);

	case Sctlqid:
		sprint(id, "%d", STREAMID(c->qid.path));
		return readstr(offset, va, n, id);

	default:
		error(Egreg);
	}
	return -1;
}

long
eiawrite(Chan *c, void *va, long n, ulong offset)
{
	USED(offset);
	return streamwrite(c, va, n, 0);
}

void
eiaremove(Chan *c)
{
	USED(c);
	error(Eperm);
}

void
eiawstat(Chan *c, char *dp)
{
	USED(c, dp);
	error(Eperm);
}
