#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"
#include "io.h"
#include "devtab.h"

#include "ether.h"

/*
 * Driver written for the 'national pcmcia ether adapter', NE 4100.
 * The manual says NE2000 compatible.
 * The interface appears to be pretty well described in the National
 * Semiconductor Local Area Network Databook (1992) as one of the
 * AT evaluation cards.
 *
 * The NE2000 is really just a DP8390[12] plus a data port
 * and a reset port.
 */
enum {
	Data		= 0x10,		/* offset from I/O base of data port */
	Misc		= 0x18,		/* offset from I/O base of miscellaneous port */
	Reset		= 0x1f,		/* offset from I/O base of reset port */
};


int
ne4100reset(Ctlr *ctlr)
{
	int i, slot;
	uchar x, *p;
	PCMmap *m;

	/*
	 * Set up the software configuration.
	 * Use defaults for port, irq, mem and size
	 * if not specified.
	 */
	if(ctlr->card.port == 0)
		ctlr->card.port = 0x320;
	if(ctlr->card.irq == 0)
		ctlr->card.irq = 10;
	if(ctlr->card.irq == 2)
		ctlr->card.irq = 9;
	if(ctlr->card.mem == 0)
		ctlr->card.mem = 0x4000;
	if(ctlr->card.size == 0)
		ctlr->card.size = 16*1024;

	ctlr->card.reset = ne4100reset;
	ctlr->card.attach = dp8390attach;
	ctlr->card.mode = dp8390mode;
	ctlr->card.read = dp8390read;
	ctlr->card.write = dp8390write;
	ctlr->card.receive = dp8390receive;
	ctlr->card.transmit = dp8390transmit;
	ctlr->card.intr = dp8390intr;
	ctlr->card.watch = dp8390watch;
	ctlr->card.overflow = dp8390overflow;

	ctlr->card.bit16 = 1;
	ctlr->card.dp8390 = ctlr->card.port;
	ctlr->card.data = ctlr->card.port+Data;

	ctlr->card.tstart = HOWMANY(ctlr->card.mem, Dp8390BufSz);
	ctlr->card.pstart = ctlr->card.tstart + HOWMANY(sizeof(Etherpkt), Dp8390BufSz);
	ctlr->card.pstop = ctlr->card.tstart + HOWMANY(ctlr->card.size, Dp8390BufSz);

	/* see if there's a pcmcia card that looks right */
	slot = pcmspecial("PC-NIC", &ctlr->card);
	if(slot < 0)
		return -1;

	/* reset ST-NIC */
	x = inb(ctlr->card.port+Reset);
	delay(2);
	outb(ctlr->card.port+Reset, x);

	/* enable interrupts */
	outb(ctlr->card.port + Misc, 1<<6);

	/* Init the (possible) chip. */
	dp8390reset(ctlr);

	/* set the ether address */
	m = pcmmap(slot, 0, 0x1000, 1);
	if(m == 0)
		return -1;
	p = (uchar*)(KZERO|m->isa);
	for(i = 0; i < sizeof(ctlr->ea); i++)
		ctlr->ea[i] = p[0xff0+2*i];
	pcmunmap(slot, m);
	dp8390setea(ctlr);

	return 0;
}

void
ether4100link(void)
{
	addethercard("NE4100", ne4100reset);
}
