#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

/*
 * 8250 UART and compatibles.
 */
enum {
	Uart0		= 0x3F8,	/* COM1 */
	Uart0IRQ	= 4,
	Uart1		= 0x2F8,	/* COM2 */
	Uart1IRQ	= 3,

	UartFREQ	= 1843200,
};

enum {					/* registers */
	Rbr		= 0,		/* Receiver Buffer (RO) */
	Thr		= 0,		/* Transmitter Holding (WO) */
	Ier		= 1,		/* Interrupt Enable */
	Iir		= 2,		/* Interrupt Identification (RO) */
	Fcr		= 2,		/* FIFO Control (WO) */
	Lcr		= 3,		/* Line Control */
	Mcr		= 4,		/* Modem Control */
	Lsr		= 5,		/* Line Status */
	Msr		= 6,		/* Modem Status */
	Scr		= 7,		/* Scratch Pad */
	Dll		= 0,		/* Divisor Latch LSB */
	Dlm		= 1,		/* Divisor Latch MSB */
};

enum {					/* Ier */
	Erda		= 0x01,		/* Enable Received Data Available */
	Ethre		= 0x02,		/* Enable Thr Empty */
	Erls		= 0x04,		/* Enable Receiver Line Status */
	Ems		= 0x08,		/* Enable Modem Status */
};

enum {					/* Iir */
	Ims		= 0x00,		/* Ms interrupt */
	Ip		= 0x01,		/* Interrupt Pending (not) */
	Ithre		= 0x02,		/* Thr Empty */
	Irda		= 0x04,		/* Received Data Available */
	Irls		= 0x06,		/* Receiver Line Status */
	Ictoi		= 0x0C,		/* Character Time-out Indication */
	IirMASK		= 0x3F,
	Ifena		= 0xC0,		/* FIFOs enabled */
};

enum {					/* Fcr */
	FIFOena		= 0x01,		/* FIFO enable */
	FIFOrclr	= 0x02,		/* clear Rx FIFO */
	FIFOtclr	= 0x04,		/* clear Tx FIFO */
	FIFO1		= 0x00,		/* Rx FIFO trigger level 1 byte */
	FIFO4		= 0x40,		/*	4 bytes */
	FIFO8		= 0x80,		/*	8 bytes */
	FIFO14		= 0xC0,		/*	14 bytes */
};

enum {					/* Lcr */
	Wls5		= 0x00,		/* Word Length Select 5 bits/byte */
	Wls6		= 0x01,		/*	6 bits/byte */
	Wls7		= 0x02,		/*	7 bits/byte */
	Wls8		= 0x03,		/*	8 bits/byte */
	WlsMASK		= 0x03,
	Stb		= 0x04,		/* 2 stop bits */
	Pen		= 0x08,		/* Parity Enable */
	Eps		= 0x10,		/* Even Parity Select */
	Stp		= 0x20,		/* Stick Parity */
	Brk		= 0x40,		/* Break */
	Dlab		= 0x80,		/* Divisor Latch Access Bit */
};

enum {					/* Mcr */
	Dtr		= 0x01,		/* Data Terminal Ready */
	Rts		= 0x02,		/* Ready To Send */
	Out1		= 0x04,		/* no longer in use */
	Ie		= 0x08,		/* IRQ Enable */
	Dm		= 0x10,		/* Diagnostic Mode loopback */
};

enum {					/* Lsr */
	Dr		= 0x01,		/* Data Ready */
	Oe		= 0x02,		/* Overrun Error */
	Pe		= 0x04,		/* Parity Error */
	Fe		= 0x08,		/* Framing Error */
	Bi		= 0x10,		/* Break Interrupt */
	Thre		= 0x20,		/* Thr Empty */
	Temt		= 0x40,		/* Tramsmitter Empty */
	FIFOerr		= 0x80,		/* error in receiver FIFO */
};

enum {					/* Msr */
	Dcts		= 0x01,		/* Delta Cts */
	Ddsr		= 0x02,		/* Delta Dsr */
	Teri		= 0x04,		/* Trailing Edge of Ri */
	Ddcd		= 0x08,		/* Delta Dcd */
	Cts		= 0x10,		/* Clear To Send */
	Dsr		= 0x20,		/* Data Set Ready */
	Ri		= 0x40,		/* Ring Indicator */
	Dcd		= 0x80,		/* Data Set Ready */
};

typedef struct Ctlr {
	Intrcommon;
	int	defirq;
	int	io;
	int	tbdf;
	int	iena;
	int	poll;

	uchar	sticky[8];

	Lock;
	int	hasfifo;
	int	checkfifo;
	int	fena;
} Ctlr;

extern PhysUart i8250physuart;

static Ctlr i8250ctlr[2] = {
{	.io	= Uart0,
	.defirq	= Uart0IRQ,
	.tbdf	= -1,
	.poll	= 0, },

{	.io	= Uart1,
	.defirq	= Uart1IRQ,
	.tbdf	= -1,
	.poll	= 0, },
};

static Uart i8250uart[2] = {
{	.regs	= &i8250ctlr[0],
	.name	= "COM1",
	.freq	= UartFREQ,
	.phys	= &i8250physuart,
	.special= 0,
	.next	= &i8250uart[1], },

{	.regs	= &i8250ctlr[1],
	.name	= "COM2",
	.freq	= UartFREQ,
	.phys	= &i8250physuart,
	.special= 0,
	.next	= nil, },
};

#define csr8r(c, r)	inb((c)->io+(r))
#define csr8w(c, r, v)	outb((c)->io+(r), (c)->sticky[(r)]|(v))
#define csr8o(c, r, v)	outb((c)->io+(r), (v))

static long
i8250status(Uart* uart, void* buf, long n, long offset)
{
	char *p;
	Ctlr *ctlr;
	uchar ier, lcr, mcr, msr;

	ctlr = uart->regs;
	p = malloc(READSTR);
	mcr = ctlr->sticky[Mcr];
	msr = csr8r(ctlr, Msr);
	ier = ctlr->sticky[Ier];
	lcr = ctlr->sticky[Lcr];
	snprint(p, READSTR,
		"b%d c%d d%d e%d l%d m%d p%c r%d s%d i%d\n"
		"dev(%d) type(%d) framing(%d) overruns(%d) "
		"berr(%d) serr(%d)%s%s%s%s\n",

		uart->baud,
		uart->hup_dcd,
		(msr & Dsr) != 0,
		uart->hup_dsr,
		(lcr & WlsMASK) + 5,
		(ier & Ems) != 0,
		(lcr & Pen) ? ((lcr & Eps) ? 'e': 'o'): 'n',
		(mcr & Rts) != 0,
		(lcr & Stb) ? 2: 1,
		ctlr->fena,

		uart->dev,
		uart->type,
		uart->ferr,
		uart->oerr,
		uart->berr,
		uart->serr,
		(msr & Cts) ? " cts": "",
		(msr & Dsr) ? " dsr": "",
		(msr & Dcd) ? " dcd": "",
		(msr & Ri) ? " ring": ""
	);
	n = readstr(offset, buf, n, p);
	free(p);

	return n;
}

static void
i8250fifo(Uart* uart, int level)
{
	Ctlr *ctlr;

	ctlr = uart->regs;
	if(ctlr->hasfifo == 0)
		return;

	/*
	 * Changing the FIFOena bit in Fcr flushes data
	 * from both receive and transmit FIFOs; there's
	 * no easy way to guarantee not losing data on
	 * the receive side, but it's possible to wait until
	 * the transmitter is really empty.
	 */
	while(!(csr8r(ctlr, Lsr) & Temt))
		pause();
	ilock(ctlr);
	while(!(csr8r(ctlr, Lsr) & Temt))
		pause();

	/*
	 * Set the trigger level, default is the max.
	 * value.
	 * Some UARTs require FIFOena to be set before
	 * other bits can take effect, so set it twice.
	 */
	ctlr->fena = level;
	switch(level){
	case 0:
		break;
	case 1:
		level = FIFO1|FIFOena;
		break;
	case 4:
		level = FIFO4|FIFOena;
		break;
	case 8:
		level = FIFO8|FIFOena;
		break;
	default:
		level = FIFO14|FIFOena;
		break;
	}
	csr8w(ctlr, Fcr, level);
	csr8w(ctlr, Fcr, level);
	iunlock(ctlr);
}

static void
i8250dtr(Uart* uart, int on)
{
	Ctlr *ctlr;

	/*
	 * Toggle DTR.
	 */
	ctlr = uart->regs;
	if(on)
		ctlr->sticky[Mcr] |= Dtr;
	else
		ctlr->sticky[Mcr] &= ~Dtr;
	csr8w(ctlr, Mcr, 0);
}

static void
i8250rts(Uart* uart, int on)
{
	Ctlr *ctlr;

	/*
	 * Toggle RTS.
	 */
	assert(uart);
	ctlr = uart->regs;
	assert(ctlr);
	if(on)
		ctlr->sticky[Mcr] |= Rts;
	else
		ctlr->sticky[Mcr] &= ~Rts;
	csr8w(ctlr, Mcr, 0);
}

static void
i8250modemctl(Uart* uart, int on)
{
	Ctlr *ctlr;

	ctlr = uart->regs;
	ilock(&uart->tlock);
	if(on){
		ctlr->sticky[Ier] |= Ems;
		csr8w(ctlr, Ier, ctlr->sticky[Ier]);
		uart->modem = 1;
		uart->cts = csr8r(ctlr, Msr) & Cts;
	}
	else{
		ctlr->sticky[Ier] &= ~Ems;
		csr8w(ctlr, Ier, ctlr->sticky[Ier]);
		uart->modem = 0;
		uart->cts = 1;
	}
	iunlock(&uart->tlock);

	/* modem needs fifo */
	(*uart->phys->fifo)(uart, on);
}

static int
i8250parity(Uart* uart, int parity)
{
	int lcr;
	Ctlr *ctlr;

	ctlr = uart->regs;
	lcr = ctlr->sticky[Lcr] & ~(Eps|Pen);

	switch(parity){
	case 'e':
		lcr |= Eps|Pen;
		break;
	case 'o':
		lcr |= Pen;
		break;
	case 'n':
		break;
	default:
		return -1;
	}
	ctlr->sticky[Lcr] = lcr;
	csr8w(ctlr, Lcr, 0);

	uart->parity = parity;

	return 0;
}

static int
i8250stop(Uart* uart, int stop)
{
	int lcr;
	Ctlr *ctlr;

	ctlr = uart->regs;
	lcr = ctlr->sticky[Lcr] & ~Stb;

	switch(stop){
	case 1:
		break;
	case 2:
		lcr |= Stb;
		break;
	default:
		return -1;
	}
	ctlr->sticky[Lcr] = lcr;
	csr8w(ctlr, Lcr, 0);

	uart->stop = stop;

	return 0;
}

static int
i8250bits(Uart* uart, int bits)
{
	int lcr;
	Ctlr *ctlr;

	ctlr = uart->regs;
	lcr = ctlr->sticky[Lcr] & ~WlsMASK;

	switch(bits){
	case 5:
		lcr |= Wls5;
		break;
	case 6:
		lcr |= Wls6;
		break;
	case 7:
		lcr |= Wls7;
		break;
	case 8:
		lcr |= Wls8;
		break;
	default:
		return -1;
	}
	ctlr->sticky[Lcr] = lcr;
	csr8w(ctlr, Lcr, 0);

	uart->bits = bits;

	return 0;
}

static int
i8250baud(Uart* uart, int baud)
{
	ulong bgc;
	Ctlr *ctlr;

	/*
	 * Set the Baud rate by calculating and setting the Baud rate
	 * Generator Constant. This will work with fairly non-standard
	 * Baud rates.
	 */
	if(uart->freq == 0 || baud <= 0)
		return -1;
	bgc = (uart->freq+8*baud-1)/(16*baud);

	ctlr = uart->regs;
	csr8w(ctlr, Lcr, Dlab);
	csr8o(ctlr, Dlm, bgc>>8);
	csr8o(ctlr, Dll, bgc);
	csr8w(ctlr, Lcr, 0);

	uart->baud = baud;

	return 0;
}

static void
i8250break(Uart* uart, int ms)
{
	Ctlr *ctlr;

	/*
	 * Send a break.
	 */
	if(ms <= 0)
		ms = 200;

	ctlr = uart->regs;
	csr8w(ctlr, Lcr, Brk);
	if (up == nil)
		delay(ms);
	else
		tsleep(&up->sleep, return0, 0, ms);
	csr8w(ctlr, Lcr, 0);
}

static void
i8250kick(Uart* uart)
{
	int i;
	Ctlr *ctlr;

	if(uart->cts == 0 || uart->blocked)
		return;

	/*
	 *  128 here is an arbitrary limit to make sure
	 *  we don't stay in this loop too long.  If the
	 *  chip's output queue is longer than 128, too
	 *  bad -- presotto
	 */
	ctlr = uart->regs;
	for(i = 0; i < 128; i++){
		if(!(csr8r(ctlr, Lsr) & Thre))
			break;
		if(uart->op >= uart->oe && uartstageoutput(uart) == 0)
			break;
		csr8o(ctlr, Thr, *(uart->op++));
	}
}

static Intrsvcret
i8250interrupt(Ureg*, void* arg)
{
	Ctlr *ctlr;
	Uart *uart;
	int iir, lsr, old, r;

	uart = arg;
	if (uart == nil)
		panic("i8250interrupt: nil uart");
	ctlr = uart->regs;
	if (ctlr == nil)
		panic("i8250interrupt: nil ctlr");
	for(iir = csr8r(ctlr, Iir); !(iir & Ip); iir = csr8r(ctlr, Iir)){
		switch(iir & IirMASK){
		case Ims:		/* Ms interrupt */
			r = csr8r(ctlr, Msr);
			if(r & Dcts){
				ilock(&uart->tlock);
				old = uart->cts;
				uart->cts = r & Cts;
				if(old == 0 && uart->cts)
					uart->ctsbackoff = 2;
				iunlock(&uart->tlock);
			}
		 	if(r & Ddsr){
				old = r & Dsr;
				if(uart->hup_dsr && uart->dsr && !old)
					uart->dohup = 1;
				uart->dsr = old;
			}
		 	if(r & Ddcd){
				old = r & Dcd;
				if(uart->hup_dcd && uart->dcd && !old)
					uart->dohup = 1;
				uart->dcd = old;
			}
			break;
		case Ithre:		/* Thr Empty */
			uartkick(uart);
			break;
		case Irda:		/* Received Data Available */
		case Irls:		/* Receiver Line Status */
		case Ictoi:		/* Character Time-out Indication */
			/*
			 * Consume any received data.
			 * If the received byte came in with a break,
			 * parity or framing error, throw it away;
			 * overrun is an indication that something has
			 * already been tossed.
			 */
			while((lsr = csr8r(ctlr, Lsr)) & Dr){
				if(lsr & (FIFOerr|Oe))
					uart->oerr++;
				if(lsr & Pe)
					uart->perr++;
				if(lsr & Fe)
					uart->ferr++;
				r = csr8r(ctlr, Rbr);
				if(!(lsr & (Bi|Fe|Pe)))
					uartrecv(uart, r);
			}
			break;

		default:
			iprint("weird uart interrupt %#2.2ux\n", iir);
			break;
		}
	}
}

static void
i8250disable(Uart* uart)
{
	Ctlr *ctlr;

	/*
	 * Turn off DTR and RTS, disable interrupts and fifos.
	 */
	(*uart->phys->dtr)(uart, 0);
	(*uart->phys->rts)(uart, 0);
	(*uart->phys->fifo)(uart, 0);

	ctlr = uart->regs;
	ctlr->sticky[Ier] = 0;
	csr8w(ctlr, Ier, ctlr->sticky[Ier]);

	if(ctlr->iena != 0){
		if(intrdisable(ctlr->vector) == 0)
			ctlr->iena = 0;
	}
}

static void
i8250enable(Uart* uart, int ie)
{
	Ctlr *ctlr;

	ctlr = uart->regs;

	/*
	 * Check if there is a FIFO.
	 * Changing the FIFOena bit in Fcr flushes data
	 * from both receive and transmit FIFOs; there's
	 * no easy way to guarantee not losing data on
	 * the receive side, but it's possible to wait until
	 * the transmitter is really empty.
	 * Also, reading the Iir outwith i8250interrupt()
	 * can be dangerous, but this should only happen
	 * once, before interrupts are enabled.
	 */
	if(!ctlr->checkfifo)
		while(!(csr8r(ctlr, Lsr) & Temt))
			pause();
	ilock(ctlr);
	if(!ctlr->checkfifo){
		/*
		 * Wait until the transmitter is really empty.
		 */
		while(!(csr8r(ctlr, Lsr) & Temt))
			pause();
		csr8w(ctlr, Fcr, FIFOena);
		if(csr8r(ctlr, Iir) & Ifena)
			ctlr->hasfifo = 1;
		csr8w(ctlr, Fcr, 0);
		ctlr->checkfifo = 1;
	}
	iunlock(ctlr);

	/*
	 * Enable interrupts and turn on DTR and RTS.
	 * Be careful if this is called to set up a polled serial line
	 * early on not to try to enable interrupts as interrupt-
	 * -enabling mechanisms might not be set up yet.
	 */
	ctlr->sticky[Ier] = 0;
	ctlr->sticky[Mcr] &= ~Ie;
	if(ie){
		if(ctlr->iena == 0 && !ctlr->poll){
			ctlr->irq = ctlr->defirq;
			ctlr->tbdf = BUSUNKNOWN;
			if (enableintr(ctlr, i8250interrupt, uart, uart->name)
			    == 0)
				ctlr->iena = 1;
			else {			/* probably no such device */
				ctlr->iena = ie = 0;
				ctlr->poll = 1;
				goto nointr;
			}
		}
		ctlr->sticky[Ier] = Ethre|Erda;
		ctlr->sticky[Mcr] |= Ie;
	}
nointr:
	csr8w(ctlr, Ier, ctlr->sticky[Ier]);
	csr8w(ctlr, Mcr, ctlr->sticky[Mcr]);

	(*uart->phys->dtr)(uart, 1);
	(*uart->phys->rts)(uart, 1);

	/*
	 * During startup, the i8259 interrupt controller is reset.
	 * This may result in a lost interrupt from the i8250 uart.
	 * The i8250 thinks the interrupt is still outstanding and does not
	 * generate any further interrupts. The workaround is to call the
	 * interrupt handler to clear any pending interrupt events.
	 * Note: this must be done after setting Ier.
	 */
	if(ie)
		i8250interrupt(nil, uart);
}

void*
i8250alloc(int io, int irq, int tbdf)
{
	Ctlr *ctlr;

	if((ctlr = malloc(sizeof(Ctlr))) != nil){
		ctlr->io = io;
		ctlr->irq = irq;
		ctlr->tbdf = tbdf;
	}

	return ctlr;
}

static Uart*
i8250pnp(void)
{
	int i, nuart, lsr;
	Ctlr *ctlr;
	Uart *head, *uart;

	nuart = 0;
	head = i8250uart;
	for(i = 0; i < nelem(i8250uart); i++){
		/*
		 * Does it exist?
		 * Should be able to write/read the Scratch Pad
		 * (except on COM1, where it seems to confuse the 8250)
		 * and reserve the I/O space.
		 */
		uart = &i8250uart[i];
		ctlr = uart->regs;
		if (0) {
			csr8o(ctlr, Scr, 0x55);
			if(csr8r(ctlr, Scr) == 0x55)
				continue;
		}
		lsr = csr8r(ctlr, Lsr);
		if ((lsr & (Thre|Temt)) != (Thre|Temt))
			continue;		/* not an idle 8250 */
		if(ioalloc(ctlr->io, 8, 0, uart->name) < 0) {
			print("pnp: %s: I/O ports already allocated\n", uart->name);
			continue;
		}
		nuart++;
		if(uart == head)
			head = uart;
		else
			(uart-1)->next = uart;
	}
	return nuart? head: nil;
}

static int
i8250getc(Uart* uart)
{
	Ctlr *ctlr;

	ctlr = uart->regs;
	while(!(csr8r(ctlr, Lsr) & Dr))
		delay(1);
	return csr8r(ctlr, Rbr);
}

static void
i8250drain(Ctlr *ctlr)
{
	int i;

	for(i = 1000; !(csr8r(ctlr, Lsr) & Thre) && i > 0; i--)
		delay(1);
}

static void
i8250putc(Uart* uart, int c)
{
	Ctlr *ctlr;

	ctlr = uart->regs;
	i8250drain(ctlr);
	csr8o(ctlr, Thr, c);
	i8250drain(ctlr);
}

void
_uartputs(char* s, int n)	/* debugging */
{
	char *e;
	int lastc;

	lastc = 0;
	for(e = s+n; s < e; s++){
		if(*s == '\n' && lastc != '\r')
			i8250putc(&i8250uart[0], '\r');
		i8250putc(&i8250uart[0], *s);
		lastc = *s;
	}
}

static void
i8250poll(Uart* uart)
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
	if(ctlr->iena || !ctlr->poll)
		return;
	i8250interrupt(nil, uart);
}

PhysUart i8250physuart = {
	.name		= "i8250",
	.pnp		= i8250pnp,
	.enable		= i8250enable,
	.disable	= i8250disable,
	.kick		= i8250kick,
	.dobreak	= i8250break,
	.baud		= i8250baud,
	.bits		= i8250bits,
	.stop		= i8250stop,
	.parity		= i8250parity,
	.modemctl	= i8250modemctl,
	.rts		= i8250rts,
	.dtr		= i8250dtr,
	.status		= i8250status,
	.fifo		= i8250fifo,
	.getc		= i8250getc,
	.putc		= i8250putc,
	.poll		= i8250poll,
};

Uart*
i8250console(char* cfg)
{
	int i;
	Uart *uart;
	Ctlr *ctlr;
	char *cmd, *p;
	ISAConf isa;

	/*
	 * Before i8250pnp() is run, we can only set the console
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
	switch(i){
	default:
		return nil;
	case 0:
	case 1:
		uart = &i8250uart[i];
		break;
	}

	memset(&isa, 0, sizeof(isa));
	ctlr = uart->regs;
	if(isaconfig("eia", i, &isa) != 0){
		if(isa.port != 0)
			ctlr->io = isa.port;
//		if(isa.irq != 0)			/* sorry */
//			ctlr->irq = isa.irq;
		if(isa.freq != 0)
			uart->freq = isa.freq;
	}

	/*
	 * Does it exist?
	 * Should be able to write/read the Scratch Pad (except COM1)
	 * but it seems to confuse the 8250.
	 */
	if (0 && i > 0) {
		csr8o(ctlr, Scr, 0x55);
		if(csr8r(ctlr, Scr) != 0x55)
			return nil;
	}
	if(ioalloc(ctlr->io, 8, 0, uart->name) < 0)
		print("console: %s: I/O ports already allocated\n", uart->name);

	(*uart->phys->enable)(uart, 0);
#ifdef CHANGE_SPEED		/* TODO */
	uartctl(uart, "b9600");
#else
	/* leave speed alone */
#endif
	uartctl(uart, "l8 pn s1 i1");
	if(*cmd != '\0')
		uartctl(uart, cmd);

	consuart = uart;
	uart->console = 1;
	return uart;
}
