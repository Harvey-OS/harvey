#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"

#include	"../port/netif.h"

/* this isn't strictly a sa1100 driver.  The rts/cts stuff is h3650 specific */

enum
{
	/* ctl[0] bits */
	Parity=		1<<0,
	Even=		1<<1,
	Stop2=		1<<2,
	Bits8=		1<<3,
	SCE=		1<<4,	/* synchronous clock enable */
	RCE=		1<<5,	/* rx on falling edge of clock */
	TCE=		1<<6,	/* tx on falling edge of clock */

	/* ctl[3] bits */
	Rena=		1<<0,	/* receiver enable */
	Tena=		1<<1,	/* transmitter enable */
	Break=		1<<2,	/* force TXD3 low */
	Rintena=	1<<3,	/* enable receive interrupt */
	Tintena=	1<<4,	/* enable transmitter interrupt */
	Loopback=	1<<5,	/* loop back data */

	/* data bits */
	DEparity=	1<<8,	/* parity error */
	DEframe=	1<<9,	/* framing error */
	DEoverrun=	1<<10,	/* overrun error */

	/* status[0] bits */
	Tint=		1<<0,	/* transmit fifo half full interrupt */
	Rint0=		1<<1,	/* receiver fifo 1/3-2/3 full */
	Rint1=		1<<2,	/* receiver fifo not empty and receiver idle */
	Breakstart=	1<<3,
	Breakend=	1<<4,
	Fifoerror=	1<<5,	/* fifo error */

	/* status[1] bits */
	Tbusy=		1<<0,	/* transmitting */
	Rnotempty=	1<<1,	/* receive fifo not empty */
	Tnotfull=	1<<2,	/* transmit fifo not full */
	ParityError=	1<<3,
	FrameError=	1<<4,
	Overrun=	1<<5,
};

Uartregs *uart3regs = UART3REGS;
Uartregs *uart1regs = UART1REGS ;

static void	sa1100_uartbaud(Uart *p, int rate);
static void	sa1100_uartkick(Uart *p);
static void	sa1100_uartrts(Uart *p, int on);
static void	sa1100_uartintr(Ureg*, void*);
static void	sa1100_uartbreak(Uart*, int);
static void	sa1100_uartbits(Uart*, int);
static void	sa1100_uartparity(Uart*, int);
static void	sa1100_uartmodemctl(Uart*, int);
static void	sa1100_uartstop(Uart*, int);
static void	sa1100_uartdtr(Uart*, int);
static long	sa1100_uartstatus(Uart*, void*, long, long);
static void	sa1100_uartenable(Uart*, int);
static void	sa1100_uartdisable(Uart*);

PhysUart sa1100_uart = {
	.enable=	sa1100_uartenable,
	.disable=	sa1100_uartdisable,
	.bits=		sa1100_uartbits,
	.kick=		sa1100_uartkick,
	.intr=		sa1100_uartintr,
	.modemctl=	sa1100_uartmodemctl,
	.baud=		sa1100_uartbaud,
	.stop=		sa1100_uartstop,
	.parity=	sa1100_uartparity,
	.dobreak=	sa1100_uartbreak,
	.rts=		sa1100_uartrts,
	.dtr=		sa1100_uartdtr,
	.status=	sa1100_uartstatus,
};

#define R(p) ((Uartregs*)(p->regs))

/*
 *  enable a port's interrupts.  set DTR and RTS
 */
static void
sa1100_uartenable(Uart *p, int intena)
{
	ulong s;

	s = R(p)->ctl[3] & ~(Rintena|Tintena|Rena|Tena);
	if(intena)
		R(p)->ctl[3] = s |Rintena|Tintena|Rena|Tena;
	else
		R(p)->ctl[3] = s | Rena|Tena;
}

/*
 *  disable interrupts. clear DTR, and RTS
 */
static void
sa1100_uartdisable(Uart *p)
{
	R(p)->ctl[3] &= ~(Rintena|Tintena|Rena|Tena);
}

static long
sa1100_uartstatus(Uart *p, void *buf, long n, long offset)
{
	char str[256];
	ulong ctl0;

	ctl0 = R(p)->ctl[0];
	snprint(str, sizeof(str),
		"b%d c%d d%d e%d l%d m%d p%c r%d s%d i%d\n"
		"dev(%d) type(%d) framing(%d) overruns(%d)%s%s%s%s\n",

		p->baud,
		p->hup_dcd, 
		0,
		p->hup_dsr,
		(ctl0 & Bits8) ? 8 : 7,
		0, 
		(ctl0 & Parity) ? ((ctl0 & Even) ? 'e' : 'o') : 'n',
		0,
		(ctl0 & Stop2) ? 2 : 1,
		1,

		p->dev,
		p->type,
		p->frame,
		p->overrun, 
		"",
		"",
		"",
		"" );
	return readstr(offset, buf, n, str);
}

/*
 *  set the buad rate
 */
static void
sa1100_uartbaud(Uart *p, int rate)
{
	ulong brconst;
	ulong ctl3;

	if(rate <= 0)
		return;

	/* disable */
	ctl3 = R(p)->ctl[3];
	R(p)->ctl[3] = 0;

	brconst = p->freq/(16*rate) - 1;
	R(p)->ctl[1] = (brconst>>8) & 0xf;
	R(p)->ctl[2] = brconst & 0xff;

	/* reenable */
	R(p)->ctl[3] = ctl3;
	p->baud = rate;
}

/*
 *  send a break
 */
static void
sa1100_uartbreak(Uart *p, int ms)
{
	if(ms == 0)
		ms = 200;

	R(p)->ctl[3] |= Break;
	tsleep(&up->sleep, return0, 0, ms);
	R(p)->ctl[3] &= ~Break;
}

/*
 *  set bits/char
 */
static void
sa1100_uartbits(Uart *p, int n)
{
	ulong ctl3;

	/* disable */
	ctl3 = R(p)->ctl[3];
	R(p)->ctl[3] = 0;

	switch(n){
	case 7:
		R(p)->ctl[0] &= ~Bits8;
		break;
	case 8:
		R(p)->ctl[0] |= Bits8;
		break;
	default:
		error(Ebadarg);
	}

	/* reenable */
	R(p)->ctl[3] = ctl3;
}

/*
 *  set stop bits
 */
static void
sa1100_uartstop(Uart *p, int n)
{
	ulong ctl3;

	/* disable */
	ctl3 = R(p)->ctl[3];
	R(p)->ctl[3] = 0;

	switch(n){
	case 1:
		R(p)->ctl[0] &= ~Stop2;
		break;
	case 2:
		R(p)->ctl[0] |= Stop2;
		break;
	default:
		error(Ebadarg);
	}

	/* reenable */
	R(p)->ctl[3] = ctl3;
}

/*
 *  turn on/off rts
 */
static void
sa1100_uartrts(Uart*, int)
{
}

/*
 *  turn on/off dtr
 */
static void
sa1100_uartdtr(Uart*, int)
{
}

/*
 *  turn on/off modem flow control on/off (rts/cts)
 */
static void
sa1100_uartmodemctl(Uart *p, int on)
{
	if(on) {
	} else {
		p->cts = 1;
	}
}

/*
 *  set parity
 */
static void
sa1100_uartparity(Uart *p, int type)
{
	ulong ctl3;

	/* disable */
	ctl3 = R(p)->ctl[3];
	R(p)->ctl[3] = 0;

	switch(type){
	case 'e':
		R(p)->ctl[0] |= Parity|Even;
		break;
	case 'o':
		R(p)->ctl[0] |= Parity;
		break;
	default:
		R(p)->ctl[0] &= ~(Parity|Even);
		break;
	}

	/* reenable */
	R(p)->ctl[3] = ctl3;
}

/*
 *  restart output if not blocked and OK to send
 */
static void
sa1100_uartkick(Uart *p)
{
	int i;

	R(p)->ctl[3] &= ~Tintena;

	if(p->cts == 0 || p->blocked)
		return;

	for(i = 0; i < 1024; i++){
		if(!(R(p)->status[1] & Tnotfull)){
			R(p)->ctl[3] |= Tintena;
			break;
		}
		if(p->op >= p->oe && uartstageoutput(p) == 0)
			break;
		R(p)->data = *p->op++;
	}
}

/*
 *  for iprint, just write it
 */
void
serialputs(char *str, int n)
{
	Uartregs *ur;

	ur = uart3regs;
	while(n-- > 0){
		/* wait for output ready */
		while((ur->status[1] & Tnotfull) == 0)
			;
		ur->data = *str++;
	}
	while((ur->status[1] & Tbusy))
		;
}

/*
 *  for iprint, just write it
 */
void
serialµcputs(uchar *str, int n)
{
	Uartregs *ur;

	ur = uart1regs;
	while(n-- > 0){
		/* wait for output ready */
		while((ur->status[1] & Tnotfull) == 0)
			;
		ur->data = *str++;
	}
	while((ur->status[1] & Tbusy))
		;
}

/*
 *  take an interrupt
 */
static void
sa1100_uartintr(Ureg*, void *x)
{
	Uart *p;
	ulong s;
	Uartregs *regs;

	p = x;
	regs = p->regs;

	/* receiver interrupt, snarf bytes */
	while(regs->status[1] & Rnotempty)
		uartrecv(p, regs->data);

	/* remember and reset interrupt causes */
	s = regs->status[0];
	regs->status[0] |= s;

	if(s & Tint){
		/* transmitter interrupt, restart */
		uartkick(p);
	}

	if(s & (ParityError|FrameError|Overrun)){
		if(s & ParityError)
			p->parity++;
		if(s & FrameError)
			p->frame++;
		if(s & Overrun)
			p->overrun++;
	}

	/* receiver interrupt, snarf bytes */
	while(regs->status[1] & Rnotempty)
		uartrecv(p, regs->data);
}

typedef struct Gpclkregs Gpclkregs;
struct Gpclkregs
{
	ulong	r0;
	ulong	r1;
	ulong	dummya;
	ulong	r2;
	ulong	r3;
};

enum
{
	/* gpclk register 0 */
	Gpclk_sus=	1<<0,	/* set uart mode */
};

Gpclkregs *gpclkregs;

/*
 *  setup all uarts (called early by main() to allow debugging output to
 *  a serial port)
 */
void
sa1100_uartsetup(int console)
{
	Uart *p;

	/* external serial port (eia0) */
	uart3regs = mapspecial(UART3REGS, 64);
	p = uartsetup(&sa1100_uart, uart3regs, ClockFreq, "serialport3");
	intrenable(IRQ, IRQuart3, sa1100_uartintr, p, p->name);

	/* set eia0 up as a console */
	if(console)
		uartspecial(p, 115200, &kbdq, &printq, kbdcr2nl);

	/* port for talking to microcontroller (eia1) */
	gpclkregs = mapspecial(GPCLKREGS, 64);
	gpclkregs->r0 = Gpclk_sus;	/* set uart mode */
	uart1regs = mapspecial(UART1REGS, 64);

	p = uartsetup(&sa1100_uart, uart1regs, ClockFreq, "serialport1");
	uartspecial(p, 115200, 0, 0, µcputc);
	intrenable(IRQ, IRQuart1b, sa1100_uartintr, p, p->name);
}
