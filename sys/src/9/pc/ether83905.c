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
 * National Semiconductor DP83905 AT/LANTIC.
 *
 * It doesn't seem possible to tell which mode (NE2000 or WD8003)
 * the chip is configured in without a lot of work. For now we'll
 * just assume WD8003 mode.
 * Driver written for the Dauphin DTR-1.
 */
enum {					/* Shared Memory Compatible Mode */
	Control1	= 0x00,		/* (WO) */
	ATDetect	= 0x01,		/* (RO) */
	Control2	= 0x05,		/* (WO) */
	Lar		= 0x08,		/* LAN Address Registers */
};

enum {					/* Control1 */
	Rst		= 0x80,		/* software reset */
	Meme		= 0x40,		/* memory enable */
};

enum {					/* ATDetect */
	ATdet		= 0x01,		/* 8 or 16-bit slot */
};

enum {					/* Control2 */
	Memw		= 0x40,		/* */
	M16		= 0x80,		/* */
};

static void*
read(Ctlr *ctlr, void *to, ulong from, ulong len)
{
	/*
	 * In this case, 'from' is an index into the shared memory.
	 */
	memmove(to, (void*)(ctlr->card.mem+from), len);
	return to;
}

static void*
write(Ctlr *ctlr, ulong to, void *from, ulong len)
{
	/*
	 * In this case, 'to' is an index into the shared memory.
	 */
	memmove((void*)(ctlr->card.mem+to), from, len);
	return (void*)to;
}

/*
 * Get configuration parameters, enable memory.
 * There are opportunities here for buckets of code.
 * We'll try to resist.
 */
static int
reset(Ctlr *ctlr)
{
	int i;
	uchar control1, control2;
	ulong port;

	/*
	 * Set up the software configuration.
	 * Use defaults for port, irq, mem and size if not specified.
	 * Defaults are set for the dumb 8003E which can't be
	 * autoconfigured.
	 */
	if(ctlr->card.port == 0)
		ctlr->card.port = 0x280;
	if(ctlr->card.irq == 0)
		ctlr->card.irq = 3;
	if(ctlr->card.mem == 0)
		ctlr->card.mem = 0xD0000;
	if(ctlr->card.size == 0)
		ctlr->card.size = 8*1024;

	ctlr->card.reset = reset;
	ctlr->card.attach = dp8390attach;
	ctlr->card.mode = dp8390mode;
	ctlr->card.read = read;
	ctlr->card.write = write;
	ctlr->card.receive = dp8390receive;
	ctlr->card.transmit = dp8390transmit;
	ctlr->card.intr = dp8390intr;
	ctlr->card.watch = 0;
	ctlr->card.ram = 1;

	port = ctlr->card.port;
	for(i = 0; i < sizeof(ctlr->ea); i++)
		ctlr->ea[i] = inb(port+Lar+i);

	control1 = Meme|((ctlr->card.mem>>13) & 0x3F);
	control2 = (ctlr->card.mem>>19) & 0x1F;
	if(ctlr->card.bit16 = (inb(port+ATDetect) & ATdet))
		control2 |= M16|Memw;
	ctlr->card.mem |= KZERO;

	/*
	 * Set the DP8390 ring addresses.
	 */
	ctlr->card.dp8390 = port+0x10;
	ctlr->card.tstart = 0;
	ctlr->card.pstart = HOWMANY(sizeof(Etherpkt), Dp8390BufSz);
	ctlr->card.pstop = HOWMANY(ctlr->card.size, Dp8390BufSz);

	/*
	 * Enable interface RAM, set interface width.
	 */
	outb(port+Control1, control1);
	outb(port+Control2, control2);

	/*
	 * Finally, init the 8390 and set the
	 * ethernet address.
	 */
	dp8390reset(ctlr);
	dp8390setea(ctlr);

	return 0;
}

void
ether83905link(void)
{
	addethercard("DP83905", reset);
}
