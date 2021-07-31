/*
 * bcm2835 mini uart (UART1)
 */

#include "u.h"
#include "../port/lib.h"
#include "../port/error.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

#define GPIOREGS	(VIRTIO+0x200000)
#define AUXREGS		(VIRTIO+0x215000)
#define	OkLed		16
#define	TxPin		14
#define	RxPin		15

/* GPIO regs */
enum {
	Fsel0	= 0x00>>2,
		FuncMask= 0x7,
		Input	= 0x0,
		Output	= 0x1,
		Alt0	= 0x4,
		Alt1	= 0x5,
		Alt2	= 0x6,
		Alt3	= 0x7,
		Alt4	= 0x3,
		Alt5	= 0x2,
	Set0	= 0x1c>>2,
	Clr0	= 0x28>>2,
	Lev0	= 0x34>>2,
	PUD	= 0x94>>2,
		Off	= 0x0,
		Pulldown= 0x1,
		Pullup	= 0x2,
	PUDclk0	= 0x98>>2,
	PUDclk1	= 0x9c>>2,
};

/* AUX regs */
enum {
	Irq	= 0x00>>2,
		UartIrq	= 1<<0,
	Enables	= 0x04>>2,
		UartEn	= 1<<0,
	MuIo	= 0x40>>2,
	MuIer	= 0x44>>2,
		RxIen	= 1<<0,
		TxIen	= 1<<1,
	MuIir	= 0x48>>2,
	MuLcr	= 0x4c>>2,
		Bitsmask= 3<<0,
		Bits7	= 2<<0,
		Bits8	= 3<<0,
	MuMcr	= 0x50>>2,
		RtsN	= 1<<1,
	MuLsr	= 0x54>>2,
		TxDone	= 1<<6,
		TxRdy	= 1<<5,
		RxRdy	= 1<<0,
	MuCntl	= 0x60>>2,
		CtsFlow	= 1<<3,
		TxEn	= 1<<1,
		RxEn	= 1<<0,
	MuBaud	= 0x68>>2,
};

extern PhysUart miniphysuart;

static Uart miniuart = {
	.regs	= (u32int*)AUXREGS,
	.name	= "uart0",
	.freq	= 250000000,
	.phys	= &miniphysuart,
};

void
gpiosel(uint pin, int func)
{	
	u32int *gp, *fsel;
	int off;

	gp = (u32int*)GPIOREGS;
	fsel = &gp[Fsel0 + pin/10];
	off = (pin % 10) * 3;
	*fsel = (*fsel & ~(FuncMask<<off)) | func<<off;
}

void
gpiopulloff(uint pin)
{
	u32int *gp, *reg;
	u32int mask;

	gp = (u32int*)GPIOREGS;
	reg = &gp[PUDclk0 + pin/32];
	mask = 1 << (pin % 32);
	gp[PUD] = Off;
	microdelay(1);
	*reg = mask;
	microdelay(1);
	*reg = 0;
}

void
gpioout(uint pin, int set)
{
	u32int *gp;
	int v;

	gp = (u32int*)GPIOREGS;
	v = set? Set0 : Clr0;
	gp[v + pin/32] = 1 << (pin % 32);
}

int
gpioin(uint pin)
{
	u32int *gp;

	gp = (u32int*)GPIOREGS;
	return (gp[Lev0 + pin/32] & (1 << (pin % 32))) != 0;
}

static void
interrupt(Ureg*, void *arg)
{
	Uart *uart;
	u32int *ap;

	uart = arg;
	ap = (u32int*)uart->regs;

	coherence();
	if(0 && (ap[Irq] & UartIrq) == 0)
		return;
	if(ap[MuLsr] & TxRdy)
		uartkick(uart);
	if(ap[MuLsr] & RxRdy){
		if(uart->console){
			if(uart->opens == 1)
				uart->putc = kbdcr2nl;
			else
				uart->putc = nil;
		}
		do{
			uartrecv(uart, ap[MuIo] & 0xFF);
		}while(ap[MuLsr] & RxRdy);
	}
	coherence();
}

static Uart*
pnp(void)
{
	Uart *uart;

	uart = &miniuart;
	if(uart->console == 0)
		kbdq = qopen(8*1024, 0, nil, nil);
	return uart;
}

static void
enable(Uart *uart, int ie)
{
	u32int *ap;

	ap = (u32int*)uart->regs;
	delay(10);
	gpiosel(TxPin, Alt5);
	gpiosel(RxPin, Alt5);
	gpiopulloff(TxPin);
	gpiopulloff(RxPin);
	ap[Enables] |= UartEn;
	ap[MuIir] = 6;
	ap[MuLcr] = Bits8;
	ap[MuCntl] = TxEn|RxEn;
	ap[MuBaud] = 250000000/(115200*8) - 1;
	if(ie){
		intrenable(IRQaux, interrupt, uart, 0, "uart");
		ap[MuIer] = RxIen|TxIen;
	}else
		ap[MuIer] = 0;
}

static void
disable(Uart *uart)
{
	u32int *ap;

	ap = (u32int*)uart->regs;
	ap[MuCntl] = 0;
	ap[MuIer] = 0;
}

static void
kick(Uart *uart)
{
	u32int *ap;

	ap = (u32int*)uart->regs;
	if(uart->blocked)
		return;
	coherence();
	while(ap[MuLsr] & TxRdy){
		if(uart->op >= uart->oe && uartstageoutput(uart) == 0)
			break;
		ap[MuIo] = *(uart->op++);
	}
	if(ap[MuLsr] & TxDone)
		ap[MuIer] &= ~TxIen;
	else
		ap[MuIer] |= TxIen;
	coherence();
}

/* TODO */
static void
dobreak(Uart *uart, int ms)
{
	USED(uart, ms);
}

static int
baud(Uart *uart, int n)
{
	u32int *ap;

	ap = (u32int*)uart->regs;
	if(uart->freq == 0 || n <= 0)
		return -1;
	ap[MuBaud] = (uart->freq + 4*n - 1) / (8 * n) - 1;
	uart->baud = n;
	return 0;
}

static int
bits(Uart *uart, int n)
{
	u32int *ap;
	int set;

	ap = (u32int*)uart->regs;
	switch(n){
	case 7:
		set = Bits7;
		break;
	case 8:
		set = Bits8;
		break;
	default:
		return -1;
	}
	ap[MuLcr] = (ap[MuLcr] & ~Bitsmask) | set;
	uart->bits = n;
	return 0;
}

static int
stop(Uart *uart, int n)
{
	if(n != 1)
		return -1;
	uart->stop = n;
	return 0;
}

static int
parity(Uart *uart, int n)
{
	if(n != 'n')
		return -1;
	uart->parity = n;
	return 0;
}

/*
 * cts/rts flow control
 *   need to bring signals to gpio pins before enabling this
 */

static void
modemctl(Uart *uart, int on)
{
	u32int *ap;

	ap = (u32int*)uart->regs;
	if(on)
		ap[MuCntl] |= CtsFlow;
	else
		ap[MuCntl] &= ~CtsFlow;
	uart->modem = on;
}

static void
rts(Uart *uart, int on)
{
	u32int *ap;

	ap = (u32int*)uart->regs;
	if(on)
		ap[MuMcr] &= ~RtsN;
	else
		ap[MuMcr] |= RtsN;
}

static long
status(Uart *uart, void *buf, long n, long offset)
{
	char *p;

	p = malloc(READSTR);
	if(p == nil)
		error(Enomem);
	snprint(p, READSTR,
		"b%d\n"
		"dev(%d) type(%d) framing(%d) overruns(%d) "
		"berr(%d) serr(%d)\n",

		uart->baud,
		uart->dev,
		uart->type,
		uart->ferr,
		uart->oerr,
		uart->berr,
		uart->serr
	);
	n = readstr(offset, buf, n, p);
	free(p);

	return n;
}

static void
donothing(Uart*, int)
{
}

void
putc(Uart*, int c)
{
	u32int *ap;

	ap = (u32int*)AUXREGS;
	while((ap[MuLsr] & TxRdy) == 0)
		;
	ap[MuIo] = c;
	while((ap[MuLsr] & TxRdy) == 0)
		;
}

int
getc(Uart*)
{
	u32int *ap;

	ap = (u32int*)AUXREGS;
	while((ap[MuLsr] & RxRdy) == 0)
		;
	return ap[MuIo] & 0xFF;
}

void
uartconsinit(void)
{
	Uart *uart;
	int n;
	char *p, *cmd;

	if((p = getconf("console")) == nil)
		return;
	n = strtoul(p, &cmd, 0);
	if(p == cmd)
		return;
	switch(n){
	default:
		return;
	case 0:
		uart = &miniuart;
		break;
	}

	if(!uart->enabled)
		(*uart->phys->enable)(uart, 0);
	uartctl(uart, "b9600 l8 pn s1");
	if(*cmd != '\0')
		uartctl(uart, cmd);

	consuart = uart;
	uart->console = 1;
}

PhysUart miniphysuart = {
	.name		= "miniuart",
	.pnp		= pnp,
	.enable		= enable,
	.disable	= disable,
	.kick		= kick,
	.dobreak	= dobreak,
	.baud		= baud,
	.bits		= bits,
	.stop		= stop,
	.parity		= parity,
	.modemctl	= donothing,
	.rts		= rts,
	.dtr		= donothing,
	.status		= status,
	.fifo		= donothing,
	.getc		= getc,
	.putc		= putc,
};

void
okay(int on)
{
	static int first;

	if(!first++)
		gpiosel(OkLed, Output);
	gpioout(OkLed, !on);
}
