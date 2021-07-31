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

/*
 * Western Digital/Standard Microsystems Corporation cards (WD80[01]3).
 * Also handles 8216 cards (Elite Ultra).
 * Configuration code based on that provided by SMC a long time ago.
 */
enum {					/* 83C584 Bus Interface Controller */
	Msr		= 0x00,		/* Memory Select Register */
	Icr		= 0x01,		/* Interface Configuration Register */
	Iar		= 0x02,		/* I/O Address Register */
	Bio		= 0x03,		/* BIOS ROM Address Register */
	Ear		= 0x03,		/* EEROM Address Register (shared with Bio) */
	Irr		= 0x04,		/* Interrupt Request Register */
	Hcr		= 0x04,		/* 8216 hardware control */
	Laar		= 0x05,		/* LA Address Register */
	Ijr		= 0x06,		/* Initialisation Jumpers */
	Gp2		= 0x07,		/* General Purpose Data Register */
	Lar		= 0x08,		/* LAN Address Registers */
	Id		= 0x0E,		/* Card ID byte */
	Cksum		= 0x0F,		/* Checksum */
};

enum {					/* Msr */
	Rst		= 0x80,		/* software reset */
	Menb		= 0x40,		/* memory enable */
};

enum {					/* Icr */
	Bit16		= 0x01,		/* 16-bit bus */
	Other		= 0x02,		/* other register access */
	Ir2		= 0x04,		/* IR2 */
	Msz		= 0x08,		/* SRAM size */
	Rla		= 0x10,		/* recall LAN address */
	Rx7		= 0x20,		/* recall all but I/O and LAN address */
	Rio		= 0x40,		/* recall I/O address from EEROM */
	Sto		= 0x80,		/* non-volatile EEROM store */
};

enum {					/* Laar */
	ZeroWS16	= 0x20,		/* zero wait states for 16-bit ops */
	L16en		= 0x40,		/* enable 16-bit LAN operation */
	M16en		= 0x80,		/* enable 16-bit memory access */
};

enum {					/* Ijr */
	Ienable		= 0x01,		/* 8216 interrupt enable */
};

/*
 * Mapping from configuration bits to interrupt level.
 */
static int irq8003[8] = {
	9, 3, 5, 7, 10, 11, 15, 4,
};

static int irq8216[8] = {
	0, 9, 3, 5, 7, 10, 11, 15,
};

static void
reset8003(Ether* ether, uchar ea[Eaddrlen], uchar ic[8])
{
	Dp8390 *ctlr;
	ulong port;

	ctlr = ether->ctlr;
	port = ether->port;

	/*
	 * Check for old, dumb 8003E, which doesn't have an interface
	 * chip. Only Msr exists out of the 1st eight registers, reads
	 * of the others just alias the 2nd eight registers, the LAN
	 * address ROM. Can check Icr, Irr and Laar against the ethernet
	 * address read above and if they match it's an 8003E (or an
	 * 8003EBT, 8003S, 8003SH or 8003WT, doesn't matter), in which
	 * case the default irq gets used.
	 */
	if(memcmp(&ea[1], &ic[1], 5) == 0){
		memset(ic, 0, sizeof(ic));
		ic[Msr] = (((ulong)ether->mem)>>13) & 0x3F;
	}
	else{
		/*
		 * As a final sanity check for the 8013EBT, which doesn't have
		 * the 83C584 interface chip, but has 2 real registers, write Gp2
		 * and if it reads back the same, it's not an 8013EBT.
		 */
		outb(port+Gp2, 0xAA);
		inb(port+Msr);				/* wiggle bus */
		if(inb(port+Gp2) != 0xAA){
			memset(ic, 0, sizeof(ic));
			ic[Msr] = (((ulong)ether->mem)>>13) & 0x3F;
		}
		else
			ether->irq = irq8003[((ic[Irr]>>5) & 0x3)|(ic[Icr] & 0x4)];

		/*
		 * Check if 16-bit card.
		 * If Bit16 is read/write, then it's an 8-bit card.
		 * If Bit16 is set, it's in a 16-bit slot.
		 */
		outb(port+Icr, ic[Icr]^Bit16);
		inb(port+Msr);				/* wiggle bus */
		if((inb(port+Icr) & Bit16) == (ic[Icr] & Bit16)){
			ctlr->width = 2;
			ic[Icr] &= ~Bit16;
		}
		outb(port+Icr, ic[Icr]);

		if(ctlr->width == 2 && (inb(port+Icr) & Bit16) == 0)
			ctlr->width = 1;
	}

	ether->mem = (ulong)KADDR((ic[Msr] & 0x3F)<<13);
	if(ctlr->width == 2)
		ether->mem |= (ic[Laar] & 0x1F)<<19;
	else
		ether->mem |= 0x80000;

	if(ic[Icr] & (1<<3))
		ether->size = 32*1024;
	if(ctlr->width == 2)
		ether->size <<= 1;

	/*
	 * Enable interface RAM, set interface width.
	 */
	outb(port+Msr, ic[Msr]|Menb);
	if(ctlr->width == 2)
		outb(port+Laar, ic[Laar]|L16en|M16en|ZeroWS16);
}

static void
reset8216(Ether* ether, uchar[8])
{
	uchar hcr, irq, x;
	ulong addr, port;
	Dp8390 *ctlr;

	ctlr = ether->ctlr;
	port = ether->port;

	ctlr->width = 2;

	/*
	 * Switch to the alternate register set and retrieve the memory
	 * and irq information.
	 */
	hcr = inb(port+Hcr);
	outb(port+Hcr, 0x80|hcr);
	addr = inb(port+0x0B) & 0xFF;
	irq = inb(port+0x0D);
	outb(port+Hcr, hcr);

	ether->mem = (ulong)KADDR(0xC0000+((((addr>>2) & 0x30)|(addr & 0x0F))<<13));
	ether->size = 8192*(1<<((addr>>4) & 0x03));
	ether->irq = irq8216[((irq>>4) & 0x04)|((irq>>2) & 0x03)];

	/*
	 * Enable interface RAM, set interface width, and enable interrupts.
	 */
	x = inb(port+Msr) & ~Rst;
	outb(port+Msr, Menb|x);
	x = inb(port+Laar);
	outb(port+Laar, M16en|x);
	outb(port+Ijr, Ienable);
}

/*
 * Get configuration parameters, enable memory.
 * There are opportunities here for buckets of code, try to resist.
 */
static int
reset(Ether* ether)
{
	int i;
	uchar ea[Eaddrlen], ic[8], id, nullea[Eaddrlen], sum;
	ulong port;
	Dp8390 *ctlr;

	/*
	 * Set up the software configuration.
	 * Use defaults for port, irq, mem and size if not specified.
	 * Defaults are set for the dumb 8003E which can't be
	 * autoconfigured.
	 */
	if(ether->port == 0)
		ether->port = 0x280;
	if(ether->irq == 0)
		ether->irq = 3;
	if(ether->mem == 0)
		ether->mem = 0xD0000;
	if(ether->size == 0)
		ether->size = 8*1024;
	if(ioalloc(ether->port, 0x20, 0, "wd8003") < 0)
		return -1;

	/*
	 * Look for the interface. Read the LAN address ROM
	 * and validate the checksum - the sum of all 8 bytes
	 * should be 0xFF.
	 * At the same time, get the (possible) interface chip
	 * registers, they'll be used later to check for aliasing.
	 */
	port = ether->port;
	sum = 0;
	for(i = 0; i < sizeof(ea); i++){
		ea[i] = inb(port+Lar+i);
		sum += ea[i];
		ic[i] = inb(port+i);
	}
	id = inb(port+Id);
	sum += id;
	sum += inb(port+Cksum);
	if(sum != 0xFF){
		iofree(ether->port);
		return -1;
	}

	ether->ctlr = malloc(sizeof(Dp8390));
	ctlr = ether->ctlr;
	ctlr->ram = 1;

	if((id & 0xFE) == 0x2A)
		reset8216(ether, ic);
	else
		reset8003(ether, ea, ic);

	/*
	 * Set the DP8390 ring addresses.
	 */
	ctlr->port = port+0x10;
	ctlr->tstart = 0;
	ctlr->pstart = HOWMANY(sizeof(Etherpkt), Dp8390BufSz);
	ctlr->pstop = HOWMANY(ether->size, Dp8390BufSz);

	/*
	 * Finally, init the 8390, set the ethernet address
	 * and claim the memory used.
	 */
	dp8390reset(ether);
	memset(nullea, 0, Eaddrlen);
	if(memcmp(nullea, ether->ea, Eaddrlen) == 0){
		for(i = 0; i < sizeof(ether->ea); i++)
			ether->ea[i] = ea[i];
	}
	dp8390setea(ether);

	if(umbrwmalloc(PADDR(ether->mem), ether->size, 0) == 0)
		print("ether8003: warning - 0x%luX unavailable\n",
			PADDR(ether->mem));

	return 0;
}

void
ether8003link(void)
{
	addethercard("WD8003", reset);
}
