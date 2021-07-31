/*
 * Supposed NE2000 PCMCIA clones, see the comments in ether2000.c
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/error.h"
#include "../port/netif.h"

#include "etherif.h"
#include "ether8390.h"

enum {
	Data		= 0x10,		/* offset from I/O base of data port */
	Reset		= 0x1F,		/* offset from I/O base of reset port */
};

typedef struct Ec2t {
	char*	name;
	int	iochecksum;
} Ec2t;

static Ec2t ec2tpcmcia[] = {
	{ "EC2T", 0, },			/* Linksys Combo PCMCIA EthernetCard */
	{ "PCMPC100", 1, },		/* EtherFast 10/100 PC Card */
	{ "PCM100", 1, },		/* EtherFast PCM100 Card */
	{ "EN2216", 0, },		/* Accton EtherPair-PCMCIA */
	{ "FA410TX", 1, },		/* Netgear FA410TX */
	{ "Network Everywhere", 0, },	/* Linksys NP10T 10BaseT Card */
	{ "10/100 Port Attached", 1, },	/* SMC 8040TX */
	{ "8041TX-10/100-PC-Card-V2", 0 }, /* SMC 8041TX */
	{ "FA411", 0 },			/* Netgear FA411 PCMCIA */
	{ nil, 0, },
};

static int
reset(Ether* ether)
{
	ushort buf[16];
	ulong port;
	Dp8390 *ctlr;
	int i, slot;
	uchar ea[Eaddrlen], sum, x;
	Ec2t *ec2t, tmpec2t;

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

	if(ioalloc(ether->port, 0x20, 0, "ec2t") < 0)
		return -1;
	slot = -1;
	for(ec2t = ec2tpcmcia; ec2t->name != nil; ec2t++){
		if((slot = pcmspecial(ec2t->name, ether)) >= 0)
			break;
	}
	if(ec2t->name == nil){
		ec2t = &tmpec2t;
		ec2t->name = nil;
		ec2t->iochecksum = 0;
		for(i = 0; i < ether->nopt; i++){
			if(cistrncmp(ether->opt[i], "id=", 3) == 0){
				ec2t->name = &ether->opt[i][3];
				slot = pcmspecial(ec2t->name, ether);
			}
			else if(cistrncmp(ether->opt[i], "iochecksum", 10) == 0)
				ec2t->iochecksum = 1;
		}
	}
	if(slot < 0){
		iofree(port);
		return -1;
	}

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
	for(i = 0; i < ether->nopt; i++){
		if(cistrcmp(ether->opt[i], "nodummyrr") == 0)
			ctlr->dummyrr = 0;
		else if(cistrncmp(ether->opt[i], "dummyrr=", 8) == 0)
			ctlr->dummyrr = strtol(&ether->opt[i][8], nil, 0);
	}

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
	if(ec2t->iochecksum){
		/*
		 * These cards have the ethernet address in I/O space.
		 * There's a checksum over 8 bytes which sums to 0xFF.
		 */
		for(i = 0; i < 8; i++){
			x = inb(port+0x14+i);
			sum += x;
			buf[i] = (x<<8)|x;
		}
	}
	else{
		memset(buf, 0, sizeof(buf));
		dp8390read(ctlr, buf, 0, sizeof(buf));
		if((buf[0x0E] & 0xFF) == 0x57 && (buf[0x0F] & 0xFF) == 0x57)
			sum = 0xFF;
	}
	if(sum != 0xFF){
		pcmspecialclose(slot);
		iofree(ether->port);
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

void
etherec2tlink(void)
{
	addethercard("EC2T", reset);
}
