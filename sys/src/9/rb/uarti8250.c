/*
 * 8250-like UART
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

enum {
	CONSOLE		= 0,		/* first uart */
	Pollstuckoutput	= 1,
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
//	Mdr		= 8,		/* Mode Def'n (omap rw) */
	Usr		= 31,		/* Uart Status Register */
	Stickyend,

	Dll		= 0,		/* Divisor Latch LSB */
	Dlm		= 1,		/* Divisor Latch MSB */
};

enum {					/* Usr */
	Busy		= 0x01,
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
//	FIFOdma		= 0x08,
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
	Ie		= 0x08,		/* IRQ Enable (cd_sts_ch on omap) */
	Dm		= 0x10,		/* Diagnostic Mode loopback */
};

enum {					/* Lsr */
	Dr		= 0x01,		/* Data Ready */
	Oe		= 0x02,		/* Overrun Error */
	Pe		= 0x04,		/* Parity Error */
	Fe		= 0x08,		/* Framing Error */
	Bi		= 0x10,		/* Break Interrupt */
	Thre		= 0x20,		/* Thr Empty */
	Temt		= 0x40,		/* Transmitter Empty */
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
	Dcd		= 0x80,		/* Carrier Detect */
};

enum {					/* Mdr */
	Modemask	= 7,
	Modeuart	= 0,
};


typedef struct Ctlr {
	u32int*	io;
	int	irq;
	int	tbdf;
	int	iena;
	int	poll;

	uchar	sticky[Stickyend];

	Lock;
	int	hasfifo;
	int	checkfifo;
	int	fena;
} Ctlr;

extern PhysUart i8250physuart;

static Ctlr i8250ctlr[] = {
{	.io	= (u32int*)PHYSCONS,
	.irq	= ILduart0,
	.tbdf	= -1,
	.poll	= 0, },
};

static Uart i8250uart[] = {
{	.regs	= &i8250ctlr[0],
	.name	= "cons",
	.freq	= 3686000,	/* Not used, we use the global i8250freq */
	.phys	= &i8250physuart,
	.console= 1,
	.next	= nil, },
};

#define csr8r(c, r)	((c)->io[r])
#define csr8w(c, r, v)	((c)->io[r] = (uchar)((c)->sticky[r] | (v)))
#define csr8o(c, r, v)	((c)->io[r] = (uchar)(v))

static long
i8250status(Uart* uart, void* buf, long n, long offset)
{
	char *p;
	Ctlr *ctlr;
	uchar ier, lcr, mcr, msr;

	ctlr = uart->regs;
	p = smalloc(READSTR);
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
	ilock(ctlr);
	while(!(csr8r(ctlr, Lsr) & Temt))
		;

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
#ifdef notdef				/* don't change the speed */
	ulong bgc;
	Ctlr *ctlr;
	extern int i8250freq;	/* In the config file */

	/*
	 * Set the Baud rate by calculating and setting the Baud rate
	 * Generator Constant. This will work with fairly non-standard
	 * Baud rates.
	 */
	if(i8250freq == 0 || baud <= 0)
		return -1;
	bgc = (i8250freq+8*baud-1)/(16*baud);

	ctlr = uart->regs;
	while(csr8r(ctlr, Usr) & Busy)
		delay(1);
	csr8w(ctlr, Lcr, Dlab);		/* begin kludge */
	csr8o(ctlr, Dlm, bgc>>8);
	csr8o(ctlr, Dll, bgc);
	csr8w(ctlr, Lcr, 0);
#endif
	uart->baud = baud;
	return 0;
}

static void
i8250break(Uart* uart, int ms)
{
	Ctlr *ctlr;

	if (up == nil)
		panic("i8250break: nil up");
	/*
	 * Send a break.
	 */
	if(ms <= 0)
		ms = 200;

	ctlr = uart->regs;
	csr8w(ctlr, Lcr, Brk);
	tsleep(&up->sleep, return0, 0, ms);
	csr8w(ctlr, Lcr, 0);
}

static void
emptyoutstage(Uart *uart, int n)
{
	_uartputs((char *)uart->op, n);
	uart->op = uart->oe = uart->ostage;
}

static void
i8250kick(Uart* uart)
{
	int i;
	Ctlr *ctlr;

	if(/* uart->cts == 0 || */ uart->blocked)
		return;

	if(!normalprint) {			/* early */
		if (uart->op < uart->oe)
			emptyoutstage(uart, uart->oe - uart->op);
		while ((i = uartstageoutput(uart)) > 0)
			emptyoutstage(uart, i);
		return;
	}

	/* nothing more to send? then disable xmit intr */
	ctlr = uart->regs;
	if (uart->op >= uart->oe && qlen(uart->oq) == 0 &&
	    (1 || csr8r(ctlr, Lsr) & Temt)) {	/* could try ignoring Temt */
		ctlr->sticky[Ier] &= ~Ethre;
		csr8w(ctlr, Ier, 0);
		return;
	}

	/*
	 *  128 here is an arbitrary limit to make sure
	 *  we don't stay in this loop too long.  If the
	 *  chip's output queue is longer than 128, too
	 *  bad -- presotto
	 */
	for(i = 0; i < 128; i++){
		if(!(csr8r(ctlr, Lsr) & Thre))
			break;
		if(uart->op >= uart->oe && uartstageoutput(uart) == 0)
			break;
		csr8o(ctlr, Thr, *uart->op++);		/* start tx */
		ctlr->sticky[Ier] |= Ethre;
		csr8w(ctlr, Ier, 0);			/* intr when done */
	}
}

void
serialkick(void)
{
	uartkick(&i8250uart[CONSOLE]);
}

static Lock i8250intrlock;

static void
i8250interrupt(void* arg)
{
	Ctlr *ctlr;
	Uart *uart;
	int iir, lsr, old, r;

	uart = arg;
	ctlr = uart->regs;
	ilock(&i8250intrlock);

	/* force Ethre on.  don't know why this is needed, but it is. */
	ctlr->sticky[Ier] |= Ethre;
	csr8w(ctlr, Ier, 0);
	/* this is probably optional.  maybe it helps fast input. */
	ctlr->sticky[Mcr] |= Ie;
	csr8w(ctlr, Mcr, 0);

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
			iprint("weird uart interrupt type %#2.2uX\n", iir);
			break;
		}
	}
	iunlock(&i8250intrlock);
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

	if(ctlr->iena != 0){
		/* bad idea if the IRQ is shared */
//		introff(1 << (ILshift + ctlr->irq));
		ctlr->iena = 0;
	}
}

static void
i8250clock(void)
{
	i8250interrupt(&i8250uart[CONSOLE]);
}

static void
i8250enable(Uart* uart, int ie)
{
	Ctlr *ctlr;

	ctlr = uart->regs;
	ctlr->sticky[Lcr] = Wls8;		/* no parity */
	csr8w(ctlr, Lcr, 0);

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
	ilock(ctlr);
	if(!ctlr->checkfifo){
		/*
		 * Wait until the transmitter is really empty.
		 */
		while(!(csr8r(ctlr, Lsr) & Temt))
			;
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
	if(ie){
		if(ctlr->iena == 0 && !ctlr->poll){
			intrenable(ctlr->irq, i8250interrupt, uart);
			ctlr->iena = 1;
		}
		ctlr->sticky[Ier] = Erda;
		ctlr->sticky[Mcr] |= Ie;
	}
	else{
		ctlr->sticky[Ier] = 0;
		ctlr->sticky[Mcr] = 0;
	}
	csr8w(ctlr, Ier, 0);
	csr8w(ctlr, Mcr, 0);

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
	if(ie) {
		i8250interrupt(uart);
		/*
		 * force output to resume if stuck.  shouldn't be needed.
		 */
		if (Pollstuckoutput)
			addclock0link(i8250clock, 10);
	}
}

static Uart*
i8250pnp(void)
{
	return i8250uart;
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
i8250putc(Uart* uart, int c)
{
	int i, s;
	Ctlr *ctlr;

	if (!normalprint) {		/* too early; use brute force */
		s = splhi();
		while (!(((ulong *)PHYSCONS)[Lsr] & Thre))
			;
		((ulong *)PHYSCONS)[Thr] = (uchar)c;
		splx(s);
		return;
	}

	ctlr = uart->regs;
	s = splhi();
	for(i = 0; !(csr8r(ctlr, Lsr) & Thre) && i < 200; i++)
		delay(5);
	csr8o(ctlr, Thr, c);
	for(i = 0; !(csr8r(ctlr, Lsr) & Thre) && i < 200; i++)
		delay(5);
	splx(s);
}

void
serialputc(int c)
{
	i8250putc(&i8250uart[CONSOLE], c);
}

void
serialputs(char* s, int n)
{
	_uartputs(s, n);
}

#ifdef PLAN9K
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
	i8250interrupt(uart);
}
#endif

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
#ifdef PLAN9K
	.poll		= i8250poll,
#endif
};

static void
i8250dumpregs(Ctlr* ctlr)
{
	int dlm, dll;
	int _uartprint(char*, ...);

	csr8w(ctlr, Lcr, Dlab);
	dlm = csr8r(ctlr, Dlm);
	dll = csr8r(ctlr, Dll);
	csr8w(ctlr, Lcr, 0);

	_uartprint("dlm %#ux dll %#ux\n", dlm, dll);
}

Uart*	uartenable(Uart *p);

/* must call this from a process's context */
int
i8250console(void)
{
	Uart *uart;

	if (up == nil)
		return -1;			/* too early */

	uart = &i8250uart[CONSOLE];
	if(uartenable(uart) != nil && uart->console){
		kbdq = uart->iq;
		assert(kbdq);
		serialoq = uart->oq;
		assert(serialoq);
		uart->putc = kbdcr2nl;
		uart->opens++;
		consuart = uart;
		/* up wasn't set when chandevreset ran, so enable it now */
		i8250disable(uart);
		i8250enable(uart, 1);
	}
	uartctl(uart, "b115200 l8 pn m0 s1 i128 w100");
	return 0;
}

void
_uartputs(char* s, int n)
{
	char *e;

	for(e = s+n; s < e; s++){
		if(*s == '\n')
			i8250putc(&i8250uart[CONSOLE], '\r');
		i8250putc(&i8250uart[CONSOLE], *s);
	}
}

int
_uartprint(char* fmt, ...)
{
	int n;
	va_list arg;
	char buf[PRINTSIZE];

	va_start(arg, fmt);
	n = vseprint(buf, buf+sizeof(buf), fmt, arg) - buf;
	va_end(arg);
	_uartputs(buf, n);

	return n;
}

void (*lprint)(char *, int) = _uartputs;
