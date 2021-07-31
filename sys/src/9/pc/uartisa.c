#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/error.h"

extern PhysUart i8250physuart;
extern PhysUart isaphysuart;
extern void* i8250alloc(int, int, int);

static Uart*
uartisa(int ctlrno, ISAConf* isa)
{
	int io;
	void *ctlr;
	Uart *uart;
	char buf[64];

	io = isa->port;
	snprint(buf, sizeof(buf), "%s%d", isaphysuart.name, ctlrno);
	if(ioalloc(io, 8, 0, buf) < 0){
		print("uartisa: I/O 0x%uX in use\n", io);
		return nil;
	}

	uart = malloc(sizeof(Uart));
	ctlr = i8250alloc(io, isa->irq, BUSUNKNOWN);
	if(ctlr == nil){
		iofree(io);
		free(uart);
		return nil;
	}

	uart->regs = ctlr;
	snprint(buf, sizeof(buf), "COM%d", ctlrno+1);
	kstrdup(&uart->name, buf);
	uart->freq = isa->freq;
	uart->phys = &i8250physuart;

	return uart;
}

static Uart*
uartisapnp(void)
{
	int ctlrno;
	ISAConf isa;
	Uart *head, *tail, *uart;

	/*
	 * Look for up to 4 discrete UARTs on the ISA bus.
	 * All suitable devices are configured to simply point
	 * to the generic i8250 driver.
	 */
	head = tail = nil;
	for(ctlrno = 2; ctlrno < 6; ctlrno++){
		memset(&isa, 0, sizeof(isa));
		if(!isaconfig("uart", ctlrno, &isa))
			continue;
		if(strcmp(isa.type, "isa") != 0)
			continue;
		if(isa.port == 0 || isa.irq == 0)
			continue;
		if(isa.freq == 0)
			isa.freq = 1843200;
		uart = uartisa(ctlrno, &isa);
		if(uart == nil)
			continue;
		if(head != nil)
			tail->next = uart;
		else
			head = uart;
		tail = uart;
	}

	return head;
}

PhysUart isaphysuart = {
	.name		= "UartISA",
	.pnp		= uartisapnp,
	.enable		= nil,
	.disable	= nil,
	.kick		= nil,
	.dobreak	= nil,
	.baud		= nil,
	.bits		= nil,
	.stop		= nil,
	.parity		= nil,
	.modemctl	= nil,
	.rts		= nil,
	.dtr		= nil,
	.status		= nil,
	.fifo		= nil,
};
