#include "all.h"
#include "io.h"
#include "mem.h"

#include "ether.h"

/*
 * Driver written for the 'Notebook Computer Ethernet LAN Adapter',
 * a plug-in to the bus-slot on the rear of the Gateway NOMAD 425DXL
 * laptop. The manual says NE2000 compatible.
 * The interface appears to be pretty well described in the National
 * Semiconductor Local Area Network Databook (1992) as one of the
 * AT evaluation cards.
 *
 * The NE2000 is really just a DP8390[12] plus a data port
 * and a reset port.
 */
enum {
	Data		= 0x10,		/* offset from I/O base of data port */
	Reset		= 0x18,		/* offset from I/O base of reset port */
};

int
ne2000reset(Ctlr *ctlr)
{
	ushort buf[16];
	int i;

	/*
	 * Set up the software configuration.
	 * Use defaults for port, irq, mem and size
	 * if not specified.
	 */
	if(ctlr->card.port == 0)
		ctlr->card.port = 0x300;
	if(ctlr->card.irq == 0)
		ctlr->card.irq = 2;
	if(ctlr->card.mem == 0)
		ctlr->card.mem = 0x4000;
	if(ctlr->card.size == 0)
		ctlr->card.size = 16*1024;

	ctlr->card.reset = ne2000reset;
	ctlr->card.attach = dp8390attach;
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
	ctlr->card.pstart = ctlr->card.tstart + HOWMANY(sizeof(Enpkt), Dp8390BufSz);
	ctlr->card.pstop = ctlr->card.tstart + HOWMANY(ctlr->card.size, Dp8390BufSz);

	/*
	 * Reset the board. This is done by doing a read
	 * followed by a write to the Reset address.
	 */
	buf[0] = inb(ctlr->card.port+Reset);
	delay(2);
	outb(ctlr->card.port+Reset, buf[0]);
	
	/*
	 * Init the (possible) chip, then use the (possible)
	 * chip to read the (possible) PROM for ethernet address
	 * and a marker byte.
	 * We could just look at the DP8390 command register after
	 * initialisation has been tried, but that wouldn't be
	 * enough, there are other ethernet boards which could
	 * match.
	 */
	dp8390reset(ctlr);
	memset(buf, 0, sizeof(buf));
	dp8390read(ctlr, buf, 0, sizeof(buf));
	if((buf[0x0E] & 0xFF) != 0x57 || (buf[0x0F] & 0xFF) != 0x57)
		return -1;

	/*
	 * Stupid machine. We asked for shorts, we got shorts,
	 * although the PROM is a byte array.
	 * Now we can set the ethernet address.
	 */
	if((ctlr->card.ea[0]|ctlr->card.ea[1]|ctlr->card.ea[2]|ctlr->card.ea[3]|ctlr->card.ea[4]|ctlr->card.ea[5]) == 0){
		for(i = 0; i < sizeof(ctlr->card.ea); i++)
			ctlr->card.ea[i] = buf[i];
	}
	dp8390setea(ctlr);

	return 0;
}
