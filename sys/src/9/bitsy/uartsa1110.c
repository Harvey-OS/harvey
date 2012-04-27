#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"

#include	"../port/netif.h"

/* this isn't strictly an sa1110 driver.  The rts/cts stuff is h3650 specific */

static void sa1110_uartpower(Uart *, int);

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

extern PhysUart sa1110physuart;

//static
Uart sa1110uart[2] = {
{	.regs		= (void*)UART3REGS,
	.name		= "serialport3",
	.freq		= ClockFreq,
	.bits		= 8,
	.stop		= 1,
	.parity		= 'n',
	.baud		= 115200,
	.phys		= &sa1110physuart,
	.special	= 0,
	.next		= &sa1110uart[1], },

{	.regs		= (void*)UART1REGS,
	.name		= "serialport1",
	.freq		= ClockFreq,
	.bits		= 8,
	.stop		= 1,
	.parity		= 'n',
	.baud		= 115200,
	.phys		= &sa1110physuart,
	.putc		= µcputc,
	.special	= 0,
	.next		= nil, },
};
static Uart* µcuart;

#define R(p) ((Uartregs*)((p)->regs))
#define SR(p) ((Uartregs*)((p)->saveregs))

/*
 *  enable a port's interrupts.  set DTR and RTS
 */
static void
sa1110_uartenable(Uart *p, int intena)
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
sa1110_uartdisable(Uart *p)
{
	R(p)->ctl[3] &= ~(Rintena|Tintena|Rena|Tena);
}

static long
sa1110_uartstatus(Uart *p, void *buf, long n, long offset)
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
		p->ferr,
		p->oerr, 
		"",
		"",
		"",
		"" );
	return readstr(offset, buf, n, str);
}

/*
 *  set the buad rate
 */
static int
sa1110_uartbaud(Uart *p, int rate)
{
	ulong brconst;
	ulong ctl3;

	if(rate <= 0)
		return -1;

	/* disable */
	ctl3 = R(p)->ctl[3];
	R(p)->ctl[3] = 0;

	brconst = p->freq/(16*rate) - 1;
	R(p)->ctl[1] = (brconst>>8) & 0xf;
	R(p)->ctl[2] = brconst & 0xff;

	/* reenable */
	R(p)->ctl[3] = ctl3;

	p->baud = rate;
	return 0;
}

/*
 *  send a break
 */
static void
sa1110_uartbreak(Uart *p, int ms)
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
static int
sa1110_uartbits(Uart *p, int n)
{
	ulong ctl0, ctl3;

	ctl0 = R(p)->ctl[0];
	switch(n){
	case 7:
		ctl0 &= ~Bits8;
		break;
	case 8:
		ctl0 |= Bits8;
		break;
	default:
		return -1;
	}

	/* disable */
	ctl3 = R(p)->ctl[3];
	R(p)->ctl[3] = 0;

	R(p)->ctl[0] = ctl0;

	/* reenable */
	R(p)->ctl[3] = ctl3;

	p->bits = n;
	return 0;
}

/*
 *  set stop bits
 */
static int
sa1110_uartstop(Uart *p, int n)
{
	ulong ctl0, ctl3;

	ctl0 = R(p)->ctl[0];
	switch(n){
	case 1:
		ctl0 &= ~Stop2;
		break;
	case 2:
		ctl0 |= Stop2;
		break;
	default:
		return -1;
	}

	/* disable */
	ctl3 = R(p)->ctl[3];
	R(p)->ctl[3] = 0;

	R(p)->ctl[0] = ctl0;

	/* reenable */
	R(p)->ctl[3] = ctl3;

	p->stop = n;
	return 0;
}

/*
 *  turn on/off rts
 */
static void
sa1110_uartrts(Uart*, int)
{
}

/*
 *  turn on/off dtr
 */
static void
sa1110_uartdtr(Uart*, int)
{
}

/*
 *  turn on/off modem flow control on/off (rts/cts)
 */
static void
sa1110_uartmodemctl(Uart *p, int on)
{
	if(on) {
	} else {
		p->cts = 1;
	}
}

/*
 *  set parity
 */
static int
sa1110_uartparity(Uart *p, int type)
{
	ulong ctl0, ctl3;

	ctl0 = R(p)->ctl[0];
	switch(type){
	case 'e':
		ctl0 |= Parity|Even;
		break;
	case 'o':
		ctl0 |= Parity;
		break;
	default:
		ctl0 &= ~(Parity|Even);
		break;
	}

	/* disable */
	ctl3 = R(p)->ctl[3];
	R(p)->ctl[3] = 0;

	R(p)->ctl[0] = ctl0;

	/* reenable */
	R(p)->ctl[3] = ctl3;

	return 0;
}

/*
 *  restart output if not blocked and OK to send
 */
static void
sa1110_uartkick(Uart *p)
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
 *  take an interrupt
 */
static void
sa1110_uartintr(Ureg*, void *x)
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
			p->ferr++;
		if(s & Overrun)
			p->oerr++;
	}

	/* receiver interrupt, snarf bytes */
	while(regs->status[1] & Rnotempty)
		uartrecv(p, regs->data);
}

static Uart*
sa1110_pnp(void)
{
	return sa1110uart;
}

static int
sa1110_getc(Uart *uart)
{
	Uartregs *ur;

	ur = uart->regs;
	while((ur->status[1] & Rnotempty) == 0)
		;
	return ur->data;
}

static void
sa1110_putc(Uart *uart, int c)
{
	Uartregs *ur;

	ur = uart->regs;
	/* wait for output ready */
	while((ur->status[1] & Tnotfull) == 0)
		;
	ur->data = c;
	while((ur->status[1] & Tbusy))
		;
}

PhysUart sa1110physuart = {
	.name=		"sa1110",
	.pnp= 		sa1110_pnp,
	.enable=	sa1110_uartenable,
	.disable=	sa1110_uartdisable,
	.bits=		sa1110_uartbits,
	.kick=		sa1110_uartkick,
	.modemctl=	sa1110_uartmodemctl,
	.baud=		sa1110_uartbaud,
	.stop=		sa1110_uartstop,
	.parity=	sa1110_uartparity,
	.dobreak=	sa1110_uartbreak,
	.rts=		sa1110_uartrts,
	.dtr=		sa1110_uartdtr,
	.status=	sa1110_uartstatus,
	.power=		sa1110_uartpower,
	.getc=		sa1110_getc,
	.putc=		sa1110_putc,
};

/*
 *  for iprint, just write it
 */
void
serialµcputs(uchar *str, int n)
{
	Uartregs *ur;

	if(µcuart == nil)
		return;
	ur = µcuart->regs;
	while(n-- > 0){
		/* wait for output ready */
		while((ur->status[1] & Tnotfull) == 0)
			;
		ur->data = *str++;
	}
	while((ur->status[1] & Tbusy))
		;
}

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
sa1110_uartsetup(int console)
{
	Uart *p;

	/* external serial port (eia0) */
	p = &sa1110uart[0];
	p->regs = mapspecial(UART3REGS, sizeof(Uartregs));
	p->saveregs = xalloc(sizeof(Uartregs));
	/* set eia0 up as a console */
	if(console){
		uartctl(p, "b115200 l8 pn s1");
		(*p->phys->enable)(p, 0);
		p->console = 1;
		consuart = p;
	}
	intrenable(IRQ, IRQuart3, sa1110_uartintr, p, p->name);

	/* port for talking to microcontroller (eia1) */
	gpclkregs = mapspecial(GPCLKREGS, sizeof(Gpclkregs));
	gpclkregs->r0 = Gpclk_sus;	/* set uart mode */

	p = &sa1110uart[1];
	p->regs = mapspecial(UART1REGS, sizeof(Uartregs));
	p->saveregs = xalloc(sizeof(Uartregs));
	uartctl(p, "b115200 l8 pn s1");
	µcuart = p;
	p->special = 1;
	(*p->phys->enable)(p, 0);
	intrenable(IRQ, IRQuart1b, sa1110_uartintr, p, p->name);
}

static  void
uartcpy(Uartregs *to, Uartregs *from)
{
	to->ctl[0] = from->ctl[0];
//	to->ctl[1] = from->ctl[1];
//	to->ctl[2] = from->ctl[2];
	to->ctl[3] = from->ctl[3];

}

static void
sa1110_uartpower(Uart *p, int powerup)
{
	if (powerup) {
		/* power up, restore the registers */
		uartcpy(R(p), SR(p));
		R(p)->status[0] = R(p)->status[0];
	} else {
		/* power down, save the registers */
		uartcpy(SR(p), R(p));
	}
}
