/*
 * Oxford Semiconductor OXPCIe95x UART driver
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/error.h"

extern PhysUart oxphysuart;

enum {
	Ccr		= 0x0000/4,	/* Class Code and Revision ID */
	Nuart		= 0x0004/4,	/* Decimal Number of UARTs */
	Gis		= 0x0008/4,	/* Global UART IRQ Status */
	Gie		= 0x000C/4,	/* Global UART IRQ Enable */
	Gid		= 0x0010/4,	/* Global UART IRQ Disable */
	Gwe		= 0x0014/4,	/* Global UART Wake Enable */
	Gwd		= 0x0018/4,	/* Global UART Wake Disable */
};

enum {
	Thr		= 0x00,		/* Transmitter Holding */
	Rhr		= 0x00,		/* Receiver Holding */
	Ier		= 0x01,		/* Interrupt Enable */
	Fcr		= 0x02,		/* FIFO Control */
	Isr		= 0x02,		/* Interrupt Status */
	Lcr		= 0x03,		/* Line Control */
	Mcr		= 0x04,		/* Modem Control */
	Lsr		= 0x05,		/* Line Status */
	Msr		= 0x06,		/* Modem Status */
	Spr		= 0x07,		/* Scratch Pad */
	Dll		= 0x00,		/* Divisor Latch LSB */
	Dlm		= 0x01,		/* Divisor Latch MSB */
	Efr		= 0x02,		/* Enhanced Feature */
};

typedef struct Port Port;
typedef struct Ctlr Ctlr;

struct Port {
	Uart;
	Ctlr	*ctlr;
	u8int	*mem;

	int	level;
	int	dtr, rts;
	int	ri;
};

struct Ctlr {
	Lock;
	char	*name;
	Pcidev	*pcidev;
	u32int	*mem;

	u32int	im;

	Port	port[0x10];
	int	nport;
};

static Uart *
oxpnp(void)
{
	Pcidev *p;
	Ctlr *ctlr;
	Port *port;
	int i;
	char *model;
	char name[12+1];
	Uart *head, *tail;
	static int ctlrno;

	p = nil;
	head = tail = nil;
	while(p = pcimatch(p, 0x1415, 0)){
		switch(p->did){
		case 0xc101:
		case 0xc105:
		case 0xc11b:
		case 0xc11f:
		case 0xc120:
		case 0xc124:
		case 0xc138:
		case 0xc13d:
		case 0xc140:
		case 0xc141:
		case 0xc144:
		case 0xc145:
		case 0xc158:
		case 0xc15d:
			model = "OXPCIe952";
			break;
		case 0xc208:
		case 0xc20d:
			model = "OXPCIe954";
			break;
		case 0xc308:
		case 0xc30d:
			model = "OXPCIe958";
			break;
		default:
			continue;
		}
		ctlr = malloc(sizeof *ctlr);
		if(ctlr == nil){
			print("oxpnp: out of memory\n");
			continue;
		}
		ctlr->pcidev = p;
		ctlr->mem = vmap(p->mem[0].bar & ~0xf, p->mem[0].size);
		if(ctlr->mem == nil){
			print("oxpnp: vmap failed\n");
			free(ctlr);
			continue;
		}
		snprint(name, sizeof name, "uartox%d", ctlrno);
		kstrdup(&ctlr->name, name);
		ctlr->nport = ctlr->mem[Nuart] & 0x1f;
		for(i = 0; i < ctlr->nport; ++i){
			port = &ctlr->port[i];
			port->ctlr = ctlr;
			port->mem = (u8int *)ctlr->mem + 0x1000 + 0x200*i;
			port->regs = port;
			snprint(name, sizeof name, "%s.%d", ctlr->name, i);
			kstrdup(&port->name, name);
			port->phys = &oxphysuart;
			if(head == nil)
				head = port;
			else
				tail->next = port;
			tail = port;
		}
		print("%s: %s: %d ports irq %d\n",
		    ctlr->name, model, ctlr->nport, p->intl);
		ctlrno++;
	}
	return head;
}

static void
oxinterrupt(Ureg *, void *arg)
{
	Ctlr *ctlr;
	Port *port;
	Uart *uart;
	int i, old;
	u8int val;
	char ch;

	ctlr = arg;

	ilock(ctlr);
	if(!(ctlr->im & ctlr->mem[Gis])){
		iunlock(ctlr);
		return;
	}
	for(i = 0; i < ctlr->nport; ++i){
		if(!(ctlr->im & 1<<i))
			continue;
		port = &ctlr->port[i];
		uart = port;	/* "Come Clarity" */
		switch(port->mem[Isr] & 0x3f){
		case 0x06:	/* Receiver status error */
		case 0x04:	/* Receiver data available */
		case 0x0c:	/* Receiver time-out */
			for(;;){
				val = port->mem[Lsr];
				if(!(val & 1<<0))	/* RxRDY */
					break;
				if(val & 1<<1)		/* Overrun Error */
					uart->oerr++;
				if(val & 1<<2)		/* Parity Error */
					uart->perr++;
				if(val & 1<<3)		/* Framing Error */
					uart->ferr++;
				ch = port->mem[Rhr];
				if(!(val & 1<<7))	/* Data Error */
					uartrecv(uart, ch);
			}
			break;
		case 0x02:	/* Transmitter THR empty */
			uartkick(uart);
			break;
		case 0x00:	/* Modem status change */
			val = port->mem[Msr];
			if(val & 1<<0){			/* Delta nCTS */
				ilock(&uart->tlock);
				old = uart->cts;
				uart->cts = val & 1<<4;	/* CTS */
				if(!old && uart->cts)
					uart->ctsbackoff = 2;
				iunlock(&uart->tlock);
			}
			if(val & 1<<1){			/* Delta nDSR */
				old = val & 1<<5;	/* DSR */
				if(!old && uart->dsr && uart->hup_dsr)
					uart->dohup = 1;
				uart->dsr = old;
			}
			port->ri = val & 1<<6;		/* RI */
			if(val & 1<<3){			/* Delta nDCD */
				old = val & 1<<7;	/* DCD */
				if(!old && uart->dcd && uart->hup_dcd)
					uart->dohup = 1;
				uart->dcd = old;
			}
			break;
		}
	}
	iunlock(ctlr);
}

#define MASK(p)	(1UL<<((p)-(p)->ctlr->port))

static void
oxenable(Uart *uart, int)
{
	Ctlr *ctlr;
	Port *port;

	port = uart->regs;
	ctlr = port->ctlr;

	ilock(ctlr);
	if(ctlr->im == 0)
		intrenable(ctlr->pcidev->intl, oxinterrupt, ctlr,
		    ctlr->pcidev->tbdf, ctlr->name);
	ctlr->im |= MASK(port);
	iunlock(ctlr);

	/* Enable 950 Mode */
	port->mem[Lcr] |= 1<<7;			/* Divisor latch access */
	port->mem[Efr] = 1<<4;			/* Enhanced mode */
	port->mem[Lcr] &= ~(1<<7);

	port->mem[Ier] = 1<<2|1<<1|1<<0;	/* Rx Stat, THRE, RxRDY */

	(*uart->phys->dtr)(uart, 1);
	(*uart->phys->rts)(uart, 1);

	/* Enable FIFO */
	(*uart->phys->fifo)(uart, ~0);
}

static void
oxdisable(Uart *uart)
{
	Ctlr *ctlr;
	Port *port;

	port = uart->regs;
	ctlr = port->ctlr;

	(*uart->phys->dtr)(uart, 0);
	(*uart->phys->rts)(uart, 0);
	(*uart->phys->fifo)(uart, 0);

	port->mem[Ier] = 0;

	ilock(ctlr);
	ctlr->im &= ~MASK(port);
	if(ctlr->im == 0)
		intrdisable(ctlr->pcidev->intl, oxinterrupt, ctlr,
		    ctlr->pcidev->tbdf, ctlr->name);
	iunlock(ctlr);
}

static void
oxkick(Uart *uart)
{
	Port *port;

	if(uart->cts == 0 || uart->blocked)
		return;

	port = uart->regs;

	for(;;){
		if(!(port->mem[Lsr] & 1<<5))	/* THR Empty */
			break;
		if(uart->op >= uart->oe && uartstageoutput(uart) == 0)
			break;
		port->mem[Thr] = *(uart->op++);
	}
}

static void
oxdobreak(Uart *uart, int ms)
{
	Port *port;

	if(ms <= 0)
		ms = 200;

	port = uart->regs;

	port->mem[Lcr] |= 1<<6;			/* Transmission break */
	if(!waserror()){
		tsleep(&up->sleep, return0, nil, ms);
		poperror();
	}
	port->mem[Lcr] &= ~(1<<6);
}

static int
oxbaud(Uart *uart, int baud)
{
	Port *port;
	u16int val;

	if(baud <= 0)
		return -1;

	port = uart->regs;

	/*
	 * We aren't terribly interested in non-standard baud rates.
	 * Rather than mess about with generator constants, we instead
	 * program DLM and DLL according to Table 37 in the datasheet.
	 */
	switch(baud){
	case 1200:
		val = 0x0cb6;
		break;
	case 2400:
		val = 0x065b;
		break;
	case 4800:
		val = 0x032d;
		break;
	case 9600:
		val = 0x0196;
		break;
	case 19200:
		val = 0x00cb;
		break;
	case 38400:
		val = 0x0066;
		break;
	case 57600:
		val = 0x0044;
		break;
	case 115200:
		val = 0x0022;
		break;
	default:
		return -1;
	}
	port->mem[Lcr] |= 1<<7;			/* Divisor latch access */
	port->mem[Dlm] = val>>8;
	port->mem[Dll] = val;
	port->mem[Lcr] &= ~(1<<7);
	uart->baud = baud;
	return 0;
}

static int
oxbits(Uart *uart, int bits)
{
	Port *port;
	u8int val;

	port = uart->regs;

	val = port->mem[Lcr] & 0x7c;
	switch(bits){
	case 8:
		val |= 0x3;			/* Data length */
		break;
	case 7:
		val |= 0x2;
		break;
	case 6:
		val |= 0x1;
		break;
	case 5:
		break;
	default:
		return -1;
	}
	port->mem[Lcr] = val;
	uart->bits = bits;
	return 0;
}

static int
oxstop(Uart *uart, int stop)
{
	Port *port;
	u8int val;

	port = uart->regs;

	val = port->mem[Lcr] & 0x7b;
	switch(stop){
	case 2:
		val |= 1<<2;			/* Number of Stop Bits */
		break;
	case 1:
		break;
	default:
		return -1;
	}
	port->mem[Lcr] = val;
	uart->stop = stop;
	return 0;
}

static int
oxparity(Uart *uart, int parity)
{
	Port *port;
	u8int val;

	port = uart->regs;

	val = port->mem[Lcr] & 0x67;
	switch(parity){
	case 'e':
		val |= 1<<4;			/* Even/Odd Parity */
	case 'o':
		val |= 1<<3;			/* Parity Enable */
		break;
	case 'n':
		break;
	default:
		return -1;
	}
	port->mem[Lcr] = val;
	uart->parity = parity;
	return 0;
}

static void
oxmodemctl(Uart *uart, int on)
{
	Ctlr *ctlr;
	Port *port;

	port = uart->regs;
	ctlr = port->ctlr;

	ilock(ctlr);
	ilock(&uart->tlock);
	if(on){
		port->mem[Ier] |= 1<<3;			/* Modem */
		uart->cts = port->mem[Msr] & 1<<4;	/* CTS */
	}else{
		port->mem[Ier] &= ~(1<<3);
		uart->cts = 1;
	}
	uart->modem = on;
	iunlock(&uart->tlock);
	iunlock(ctlr);
}

static void
oxrts(Uart *uart, int on)
{
	Port *port;

	port = uart->regs;

	if(on)
		port->mem[Mcr] |= 1<<1;		/* RTS */
	else
		port->mem[Mcr] &= ~(1<<1);
	port->rts = on;
}

static void
oxdtr(Uart *uart, int on)
{
	Port *port;

	port = uart->regs;

	if(on)
		port->mem[Mcr] |= 1<<0;		/* DTR */
	else
		port->mem[Mcr] &= ~(1<<0);
	port->dtr = on;
}

static long
oxstatus(Uart *uart, void *buf, long n, long offset)
{
	Port *port;

	if(offset > 0)
		return 0;

	port = uart->regs;

	return snprint(buf, n,
	    "b%d c%d d%d e%d l%d m%d p%c r%d s%d i%d\n"
	    "dev(%d) type(%d) framing(%d) overruns(%d) "
	    "berr(%d) serr(%d)%s%s%s%s\n",

	    uart->baud,
	    uart->hup_dcd,
	    port->dtr,
	    uart->hup_dsr,
	    uart->bits,
	    uart->modem,
	    uart->parity,
	    port->rts,
	    uart->stop,
	    port->level,

	    uart->dev,
	    uart->type,
	    uart->ferr,
	    uart->oerr,
	    uart->berr,
	    uart->serr,
	    uart->cts ? " cts": "",
	    uart->dsr ? " dsr": "",
	    port->ri ? " ring": "",
	    uart->dcd ? " dcd": ""
	);
}

static void
oxfifo(Uart *uart, int level)
{
	Ctlr *ctlr;
	Port *port;

	port = uart->regs;
	ctlr = port->ctlr;

	/*
	 * 950 Mode FIFOs have a depth of 128 bytes; devuart only
	 * cares about setting RHR trigger levels.  THR trigger
	 * levels are not supported.
	 */
	ilock(ctlr);
	if(level == 0)
		port->mem[Fcr] = 0;		/* Disable FIFO */
	else{
		port->mem[Fcr] = 1<<0;		/* Enable FIFO */
		switch(level){
		default:
			level = 112;
		case 112:
			port->mem[Fcr] = 0x03<<6|1<<0;	/* RHR Trigger Level */
			break;
		case 64:
			port->mem[Fcr] = 0x02<<6|1<<0;
			break;
		case 32:
			port->mem[Fcr] = 0x01<<6|1<<0;
			break;
		case 16:
			break;
		}
	}
	port->level = level;
	iunlock(ctlr);
}

PhysUart oxphysuart = {
	.name		= "OXPCIe95x",
	.pnp		= oxpnp,
	.enable		= oxenable,
	.disable	= oxdisable,
	.kick		= oxkick,
	.dobreak	= oxdobreak,
	.baud		= oxbaud,
	.bits		= oxbits,
	.stop		= oxstop,
	.parity		= oxparity,
	.modemctl	= oxmodemctl,
	.rts		= oxrts,
	.dtr		= oxdtr,
	.status		= oxstatus,
	.fifo		= oxfifo,
};
