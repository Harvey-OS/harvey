/*
 * National Semiconductor DP8390
 * and SMC 83C90
 * Network Interface Controller.
 */
#include "u.h"
#include "lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

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
	Ps0		= 0x40,		/* page select */
	Ps1		= 0x80,		/* page select */
	Page0		= 0x00,
	Page1		= Ps0,
	Page2		= Ps1,
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
	Crda1		= 0x09,		/* current remote DMA address 1 */
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

static void
dp8390disable(Ctlr *ctlr)
{
	ulong dp8390;
	int timo;

	dp8390 = ctlr->card.dp8390;
	/*
	 * Stop the chip. Set the Stp bit and wait for the chip
	 * to finish whatever was on its tiny mind before it sets
	 * the Rst bit.
	 * We need the timeout because there may not be a real
	 * chip there if this is called when probing for a device
	 * at boot.
	 */
	dp8390outb(dp8390+Cr, Page0|RDMAabort|Stp);
	dp8390outb(dp8390+Rbcr0, 0);
	dp8390outb(dp8390+Rbcr1, 0);
	for(timo = 10000; (dp8390inb(dp8390+Isr) & Rst) == 0 && timo; timo--)
			;
}

static void
dp8390ring(Ctlr *ctlr)
{
	ulong dp8390;

	dp8390 = ctlr->card.dp8390;
	dp8390outb(dp8390+Pstart, ctlr->card.pstart);
	dp8390outb(dp8390+Pstop, ctlr->card.pstop);
	dp8390outb(dp8390+Bnry, ctlr->card.pstop-1);

	dp8390outb(dp8390+Cr, Page1|RDMAabort|Stp);
	dp8390outb(dp8390+Curr, ctlr->card.pstart);
	dp8390outb(dp8390+Cr, Page0|RDMAabort|Stp);

	ctlr->card.nxtpkt = ctlr->card.pstart;
}

void
dp8390reset(Ctlr *ctlr)
{
	ulong dp8390;

	dp8390 = ctlr->card.dp8390;
	/*
	 * This is the initialisation procedure described
	 * as 'mandatory' in the datasheet, with references
	 * to the 3Com503 technical reference manual.
	 */ 
	dp8390disable(ctlr);
	if(ctlr->card.bit16)
		dp8390outb(dp8390+Dcr, Ft4|Ls|Wts);
	else
		dp8390outb(dp8390+Dcr, Ft4|Ls);

	dp8390outb(dp8390+Rbcr0, 0);
	dp8390outb(dp8390+Rbcr1, 0);

	dp8390outb(dp8390+Tcr, 0);
	dp8390outb(dp8390+Rcr, Mon);

	/*
	 * Init the ring hardware and software ring pointers.
	 * Can't initialise ethernet address as we may not know
	 * it yet.
	 */
	dp8390ring(ctlr);
	dp8390outb(dp8390+Tpsr, ctlr->card.tstart);

	dp8390outb(dp8390+Isr, 0xFF);
	dp8390outb(dp8390+Imr, Cnte|Ovwe|Txee|Rxee|Ptxe|Prxe);

	/*
	 * Leave the chip initialised,
	 * but in monitor mode.
	 */
	dp8390outb(dp8390+Cr, Page0|RDMAabort|Sta);
}

void
dp8390attach(Ctlr *ctlr)
{
	/*
	 * Enable the chip for transmit/receive.
	 * The init routine leaves the chip in monitor
	 * mode. Clear the missed-packet counter, it
	 * increments while in monitor mode.
	 */
	dp8390outb(ctlr->card.dp8390+Rcr, Ab);
	dp8390inb(ctlr->card.dp8390+Cntr2);
}

void
dp8390mode(Ctlr *ctlr, int on)
{
	/*
	 * Set/reset promiscuous mode.
	 */
	if(on)
		dp8390outb(ctlr->card.dp8390+Rcr, Pro|Ab);
	else
		dp8390outb(ctlr->card.dp8390+Rcr, Ab);
}

void
dp8390setea(Ctlr *ctlr)
{
	ulong dp8390;
	uchar cr;
	int i;

	dp8390 = ctlr->card.dp8390;
	/*
	 * Set the ethernet address into the chip.
	 * Take care to restore the command register
	 * afterwards. We don't care about multicast
	 * addresses as we never set the multicast
	 * enable.
	 */
	cr = dp8390inb(dp8390+Cr) & ~Txp;
	dp8390outb(dp8390+Cr, Page1|(~(Ps1|Ps0) & cr));
	for(i = 0; i < sizeof(ctlr->card.ea); i++)
		dp8390outb(dp8390+Par0+i, ctlr->card.ea[i]);
	dp8390outb(dp8390+Cr, cr);
}

void
dp8390getea(Ctlr *ctlr)
{
	ulong dp8390;
	uchar cr;
	int i;

	dp8390 = ctlr->card.dp8390;
	/*
	 * Set the ethernet address into the chip.
	 * Take care to restore the command register
	 * afterwards. We don't care about multicast
	 * addresses as we never set the multicast
	 * enable.
	 */
	cr = dp8390inb(dp8390+Cr) & ~Txp;
	dp8390outb(dp8390+Cr, Page1|(~(Ps1|Ps0) & cr));
	for(i = 0; i < sizeof(ctlr->card.ea); i++)
		ctlr->card.ea[i] = dp8390inb(dp8390+Par0+i);
	dp8390outb(dp8390+Cr, cr);
}

void*
dp8390read(Ctlr *ctlr, void *to, ulong from, ulong len)
{
	ulong dp8390;
	uchar cr;
	int timo;

	dp8390 = ctlr->card.dp8390;
	/*
	 * Read some data at offset 'from' in the card's memory
	 * using the DP8390 remote DMA facility, and place it at
	 * 'to' in main memory, via the I/O data port.
	 */
	cr = dp8390inb(dp8390+Cr) & ~Txp;
	dp8390outb(dp8390+Cr, Page0|RDMAabort|Sta);
	dp8390outb(dp8390+Isr, Rdc);

	/*
	 * Set up the remote DMA address and count.
	 */
	if(ctlr->card.bit16)
		len = ROUNDUP(len, 2);
	dp8390outb(dp8390+Rbcr0, len & 0xFF);
	dp8390outb(dp8390+Rbcr1, (len>>8) & 0xFF);
	dp8390outb(dp8390+Rsar0, from & 0xFF);
	dp8390outb(dp8390+Rsar1, (from>>8) & 0xFF);

	/*
	 * Start the remote DMA read and suck the data
	 * out of the I/O port.
	 */
	dp8390outb(dp8390+Cr, Page0|RDMAread|Sta);
	if(ctlr->card.bit16)
		inss(ctlr->card.data, to, len/2);
	else
		insb(ctlr->card.data, to, len);

	/*
	 * Wait for the remote DMA to complete. The timeout
	 * is necessary because we may call this routine on
	 * a non-existent chip during initialisation and, due
	 * to the miracles of the bus, we could get this far
	 * and still be talking to a slot full of nothing.
	 */
	for(timo = 10000; (dp8390inb(dp8390+Isr) & Rdc) == 0 && timo; timo--)
			;

	dp8390outb(dp8390+Isr, Rdc);
	dp8390outb(dp8390+Cr, cr);
	return to;
}

void*
dp8390write(Ctlr *ctlr, ulong to, void *from, ulong len)
{
	ulong dp8390, crda;
	uchar cr;

	dp8390 = ctlr->card.dp8390;
	/*
	 * Write some data to offset 'to' in the card's memory
	 * using the DP8390 remote DMA facility, reading it at
	 * 'from' in main memory, via the I/O data port.
	 */
	cr = dp8390inb(dp8390+Cr) & ~Txp;
	dp8390outb(dp8390+Cr, Page0|RDMAabort|Sta);
	dp8390outb(dp8390+Isr, Rdc);

	if(ctlr->card.bit16)
		len = ROUNDUP(len, 2);

	/*
	 * Set up the remote DMA address and count.
	 * This is straight from the DP8390[12D] datasheet, hence
	 * the initial set up for read.
	 */
	crda = to-1-ctlr->card.bit16;
	dp8390outb(dp8390+Rbcr0, (len+1+ctlr->card.bit16) & 0xFF);
	dp8390outb(dp8390+Rbcr1, ((len+1+ctlr->card.bit16)>>8) & 0xFF);
	dp8390outb(dp8390+Rsar0, crda & 0xFF);
	dp8390outb(dp8390+Rsar1, (crda>>8) & 0xFF);
	dp8390outb(dp8390+Cr, Page0|RDMAread|Sta);

	for(;;){
		crda = dp8390inb(dp8390+Crda0);
		crda |= dp8390inb(dp8390+Crda1)<<8;
		if(crda == to){
			/*
			 * Start the remote DMA write and make sure
			 * the registers are correct.
			 */
			dp8390outb(dp8390+Cr, Page0|RDMAwrite|Sta);

			crda = dp8390inb(dp8390+Crda0);
			crda |= dp8390inb(dp8390+Crda1)<<8;
			if(crda != to)
				panic("crda write %d to %d\n", crda, to);

			break;
		}
	}

	/*
	 * Pump the data into the I/O port.
	 */
	if(ctlr->card.bit16)
		outss(ctlr->card.data, from, len/2);
	else
		outsb(ctlr->card.data, from, len);

	/*
	 * Wait for the remote DMA to finish. We'll need
	 * a timeout here if this ever gets called before
	 * we know there really is a chip there.
	 */
	while((dp8390inb(dp8390+Isr) & Rdc) == 0)
			;

	dp8390outb(dp8390+Isr, Rdc);
	dp8390outb(dp8390+Cr, cr);
	return (void*)to;
}

static uchar
getcurr(ulong dp8390)
{
	uchar cr, curr;

	cr = dp8390inb(dp8390+Cr) & ~Txp;
	dp8390outb(dp8390+Cr, Page1|(~(Ps1|Ps0) & cr));
	curr = dp8390inb(dp8390+Curr);
	dp8390outb(dp8390+Cr, cr);
	return curr;
}

void
dp8390receive(Ctlr *ctlr)
{
	RingBuf *ring;
	uchar curr, len1, *pkt;
	Hdr hdr;
	ulong dp8390, data, len;

	dp8390 = ctlr->card.dp8390;
	for(curr = getcurr(dp8390); ctlr->card.nxtpkt != curr; curr = getcurr(dp8390)){
		ctlr->inpackets++;

		data = ctlr->card.nxtpkt*Dp8390BufSz;
		(*ctlr->card.read)(ctlr, &hdr, data, sizeof(Hdr));

		/*
		 * Don't believe the upper byte count, work it
		 * out from the software next-page pointer and
		 * the current next-page pointer.
		 */
		if(hdr.next > ctlr->card.nxtpkt)
			len1 = hdr.next - ctlr->card.nxtpkt - 1;
		else
			len1 = (ctlr->card.pstop-ctlr->card.nxtpkt) + (hdr.next-ctlr->card.pstart) - 1;
		if(hdr.len0 > (Dp8390BufSz-sizeof(Hdr)))
			len1--;

		len = ((len1<<8)|hdr.len0)-4;

		/*
		 * Chip is badly scrogged, reinitialise the ring.
		 */
		if(hdr.next < ctlr->card.pstart || hdr.next >= ctlr->card.pstop
		  || len < 60 || len > sizeof(Etherpkt)){
			print("H#%2.2ux#%2.2ux#%2.2ux#%2.2ux,%d|",
				hdr.status, hdr.next, hdr.len0, hdr.len1, len);
			dp8390outb(dp8390+Cr, Page0|RDMAabort|Stp);
			dp8390ring(ctlr);
			dp8390outb(dp8390+Cr, Page0|RDMAabort|Sta);
			return;
		}

		/*
		 * If it's a good packet and we have a place to put it,
		 * read it in to the software ring.
		 * If the packet wraps round the hardware ring, read it
		 * in two pieces.
		 */
		ring = &ctlr->rb[ctlr->ri];
		if((hdr.status & (Fo|Fae|Crce|Prxok)) == Prxok && ring->owner == Interface){
			pkt = ring->pkt;
			data += sizeof(Hdr);
			ring->len = len;

			if((data+len) >= ctlr->card.pstop*Dp8390BufSz){
				ulong count = ctlr->card.pstop*Dp8390BufSz - data;

				(*ctlr->card.read)(ctlr, pkt, data, count);
				pkt += count;
				data = ctlr->card.pstart*Dp8390BufSz;
				len -= count;
			}
			if(len)
				(*ctlr->card.read)(ctlr, pkt, data, len);

			ring->owner = Host;
			ctlr->ri = NEXT(ctlr->ri, ctlr->nrb);
		}

		/*
		 * Finished woth this packet, update the
		 * hardware and software ring pointers.
		 */
		ctlr->card.nxtpkt = hdr.next;

		hdr.next--;
		if(hdr.next < ctlr->card.pstart)
			hdr.next = ctlr->card.pstop-1;
		dp8390outb(dp8390+Bnry, hdr.next);
	}
}

/*
 * Initiate a transmission. Must be called splhi().
 */
void
dp8390transmit(Ctlr *ctlr)
{
	ulong dp8390;
	RingBuf *ring;

	dp8390 = ctlr->card.dp8390;
	ring = &ctlr->tb[ctlr->ti];
	if(ctlr->tbusy == 0 && ring->owner == Interface){

		ctlr->tbusy = 1;

		(*ctlr->card.write)(ctlr, ctlr->card.tstart*Dp8390BufSz, ring->pkt, ring->len);

		dp8390outb(dp8390+Tbcr0, ring->len & 0xFF);
		dp8390outb(dp8390+Tbcr1, (ring->len>>8) & 0xFF);
		dp8390outb(dp8390+Cr, Page0|RDMAabort|Txp|Sta);
	}
}

void
dp8390overflow(Ctlr *ctlr)
{
	ulong dp8390;
	uchar txp;
	int resend;

	dp8390 = ctlr->card.dp8390;
	/*
	 * The following procedure is taken from the DP8390[12D] datasheet,
	 * it seems pretty adamant that this is what has to be done.
	 */
	txp = dp8390inb(dp8390+Cr) & Txp;
	dp8390outb(dp8390+Cr, Page0|RDMAabort|Stp);
	delay(2);
	dp8390outb(dp8390+Rbcr0, 0);
	dp8390outb(dp8390+Rbcr1, 0);

	resend = 0;
	if(txp && (dp8390inb(dp8390+Isr) & (Txe|Ptx)) == 0)
		resend = 1;

	dp8390outb(dp8390+Tcr, Lb);
	dp8390outb(dp8390+Cr, Page0|RDMAabort|Sta);
	(*ctlr->card.receive)(ctlr);
	dp8390outb(dp8390+Isr, Ovw);
	dp8390outb(dp8390+Tcr, 0);

	if(resend)
		dp8390outb(dp8390+Cr, Page0|RDMAabort|Txp|Sta);
}

void
dp8390intr(Ureg*, Ctlr *ctlr)
{
	ulong dp8390;
	RingBuf *ring;
	uchar isr, r;

	dp8390 = ctlr->card.dp8390;
	/*
	 * While there is something of interest,
	 * clear all the interrupts and process.
	 */
	dp8390outb(dp8390+Imr, 0x00);
	while(isr = dp8390inb(dp8390+Isr)){

		if(isr & Ovw){
			if(ctlr->card.overflow)
				(*ctlr->card.overflow)(ctlr);
			dp8390outb(dp8390+Isr, Ovw);
			ctlr->overflows++;
		}

		/*
		 * We have received packets.
		 * Take a spin round the ring and
		 * wakeup the kproc.
		 */
		if(isr & (Rxe|Prx)){
			(*ctlr->card.receive)(ctlr);
			dp8390outb(dp8390+Isr, Rxe|Prx);
		}

		/*
		 * A packet completed transmission, successfully or
		 * not. Start transmission on the next buffered packet,
		 * and wake the output routine.
		 */
		if(isr & (Txe|Ptx)){
			r = dp8390inb(dp8390+Tsr);
			if(isr & Txe){
				if((r & (Cdh|Fu|Crs|Abt)))
					print("Tsr#%2.2ux|", r);
				ctlr->oerrs++;
			}

			if(isr & Ptx)
				ctlr->outpackets++;

			dp8390outb(dp8390+Isr, Txe|Ptx);

			ring = &ctlr->tb[ctlr->ti];
			ring->owner = Host;
			ctlr->tbusy = 0;
			ctlr->ti = NEXT(ctlr->ti, ctlr->ntb);
			(*ctlr->card.transmit)(ctlr);
		}

		if(isr & Cnt){
			ctlr->frames += dp8390inb(dp8390+Cntr0);
			ctlr->crcs += dp8390inb(dp8390+Cntr1);
			ctlr->buffs += dp8390inb(dp8390+Cntr2);
			dp8390outb(dp8390+Isr, Cnt);
		}
	}
	dp8390outb(dp8390+Imr, Cnte|Ovwe|Txee|Rxee|Ptxe|Prxe);
}
