/*
 * National Semiconductor DP8390
 * and SMC 83C90
 * Network Interface Controller.
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"
#include "io.h"
#include "devtab.h"

#include "ether.h"

enum {
	Cr		= 0x00,		/* command register, all pages */

	Stp		= 0x01,		/* stop */
	Sta		= 0x02,		/* start */
	Txp		= 0x04,		/* transmit packet */
	RDMAread	= (1<<3),	/* remote DMA read */
	RDMAwrite	= (2<<3),	/* remote DMA write */
	RDMAsend	= (3<<3),	/* remote DMA send packet */
	RDMAabort	= (4<<3),	/* abort/complete remote DMA */
	Page0		= (0x00<<6),	/* page select */
	Page1		= (0x01<<6),
	Page2		= (0x02<<6),
};

enum {					/* Page 0, read */
	Clda0		= 0x01,		/* current local DMA address 0 */
	Clda1		= 0x02,		/* current local DMA address 1 */
	Bnry		= 0x03,		/* boundary pointer (R/W) */
	Tsr		= 0x04,		/* transmit status register */
	Ncr		= 0x05,		/* number of collisions register */
	Fifo		= 0x06,		/* FIFO */
	Isr		= 0x07,		/* interrupt status register (R/W) */
	Crda0		= 0x08,		/* current remote DMA address 0 */
	Crda1		= 0x08,		/* current remote DMA address 1 */
	Rsr		= 0x0C,		/* receive status register */
	Cntr0		= 0x0D,		/* frame alignment errors */
	Cntr1		= 0x0E,		/* CRC errors */
	Cntr2		= 0x0F,		/* missed packet errors */
};

enum {					/* Page 0, write */
	Pstart		= 0x01,		/* page start register */
	Pstop		= 0x02,		/* page stop register */
	Tpsr		= 0x04,		/* transmit page start address */
	Tbcr0		= 0x05,		/* transmit byte count register 0 */
	Tbcr1		= 0x06,		/* transmit byte count register 1 */
	Rsar0		= 0x08,		/* remote start address register 0 */
	Rsar1		= 0x09,		/* remote start address register 1 */
	Rbcr0		= 0x0A,		/* remote byte count register 0 */
	Rbcr1		= 0x0B,		/* remote byte count register 1 */
	Rcr		= 0x0C,		/* receive configuration register */
	Tcr		= 0x0D,		/* transmit configuration register */
	Dcr		= 0x0E,		/* data configuration register */
	Imr		= 0x0F,		/* interrupt mask */
};

enum {					/* Page 1, read/write */
	Par0		= 0x01,		/* physical address register 0 */
	Curr		= 0x07,		/* current page register */
	Mar0		= 0x08,		/* multicast address register 0 */
};

enum {					/* Interrupt Status Register */
	Prx		= 0x01,		/* packet received */
	Ptx		= 0x02,		/* packet transmitted */
	Rxe		= 0x04,		/* receive error */
	Txe		= 0x08,		/* transmit error */
	Ovw		= 0x10,		/* overwrite warning */
	Cnt		= 0x20,		/* counter overflow */
	Rdc		= 0x40,		/* remote DMA complete */
	Rst		= 0x80,		/* reset status */
};

enum {					/* Interrupt Mask Register */
	Prxe		= 0x01,		/* packet received interrupt enable */
	Ptxe		= 0x02,		/* packet transmitted interrupt enable */
	Rxee		= 0x04,		/* receive error interrupt enable */
	Txee		= 0x08,		/* transmit error interrupt enable */
	Ovwe		= 0x10,		/* overwrite warning interrupt enable */
	Cnte		= 0x20,		/* counter overflow interrupt enable */
	Rdce		= 0x40,		/* DMA complete interrupt enable */
};

enum {					/* Data Configuration register */
	Wts		= 0x01,		/* word transfer select */
	Bos		= 0x02,		/* byte order select */
	Las		= 0x04,		/* long address select */
	Ls		= 0x08,		/* loopback select */
	Arm		= 0x10,		/* auto-initialise remote */
	Ft1		= (0x00<<5),	/* FIFO threshhold select 1 byte/word */
	Ft2		= (0x01<<5),	/* FIFO threshhold select 2 bytes/words */
	Ft4		= (0x02<<5),	/* FIFO threshhold select 4 bytes/words */
	Ft6		= (0x03<<5),	/* FIFO threshhold select 6 bytes/words */
};

enum {					/* Transmit Configuration Register */
	Crc		= 0x01,		/* inhibit CRC */
	Lb		= 0x02,		/* internal loopback */
	Atd		= 0x08,		/* auto transmit disable */
	Ofst		= 0x10,		/* collision offset enable */
};

enum {					/* Transmit Status Register */
	Ptxok		= 0x01,		/* packet transmitted */
	Col		= 0x04,		/* transmit collided */
	Abt		= 0x08,		/* tranmit aborted */
	Crs		= 0x10,		/* carrier sense lost */
	Fu		= 0x20,		/* FIFO underrun */
	Cdh		= 0x40,		/* CD heartbeat */
	Owc		= 0x80,		/* out of window collision */
};

enum {					/* Receive Configuration Register */
	Sep		= 0x01,		/* save errored packets */
	Ar		= 0x02,		/* accept runt packets */
	Ab		= 0x04,		/* accept broadcast */
	Am		= 0x08,		/* accept multicast */
	Pro		= 0x10,		/* promiscuous physical */
	Mon		= 0x20,		/* monitor mode */
};

enum {					/* Receive Status Register */
	Prxok		= 0x01,		/* packet received intact */
	Crce		= 0x02,		/* CRC error */
	Fae		= 0x04,		/* frame alignment error */
	Fo		= 0x08,		/* FIFO overrun */
	Mpa		= 0x10,		/* missed packet */
	Phy		= 0x20,		/* physical/multicast address */
	Dis		= 0x40,		/* receiver disabled */
	Dfr		= 0x80,		/* deferring */
};

typedef struct {
	uchar	status;
	uchar	next;
	uchar	len0;
	uchar	len1;
} Hdr;

typedef struct {
	Hdr;
	uchar	data[Dp8390BufSz-sizeof(Hdr)];
} Buf;

static void
dp8390disable(Ctlr *ctlr)
{
	Board *board = ctlr->board;
	int timo;

	/*
	 * Stop the chip. Set the Stp bit and wait for the chip
	 * to finish whatever was on its tiny mind before it sets
	 * the Rst bit.
	 * We need the timeout because there may not be a real
	 * chip there if this is called when probing for a device
	 * at boot.
	 */
	outb(board->dp8390+Cr, Page0|RDMAabort|Stp);
	for(timo = 10000; (inb(board->dp8390+Isr) & Rst) == 0 && timo; timo--)
			;
}

void
dp8390reset(Ctlr *ctlr)
{
	Board *board = ctlr->board;

	/*
	 * This is the initialisation procedure described
	 * as 'mandatory' in the datasheet.
	 */ 
	dp8390disable(ctlr);
	if(board->bit16)
		outb(board->dp8390+Dcr, Ft4|Ls|Wts);
	else
		outb(board->dp8390+Dcr, Ft4|Ls);

	outb(board->dp8390+Rbcr0, 0);
	outb(board->dp8390+Rbcr1, 0);

	outb(board->dp8390+Rcr, Ab);
	outb(board->dp8390+Tcr, Lb);

	outb(board->dp8390+Bnry, board->pstart);
	outb(board->dp8390+Pstart, board->pstart);
	outb(board->dp8390+Pstop, board->pstop);

	outb(board->dp8390+Isr, 0xFF);
	outb(board->dp8390+Imr, Ovwe|Txee|Rxee|Ptxe|Prxe);

	outb(board->dp8390+Cr, Page1|RDMAabort|Stp);

	/*
	 * Can't initialise ethernet address as we may
	 * not know it yet.
	 */
	outb(board->dp8390+Curr, board->pstart+1);

	/*
	 * Leave the chip initialised,
	 * but in internal loopback mode.
	 */
	outb(board->dp8390+Cr, Page0|RDMAabort|Sta);

	outb(board->dp8390+Tpsr, board->tstart);
}

void
dp8390attach(Ctlr *ctlr)
{
	/*
	 * Enable the chip for transmit/receive.
	 * The init routine leaves the chip in internal
	 * loopback.
	 */
	outb(ctlr->board->dp8390+Tcr, 0);
}

void
dp8390mode(Ctlr *ctlr, int on)
{
	if(on)
		outb(ctlr->board->dp8390+Rcr, Pro|Ab);
	else
		outb(ctlr->board->dp8390+Rcr, Ab);
}

void
dp8390setea(Ctlr *ctlr)
{
	Board *board = ctlr->board;
	uchar cr;
	int i;

	/*
	 * Set the ethernet address into the chip.
	 * Take care to restore the command register
	 * afterwards. We don't care about multicast
	 * addresses as we never set the multicast
	 * enable.
	 */
	cr = inb(board->dp8390+Cr);
	outb(board->dp8390+Cr, Page1|(~Page0 & cr));
	for(i = 0; i < sizeof(ctlr->ea); i++)
		outb(board->dp8390+Par0+i, ctlr->ea[i]);
	outb(board->dp8390+Cr, cr);
}

void*
dp8390read(Ctlr *ctlr, void *to, ulong from, ulong len)
{
	Board *board = ctlr->board;
	uchar cr;
	int timo;

	/*
	 * If the interface has shared memory, just
	 * do a memmove.
	 * In this case, 'from' is an index into the shared memory.
	 */
	if(board->ram){
		memmove(to, (void*)(board->ramstart+from), len);
		return to;
	}

	/*
	 * Read some data at offset 'from' in the board's memory
	 * using the DP8390 remote DMA facility, and place it at
	 * 'to' in main memory, via the I/O data port.
	 */
	cr = inb(board->dp8390+Cr);
	outb(board->dp8390+Cr, Page0|RDMAabort|Sta);
	outb(board->dp8390+Isr, Rdc);

	/*
	 * Set up the remote DMA address and count.
	 */
	if(board->bit16)
		len = ROUNDUP(len, 2);
	outb(board->dp8390+Rbcr0, len & 0xFF);
	outb(board->dp8390+Rbcr1, (len>>8) & 0xFF);
	outb(board->dp8390+Rsar0, from & 0xFF);
	outb(board->dp8390+Rsar1, (from>>8) & 0xFF);

	/*
	 * Start the remote DMA read and suck the data
	 * out of the I/O port.
	 */
	outb(board->dp8390+Cr, Page0|RDMAread|Sta);
	if(board->bit16)
		inss(board->data, to, len/2);
	else
		insb(board->data, to, len);

	/*
	 * Wait for the remote DMA to complete. The timeout
	 * is necessary because we may call this routine on
	 * a non-existent chip during initialisation and, due
	 * to the miracles of the bus, we could get this far
	 * and still be talking to a slot full of nothing.
	 */
	for(timo = 10000; (inb(board->dp8390+Isr) & Rdc) == 0 && timo; timo--)
			;

	outb(board->dp8390+Isr, Rdc);
	outb(board->dp8390+Cr, cr);
	return to;
}

void*
dp8390write(Ctlr *ctlr, ulong to, void *from, ulong len)
{
	Board *board = ctlr->board;
	uchar cr;

	/*
	 * If the interface has shared memory, just
	 * do a memmove.
	 * In this case, 'to' is an index into the shared memory.
	 */
	if(board->ram){
		memmove((void*)(board->ramstart+to), from, len);
		return (void*)to;
	}

	/*
	 * Write some data to offset 'to' in the board's memory
	 * using the DP8390 remote DMA facility, reading it at
	 * 'from' in main memory, via the I/O data port.
	 */
	cr = inb(board->dp8390+Cr);
	outb(board->dp8390+Cr, Page0|RDMAabort|Sta);
	outb(board->dp8390+Isr, Rdc);

	/*
	 * Set up the remote DMA address and count.
	 * This is straight from the datasheet, hence
	 * the initial write to Rbcr0 and Cr.
	 */
	outb(board->dp8390+Rbcr0, 0xFF);
	outb(board->dp8390+Cr, Page0|RDMAread|Sta);

	if(board->bit16)
		len = ROUNDUP(len, 2);
	outb(board->dp8390+Rbcr0, len & 0xFF);
	outb(board->dp8390+Rbcr1, (len>>8) & 0xFF);
	outb(board->dp8390+Rsar0, to & 0xFF);
	outb(board->dp8390+Rsar1, (to>>8) & 0xFF);

	/*
	 * Start the remote DMA write and pump the data
	 * into the I/O port.
	 */
	outb(board->dp8390+Cr, Page0|RDMAwrite|Sta);
	if(board->bit16)
		outss(board->data, from, len/2);
	else
		outsb(board->data, from, len);

	/*
	 * Wait for the remote DMA to finish. We'll need
	 * a timeout here if this ever gets called before
	 * we know there really is a chip there.
	 */
	while((inb(board->dp8390+Isr) & Rdc) == 0)
			;

	outb(board->dp8390+Isr, Rdc);
	outb(board->dp8390+Cr, cr);
	return (void*)to;
}

void
dp8390receive(Ctlr *ctlr)
{
	Board *board = ctlr->board;
	RingBuf *ring;
	uchar bnry, curr, next;
	Hdr hdr;
	ulong data;
	int i, len;

	bnry = inb(board->dp8390+Bnry);
	next = bnry+1;
	if(next >= board->pstop)
		next = board->pstart;

	for(i = 0; ; i++){
		outb(board->dp8390+Cr, Page1|RDMAabort|Sta);
		curr = inb(board->dp8390+Curr);
		outb(board->dp8390+Cr, Page0|RDMAabort|Sta);
		if(next == curr)
			break;

		/*
		 * Hack to keep away from the card's memory while it is receiving
		 * a packet. This is only a problem on the NCR 3170 Safari.
		 *
		 * We peek at the current local DMA registers and, if they are
		 * changing, wait.
		 */
		if(strcmp(arch->id, "NCRD.0") == 0){
			uchar a, b, c;
		
			for(;;delay(10)){
				a = inb(board->dp8390+Clda0);
				b = inb(board->dp8390+Clda0);
				if(a != b)
					continue;
				c = inb(board->dp8390+Clda0);
				if(c != b)
					continue;
				break;
			}
		}

		ctlr->inpackets++;

		data = next*Dp8390BufSz;
		dp8390read(ctlr, &hdr, data, sizeof(Hdr));
		len = ((hdr.len1<<8)|hdr.len0)-4;

		/*
		 * Chip is badly scrogged, reinitialise it.
		 * dp8390reset() calls the disable function.
		 * There's no need to reload the ethernet
		 * address.
		 *
		 * Untested.
		 */
		if(hdr.next < board->pstart || hdr.next >= board->pstop || len < 60){
print("scrogged\n");
			dp8390reset(ctlr);
			dp8390attach(ctlr);
			return;
		}

		ring = &ctlr->rb[ctlr->ri];
		if(ring->owner == Interface){
			ring->len = len;
			data += sizeof(Hdr);
			if((data+len) >= board->pstop*Dp8390BufSz){
				len = board->pstop*Dp8390BufSz - data;
				dp8390read(ctlr, ring->pkt+len, board->pstart*Dp8390BufSz,
					(data+ring->len) - board->pstop*Dp8390BufSz);
			}
			dp8390read(ctlr, ring->pkt, data, len);
			ring->owner = Host;
			ctlr->ri = NEXT(ctlr->ri, ctlr->nrb);
		}

		next = hdr.next;
		bnry = next-1;
		if(bnry < board->pstart)
			bnry = board->pstop-1;
		outb(board->dp8390+Bnry, bnry);
	}
}

void
dp8390transmit(Ctlr *ctlr)
{
	Board *board;
	RingBuf *ring;
	int s;

	s = splhi();
	board = ctlr->board;
	ring = &ctlr->tb[ctlr->ti];
	if(ring->busy == 0 && ring->owner == Interface){
		dp8390write(ctlr, board->tstart*Dp8390BufSz, ring->pkt, ring->len);
		outb(board->dp8390+Tbcr0, ring->len & 0xFF);
		outb(board->dp8390+Tbcr1, (ring->len>>8) & 0xFF);
		outb(board->dp8390+Cr, Page0|RDMAabort|Txp|Sta);
		ring->busy = 1;
	}
	splx(s);
}

void
dp8390intr(Ctlr *ctlr)
{
	Board *board = ctlr->board;
	RingBuf *ring;
	uchar isr;

	/*
	 * While there is something of interest,
	 * clear all the interrupts and process.
	 */
	while(isr = inb(board->dp8390+Isr)){
		outb(board->dp8390+Isr, isr);

		if(isr & Ovw){
			/*
			 * Need to do some work here:
			 *   stop the chip;
			 *   clear out the ring;
			 *   re-enable the chip.
			 * The procedure to put the chip back on
			 * an active network is to first put it
			 * into internal loopback mode, enable it,
			 * then take it out of loopback.
			 *
			 * Untested.
			 */
			ctlr->overflows++;

			dp8390disable(ctlr);
			(*ctlr->board->receive)(ctlr);
			outb(board->dp8390+Tcr, Lb);
			outb(board->dp8390+Cr, Page0|RDMAabort|Sta);
			dp8390attach(ctlr);
		}

		/*
		 * We have received packets.
		 * Take a spin round the ring and
		 * wakeup the kproc.
		 */
		if(isr & (Rxe|Prx)){
			(*ctlr->board->receive)(ctlr);
			wakeup(&ctlr->rr);
		}

		/*
		 * Log errors and successful transmissions.
		 */
		if(isr & Txe)
			ctlr->oerrs++;
		if(isr & Rxe){
			ctlr->frames += inb(board->dp8390+Cntr0);
			ctlr->crcs += inb(board->dp8390+Cntr1);
			ctlr->buffs += inb(board->dp8390+Cntr2);
		}
		if(isr & Ptx)
			ctlr->outpackets++;

		/*
		 * A packet completed transmission, successfully or
		 * not. Start transmission on the next buffered packet,
		 * and wake the output routine.
		 */
		if(isr & (Txe|Ptx)){
			ring = &ctlr->tb[ctlr->ti];
			ring->owner = Host;
			ring->busy = 0;
			ctlr->ti = NEXT(ctlr->ti, ctlr->ntb);
			(*ctlr->board->transmit)(ctlr);
			wakeup(&ctlr->tr);
		}
	}
}
