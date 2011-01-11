/*
 * marvell kirkwood uart (supposed to be a 16550)
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/error.h"
// #include "../port/uart.h"

enum {
	UartFREQ =	0, // xxx

	IERrx		= 1<<0,
	IERtx		= 1<<1,

	IRRintrmask	= (1<<4)-1,
	IRRnointr	= 1,
	IRRthrempty	= 2,
	IRRrxdata	= 4,
	IRRrxstatus	= 6,
	IRRtimeout	= 12,

	IRRfifomask	= 3<<6,
	IRRfifoenable	= 3<<6,

	FCRenable	= 1<<0,
	FCRrxreset	= 1<<1,
	FCRtxreset	= 1<<2,
	/* reserved */
	FCRrxtriggermask	= 3<<6,
	FCRrxtrigger1	= 0<<6,
	FCRrxtrigger4	= 1<<6,
	FCRrxtrigger8	= 2<<6,
	FCRrxtrigger14	= 3<<6,

	LCRbpcmask	= 3<<0,
	LCRbpc5		= 0<<0,
	LCRbpc6		= 1<<0,
	LCRbpc7		= 2<<0,
	LCRbpc8		= 3<<0,
	LCRstop2b	= 1<<2,
	LCRparity	= 1<<3,
	LCRparityeven	= 1<<4,
	LCRbreak	= 1<<6,
	LCRdivlatch	= 1<<7,

	LSRrx		= 1<<0,
	LSRrunerr	= 1<<1,
	LSRparerr	= 1<<2,
	LSRframeerr	= 1<<3,
	LSRbi		= 1<<4,
	LSRthre		= 1<<5,
	LSRtxempty	= 1<<6,
	LSRfifoerr	= 1<<7,
};

extern PhysUart kwphysuart;

typedef struct UartReg UartReg;
struct UartReg
{
	union {
		ulong	thr;
		ulong	dll;
		ulong	rbr;
	};
	union {
		ulong	ier;
		ulong	dlh;
	};
	union {
		ulong	iir;
		ulong	fcr;
	};
	ulong	lcr;
	ulong	mcr;
	ulong	lsr;
	ulong	scr;
};

typedef struct Ctlr Ctlr;
struct Ctlr {
	UartReg*regs;
	int	irq;
	Lock;
};

static Ctlr kirkwoodctlr[] = {
{
	.regs   = nil,			/* filled in below */
	.irq    = IRQ1uart0, },
};

static Uart kirkwooduart[] = {
{
	.regs	= &kirkwoodctlr[0],
	.name	= "eia0",
	.freq	= UartFREQ,
	.phys	= &kwphysuart,
	.special= 0,
	.console= 1,
	.next	= nil, },
};

static void
kw_read(Uart *uart)
{
	Ctlr *ctlr = uart->regs;
	UartReg *regs = ctlr->regs;
	ulong lsr;
	char c;

	while ((lsr = regs->lsr) & LSRrx) {
		if(lsr&LSRrunerr)
			uart->oerr++;
		if(lsr&LSRparerr)
			uart->perr++;
		if(lsr&LSRframeerr)
			uart->ferr++;
		c = regs->rbr;
		if((lsr & (LSRbi|LSRframeerr|LSRparerr)) == 0)
			uartrecv(uart, c);
	}
}

static void
kw_intr(Ureg*, void *arg)
{
	Uart *uart = arg;
	Ctlr *ctlr = uart->regs;
	UartReg *regs = ctlr->regs;
	ulong v;

	if(regs == 0) {
		kirkwoodctlr[0].regs = (UartReg *)soc.uart[0];
		regs = (UartReg *)soc.uart[0];		/* caution */
		coherence();
	}
	v = regs->iir;
	if(v & IRRthrempty)
		uartkick(uart);
	if(v & IRRrxdata)
		kw_read(uart);

	intrclear(Irqhi, ctlr->irq);
}

static Uart*
kw_pnp(void)
{
	kirkwoodctlr[0].regs = (UartReg *)soc.uart[0];
	coherence();
	return kirkwooduart;
}

static void
kw_enable(Uart* uart, int ie)
{
	Ctlr *ctlr = uart->regs;
	UartReg *regs = ctlr->regs;

	if(regs == 0) {
		kirkwoodctlr[0].regs = (UartReg *)soc.uart[0];
		regs = (UartReg *)soc.uart[0];		/* caution */
		coherence();
	}
	USED(ie);
	regs->fcr = FCRenable|FCRrxtrigger4;
	regs->ier = IERrx|IERtx;
	intrenable(Irqhi, ctlr->irq, kw_intr, uart, uart->name);

	(*uart->phys->dtr)(uart, 1);
	(*uart->phys->rts)(uart, 1);
}

static void
kw_disable(Uart* uart)
{
	Ctlr *ctlr = uart->regs;

	(*uart->phys->dtr)(uart, 0);
	(*uart->phys->rts)(uart, 0);
	(*uart->phys->fifo)(uart, 0);

	intrdisable(Irqhi, ctlr->irq, kw_intr, uart, uart->name);
}

static void
kw_kick(Uart* uart)
{
	Ctlr *ctlr = uart->regs;
	UartReg *regs = ctlr->regs;
	int i;

	if(uart->cts == 0 || uart->blocked)
		return;

	for(i = 0; i < 16; i++) {
		if((regs->lsr & LSRthre) == 0 ||
		    uart->op >= uart->oe && uartstageoutput(uart) == 0)
			break;
		regs->thr = *uart->op++;
	}
}

static void
kw_break(Uart* uart, int ms)
{
	USED(uart, ms);
}

static int
kw_baud(Uart* uart, int baud)
{
	USED(uart, baud);
	return 0;
}

static int
kw_bits(Uart* uart, int bits)
{
	USED(uart, bits);
	return 0;
}

static int
kw_stop(Uart* uart, int stop)
{
	USED(uart, stop);
	return 0;
}

static int
kw_parity(Uart* uart, int parity)
{
	USED(uart, parity);
	return 0;
}

static void
kw_modemctl(Uart* uart, int on)
{
	USED(uart, on);
}

static void
kw_rts(Uart* uart, int on)
{
	USED(uart, on);
}

static void
kw_dtr(Uart* uart, int on)
{
	USED(uart, on);
}

static long
kw_status(Uart* uart, void* buf, long n, long offset)
{
	USED(uart, buf, n, offset);
	return 0;
}

static void
kw_fifo(Uart* uart, int level)
{
	USED(uart, level);
}

static int
kw_getc(Uart *uart)
{
	Ctlr *ctlr = uart->regs;
	UartReg *regs = ctlr->regs;

	while((regs->lsr&LSRrx) == 0)
		;
	return regs->rbr;
}

static void
kw_putc(Uart *uart, int c)
{
	Ctlr *ctlr = uart->regs;
	UartReg *regs = ctlr->regs;

	/* can be called from iprint, among many other places */
	if(regs == 0) {
		kirkwoodctlr[0].regs = (UartReg *)soc.uart[0];
		regs = (UartReg *)soc.uart[0];		/* caution */
		coherence();
	}
	while((regs->lsr&LSRthre) == 0)
		;
	regs->thr = c;
	coherence();
}

PhysUart kwphysuart = {
	.name		= "kirkwood",
	.pnp		= kw_pnp,
	.enable		= kw_enable,
	.disable	= kw_disable,
	.kick		= kw_kick,
	.dobreak	= kw_break,
	.baud		= kw_baud,
	.bits		= kw_bits,
	.stop		= kw_stop,
	.parity		= kw_parity,
	.modemctl	= kw_modemctl,
	.rts		= kw_rts,
	.dtr		= kw_dtr,
	.status		= kw_status,
	.fifo		= kw_fifo,
	.getc		= kw_getc,
	.putc		= kw_putc,
};

void
uartkirkwoodconsole(void)
{
	Uart *uart;

	uart = &kirkwooduart[0];
	(*uart->phys->enable)(uart, 0);
	uartctl(uart, "b115200 l8 pn s1 i1");
	uart->console = 1;
	consuart = uart;
//serialputs("uart0 kirkwood\n", strlen("uart0 kirkwood\n"));
}

void
serialputc(int c)
{
	int cnt, s;
	UartReg *regs = (UartReg *)soc.uart[0];

	s = splhi();
	cnt = m->cpuhz;
	if (cnt <= 0)			/* cpuhz not set yet? */
		cnt = 1000000;
	while((regs->lsr & LSRthre) == 0 && --cnt > 0)
		;
	regs->thr = c;
	coherence();
	delay(1);
	splx(s);
}

void
serialputs(char *p, int len)
{
	while(--len >= 0) {
		if(*p == '\n')
			serialputc('\r');
		serialputc(*p++);
	}
}
