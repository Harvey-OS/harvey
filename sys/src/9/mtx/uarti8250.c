#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/error.h"

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

enum {					/* I/O ports */
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
	Ife		= 0xC0,		/* FIFOs enabled */
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
	int	io;
	int	irq;
	int	tbdf;
	int	iena;

	uchar	sticky[8];

	Lock;
	int	fifo;
	int	fena;
} Ctlr;

extern PhysUart i8250physuart;

static Ctlr i8250ctlr[2] = {
{	.io	= Uart0,
	.irq	= Uart0IRQ,
	.tbdf	= BUSUNKNOWN, },

{	.io	= Uart1,
	.irq	= Uart1IRQ,
	.tbdf	= BUSUNKNOWN, },
};

static Uart i8250uart[2] = {
{	.regs	= &i8250ctlr[0],
	.name	= "COM1",
	.freq	= UartFREQ,
	.phys	= &i8250physuart,
	.special=0,
	.next	= &i8250uart[1], },

{	.regs	= &i8250ctlr[1],
	.name	= "COM2",
	.freq	= UartFREQ,
	.phys	= &i8250physuart,
	.special=0,
	.next	= nil, },
};

#define csr8r(c, r)	inb((c)->io+(r))
#define csr8w(c, r, v)	outb((c)->io+(r), (c)->sticky[(r)]|(v))

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
		"dev(%d) type(%d) framing(%d) overruns(%d)%s%s%s%s\n",

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
i8250fifo(Uart* uart, int on)
{
	int i;
	Ctlr *ctlr;

	/*
	 * Toggle FIFOs:
	 * if none, do nothing;
	 * reset the Rx and Tx FIFOs;
	 * empty the Rx buffer and clear any interrupt conditions;
	 * if enabling, try to turn them on.
	 */
	ctlr = uart->regs;

	ilock(ctlr);
	if(!ctlr->fifo){
		csr8w(ctlr, Fcr, FIFOtclr|FIFOrclr);
		for(i = 0; i < 16; i++){
			csr8r(ctlr, Iir);
			csr8r(ctlr, Rbr);
		}
  
		ctlr->fena = 0;
		if(on){
			csr8w(ctlr, Fcr, FIFO4|FIFOena);
			if(!(csr8r(ctlr, Iir) & Ife))
				ctlr->fifo = 1;
			ctlr->fena = 1;
		}
	}
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
	ctlr = uart->regs;
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
		csr8w(ctlr, Ier, 0);
		uart->modem = 1;
		uart->cts = csr8r(ctlr, Msr) & Cts;
	}
	else{
		ctlr->sticky[Ier] &= ~Ems;
		csr8w(ctlr, Ier, 0);
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
	default:
		break;
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
	outb(ctlr->io+Dlm, bgc>>8);
	outb(ctlr->io+Dll, bgc);
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
	if(ms == 0)
		ms = 200;

	ctlr = uart->regs;
	csr8w(ctlr, Lcr, Brk);
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
		outb(ctlr->io+Thr, *(uart->op++));
	}
}

static void
i8250interrupt(Ureg*, void* arg)
{
	Ctlr *ctlr;
	Uart *uart;
	int iir, lsr, old, r;

	uart = arg;

	ctlr = uart->regs;
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
		case Ictoi:		/* Character Time-out Indication */
			/*
			 * Consume any received data.
			 * If the received byte came in with a break,
			 * parity or framing error, throw it away;
			 * overrun is an indication that something has
			 * already been tossed.
			 */
			while((lsr = csr8r(ctlr, Lsr)) & Dr){
				if(lsr & Oe)
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
			iprint("weird uart interrupt 0x%2.2uX\n", iir);
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
	csr8w(ctlr, Ier, 0);
}

static void
i8250enable(Uart* uart, int ie)
{
	Ctlr *ctlr;

	/*
 	 * Enable interrupts and turn on DTR and RTS.
	 * Be careful if this is called to set up a polled serial line
	 * early on not to try to enable interrupts as interrupt-
	 * -enabling mechanisms might not be set up yet.
	 */
	ctlr = uart->regs;
	if(ie){
		if(ctlr->iena == 0){
			intrenable(ctlr->irq, i8250interrupt, uart, ctlr->tbdf, uart->name);
			ctlr->iena = 1;
		}
		ctlr->sticky[Ier] = Ethre|Erda;
		ctlr->sticky[Mcr] |= Ie;
	}
	else{
		ctlr->sticky[Ier] = 0;
		ctlr->sticky[Mcr] = 0;
	}
	csr8w(ctlr, Ier, ctlr->sticky[Ier]);
	csr8w(ctlr, Mcr, ctlr->sticky[Mcr]);

	(*uart->phys->dtr)(uart, 1);
	(*uart->phys->rts)(uart, 1);
}

static Uart*
i8250pnp(void)
{
	return i8250uart;
}

static int
i8250getc(Uart *uart)
{
	Ctlr *ctlr;

	ctlr = uart->regs;
	while(!(csr8r(ctlr, Lsr)&Dr))
		delay(1);
	return csr8r(ctlr, Rbr);
}

static void
i8250putc(Uart *uart, int c)
{
	int i;
	Ctlr *ctlr;

	ctlr = uart->regs;
	for(i = 0; !(csr8r(ctlr, Lsr)&Thre) && i < 128; i++)
		delay(1);
	outb(ctlr->io+Thr, c);
	for(i = 0; !(csr8r(ctlr, Lsr)&Thre) && i < 128; i++)
		delay(1);
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
};

void
i8250console(void)
{
	Uart *uart;
	int n;
	char *cmd, *p;

	if((p = getconf("console")) == nil)
		return;
	n = strtoul(p, &cmd, 0);
	if(p == cmd)
		return;
	switch(n){
	default:
		return;
	case 0:
		uart = &i8250uart[0];
		break;
	case 1:
		uart = &i8250uart[1];
		break;	
	}

	uartctl(uart, "b9600 l8 pn s1");
	if(*cmd != '\0')
		uartctl(uart, cmd);
	(*uart->phys->enable)(uart, 0);

	consuart = uart;
	uart->console = 1;
} 
