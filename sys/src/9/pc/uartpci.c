#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/error.h"

extern PhysUart i8250physuart;
extern PhysUart pciphysuart;
extern void* i8250alloc(int, int, int);

static Uart*
uartpci(int ctlrno, Pcidev* p, int barno, int n, int freq, char* name)
{
	int i, io;
	void *ctlr;
	char buf[64];
	Uart *head, *uart;

	io = p->mem[barno].bar & ~0x01;
	snprint(buf, sizeof(buf), "%s%d", pciphysuart.name, ctlrno);
	if(ioalloc(io, p->mem[barno].size, 0, buf) < 0){
		print("uartpci: I/O 0x%uX in use\n", io);
		return nil;
	}

	head = uart = malloc(sizeof(Uart)*n);

	for(i = 0; i < n; i++){
		ctlr = i8250alloc(io, p->intl, p->tbdf);
		io += 8;
		if(ctlr == nil)
			continue;

		uart->regs = ctlr;
		snprint(buf, sizeof(buf), "%s.%8.8uX", name, p->tbdf);
		kstrdup(&uart->name, buf);
		uart->freq = freq;
		uart->phys = &i8250physuart;
		if(uart != head)
			(uart-1)->next = uart;
		uart++;
	}

	return head;
}

static Uart*
uartpcipnp(void)
{
	Pcidev *p;
	char *name;
	int ctlrno, n, subid;
	Uart *head, *tail, *uart;

	/*
	 * Loop through all PCI devices looking for simple serial
	 * controllers (ccrb == 0x07) and configure the ones which
	 * are familiar. All suitable devices are configured to
	 * simply point to the generic i8250 driver.
	 */
	head = tail = nil;
	ctlrno = 0;
	for(p = pcimatch(nil, 0, 0); p != nil; p = pcimatch(p, 0, 0)){
		if(p->ccrb != 0x07 || p->ccru != 0)
			continue;

		switch((p->did<<16)|p->vid){
		default:
			continue;
		case (0x9050<<16)|0x10B5:	/* Perle PCI-Fast4 */
			/*
			 * These devices consists of a PLX bridge (the above
			 * PCI VID+DID) behind which are some 16C654 UARTs.
			 * Must check the subsystem VID and DID for correct
			 * match.
			 */
			subid = pcicfgr16(p, PciSVID);
			subid |= pcicfgr16(p, PciSID)<<16;
			switch(subid){
			default:
				continue;
			case (0x0011<<16)|0x12E0:	/* Perle PCI-Fast16 */
				n = 16;
				name = "PCI-Fast16";
				break;
			case (0x0021<<16)|0x12E0:	/* Perle PCI-Fast8 */
				n = 8;
				name = "PCI-Fast8";
				break;
			case (0x0031<<16)|0x12E0:	/* Perle PCI-Fast4 */
				n = 4;
				name = "PCI-Fast4";
				break;
			}
			uart = uartpci(ctlrno, p, 2, n, 7372800, name);
			if(uart == nil)
				continue;
			break;
		}

		if(head != nil)
			tail->next = uart;
		else
			head = uart;
		for(tail = uart; tail->next != nil; tail = tail->next)
			;
		ctlrno++;
	}

	return head;
}

PhysUart pciphysuart = {
	.name		= "UartPCI",
	.pnp		= uartpcipnp,
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
