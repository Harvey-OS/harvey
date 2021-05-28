/*
 * Xilinx uartlite driver.
 * uartlite has 16-byte fifos.
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"

#include "io.h"

enum {
	/* control register bit positions */
	Ctlintrena	= 0x10,		/* enable interrupt */
	Rxfiforst	= 0x02,		/* reset receive FIFO */
	Txfiforst	= 0x01,		/* reset transmit FIFO */

	/* status register bit positions */
	Parerr		= 0x80,
	Framerr		= 0x40,
	Overrunerr	= 0x20,
	Stsintrena	= 0x10,		/* interrupt enabled */
	Txfifofull	= 0x08,		/* transmit FIFO full */
	Txfifoempty	= 0x04,		/* transmit FIFO empty */
	Rxfifofull	= 0x02,		/* receive FIFO full */
	Rxfifohasdata	= 0x01,		/* data in receive FIFO */
};

typedef struct Uartregs Uartregs;
struct Uartregs {
	ulong	rxfifo;
	ulong	txfifo;
	ulong	status;			/* read-only; read clears error bits */
	ulong	ctl;			/* write-only */
};

typedef struct Ctlr {
	int	port;
	Uartregs *regs;
	int	irq;
	int	tbdf;
	int	iena;
	int	poll;

	int	fena;
//	Rendez	xmitrend;
//	Rendez	rcvrend;
} Ctlr;

extern PhysUart litephysuart;

static Ctlr litectlr[1] = {
{
	.port = 0,
	.regs = (Uartregs *)Uartlite,
	.poll	= 0, },			/* was 1 */
};

static Uart liteuart[1] = {
{	.regs	= &litectlr[0],
	.name	= "eia0",
	.phys	= &litephysuart,
	.special= 0,
	.next	= nil, },
};

static int
canxmit(void *stsp)
{
	return !(*(ulong *)stsp & Txfifofull);
}

static int
canread(void *stsp)
{
	return (*(ulong *)stsp & Rxfifohasdata) != 0;
}

static long
litestatus(Uart* uart, void* buf, long n, long offset)
{
	char *p;
	Ctlr *ctlr;

	ctlr = uart->regs;
	p = malloc(READSTR);
	snprint(p, READSTR,
		"b%d c%d d%d e%d l%d m%d p%c r%d s%d i%d\n"
		"dev(%d) type(%d) framing(%d) overruns(%d) "
		"berr(%d) serr(%d)%s%s%s%s\n",

		uart->baud,
		uart->hup_dcd,
		0,
		uart->hup_dsr,
		8,
		0,
		'n',
		0,
		1,
		ctlr->fena,

		uart->dev,
		uart->type,
		uart->ferr,
		uart->oerr,
		uart->berr,
		uart->serr,
		"",
		"",
		"",
		""
	);
	n = readstr(offset, buf, n, p);
	free(p);
	return n;
}

#define litedtr litefifo
#define literts litefifo
#define litemodemctl litefifo
#define litebreak litefifo

static void
litefifo(Uart*, int)
{
}

#define litestop liteparity
#define litebits liteparity
#define litebaud liteparity

static int
liteparity(Uart*, int)
{
	return 0;
}

static void
litekick(Uart* uart)
{
	Ctlr *ctlr;
	Uartregs *urp;

	if(uart == nil || uart->blocked)
		return;
	ctlr = uart->regs;
	urp = ctlr->regs;
	while (!(urp->status & Txfifofull)) {
		if(uart->op >= uart->oe && uartstageoutput(uart) == 0)
			break;
		uartputc(*uart->op++);
	}
}

static int
liteinterrupt(ulong bit)
{
	int sts;
	Ctlr *ctlr;
	Uart *uart;
	Uartregs *urp;

	uart = liteuart;
	if (uart == nil) 
		return 0;
	ctlr = uart->regs;
	urp = ctlr->regs;
	sts = urp->status;		/* clears error bits */

	while (urp->status & Rxfifohasdata) {
		uartrecv(uart, urp->rxfifo);
//		wakeup(&ctlr->rcvrend);
	}
	if (!(urp->status & Txfifofull)) {
		uartkick(uart);
//		wakeup(&ctlr->xmitrend);
	}
	if (sts & (Parerr|Framerr|Overrunerr|Txfifoempty|Rxfifofull|
	    Txfifofull|Rxfifohasdata)) {
		intrack(bit);
		return 1;
	}
	return 0;
}

static void
litedisable(Uart* uart)
{
	Ctlr *ctlr;
	Uartregs *urp;

	ctlr = uart->regs;
	if(ctlr->iena != 0){
		urp = ctlr->regs;
		/* wait for output to drain */
		while(!(urp->status & Txfifoempty))
			delay(1);
//		intrdisable(Intuart, liteinterrupt);
		urp->ctl = Txfiforst | Rxfiforst;
		barriers();
	}
}

static void
liteenable(Uart* uart, int ie)
{
	int ier;
	Ctlr *ctlr;

	ctlr = uart->regs;

	/*
 	 * Enable interrupts and turn on DTR and RTS.
	 * Be careful if this is called to set up a polled serial line
	 * early on not to try to enable interrupts as interrupt-
	 * -enabling mechanisms might not be set up yet.
	 */
	ier = 0;
	if(ie){
		if(ctlr->iena == 0 && !ctlr->poll){
			intrenable(Intuart, liteinterrupt, "uartlite");
			barriers();
			ier = Ctlintrena;
			ctlr->iena++;
		}
	}
	ctlr->regs->ctl = ier;
	barriers();
}

static Uart*
litepnp(void)
{
	return liteuart;
}

static int
litegetc(Uart *uart)
{
	Ctlr *ctlr;
	Uartregs *urp;

	ctlr = uart->regs;
	urp = ctlr->regs;
	while(!(urp->status & Rxfifohasdata))
		delay(1);
	return (uchar)urp->rxfifo;
}

static void
liteputc(Uart *uart, int c)
{
	Ctlr *ctlr;
	Uartregs *urp;

	ctlr = uart->regs;
	urp = ctlr->regs;
	while(urp->status & Txfifofull)
		delay(1);
//		sleep(&ctlr->xmitrend, canxmit, &urp->status);
	urp->txfifo = (uchar)c;
	barriers();
	while(urp->status & Txfifofull)
		delay(1);
}

/*
 * for debugging.  no function calls are possible as this is called before
 * we have a valid stack pointer.
 */
void
uartlputc(int c)
{
	ulong cnt;
	Uartregs *urp;

	urp = (Uartregs *)Uartlite;
	for (cnt = m->cpuhz; urp->status & Txfifofull && cnt-- > 0; )
		;
	urp->txfifo = (uchar)c;
	barriers();
	for (cnt = m->cpuhz; urp->status & Txfifofull && cnt-- > 0; )
		;
}

void
uartlputs(char *s)
{
	while (*s != '\0')
		uartlputc(*s++);
}

static void
litepoll(Uart* uart)
{
	Ctlr *ctlr;

	/*
	 * If PhysUart has a non-nil .poll member, this
	 * routine will be called from the uartclock timer.
	 * If the Ctlr .poll member is non-zero, when the
	 * Uart is enabled interrupts will not be enabled
	 * and the result is polled input and output.
	 * Not very useful here, but ports to new hardware
	 * or simulators can use this to get serial I/O
	 * without setting up the interrupt mechanism.
	 */
	ctlr = uart->regs;
	if(ctlr && !ctlr->iena && ctlr->poll)
		liteinterrupt(Intuart);
}

PhysUart litephysuart = {
	.name		= "lite",
	.pnp		= litepnp,
	.enable		= liteenable,
	.disable	= litedisable,
	.kick		= litekick,
	.dobreak	= litebreak,
	.baud		= litebaud,
	.bits		= litebits,
	.stop		= litestop,
	.parity		= liteparity,
	.modemctl	= litemodemctl,
	.rts		= literts,
	.dtr		= litedtr,
	.status		= litestatus,
	.fifo		= litefifo,
	.getc		= litegetc,
	.putc		= liteputc,
//	.poll		= litepoll,
};

void
uartliteconsole(void)
{
	Uart *uart;

	uart = &liteuart[0];
	(*uart->phys->enable)(uart, 0);
	uartctl(uart, "b115200 l8 pn s1 i1 x0");
	uart->console = 1;
	consuart = uart;
}
