#include "all.h"
#include "io.h"
#include "mem.h"

#include "ether.h"

/*
 * Western Digital/Standard Microsystems Corporation cards (WD80[01]3).
 * Configuration code based on that provided by SMC.
 */
enum {					/* 83C584 Bus Interface Controller */
	Msr		= 0x00,		/* Memory Select Register */
	Icr		= 0x01,		/* Interface Configuration Register */
	Iar		= 0x02,		/* I/O Address Register */
	Bio		= 0x03,		/* BIOS ROM Address Register */
	Ear		= 0x03,		/* EEROM Address Register (shared with Bio) */
	Irr		= 0x04,		/* Interrupt Request Register */
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
	ZeroWS16	= (1<<5),	/* zero wait states for 16-bit ops */
	L16en		= (1<<6),	/* enable 16-bit LAN operation */
	M16en		= (1<<7),	/* enable 16-bit memory access */
};

/*
 * Mapping from configuration bits to interrupt level.
 */
static int intrmap[] = {
	9, 3, 5, 7, 10, 11, 15, 4,
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

static void
watch(Ctlr *ctlr)
{
	uchar msr;
	int s;

	s = splhi();
	msr = inb(ctlr->card.port+Msr);
	/*
	 * If the card has reset itself,
	 * start again.
	 */
	if((msr & Menb) == 0){
		delay(100);

		dp8390reset(ctlr);
		etherinit();

		wakeup(&ctlr->tr);
		wakeup(&ctlr->rr);
	}
	splx(s);
}

/*
 * Get configuration parameters, enable memory.
 * There are opportunities here for buckets of code.
 * We'll try to resist.
 */
int
wd8003reset(Ctlr *ctlr)
{
	int i;
	uchar ea[Easize], ic[8], sum;
	ulong wd8003;

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

	ctlr->card.reset = wd8003reset;
	ctlr->card.attach = dp8390attach;
	ctlr->card.read = read;
	ctlr->card.write = write;
	ctlr->card.receive = dp8390receive;
	ctlr->card.transmit = dp8390transmit;
	ctlr->card.intr = dp8390intr;
	ctlr->card.watch = watch;
	ctlr->card.ram = 1;

	wd8003 = ctlr->card.port;
	/*
	 * Look for the interface. We read the LAN address ROM
	 * and validate the checksum - the sum of all 8 bytes
	 * should be 0xFF.
	 * While we're at it, get the (possible) interface chip
	 * registers, we'll use them to check for aliasing later.
	 */
	sum = 0;
	for(i = 0; i < sizeof(ea); i++){
		ea[i] = inb(wd8003+Lar+i);
		sum += ea[i];
		ic[i] = inb(wd8003+i);
	}
	sum += inb(wd8003+Id);
	sum += inb(wd8003+Cksum);
	if(sum != 0xFF)
		return -1;

	/*
	 * Check for old, dumb 8003E, which doesn't have an interface
	 * chip. Only the msr exists out of the 1st eight registers, reads
	 * of the others just alias the 2nd eight registers, the LAN
	 * address ROM. We can check icr, irr and laar against the ethernet
	 * address read above and if they match it's an 8003E (or an
	 * 8003EBT, 8003S, 8003SH or 8003WT, we don't care), in which
	 * case the default irq gets used.
	 */
	if(memcmp(&ea[1], &ic[1], 5) == 0){
		memset(ic, 0, sizeof(ic));
		ic[Msr] = (((ulong)ctlr->card.mem)>>13) & 0x3F;
		ctlr->card.watch = 0;
	}
	else{
		/*
		 * As a final sanity check for the 8013EBT, which doesn't have
		 * the 83C584 interface chip, but has 2 real registers, write Gp2 and if
		 * it reads back the same, it's not an 8013EBT.
		 */
		outb(wd8003+Gp2, 0xAA);
		inb(wd8003+Msr);				/* wiggle bus */
		if(inb(wd8003+Gp2) != 0xAA){
			memset(ic, 0, sizeof(ic));
			ic[Msr] = (((ulong)ctlr->card.mem)>>13) & 0x3F;
			ctlr->card.watch = 0;
		}
		else
			ctlr->card.irq = intrmap[((ic[Irr]>>5) & 0x3)|(ic[Icr] & 0x4)];

		/*
		 * Check if 16-bit card.
		 * If Bit16 is read/write, then we have an 8-bit card.
		 * If Bit16 is set, we're in a 16-bit slot.
		 */
		outb(wd8003+Icr, ic[Icr]^Bit16);
		inb(wd8003+Msr);				/* wiggle bus */
		if((inb(wd8003+Icr) & Bit16) == (ic[Icr] & Bit16)){
			ctlr->card.bit16 = 1;
			ic[Icr] &= ~Bit16;
		}
		outb(wd8003+Icr, ic[Icr]);

		if(ctlr->card.bit16 && (inb(wd8003+Icr) & Bit16) == 0)
			ctlr->card.bit16 = 0;
	}

	ctlr->card.mem = KZERO|((ic[Msr] & 0x3F)<<13);
	if(ctlr->card.bit16)
		ctlr->card.mem |= (ic[Laar] & 0x1F)<<19;
	else
		ctlr->card.mem |= 0x80000;

	if(ic[Icr] & (1<<3))
		ctlr->card.size = 32*1024;
	if(ctlr->card.bit16)
		ctlr->card.size <<= 1;

	/*
	 * Set the DP8390 ring addresses.
	 */
	ctlr->card.dp8390 = wd8003+0x10;
	ctlr->card.tstart = 0;
	ctlr->card.pstart = HOWMANY(sizeof(Enpkt), Dp8390BufSz);
	ctlr->card.pstop = HOWMANY(ctlr->card.size, Dp8390BufSz);

	/*
	 * Enable interface RAM, set interface width.
	 */
	outb(wd8003+Msr, ic[Msr]|Menb);
	if(ctlr->card.bit16)
		outb(wd8003+Laar, ic[Laar]|L16en|M16en|ZeroWS16);

	/*
	 * Finally, init the 8390 and set the
	 * ethernet address.
	 */
	dp8390reset(ctlr);
	if((ctlr->card.ea[0]|ctlr->card.ea[1]|ctlr->card.ea[2]|ctlr->card.ea[3]|ctlr->card.ea[4]|ctlr->card.ea[5]) == 0)
		memmove(ctlr->card.ea, ea, Easize);
	dp8390setea(ctlr);

	return 0;
}
