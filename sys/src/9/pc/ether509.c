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
	IDport		= 0x0100,	/* anywhere between 0x0100 and 0x01F0 */

					/* Commands */
	SelectWindow	= 0x01,		/* SelectWindow command */
	StartCoax	= 0x02,		/* Start Coaxial Transceiver */
	RxDisable	= 0x03,		/* RX Disable */
	RxEnable	= 0x04,		/* RX Enable */
	RxDiscard	= 0x08,		/* RX Discard Top Packet */
	TxEnable	= 0x09,		/* TX Enable */
	TxDisable	= 0x0A,		/* TX Disable */
	TxReset		= 0x0B,		/* TX Reset */
	AckIntr		= 0x0D,		/* Acknowledge Interrupt */
	SetIntrMask	= 0x0E,		/* Set Interrupt Mask */
	SetReadZeroMask	= 0x0F,		/* Set Read Zero Mask */
	SetRxFilter	= 0x10,		/* Set RX Filter */
	SetTxAvailable	= 0x12,		/* Set TX Available Threshold */

					/* RX Filter Command Bits */
	MyEtherAddr	= 0x01,		/* Individual address */
	Multicast	= 0x02,		/* Group (multicast) addresses */
	Broadcast	= 0x04,		/* Broadcast address */
	Promiscuous	= 0x08,		/* All addresses (promiscuous mode */

					/* Window Register Offsets */
	Command		= 0x0E,		/* all windows */
	Status		= 0x0E,

	EEPROMdata	= 0x0C,		/* window 0 */
	EEPROMcmd	= 0x0A,
	ResourceConfig	= 0x08,
	ConfigControl	= 0x04,

	TxFreeBytes	= 0x0C,		/* window 1 */
	TxStatus	= 0x0B,
	RxStatus	= 0x08,
	Fifo		= 0x00,

					/* Status/Interrupt Bits */
	Latch		= 0x0001,	/* Interrupt Latch */
	TxComplete	= 0x0004,	/* TX Complete */
	TxAvailable	= 0x0008,	/* TX Available */
	RxComplete	= 0x0010,	/* RX Complete */
	AllIntr		= 0x00FE,	/* All Interrupt Bits */
	CmdInProgress	= 0x1000,	/* Command In Progress */

					/* TxStatus Bits */
	TxJabber	= 0x20,		/* Jabber Error */
	TxUnderrun	= 0x10,		/* Underrun */
	TxMaxColl	= 0x08,		/* Maximum Collisions */

					/* RxStatus Bits */
	RxByteMask	= 0x07FF,	/* RX Bytes (0-1514) */
	RxErrMask	= 0x3800,	/* Type of Error: */
	RxErrOverrun	= 0x0000,	/*   Overrrun */
	RxErrOversize	= 0x0800,	/*   Oversize Packet (>1514) */
	RxErrDribble	= 0x1000,	/*   Dribble Bit(s) */
	RxErrRunt	= 0x1800,	/*   Runt Packet */
	RxErrFraming	= 0x2000,	/*   Alignment (Framing) */
	RxErrCRC	= 0x2800,	/*   CRC */
	RxError		= 0x4000,	/* Error */
	RxEmpty		= 0x8000,	/* Incomplete or FIFO empty */
};

#define COMMAND(board, cmd, a)	outs(board->io+Command, ((cmd)<<11)|(a))

/*
 * Write two 0 bytes to identify the IDport and then reset the
 * ID sequence. Then send the ID sequence to the card to get
 * the card into command state.
 */
static void
idseq(void)
{
	int i;
	uchar al;

	outb(IDport, 0);
	outb(IDport, 0);
	for(al = 0xFF, i = 0; i < 255; i++){
		outb(IDport, al);
		if(al & 0x80){
			al <<= 1;
			al ^= 0xCF;
		}
		else
			al <<= 1;
	}
}

/*
 * Get configuration parameters.
 */
static int
reset(Ctlr *ctlr)
{
	Board *board = ctlr->board;
	int i, ea;
	ushort x, acr;

	/*
	 * Do the little configuration dance. We only look
	 * at the first board that responds, if we ever have more
	 * than one we'll need to modify this sequence.
	 *
	 * 2. get to command state, reset, then return to command state
	 */
	idseq();
	outb(IDport, 0xC1);
	delay(2);
	idseq();

	/*
	 * 3. Read the Product ID from the EEPROM.
	 *    This is done by writing the IDPort with 0x83 (0x80
	 *    is the 'read EEPROM command, 0x03 is the offset of
	 *    the Product ID field in the EEPROM).
	 *    The data comes back 1 bit at a time.
	 *    We seem to need a delay here between reading the bits.
	 *
	 * If the ID doesn't match the 3C509 ID code, the adapter
	 * probably isn't there, so barf.
	 */
	outb(IDport, 0x83);
	for(x = 0, i = 0; i < 16; i++){
		delay(5);
		x <<= 1;
		x |= inb(IDport) & 0x01;
	}
	if((x & 0xF0FF) != 0x9050)
		return -1;

	/*
	 * 3. Read the Address Configuration from the EEPROM.
	 *    The Address Configuration field is at offset 0x08 in the EEPROM).
	 * 6. Activate the adapter by writing the Activate command
	 *    (0xFF).
	 */
	outb(IDport, 0x88);
	for(acr = 0, i = 0; i < 16; i++){
		delay(20);
		acr <<= 1;
		acr |= inb(IDport) & 0x01;
	}
	outb(IDport, 0xFF);

	/*
	 * 8. Now we can talk to the adapter's I/O base addresses.
	 *    We get the I/O base address from the acr just read.
	 *
	 *    Enable the adapter. 
	 */
	board->io = (acr & 0x1F)*0x10 + 0x200;
	outb(board->io+ConfigControl, 0x01);

	/*
	 * Read the IRQ from the Resource Configuration Register
	 * and the ethernet address from the EEPROM.
	 * The EEPROM command is 8bits, the lower 6 bits being
	 * the address offset.
	 */
	board->irq = (ins(board->io+ResourceConfig)>>12) & 0x0F;
	for(ea = 0, i = 0; i < 3; i++, ea += 2){
		while(ins(board->io+EEPROMcmd) & 0x8000)
			;
		outs(board->io+EEPROMcmd, (2<<6)|i);
		while(ins(board->io+EEPROMcmd) & 0x8000)
			;
		x = ins(board->io+EEPROMdata);
		ctlr->ea[ea] = (x>>8) & 0xFF;
		ctlr->ea[ea+1] = x & 0xFF;
	}

	/*
	 * Finished with window 0. Now set the ethernet address
	 * in window 2.
	 * Commands have the format 'CCCCCAAAAAAAAAAA' where C
	 * is a bit in the command and A is a bit in the argument.
	 */
	COMMAND(board, SelectWindow, 2);
	for(i = 0; i < 6; i++)
		outb(board->io+i, ctlr->ea[i]);

	/*
	 * Finished with window 2.
	 * Set window 1 for normal operation.
	 */
	COMMAND(board, SelectWindow, 1);

	/*
	 * If we have a 10BASE2 transceiver, start the DC-DC
	 * converter. Wait > 800 microseconds.
	 */
	if(((acr>>14) & 0x03) == 0x03){
		COMMAND(board, StartCoax, 0);
		delay(1);
	}

	print("3C509 I/O addr %lux irq %d:", board->io, board->irq);
	for(i = 0; i < sizeof(ctlr->ea); i++)
		print(" %2.2ux", ctlr->ea[i]);
	print("\n");

	return 0;
}

static void
attach(Ctlr *ctlr)
{
	Board *board = ctlr->board;

	/*
	 * Set the receiver packet filter for our own and
	 * and broadcast addresses, set the interrupt masks
	 * for all interrupts, and enable the receiver and transmitter.
	 * The only interrupt we should see under normal conditions
	 * is the receiver interrupt. If the transmit FIFO fills up,
	 * we will also see TxAvailable interrupts.
	 */
	COMMAND(board, SetRxFilter, Broadcast|MyEtherAddr);
	COMMAND(board, SetReadZeroMask, AllIntr|Latch);
	COMMAND(board, SetIntrMask, AllIntr|Latch);
	COMMAND(board, RxEnable, 0);
	COMMAND(board, TxEnable, 0);
}

static void
mode(Ctlr *ctlr, int on)
{
	Board *board = ctlr->board;

	if(on)
		COMMAND(board, SetRxFilter, Promiscuous|Broadcast|MyEtherAddr);
	else
		COMMAND(board, SetRxFilter, Broadcast|MyEtherAddr);
}

static int
getdiag(Board *board)
{
	int bytes;

	COMMAND(board, SelectWindow, 4);
	bytes = ins(board->io+0x04);
	COMMAND(board, SelectWindow, 1);
	return bytes & 0xFFFF;
}

static void
receive(Ctlr *ctlr)
{
	Board *board = ctlr->board;
	ushort status;
	RingBuf *rb;
	int len;

	while(((status = ins(board->io+RxStatus)) & RxEmpty) == 0){
		/*
		 * If we had an error, log it and continue
		 * without updating the ring.
		 */
		if(status & RxError){
			switch(status & RxErrMask){

			case RxErrOverrun:	/* Overrrun */
				ctlr->overflows++;
				break;

			case RxErrOversize:	/* Oversize Packet (>1514) */
			case RxErrRunt:		/* Runt Packet */
				ctlr->buffs++;
				break;
			case RxErrFraming:	/* Alignment (Framing) */
				ctlr->frames++;
				break;

			case RxErrCRC:		/* CRC */
				ctlr->crcs++;
				break;
			}
		}
		else {
			/*
			 * We have a packet. Read it into the next
			 * free ring buffer, if any.
			 * The CRC is already stripped off.
			 */
			rb = &ctlr->rb[ctlr->ri];
			if(rb->owner == Interface){
				len = (status & RxByteMask);
				rb->len = len;
	
				/*
				 * Must read len bytes padded to a
				 * doubleword. We can pick them out 16-bits
				 * at a time (can try 32-bits at a time
				 * later).
				insl(board->io+Fifo, rb->pkt, HOWMANY(len, 4));
				 */
				inss(board->io+Fifo, rb->pkt, HOWMANY(len, 2));

				/*
				 * Update the ring.
				 */
				rb->owner = Host;
				ctlr->ri = NEXT(ctlr->ri, ctlr->nrb);
			}
		}

		/*
		 * Discard the packet as we're done with it.
		 * Wait for discard to complete.
		 */
		COMMAND(board, RxDiscard, 0);
		while(ins(board->io+Status) & CmdInProgress)
			;
	}
}

static void
transmit(Ctlr *ctlr)
{
	Board *board = ctlr->board;
	RingBuf *tb;
	int s;
	ushort len;

	s = splhi();
	for(tb = &ctlr->tb[ctlr->ti]; tb->owner == Interface; tb = &ctlr->tb[ctlr->ti]){
		/*
		 * If there's no room in the FIFO for this packet,
		 * set up an interrupt for when space becomes available.
		 */
		if(tb->len > ins(board->io+TxFreeBytes)){
			COMMAND(board, SetTxAvailable, tb->len);
			break;
		}

		/*
		 * There's room, copy the packet to the FIFO and free
		 * the buffer back to the host.
		 * Output packet must be a multiple of 4 in length.
		 */
		len = ROUNDUP(tb->len, 4)/2;
		outs(board->io+Fifo, tb->len);
		outs(board->io+Fifo, 0);
		outss(board->io+Fifo, tb->pkt, len);
		tb->owner = Host;
		ctlr->ti = NEXT(ctlr->ti, ctlr->ntb);
	}
	splx(s);
}

static void
interrupt(Ctlr *ctlr)
{
	Board *board = ctlr->board;
	ushort status;
	uchar txstatus, x;

	status = ins(board->io+Status);

	if(status & RxComplete){
		receive(ctlr);
		wakeup(&ctlr->rr);
		status &= ~RxComplete;
	}

	if(status & TxComplete){
		/*
		 * Pop the TX Status stack, accumulating errors.
		 * If there was a Jabber or Underrun error, reset
		 * the transmitter. For all conditions enable
		 * the transmitter.
		 */
		txstatus = 0;
		do{
			if(x = inb(board->io+TxStatus))
				outb(board->io+TxStatus, 0);
			txstatus |= x;
		}while(ins(board->io+Status) & TxComplete);

		if(txstatus & (TxJabber|TxUnderrun))
			COMMAND(board, TxReset, 0);
		COMMAND(board, TxEnable, 0);
		ctlr->oerrs++;
	}

	if(status & (TxAvailable|TxComplete)){
		/*
		 * Reset the Tx FIFO threshold.
		 */
		if(status & TxAvailable)
			COMMAND(board, AckIntr, TxAvailable);
		transmit(ctlr);
		wakeup(&ctlr->tr);
		status &= ~(TxAvailable|TxComplete);
	}

	/*
	 * Panic if there are any interrupt bits on we haven't
	 * dealt with other than Latch.
	 * Otherwise, acknowledge the interrupt.
	 */
	if(status & AllIntr)
		panic("ether509 interrupt: #%lux, #%ux\n", status, getdiag(board));

	COMMAND(board, AckIntr, Latch);
}

Board ether509 = {
	reset,
	0,			/* init */
	attach,
	mode,
	0,			/* receive */
	transmit,
	interrupt,
	0,			/* watch */
};
