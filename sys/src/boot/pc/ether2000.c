#include "u.h"
#include "lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

#include "etherif.h"
#include "ether8390.h"

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
	Reset		= 0x1F,		/* offset from I/O base of reset port */
};

typedef struct Ctlr Ctlr;
typedef struct Ctlr {
	Pcidev*	pcidev;
	Ctlr*	next;
	int	active;
} Ctlr;

static Ctlr* ctlrhead;
static Ctlr* ctlrtail;

static struct {
	char*	name;
	int	id;
} ne2000pci[] = {
	{ "Realtek 8029",	(0x8029<<16)|0x10EC, },
	{ "Winbond 89C940",	(0x0940<<16)|0x1050, },
	{ nil },
};

static int nextport;
static ulong portsinuse[16];

static int
inuse(ulong port)
{
	int i;

	for (i = 0; i < nextport; i++)
		if (portsinuse[i] == port)
			return 1;

	/* port is available.  mark it as used now. */
	if (nextport >= nelem(portsinuse))
		return 1;		/* too many ports in use */
	portsinuse[nextport++] = port;
	return 0;
}

static Ctlr*
ne2000match(Ether* edev, int id)
{
	int port;
	Pcidev *p;
	Ctlr *ctlr;

	/*
	 * Any adapter matches if no edev->port is supplied,
	 * otherwise the ports must match.
	 */
	for(ctlr = ctlrhead; ctlr != nil; ctlr = ctlr->next){
		if(ctlr->active)
			continue;
		p = ctlr->pcidev;
		if(((p->did<<16)|p->vid) != id)
			continue;
		port = p->mem[0].bar & ~0x01;
		if(edev->port != 0 && edev->port != port)
			continue;

		/*
		 * It suffices to fill these in,
		 * the rest is gleaned from the card.
		 */
		edev->port = port;
		edev->irq = p->intl;

		ctlr->active = 1;
		return ctlr;
	}

	return nil;
}

static void
ne2000pnp(Ether* edev)
{
	int i, id;
	Pcidev *p;
	Ctlr *ctlr;

	/*
	 * Make a list of all ethernet controllers
	 * if not already done.
	 */
	if(ctlrhead == nil){
		p = nil;
		while(p = pcimatch(p, 0, 0)){
			if(p->ccrb != 0x02 || p->ccru != 0)
				continue;
			ctlr = malloc(sizeof(Ctlr));
			ctlr->pcidev = p;

			if(ctlrhead != nil)
				ctlrtail->next = ctlr;
			else
				ctlrhead = ctlr;
			ctlrtail = ctlr;
		}
	}

	/*
	 * Is it a card with an unrecognised vid+did?
	 * Normally a search is made through all the found controllers
	 * for one which matches any of the known vid+did pairs.
	 * If a vid+did pair is specified a search is made for that
	 * specific controller only.
	 */
	id = 0;
	for(i = 0; i < edev->nopt; i++){
		if(cistrncmp(edev->opt[i], "id=", 3) == 0)
			id = strtol(&edev->opt[i][3], nil, 0);
	}

	if(id != 0)
		ne2000match(edev, id);
	else for(i = 0; ne2000pci[i].name; i++){
		if(ne2000match(edev, ne2000pci[i].id) != nil)
			break;
	}
}

/* may be called more than once */
int
ne2000reset(Ether* ether)
{
	ushort buf[16];
	ulong port;
	Dp8390 *ctlr;
	int i;
	uchar ea[Eaddrlen];

	if(ether->port == 0)
		ne2000pnp(ether);

	/*
	 * port == 0 is a desperate attempt to find something at the
	 * default port.  only allow one of these to be found, and
	 * only if no other card has been found.
	 */
	if(ether->port == 0 && ether->ctlrno > 0)
		return -1;
	/*
	 * Set up the software configuration.
	 * Use defaults for port, irq, mem and size
	 * if not specified.
	 */
	if(ether->port == 0)
		ether->port = 0x300;
	if(ether->irq == 0)
		ether->irq = 2;
	if(ether->mem == 0)
		ether->mem = 0x4000;
	if(ether->size == 0)
		ether->size = 16*1024;
	port = ether->port;

	if(inuse(ether->port))
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

	ctlr->dummyrr = 1;
	for(i = 0; i < ether->nopt; i++){
		if(strcmp(ether->opt[i], "nodummyrr"))
			continue;
		ctlr->dummyrr = 0;
		break;
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
	 * Parallels has buf[0x0E] == 0x00 whereas real hardware
	 * usually has 0x57.
	 */
	dp8390reset(ether);
	memset(buf, 0, sizeof(buf));
	dp8390read(ctlr, buf, 0, sizeof(buf));
	i = buf[0x0E] & 0xFF;
	if((i != 0x00 && i != 0x57) || (buf[0x0F] & 0xFF) != 0x57){
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
