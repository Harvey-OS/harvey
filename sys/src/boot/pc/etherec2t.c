/*
 * Linksys Combo PCMCIA EthernetCard (EC2T)
 * and EtherFast 10/100 PC Card (PCMPC100).
 * Supposedly NE2000 clones, see the comments in ether2000.c
 */
#include "u.h"
#include "lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

#include "etherif.h"
#include "ether8390.h"

enum {
	Data		= 0x10,		/* offset from I/O base of data port */
	Reset		= 0x1F,		/* offset from I/O base of reset port */
};

static char* ec2tpcmcia[] = {
	"EC2T",
	"PCMPC100",
	nil,
};

int
ec2treset(Ether* ether)
{
	ushort buf[16];
	ulong port;
	Dp8390 *ctlr;
	int i, slot;
	uchar ea[Eaddrlen], sum, x;
	char *type;

	/*
	 * Set up the software configuration.
	 * Use defaults for port, irq, mem and size
	 * if not specified.
	 * The manual says 16KB memory, the box
	 * says 32KB. The manual seems to be correct.
	 */
	if(ether->port == 0)
		ether->port = 0x300;
	if(ether->irq == 0)
		ether->irq = 9;
	if(ether->mem == 0)
		ether->mem = 0x4000;
	if(ether->size == 0)
		ether->size = 16*1024;
	port = ether->port;

	slot = -1;
	type = nil;
	for(i = 0; ec2tpcmcia[i] != nil; i++){
		type = ec2tpcmcia[i];
		if((slot = pcmspecial(type, ether)) >= 0)
			break;
	}
	if(slot < 0)
		return -1;

	ether->ctlr = malloc(sizeof(Dp8390));
	ctlr = ether->ctlr;
	ctlr->width = 2;
	ctlr->ram = 0;

	ctlr->port = port;
	ctlr->data = port+Data;

	ctlr->tstart = HOWMANY(ether->mem, Dp8390BufSz);
	ctlr->pstart = ctlr->tstart + HOWMANY(sizeof(Etherpkt), Dp8390BufSz);
	ctlr->pstop = ctlr->tstart + HOWMANY(ether->size, Dp8390BufSz);

	ctlr->dummyrr = 0;

	/*
	 * Reset the board. This is done by doing a read
	 * followed by a write to the Reset address.
	 */
	buf[0] = inb(port+Reset);
	delay(2);
	outb(port+Reset, buf[0]);
	delay(2);

	/*
	 * Init the (possible) chip, then use the (possible)
	 * chip to read the (possible) PROM for ethernet address
	 * and a marker byte.
	 * Could just look at the DP8390 command register after
	 * initialisation has been tried, but that wouldn't be
	 * enough, there are other ethernet boards which could
	 * match.
	 */
	dp8390reset(ether);
	sum = 0;
	if(strcmp(type, "PCMPC100")){
		memset(buf, 0, sizeof(buf));
		dp8390read(ctlr, buf, 0, sizeof(buf));
		if((buf[0x0E] & 0xFF) == 0x57 && (buf[0x0F] & 0xFF) == 0x57)
			sum = 0xFF;
	}
	else{
		/*
		 * The PCMPC100 has the ethernet address in I/O space.
		 * There's a checksum over 8 bytes which sums to 0xFF.
		 */
		for(i = 0; i < 8; i++){
			x = inb(port+0x14+i);
			sum += x;
			buf[i] = (x<<8)|x;
		}
	}
	if(sum != 0xFF){
		pcmspecialclose(slot);
		free(ether->ctlr);
		return -1;
	}

	/*
	 * Stupid machine. Shorts were asked for,
	 * shorts were delivered, although the PROM is a byte array.
	 * Set the ethernet address.
	 */
	memset(ea, 0, Eaddrlen);
	if(memcmp(ea, ether->ea, Eaddrlen) == 0){
		for(i = 0; i < sizeof(ether->ea); i++)
			ether->ea[i] = buf[i];
	}
	dp8390setea(ether);

	return 0;
}
