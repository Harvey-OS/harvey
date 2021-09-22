/*
 * SiFive UART (serial port) driver.  *Not* an 8250.
 * Has 8-byte receive and transmit FIFOs.
 *
 * Admittedly, the 8250 is brain-damaged (it *is* from Intel),
 * but it's de facto standard.
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

#include "riscv.h"

/* can we print uart error messages on the console? should trigger recursion */
#define print(...)

enum {
	Change_speed	= 0,		/* flag: set baud rate */
//	Txfifomax	= 1,		/* require tx intr per byte */
	Txfifomax	= 16,		/* require tx intr per byte */

	Uart0		= PAUart0,
};

typedef struct Sifiveuart Sifiveuart;
struct Sifiveuart {
	ulong	txdata;
	ulong	rxdata;
	ulong	txctrl;
	ulong	rxctrl;
	ulong	ie;
	ulong	ip;
	ulong	div;
};

/* data registers bit */
enum {
	Notready	= 1ul<<31,
};

/* control registers bits */
enum {
	Enable		= 1<<0,
	Stopbits	= 1<<1,
	Wmlvlshft	= 16,		/* watermark level */
	Wmlvl		= MASK(3)<<Wmlvlshft,
};

/* interrupt registers bits */
enum {
	Txwm		= 1<<0,
	Rxwm		= 1<<1,
};

typedef struct Ctlr {
	Intrcommon;
	int	defirq;
	ulong	*io;		/* mapped va */
	ulong	*phyio;		/* unmapped pa */
	ulong	tbdf;
	int	iena;
	int	poll;
	int	unit;

	Lock;
	int	checkfifo;
	int	fena;
} Ctlr;

extern PhysUart sifivephysuart;

static Ctlr sifivectlr[] = {
{	.io	= (ulong *)Uart0,
	.phyio	= (ulong *)Uart0,
	.tbdf	= -1,
	.unit	= 0,
	.poll	= 0, },
#ifdef UART1				/* 2nd uart */
{	.io	= (ulong *)Uart1,
	.phyio	= (ulong *)Uart1,
	.tbdf	= -1,
	.unit	= 1,
	.poll	= 0, },
#endif
};

/* static */ Uart sifiveuart[] = {
{	.regs	= &sifivectlr[0],
	.name	= "eia0",
	.phys	= &sifivephysuart,
	.special= 0,
#ifdef UART1				/* 2nd uart */
	.next	= &sifiveuart[1], },
{	.regs	= &sifivectlr[1],
	.name	= "eia1",
	.phys	= &sifivephysuart,
	.special= 0,
#endif
	.next	= nil, },
};

#define regbase(c)	((Sifiveuart *)(m->virtdevaddrs? (c)->io: (c)->phyio))
#define csr32r(c, r)	(coherence(), regbase(c)->r)
#define csr32o(c, r, v)	((regbase(c)->r = (v)), coherence())
#define csr32w csr32o

void
uartsetregs(int i, uintptr regs)
{
	Ctlr *ctlr;
	Uart *uart;

	if (i == 0)
		uart0regs = regs;
	if((uint)i >= nuart) {
		print("uartconsole: no uart%d\n", i);
		return;
	}
	uartregs[i] = regs;
	uart = &sifiveuart[i];
	ctlr = uart->regs;
	if (ctlr == nil)
		panic("uartsegregs: uart%d: nil uart->regs", i);
	ctlr->io = (ulong *)regs;
	if (consuart == nil) {		/* set consuart on first use */
		consuart = uart;
		uart->console = 1;
	}
}

static long
sifivestatus(Uart* uart, void* buf, long n, long offset)
{
	char *p;
	Ctlr *ctlr;

	ctlr = uart->regs;
	p = malloc(READSTR);
	snprint(p, READSTR,
		"b%d c%d d%d e%d l%d m%d p%c r%d s%d i%d\n"
		"dev(%d) type(%d) framing(%d) overruns(%d) "
		"berr(%d) serr(%d)\n",
		uart->baud, uart->hup_dcd, 0, uart->hup_dsr, 8, 				0, 'n', 0, (csr32r(ctlr, txdata) & Stopbits)? 2: 1, ctlr->fena, 
		uart->dev, uart->type, uart->ferr, uart->oerr,
		uart->berr, uart->serr);
	n = readstr(offset, buf, n, p);
	free(p);

	return n;
}

/* wait for the output fifo to drain, but don't wait forever */
static void
drainfifo(Ctlr *ctlr)
{
	int loops;

	for(loops = 1000; --loops > 0 && csr32r(ctlr, txdata) & Notready; )
		microdelay(10);
}

static void
sifivefifo(Uart* uart, int level)
{
	Ctlr *ctlr;

	ctlr = uart->regs;
	drainfifo(ctlr);
	ilock(ctlr);
	drainfifo(ctlr);

	/*
	 * Set the trigger level.
	 */
	ctlr->fena = level;
	csr32w(ctlr, txctrl, level<<Wmlvlshft | Enable);
	csr32w(ctlr, rxctrl, level<<Wmlvlshft | Enable);
	iunlock(ctlr);
}

/* can't toggle dtr nor rts, adjust parity,  generata a break */
static void
sifivenop(Uart*, int)
{
}

static void
sifivemodemctl(Uart* uart, int on)
{
	/* have no modem-control signals, but modem needs fifo */
	(*uart->phys->fifo)(uart, on);
}

static int
sifiveparity(Uart* uart, int parity)
{
	USED(uart, parity);		/* no parity control */
	return 0;
}

static int
sifivestop(Uart* uart, int stop)
{
	Ctlr *ctlr;

	ctlr = uart->regs;
	csr32w(ctlr, txctrl, (stop == 2? Stopbits: 0) | Enable |
		csr32r(ctlr, txctrl));
	uart->stop = stop;
	return 0;
}

static int
sifivebits(Uart* uart, int bits)
{
	uart->bits = 8;		/* no control of character size */
	USED(bits);
	return 0;
}

static int
sifivebaud(Uart* uart, int baud)
{
	ulong bgc;
	Ctlr *ctlr;

	/*
	 * Set the Baud rate by calculating and setting the Baud rate
	 * Generator Constant. This will work with fairly non-standard
	 * Baud rates.
	 */
	if(baud <= 0)
		return -1;
	if (Change_speed) {
		bgc = (uartfreq+8*baud-1) / (16*baud);
		ctlr = uart->regs;
		csr32o(ctlr, div, bgc);
		uart->baud = baud;
	}
	USED(uart);
	return 0;
}

static void
sifivekick(Uart* uart)
{
	int i;
	Ctlr *ctlr;

	if(uart->blocked || uart->oq == nil)
		return;

	ctlr = uart->regs;

	/*
	 *  Txfifomax here is an arbitrary limit to make sure
	 *  we don't stay in this loop too long.
	 */
	ilock(ctlr);
	for(i = 0; i < Txfifomax; i++){
		if(csr32r(ctlr, txdata) & Notready)	/* tx fifo full? */
			break;
		if(uart->op >= uart->oe && uartstageoutput(uart) == 0)
			break;
		csr32o(ctlr, txdata, *uart->op++);
	}
	if (i > 0 && ctlr->iena)
		csr32w(ctlr, ie, csr32r(ctlr, ie) | Txwm);
	iunlock(ctlr);
}

static Intrsvcret
sifiveinterrupt(Ureg*, void* arg)
{
	Ctlr *ctlr;
	Uart *uart;
	int iir, r, dokick;
	static int count;

	uart = arg;
	if (uart == nil)
		return;		// panic("sifiveinterrupt: nil uart");
	ctlr = uart->regs;
	if (ctlr == nil)
		return;		// panic("sifiveinterrupt: nil ctlr");
	ilock(ctlr);
	iir = csr32r(ctlr, ip);
	if (iir & Rxwm)
		/*
		 * Consume any received data; extinguishes ip bit.
		 */
		while(((r = csr32r(ctlr, rxdata)) & Notready) == 0)
			uartrecv(uart, (uchar)r);
	dokick = iir & Txwm;
	if (dokick)
		csr32w(ctlr, ie, csr32r(ctlr, ie) & ~Txwm);
	iunlock(ctlr);
	if (dokick)
		uartkick(uart);
//	if (ainc(&count) % 5000 == 0 && ctlr->iena)
//		iprint("%d uart intrs\n", count);
}

/* called to poll from extintr() */
void
uartextintr(Ureg *ureg)
{
	sifiveinterrupt(ureg, &sifiveuart[0]);
}

static void
sifivedisable(Uart* uart)
{
	Ctlr *ctlr;

	/*
	 * disable interrupts and fifos.
	 */
	(*uart->phys->fifo)(uart, 0);

	/* if uart is our console, this will stop (i)print output */
	ctlr = uart->regs;
	csr32w(ctlr, ie, 0);
	if(ctlr->iena != 0){
		if(intrdisable(ctlr->vector) == 0)
			ctlr->iena = 0;
	}
	csr32w(ctlr, txctrl, 0);		/* clear enable bits */
	csr32w(ctlr, rxctrl, 0);
}

static void
sifiveenable(Uart* uart, int ie)
{
	Ctlr *ctlr;

	ctlr = uart->regs;
	if (ctlr->io == 0)
		return;
	if(!ctlr->checkfifo)
		drainfifo(ctlr);
	ilock(ctlr);
	if(!ctlr->checkfifo){
		/*
		 * Wait until the transmitter is really empty.
		 */
		drainfifo(ctlr);
		/* interrupt on empty tx fifo, non-empty rx fifo */
		csr32w(ctlr, txctrl, 1<<Wmlvlshft |
			csr32r(ctlr, txctrl) & ~Wmlvl);
		csr32w(ctlr, rxctrl, 0<<Wmlvlshft |
			csr32r(ctlr, rxctrl) & ~Wmlvl);
		if (csr32r(ctlr, rxctrl) & Wmlvl)
			ctlr->checkfifo = 1;
	}
	csr32w(ctlr, txctrl, Enable | csr32r(ctlr, txctrl));
	csr32w(ctlr, rxctrl, Enable | csr32r(ctlr, rxctrl));
	iunlock(ctlr);

	/*
	 * Enable interrupts.
	 * Be careful if this is called to set up a polled serial line
	 * early on not to try to enable interrupts as interrupt-
	 * -enabling mechanisms might not be set up yet.
	 */
	if(ie){
		if(ctlr->iena == 0 && !ctlr->poll){
			ctlr->irq = ioconf("uart", ctlr->unit)->irq;
			ctlr->tbdf = BUSUNKNOWN;
			enableintr(ctlr, sifiveinterrupt, uart, uart->name);
			ctlr->iena = 1;
		}
		csr32w(ctlr, ie, Rxwm);	/* set Txwm only when transmitting */
	} else
		csr32w(ctlr, ie, 0);

	/*
	 * During startup, the interrupt controller is reset.
	 * This may result in a lost interrupt from the sifive uart.
	 * The sifive thinks the interrupt is still outstanding and does not
	 * generate any further interrupts. The workaround is to call the
	 * interrupt handler to clear any pending interrupt events.
	 * Note: this must be done after setting the ie reg.
	 */
	if(ie)
		sifiveinterrupt(nil, uart);
	return Intrunconverted;
}

void*
sifivealloc(uintptr io, int irq, int tbdf)
{
	Ctlr *ctlr;

	if((ctlr = malloc(sizeof(Ctlr))) != nil){
		ctlr->io = (void *)io;
		ctlr->phyio = (void *)io;
		ctlr->irq = irq;
		ctlr->tbdf = tbdf;
	}

	return ctlr;
}

static Uart*
sifivepnp(void)
{
	int i;
	Ctlr *ctlr;
	Uart *head, *uart;

	head = sifiveuart;
	for(i = 0; i < nelem(sifiveuart) && i < nuart; i++){
		uart = &sifiveuart[i];
		ctlr = uart->regs;
		if (ctlr == nil) {
			print("sifivepnp: no Ctlr for uart %d\n", i);
			continue;
		}
		if (ctlr->io == 0) {
			print("sifivepnp: nil Ctlr->io for uart %d\n", i);
			continue;
		}
		if(uart == head)
			head = uart;
		else
			(uart-1)->next = uart;
	}

	return head;
}

int
sifivegetc(Uart* uart)
{
	ulong r;
	Ctlr *ctlr;

	ctlr = uart->regs;
	while((r = csr32r(ctlr, rxdata)) & Notready)
		delay(1);
	return (uchar)r;
}

static void
sifivedrain(Ctlr *ctlr)
{
	int i;

	for(i = 1000; csr32r(ctlr, txdata) & Notready && i > 0; i--)
		delay(1);
}

void
sifiveputc(Uart* uart, int c)
{
	Ctlr *ctlr;

	ctlr = uart->regs;
	sifivedrain(ctlr);
	csr32o(ctlr, txdata, c);
	sifivedrain(ctlr);
}

void
_uartputs(char* s, int n)	/* debugging */
{
	char *e;
	int lastc;

	lastc = 0;
	for(e = s+n; s < e; s++){
		if(*s == '\n' && lastc != '\r')
			sifiveputc(&sifiveuart[0], '\r');
		sifiveputc(&sifiveuart[0], *s);
		lastc = *s;
	}
}

static void
sifivepoll(Uart* uart)
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
	if(!ctlr->iena && ctlr->poll)
		sifiveinterrupt(nil, uart);
}

PhysUart sifivephysuart = {
	.name		= "sifive",
	.pnp		= sifivepnp,
	.enable		= sifiveenable,
	.disable	= sifivedisable,
	.kick		= sifivekick,
	.dobreak	= sifivenop,
	.baud		= sifivebaud,
	.bits		= sifivebits,
	.stop		= sifivestop,
	.parity		= sifiveparity,
	.modemctl	= sifivemodemctl,
	.rts		= sifivenop,
	.dtr		= sifivenop,
	.status		= sifivestatus,
	.fifo		= sifivefifo,
	.getc		= sifivegetc,
	.putc		= sifiveputc,
	.poll		= sifivepoll,
};

Uart*
sifiveconsole(char* cfg)
{
	int i;
	Uart *uart;
	Ctlr *ctlr;
	char *cmd, *p;

	if (nuart == 0) {
		print("sifiveconsole: no uarts\n");
		return nil;
	}

	/*
	 * Before sifivepnp() is run, we can only set the console
	 * to 0 or 1 because those are the only uart structs which
	 * will be the same before and after that.
	 */
	if((p = getconf("console")) == nil && (p = cfg) == nil)
		return nil;
	i = strtoul(p, &cmd, 0);
	if(p == cmd)
		return nil;
	if((uart = uartconsole(i, cmd)) != nil){	/* already enabled? */
		consuart = uart;
		return uart;
	}

	/* set it up */
	if (i >= nuart) {
		print("sifiveconsole: uart %d out of range\n", i);
		return nil;
	}
	uart = &sifiveuart[i];
	ctlr = uart->regs;
	if (ctlr->io == 0) {
		print("sifiveconsole: ctlr->io nil\n");
		return nil;
	}

	(*uart->phys->enable)(uart, 0);
	if (Change_speed)
		uartctl(uart, "b115200");
	uartctl(uart, "l8 pn s1 i1");
	if(*cmd != '\0')
		uartctl(uart, cmd);
	consuart = uart;
	uart->console = 1;
	return uart;
}
