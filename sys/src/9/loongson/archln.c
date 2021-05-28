#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/error.h"
#include "ureg.h"
#include "../port/netif.h"
#include "etherif.h"

int
archether(unsigned ctlrno, Ether *ether)
{
	switch(ctlrno) {
	case 0:
		ether->type = "rtl8139";
		ether->ctlrno = ctlrno;
		ether->irq = ILpci;
		ether->nopt = 0;
		ether->mbps = 100;
		return 1;
	}
	return -1;
}

/* port i/o */
void
insb(int port, void *p, int count)
{
	uchar *q = p;

	for(; count > 0; count--)
		*q++ = inb(port);
}

void
inss(int port, void *p, int count)
{
	ushort *q = p;

	for(; count > 0; count--)
		*q++ = ins(port);
}

void
insl(int port, void *p, int count)
{
	ulong *q = p;

	for(; count > 0; count--)
		*q++ = inl(port);
}

void
outsb(int port, void *p, int count)
{
	uchar *q = p;

	for(; count > 0; count--)
		outb(port, *q++);
}

void
outss(int port, void *p, int count)
{
	ushort *q = p;

	for(; count > 0; count--)
		outs(port, *q++);
}

void
outsl(int port, void *p, int count)
{
	ulong *q = p;

	for(; count > 0; count--)
		outl(port, *q++);
}

void
fptrap(Ureg*)
{
}
