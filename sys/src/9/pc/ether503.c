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
 * 3Com 503 (EtherLink II).
 * This driver doesn't work under load, the card gets blasted by
 * overwrite-warning interrupts and eventually just hangs. Always clearing
 * the Ovw bit in the DP8390 Isr along with the Rxe and Prx bits stops
 * the overwrite-warning interrupts, but the card eventually hangs anyway.
 * I really don't understand why this card doesn't work,
 * there is plenty of evidence that other systems manage to use it.
 * The only good thing about this driver is that both shared
 * memory and polled I/O work (given the above restraints), although
 * the polled I/O should be tuned.
 */
enum {					/* Gate Array Registers */
	Pstr		= 0x400,	/* Page Start Register */
	Pspr		= 0x401,	/* Page Stop Register */
	Dqtr		= 0x402,	/* Drq Timer Register */
	Bcfr		= 0x403,	/* Base Configuration Register */
	Pcfr		= 0x404,	/* EPROM Configuration Register */
	Gacfr		= 0x405,	/* GA Configuration Register */
	Ctrl		= 0x406,	/* Control Register */
	Streg		= 0x407,	/* Status Register */
	Idcfr		= 0x408,	/* Interrupt/DMA Configuration Register */
	Damsb		= 0x409,	/* DMA Address Register MSB */
	Dalsb		= 0x40A,	/* DMA Address Register LSB */
	Vptr2		= 0x40B,	/* Vector Pointer Register 2 */
	Vptr1		= 0x40C,	/* Vector Pointer Register 1 */
	Vptr0		= 0x40D,	/* Vector Pointer Register 0 */
	Rfmsb		= 0x40E,	/* Register File Access MSB */
	Rflsb		= 0x40F,	/* Register File Access LSB */
};

enum {					/* Pcfr */
	C8xxx		= 0x10,		/* memory address 0xC8000 */
	CCxxx		= 0x20,		/* memory address 0xCC000 */
	D8xxx		= 0x40,		/* memory address 0xD8000 */
	DCxxx		= 0x80,		/* memory address 0xDC000 */
};

enum {					/* Gacfr */
	Mbs0		= 0x01,		/* memory bank select 0 */
	Mbs1		= 0x02,		/* memory bank select 1 */
	Mbs2		= 0x04,		/* memory bank select 2 */
	Rsel		= 0x08,		/* RAM select */
	Test		= 0x10,		/* test */
	Ows		= 0x20,		/* 0 wait state */
	Tcm		= 0x40,		/* terminal count mask */
	Nim		= 0x80,		/* MIC interrupt mask */
};

enum {					/* Ctrl */
	Rst		= 0x01,		/* software reset */
	Xsel		= 0x02,		/* transceiver select (BNC = 1, AUI = 0) */
	Ealo		= 0x04,		/* ethernet address low */
	Eahi		= 0x08,		/* ethernet address high */
	Share		= 0x10,		/* interrupt share */
	Dbsel		= 0x20,		/* double buffer select */
	Ddir		= 0x40,		/* DMA direction */
	Start		= 0x80,		/* DMA start */
};

enum {					/* Streg */
	Rev0		= 0x01,		/* gate array revision bit 0 */
	Rev1		= 0x02,		/* gate array revision bit 1 */
	Rev2		= 0x04,		/* gate array revision bit 2 */
	Dip		= 0x08,		/* DMA in progress */
	Dtc		= 0x10,		/* DMA terminal count */
	Oflw		= 0x20,		/* overflow */
	Uflw		= 0x40,		/* underflow */
	Dprdy		= 0x80,		/* data port ready */
};

enum {					/* Idcfr */
	Drq1		= 0x01,		/* DMA request 1 */
	Drq2		= 0x02,		/* DMA request 2 */
	Drq3		= 0x04,		/* DMA request 3 */
	Irq2		= 0x10,		/* interrupt request 2 */
	Irq3		= 0x20,		/* interrupt request 3 */
	Irq4		= 0x40,		/* interrupt request 4 */
	Irq5		= 0x80,		/* interrupt request 5 */
};

enum {
	RamOffset	= 0x2000,	/* where the card sees the shared memory */
};

typedef struct {
	ulong	io;			/* I/O base address */
	uchar	irq;			/* interrupt level */
	uchar	bnc;			/* use BNC connector */
} Table;

/*
 * The only configuration info we can get from the card is the
 * I/O base address and the shared-memory address. So let's make
 * a little hard-wired configuration table.
 */
static Table table[] = {
	{ 0x300, Irq2, 0, },		/* external transceiver */
	{ 0x310, Irq3, 0, },
	{ 0x330, Irq4, 0, },
	{ 0x350, Irq5, 0, },

	{ 0x250, Irq2, Xsel, },		/* BNC */
	{ 0x280, Irq3, Xsel, },
	{ 0x2A0, Irq4, Xsel, },
	{ 0x2E0, Irq5, Xsel, },

	{ 0 },
};


static int
reset(Ctlr *ctlr)
{
	Table *tp;
	int i;
	uchar cfr;

	/*
	 * The Gate Array Ctrl register bits <3,2> determine what is
	 * mapped at the I/O base address. The options are the DP8390,
	 * PROM bytes 0-15 or PROM bytes 16-31.
	 * At power-on, the I/O base address contains PROM bytes 16-31.
	 */
	for(tp = table; ctlr->card.io = tp->io; tp++){
		/*
		 * Toggle the software reset bit, this should initialise the
		 * Ctrl register to 0x0A (Eahi|Xsel).
		 * Hopefully there is no other card at the gate-array address.
		 */
		outb(ctlr->card.io+Ctrl, Rst);
		delay(1);
		outb(ctlr->card.io+Ctrl, 0);
		if(inb(ctlr->card.io+Ctrl) == (Eahi|Xsel))
			break;
	}
	if(ctlr->card.io == 0)
		return -1;

	/*
	 * Map PROM bytes 0-15 to the I/O base address
	 * and read the ethernet address.
	 * When done, map the DP8390 at the I/O base address
	 * and set the transceiver type and irq.
	 */
	outb(ctlr->card.io+Ctrl, Ealo|Xsel);
	for(i = 0; i < sizeof(ctlr->ea); i++)
		ctlr->ea[i] = inb(ctlr->card.io+i);

	outb(ctlr->card.io+Gacfr, Nim|Tcm);

	outb(ctlr->card.io+Ctrl, 0|tp->bnc);
	outb(ctlr->card.io+Idcfr, tp->irq);

	if(tp->irq == Irq2)
		ctlr->card.irq = 2;
	else if(tp->irq == Irq3)
		ctlr->card.irq = 3;
	else if(tp->irq == Irq4)
		ctlr->card.irq = 4;
	else if(tp->irq == Irq5)
		ctlr->card.irq = 5;
	else
		return -1;

	/*
	 * Try to configure the shared memory.
	 * Assume this is an 8K card.
	 */
	ctlr->card.ram = 1;
	cfr = inb(ctlr->card.io+Pcfr);
	if(cfr & C8xxx)
		ctlr->card.ramstart = KZERO|0xC8000;
	else if(cfr & CCxxx)
		ctlr->card.ramstart = KZERO|0xCC000;
	else if(cfr & D8xxx)
		ctlr->card.ramstart = KZERO|0xD8000;
	else if(cfr & DCxxx)
		ctlr->card.ramstart = KZERO|0xDC000;
	else
		ctlr->card.ram = 0;

	ctlr->card.ramstop = ctlr->card.ramstart + 8*1024;

	/*
	 * Finish initialising the gate-array.
	 * The Pstr and Pspr registers must be the same as those
	 * given to the DP8390.
	 * Gacfr is set to enable shared memory and block DMA
	 * interrupts.
	 */
	outb(ctlr->card.io+Pstr, ctlr->card.pstart);
	outb(ctlr->card.io+Pspr, ctlr->card.pstop);

	outb(ctlr->card.io+Dqtr, 8);
	outb(ctlr->card.io+Damsb, ctlr->card.tstart);
	outb(ctlr->card.io+Dalsb, 0);

	outb(ctlr->card.io+Vptr2, 0xFF);
	outb(ctlr->card.io+Vptr1, 0xFF);
	outb(ctlr->card.io+Vptr0, 0x00);

	/*
	 * Finally, init the 8390, set the ethernet address
	 * and turn on the shared memory if any.
	 */
	ctlr->card.dp8390 = ctlr->card.io;
	dp8390reset(ctlr);
	dp8390setea(ctlr);

	if(ctlr->card.ram)
		outb(ctlr->card.io+Gacfr, Tcm|Rsel|Mbs0);
	else
		outb(ctlr->card.io+Gacfr, Tcm);

	return 0;
}

static void*
read(Ctlr *ctlr, void *to, ulong from, ulong len)
{
	uchar *addr, ctrl;

	/*
	 * If the interface has shared memory, just do a memmove.
	 * In this case, 'from' is an index into the shared memory.
	 * Due to 'hardware considerations', the memory is seen by
	 * the adapter at RamOffset, so we have to adjust.
	 */
	if(ctlr->card.ram)
		return memmove(to, (void*)(ctlr->card.ramstart+from-RamOffset), len);

	/*
	 * Polled I/O. This should be tuned.
	 * Set up the dma registers and pump the fifo.
	 */
	outb(ctlr->card.io+Dalsb, from & 0xFF);
	outb(ctlr->card.io+Damsb, (from>>8) & 0xFF);
	ctrl = inb(ctlr->card.io+Ctrl);
	outb(ctlr->card.io+Ctrl, Start|ctrl);
	addr = to;
	while(len){
		/*
		 * The fifo is 8 bytes deep, we have to wait
		 * until it's ready before stuffing it.
		 * The fifo is in its default configuration,
		 * so we can do word accesses.
		 */
		while((inb(ctlr->card.io+Streg) & Dprdy) == 0)
			;
		if(len >= 8){
			inss(ctlr->card.io+Rfmsb, addr, 4);
			addr += 8;
			len -= 8;
		}
		else{
			insb(ctlr->card.io+Rfmsb, addr, len);
			break;
		}
	}
	outb(ctlr->card.io+Ctrl, ctrl);
	return to;
}

static void*
write(Ctlr *ctlr, ulong to, void *from, ulong len)
{
	uchar *addr, ctrl;

	/*
	 * See the comments in 'read()' above.
	 */
	if(ctlr->card.ram)
		return memmove((void*)(ctlr->card.ramstart+to-RamOffset), from, len);

	outb(ctlr->card.io+Dalsb, to & 0xFF);
	outb(ctlr->card.io+Damsb, (to>>8) & 0xFF);
	ctrl = inb(ctlr->card.io+Ctrl);
	outb(ctlr->card.io+Ctrl, Start|Ddir|ctrl);
	addr = from;
	while(len){
		while((inb(ctlr->card.io+Streg) & Dprdy) == 0)
			;
		if(len >= 8){
			outss(ctlr->card.io+Rfmsb, addr, 4);
			addr += 8;
			len -= 8;
		}
		else{
			outsb(ctlr->card.io+Rfmsb, addr, len);
			break;
		}
	}
	outb(ctlr->card.io+Ctrl, ctrl);
	return (void*)to;
}

Card ether503 = {
	"3Com503",			/* ident */

	reset,				/* reset */
	0,				/* init */
	dp8390attach,			/* attach */
	dp8390mode,			/* mode */

	read,				/* read */
	write,				/* write */

	dp8390receive,			/* receive */
	dp8390transmit,			/* transmit */
	dp8390intr,			/* interrupt */
	dp8390watch,			/* watch */
	dp8390overflow,			/* overflow */

	0,				/* io */
	0,				/* irq */
	0,				/* bit16 */

	0,				/* ram */
	0,				/* ramstart */
	0,				/* ramstop */

	0,				/* dp8390 */
	0,				/* data */
	0,				/* software bndry */
	RamOffset/Dp8390BufSz,		/* tstart */
	(RamOffset+0x600)/Dp8390BufSz,	/* pstart */
	(RamOffset+8*1024)/Dp8390BufSz,	/* pstop */
};

void
ether503link(void)
{
	addethercard("3C503",  ccc503reset);
}
