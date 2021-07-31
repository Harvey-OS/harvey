/*
 * Etherlink III and Fast EtherLink adapters.
 * To do:
 *	autoSelect;
 *	busmaster channel;
 *	PCMCIA;
 *	PCI latency timer and master enable;
 *	errata list.
 *
 * Product ID:
 *	9150 ISA	3C509[B]
 *	9050 ISA	3C509[B]-TP
 *	9450 ISA	3C509[B]-COMBO
 *	9550 ISA	3C509[B]-TPO
 *
 *	9350 EISA	3C579
 *	9250 EISA	3C579-TP
 *
 *	5920 EISA	3C592-[TP|COMBO|TPO]
 *	5970 EISA	3C597-TX	Fast Etherlink 10BASE-T/100BASE-TX
 *	5971 EISA	3C597-T4	Fast Etherlink 10BASE-T/100BASE-T4
 *	5972 EISA	3C597-MII	Fast Etherlink 10BASE-T/MII
 *
 *	5900 PCI	3C590-[TP|COMBO|TPO]
 *	5950 PCI	3C595-TX	Fast Etherlink Shared 10BASE-T/100BASE-TX
 *	5951 PCI	3C595-T4	Fast Etherlink Shared 10BASE-T/100BASE-T4
 *	5952 PCI	3C595-MII	Fast Etherlink 10BASE-T/MII
 *
 *	9058 PCMCIA	3C589[B]-[TP|COMBO]
 *
 *	627C MCA	3C529
 *	627D MCA	3C529-TP
 */
#include "u.h"
#include "lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

#include "etherif.h"

enum {
	IDport			= 0x0110,	/* anywhere between 0x0100 and 0x01F0 */
};

enum {						/* all windows */
	Command			= 0x000E,
	IntStatus		= 0x000E,
};

enum {						/* Commands */
	GlobalReset		= 0x0000,
	SelectRegisterWindow	= 0x0001,	
	EnableDcConverter	= 0x0002,
	RxDisable		= 0x0003,
	RxEnable		= 0x0004,
	RxReset			= 0x0005,
	TxDone			= 0x0007,	
	RxDiscard		= 0x0008,
	TxEnable		= 0x0009,
	TxDisable		= 0x000A,
	TxReset			= 0x000B,
	RequestInterrupt	= 0x000C,
	AcknowledgeInterrupt	= 0x000D,
	SetInterruptEnable	= 0x000E,
	SetIndicationEnable	= 0x000F,	/* SetReadZeroMask */
	SetRxFilter		= 0x0010,
	SetRxEarlyThresh	= 0x0011,
	SetTxAvailableThresh	= 0x0012,
	SetTxStartThresh	= 0x0013,
	StartDma		= 0x0014,	/* initiate busmaster operation */
	StatisticsEnable	= 0x0015,
	StatisticsDisable	= 0x0016,
	DisableDcConverter	= 0x0017,
	SetTxReclaimThresh	= 0x0018,	/* PIO-only adapters */
	PowerUp			= 0x001B,	/* not all adapters */
	PowerDownFull		= 0x001C,	/* not all adapters */
	PowerAuto		= 0x001D,	/* not all adapters */
};

enum {						/* (Global|Rx|Tx)Reset command bits */
	tpAuiReset		= 0x0001,	/* 10BaseT and AUI transceivers */
	endecReset		= 0x0002,	/* internal Ethernet encoder/decoder */
	networkReset		= 0x0004,	/* network interface logic */
	fifoReset		= 0x0008,	/* FIFO control logic */
	aismReset		= 0x0010,	/* autoinitialise state-machine logic */
	hostReset		= 0x0020,	/* bus interface logic */
	dmaReset		= 0x0040,	/* bus master logic */
	vcoReset		= 0x0080,	/* on-board 10Mbps VCO */

	resetMask		= 0x00FF,
};

enum {						/* SetRxFilter command bits */
	receiveIndividual	= 0x0001,	/* match station address */
	receiveMulticast	= 0x0002,
	receiveBroadcast	= 0x0004,
	receiveAllFrames	= 0x0008,	/* promiscuous */
};

enum {						/* StartDma command bits */
	Upload			= 0x0000,	/* transfer data from adapter to memory */
	Download		= 0x0001,	/* transfer data from memory to adapter */
};

enum {						/* IntStatus bits */
	interruptLatch		= 0x0001,
	hostError		= 0x0002,	/* Adapter Failure */
	txComplete		= 0x0004,
	txAvailable		= 0x0008,
	rxComplete		= 0x0010,
	rxEarly			= 0x0020,
	intRequested		= 0x0040,
	updateStats		= 0x0080,
	transferInt		= 0x0100,	/* Bus Master Transfer Complete */
	busMasterInProgress	= 0x0800,
	commandInProgress	= 0x1000,

	interruptMask		= 0x01FE,
};

#define COMMAND(port, cmd, a)	outs((port)+Command, ((cmd)<<11)|(a))
#define STATUS(port)		ins((port)+IntStatus)

enum {						/* Window 0 - setup */
	Wsetup			= 0x0000,
						/* registers */
	ManufacturerID		= 0x0000,	/* 3C5[08]*, 3C59[27] */
	ProductID		= 0x0002,	/* 3C5[08]*, 3C59[27] */
	ConfigControl		= 0x0004,	/* 3C5[08]*, 3C59[27] */
	AddressConfig		= 0x0006,	/* 3C5[08]*, 3C59[27] */
	ResourceConfig		= 0x0008,	/* 3C5[08]*, 3C59[27] */
	EepromCommand		= 0x000A,
	EepromData		= 0x000C,
						/* AddressConfig Bits */
	autoSelect9		= 0x0080,
	xcvrMask9		= 0xC000,
						/* ConfigControl bits */
	Ena			= 0x0001,
						/* EepromCommand bits */
	EepromReadRegister	= 0x0080,
	EepromBusy		= 0x8000,
};

#define EEPROMCMD(port, cmd, a)	outs((port)+EepromCommand, (cmd)|(a))
#define EEPROMBUSY(port)	(ins((port)+EepromCommand) & EepromBusy)
#define EEPROMDATA(port)	ins((port)+EepromData)

enum {						/* Window 1 - operating set */
	Wop			= 0x0001,
						/* registers */
	Fifo			= 0x0000,
	RxError			= 0x0004,	/* 3C59[0257] only */
	RxStatus		= 0x0008,
	Timer			= 0x000A,
	TxStatus		= 0x000B,
	TxFree			= 0x000C,
						/* RxError bits */
	rxOverrun		= 0x0001,
	runtFrame		= 0x0002,
	alignmentError		= 0x0004,	/* Framing */
	crcError		= 0x0008,
	oversizedFrame		= 0x0010,
	dribbleBits		= 0x0080,
						/* RxStatus bits */
	rxBytes			= 0x1FFF,	/* 3C59[0257] mask */
	rxBytes9		= 0x07FF,	/* 3C5[078]9 mask */
	rxError9		= 0x3800,	/* 3C5[078]9 error mask */
	rxOverrun9		= 0x0000,
	oversizedFrame9		= 0x0800,
	dribbleBits9		= 0x1000,
	runtFrame9		= 0x1800,
	alignmentError9		= 0x2000,	/* Framing */
	crcError9		= 0x2800,
	rxError			= 0x4000,
	rxIncomplete		= 0x8000,
						/* TxStatus Bits */
	txStatusOverflow	= 0x0004,
	maxCollisions		= 0x0008,
	txUnderrun		= 0x0010,
	txJabber		= 0x0020,
	interruptRequested	= 0x0040,
	txStatusComplete	= 0x0080,
};

enum {						/* Window 2 - station address */
	Wstation		= 0x0002,
};

enum {						/* Window 3 - FIFO management */
	Wfifo			= 0x0003,
						/* registers */
	InternalConfig		= 0x0000,	/* 3C509B, 3C589, 3C59[0257] */
	OtherInt		= 0x0004,	/* 3C59[0257] */
	RomControl		= 0x0006,	/* 3C509B, 3C59[27] */
	MacControl		= 0x0006,	/* 3C59[0257] */
	ResetOptions		= 0x0008,	/* 3C59[0257] */
	RxFree			= 0x000A,
						/* InternalConfig bits */
	disableBadSsdDetect	= 0x00000100,
	ramLocation		= 0x00000200,	/* 0 external, 1 internal */
	ramPartition5to3	= 0x00000000,
	ramPartition3to1	= 0x00010000,
	ramPartition1to1	= 0x00020000,
	ramPartition3to5	= 0x00030000,
	ramPartitionMask	= 0x00030000,
	xcvr10BaseT		= 0x00000000,
	xcvrAui			= 0x00100000,	/* 10BASE5 */
	xcvr10Base2		= 0x00300000,
	xcvr100BaseTX		= 0x00400000,
	xcvr100BaseFX		= 0x00500000,
	xcvrMii			= 0x00600000,
	xcvrMask		= 0x00700000,
	autoSelect		= 0x01000000,
						/* MacControl bits */
	deferExtendEnable	= 0x0001,
	deferTimerSelect	= 0x001E,	/* mask */
	fullDuplexEnable	= 0x0020,
	allowLargePackets	= 0x0040,
						/* ResetOptions bits */
	baseT4Available		= 0x0001,
	baseTXAvailable		= 0x0002,
	baseFXAvailable		= 0x0004,
	base10TAvailable	= 0x0008,
	coaxAvailable		= 0x0010,
	auiAvailable		= 0x0020,
	miiConnector		= 0x0040,
};

enum {						/* Window 4 - diagnostic */
	Wdiagnostic		= 0x0004,
						/* registers */
	VcoDiagnostic		= 0x0002,
	FifoDiagnostic		= 0x0004,
	NetworkDiagnostic	= 0x0006,
	PhysicalMgmt		= 0x0008,
	MediaStatus		= 0x000A,
	BadSSD			= 0x000C,
						/* FifoDiagnostic bits */
	txOverrun		= 0x0400,
	rxUnderrun		= 0x2000,
	receiving		= 0x8000,
						/* MediaStatus bits */
	dataRate100		= 0x0002,
	crcStripDisable		= 0x0004,
	enableSqeStats		= 0x0008,
	collisionDetect		= 0x0010,
	carrierSense		= 0x0020,
	jabberGuardEnable	= 0x0040,
	linkBeatEnable		= 0x0080,
	jabberDetect		= 0x0200,
	polarityReversed	= 0x0400,
	linkBeatDetect		= 0x0800,
	txInProg		= 0x1000,
	dcConverterEnabled	= 0x4000,
	auiDisable		= 0x8000,
};

enum {						/* Window 5 - internal state */
	Wstate			= 0x0005,
						/* registers */
	TxStartThresh		= 0x0000,
	TxAvalableThresh	= 0x0002,
	RxEarlyThresh		= 0x0006,
	RxFilter		= 0x0008,
	InterruptEnable		= 0x000A,
	IndicationEnable	= 0x000C,
};

enum {						/* Window 6 - statistics */
	Wstatistics		= 0x0006,
						/* registers */
	CarrierLost		= 0x0000,
	SqeErrors		= 0x0001,
	MultipleColls		= 0x0002,
	SingleCollFrames	= 0x0003,
	LateCollisions		= 0x0004,
	RxOverruns		= 0x0005,
	FramesXmittedOk		= 0x0006,
	FramesRcvdOk		= 0x0007,
	FramesDeferred		= 0x0008,
	UpperFramesOk		= 0x0009,
	BytesRcvdOk		= 0x000A,
	BytesXmittedOk		= 0x000C,
};

enum {						/* Window 7 - bus master operations */
	Wmaster			= 0x0007,
						/* registers */
	MasterAddress		= 0x0000,
	MasterLen		= 0x0006,
	MasterStatus		= 0x000C,
						/* MasterStatus bits */
	masterAbort		= 0x0001,
	targetAbort		= 0x0002,
	targetRetry		= 0x0004,
	targetDisc		= 0x0008,
	masterDownload		= 0x1000,
	masterUpload		= 0x4000,
	masterInProgress	= 0x8000,

	masterMask		= 0xD00F,
};	

typedef struct {
	int	txthreshold;
} Ctlr;

static void
attach(Ether* ether)
{
	int port, x;

	port = ether->port;

	/*
	 * Set the receiver packet filter for this and broadcast addresses,
	 * set the interrupt masks for all interrupts, enable the receiver
	 * and transmitter.
	 */
	x = receiveBroadcast|receiveIndividual;
	COMMAND(port, SetRxFilter, x);

	x = interruptMask|interruptLatch;
	COMMAND(port, SetIndicationEnable, x);
	COMMAND(port, SetInterruptEnable, x);

	COMMAND(port, RxEnable, 0);
	COMMAND(port, TxEnable, 0);
}

static void
transmit(Ether* ether)
{
	int port, len;
	RingBuf *tb;

	/*
	 * Attempt to top-up the transmit FIFO. If there's room simply
	 * stuff in the packet length (unpadded to a dword boundary), the
	 * packet data (padded) and remove the packet from the queue.
	 * If there's no room post an interrupt for when there is.
	 * This routine is called both from the top level and from interrupt
	 * level.
	 */
	port = ether->port;
	for(tb = &ether->tb[ether->ti]; tb->owner == Interface; tb = &ether->tb[ether->ti]){
		len = ROUNDUP(tb->len, 4);
		if(len+4 <= ins(port+TxFree)){
			outl(port+Fifo, tb->len);
			outsl(port+Fifo, tb->pkt, len/4);
			tb->owner = Host;
			ether->ti = NEXT(ether->ti, ether->ntb);
		}
		else{
			COMMAND(port, SetTxAvailableThresh, len);
			break;
		}
	}
}

static void
receive(Ether* ether)
{
	int len, port, rxstatus;
	RingBuf *rb;

	port = ether->port;

	while(((rxstatus = ins(port+RxStatus)) & rxIncomplete) == 0){
		/*
		 * If there was an error, throw it away and continue.
		 * The 3C5[078]9 has the error info in the status register
		 * and the 3C59[0257] implement a separate RxError register.
		 */
		if((rxstatus & rxError) == 0){
			/*
			 * Packet received. Read it into the next free
			 * ring buffer, if any. Must read len bytes padded
			 * to a doubleword, can be picked out 32-bits at
			 * a time. The CRC is already stripped off.
			 */
			rb = &ether->rb[ether->ri];
			if(rb->owner == Interface){
				len = (rxstatus & rxBytes9);
				rb->len = len;
				insl(port+Fifo, rb->pkt, HOWMANY(len, 4));

				rb->owner = Host;
				ether->ri = NEXT(ether->ri, ether->nrb);
			}
		}

		/*
		 * All done, discard the packet.
		 */
		COMMAND(port, RxDiscard, 0);
		while(STATUS(port) & commandInProgress)
			;
	}
}

static void
statistics(Ether* ether)
{
	int i, port, w;

	port = ether->port;
	w = (STATUS(port)>>13) & 0x07;
	COMMAND(port, SelectRegisterWindow, Wop);
	COMMAND(port, SelectRegisterWindow, Wstatistics);

	for(i = 0; i < 0x0A; i++)
		inb(port+i);
	ins(port+BytesRcvdOk);
	ins(port+BytesXmittedOk);

	COMMAND(port, SelectRegisterWindow, w);
}

static void
interrupt(Ureg*, void* arg)
{
	Ether *ether;
	int port, status, txstatus, w, x;
	Ctlr *ctlr;

	ether = arg;
	port = ether->port;
	ctlr = ether->ctlr;

	w = (STATUS(port)>>13) & 0x07;
	COMMAND(port, SelectRegisterWindow, Wop);

	for(;;){
		/*
		 * Clear the interrupt latch.
		 * It's possible to receive a packet and for another
		 * to become complete before exiting the interrupt
		 * handler so this must be done first to ensure another
		 * interrupt will occur.
		 */
		COMMAND(port, AcknowledgeInterrupt, interruptLatch);
		status = STATUS(port);
		if((status & interruptMask) == 0)
			break;

		if(status & hostError){
			/*
			 * Adapter failure, try to find out why, reset if
			 * necessary. What happens if Tx is active and a reset
			 * occurs, need to retransmit? This probably isn't right.
			 */
			COMMAND(port, SelectRegisterWindow, Wdiagnostic);
			x = ins(port+FifoDiagnostic);
			COMMAND(port, SelectRegisterWindow, Wop);
			print("elnk3#%d: status 0x%uX, diag 0x%uX\n",
			    ether->ctlrno, status, x);

			if(x & txOverrun){
				COMMAND(port, TxReset, 0);
				COMMAND(port, TxEnable, 0);
			}

			if(x & rxUnderrun){
				/*
				 * This shouldn't happen...
				 * Need to restart any busmastering?
				 */
				COMMAND(port, RxReset, 0);
				while(STATUS(port) & commandInProgress)
					;
				COMMAND(port, RxEnable, 0);
			}

			status &= ~hostError;
		}

		if(status & (transferInt|rxComplete)){
			receive(ether);
			status &= ~(transferInt|rxComplete);
		}

		if(status & txComplete){
			/*
			 * Pop the TxStatus stack, accumulating errors.
			 * Adjust the TX start threshold if there was an underrun.
			 * If there was a Jabber or Underrun error, reset
			 * the transmitter.
			 * For all conditions enable the transmitter.
			 */
			txstatus = 0;
			do{
				if(x = inb(port+TxStatus))
					outb(port+TxStatus, 0);
				txstatus |= x;
			}while(STATUS(port) & txComplete);

			if(txstatus & txUnderrun){
				COMMAND(port, SelectRegisterWindow, Wdiagnostic);
				while(ins(port+MediaStatus) & txInProg)
					;
				COMMAND(port, SelectRegisterWindow, Wop);
				if(ctlr->txthreshold < ETHERMAXTU)
					ctlr->txthreshold += ETHERMINTU;
			}

			if(txstatus & (txJabber|txUnderrun)){
				COMMAND(port, TxReset, 0);
				while(STATUS(port) & commandInProgress)
					;
				COMMAND(port, SetTxStartThresh, ctlr->txthreshold);
			}
			COMMAND(port, TxEnable, 0);
			status &= ~txComplete;
			status |= txAvailable;
		}

		if(status & txAvailable){
			COMMAND(port, AcknowledgeInterrupt, txAvailable);
			transmit(ether);
			status &= ~txAvailable;
		}

		if(status & updateStats){
			statistics(ether);
			status &= ~updateStats;
		}

		/*
		 * Panic if there are any interrupts not dealt with.
		 */
		if(status & interruptMask)
			panic("elnk3#%d: interrupt mask 0x%uX\n", ether->ctlrno, status);
	}

	COMMAND(port, SelectRegisterWindow, w);
}

typedef struct Adapter {
	int	port;
	int	irq;
	int	tbdf;
} Adapter;
static Block* adapter;

static void
tcmadapter(int port, int irq, int tbdf)
{
	Block *bp;
	Adapter *ap;

	bp = allocb(sizeof(Adapter));
	ap = (Adapter*)bp->rp;
	ap->port = port;
	ap->irq = irq;
	ap->tbdf = tbdf;

	bp->next = adapter;
	adapter = bp;
}

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
	static int reset, untag;

	/*
	 * One time only:
	 *	reset any adapters listening
	 */
	if(reset == 0){
		outb(IDport, 0);
		outb(IDport, 0);
		outb(IDport, 0xC0);
		delay(20);
		reset = 1;
	}

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

	/*
	 * One time only:
	 *	write ID sequence to get the attention of all adapters;
	 *	untag all adapters.
	 * If we do a global reset here on all adapters we'll confuse any
	 * ISA cards configured for EISA mode.
	 */
	if(untag == 0){
		outb(IDport, 0xD0);
		untag = 1;
	}
}

static ulong
activate(void)
{
	int i;
	ushort x, acr;

	/*
	 * Do the little configuration dance:
	 *
	 * 2. write the ID sequence to get to command state.
	 */
	idseq();

	/*
	 * 3. Read the Manufacturer ID from the EEPROM.
	 *    This is done by writing the IDPort with 0x87 (0x80
	 *    is the 'read EEPROM' command, 0x07 is the offset of
	 *    the Manufacturer ID field in the EEPROM).
	 *    The data comes back 1 bit at a time.
	 *    We seem to need a delay here between reading the bits.
	 *
	 * If the ID doesn't match, there are no more adapters.
	 */
	outb(IDport, 0x87);
	delay(20);
	for(x = 0, i = 0; i < 16; i++){
		delay(20);
		x <<= 1;
		x |= inb(IDport) & 0x01;
	}
	if(x != 0x6D50)
		return 0;

	/*
	 * 3. Read the Address Configuration from the EEPROM.
	 *    The Address Configuration field is at offset 0x08 in the EEPROM).
	 */
	outb(IDport, 0x88);
	for(acr = 0, i = 0; i < 16; i++){
		delay(20);
		acr <<= 1;
		acr |= inb(IDport) & 0x01;
	}

	return (acr & 0x1F)*0x10 + 0x200;
}

#ifdef notjustpcmcia
static void
tcm509isa(void)
{
	int irq, port;

	/*
	 * Attempt to activate all adapters. If adapter is set for
	 * EISA mode (0x3F0), tag it and ignore. Otherwise, activate
	 * it fully.
	 */
	while(port = activate()){
		/*
		 * 6. Tag the adapter so it won't respond in future.
		 */
		outb(IDport, 0xD1);
		if(port == 0x3F0)
			continue;

		/*
		 * 6. Activate the adapter by writing the Activate command
		 *    (0xFF).
		 */
		outb(IDport, 0xFF);
		delay(20);

		/*
		 * 8. Can now talk to the adapter's I/O base addresses.
		 *    Use the I/O base address from the acr just read.
		 *
		 *    Enable the adapter and clear out any lingering status
		 *    and interrupts.
		 */
		while(STATUS(port) & commandInProgress)
			;
		COMMAND(port, SelectRegisterWindow, Wsetup);
		outs(port+ConfigControl, Ena);

		COMMAND(port, TxReset, 0);
		COMMAND(port, RxReset, 0);
		COMMAND(port, AcknowledgeInterrupt, 0xFF);

		irq = (ins(port+ResourceConfig)>>12) & 0x0F;
		tcmadapter(port, irq, BUSUNKNOWN);
	}
}

static void
tcm5XXeisa(void)
{
	ushort x;
	int irq, port, slot;

	/*
	 * Check if this is an EISA machine.
	 * If not, nothing to do.
	 */
	if(strncmp((char*)(KZERO|0xFFFD9), "EISA", 4))
		return;

	/*
	 * Continue through the EISA slots looking for a match on both
	 * 3COM as the manufacturer and 3C579-* or 3C59[27]-* as the product.
	 * If an adapter is found, select window 0, enable it and clear
	 * out any lingering status and interrupts.
	 */
	for(slot = 1; slot < MaxEISA; slot++){
		port = slot*0x1000;
		if(ins(port+0xC80+ManufacturerID) != 0x6D50)
			continue;
		x = ins(port+0xC80+ProductID);
		if((x & 0xF0FF) != 0x9050 && (x & 0xFF00) != 0x5900)
			continue;

		COMMAND(port, SelectRegisterWindow, Wsetup);
		outs(port+ConfigControl, Ena);

		COMMAND(port, TxReset, 0);
		COMMAND(port, RxReset, 0);
		COMMAND(port, AcknowledgeInterrupt, 0xFF);

		irq = (ins(port+ResourceConfig)>>12) & 0x0F;
		tcmadapter(port, irq, BUSUNKNOWN);
	}
}

static void
tcm59Xpci(void)
{
	Pcidev *p;
	int irq, port;

	p = nil;
	while(p = pcimatch(p, 0x10B7, 0)){
		port = p->mem[0].bar & ~0x01;
		irq = p->intl;
		COMMAND(port, GlobalReset, 0);
		while(STATUS(port) & commandInProgress)
			;

		tcmadapter(port, irq, p->tbdf);
	}
}
#endif /* notjustpcmcia */

static char* tcmpcmcia[] = {
	"3C589",			/* 3COM 589[ABCD] */
	"3C562",			/* 3COM 562 */
	"589E",				/* 3COM Megahertz 589E */
	nil,
};

static int
tcm5XXpcmcia(Ether* ether)
{
	int i;

	for(i = 0; tcmpcmcia[i] != nil; i++){
		if(!cistrcmp(ether->type, tcmpcmcia[i]))
			return ether->port;
	}

	return 0;
}

static int
autoselect(int port, int rxstatus9)
{
	int media, x;

	/*
	 * Pathetic attempt at automatic media selection.
	 * Really just to get the Fast Etherlink 10BASE-T/100BASE-TX
	 * cards operational.
	 */
	media = auiAvailable|coaxAvailable|base10TAvailable;
	if(rxstatus9 == 0){
		COMMAND(port, SelectRegisterWindow, Wfifo);
		media = ins(port+ResetOptions);
	}

	if(media & miiConnector)
		return xcvrMii;

	if(media & baseTXAvailable){
		/*
		 * Must have InternalConfig register.
		 */
		COMMAND(port, SelectRegisterWindow, Wfifo);
		x = inl(port+InternalConfig) & ~xcvrMask;
		x |= xcvr100BaseTX;
		outl(port+InternalConfig, x);
		COMMAND(port, TxReset, 0);
		while(STATUS(port) & commandInProgress)
			;
		COMMAND(port, RxReset, 0);
		while(STATUS(port) & commandInProgress)
			;

		COMMAND(port, SelectRegisterWindow, Wdiagnostic);
		x = ins(port+MediaStatus) & ~(dcConverterEnabled|jabberGuardEnable);
		outs(port+MediaStatus, linkBeatEnable|x);
		delay(10);

{ int i, v;
  for(i = 0; i < 10000; i++){
	v = ins(port+MediaStatus);
	if(v & linkBeatDetect){
		print("count %d v %uX\n", i, v);
		return xcvr100BaseTX;
	}
	delay(1);
  }
print("count %d v %uX\n", i, ins(port+MediaStatus));
}
		if(ins(port+MediaStatus) & linkBeatDetect)
			return xcvr100BaseTX;
		outs(port+MediaStatus, x);
	}

	if(media & base10TAvailable){
		if(rxstatus9 == 0){
			COMMAND(port, SelectRegisterWindow, Wfifo);
			x = inl(port+InternalConfig) & ~xcvrMask;
			x |= xcvr10BaseT;
			outl(port+InternalConfig, x);
		}
		else{
			COMMAND(port, SelectRegisterWindow, Wsetup);
			x = ins(port+AddressConfig) & ~xcvrMask9;
			x |= (xcvr10BaseT>>20)<<14;
			outs(port+AddressConfig, x);
		}
		COMMAND(port, TxReset, 0);
		while(STATUS(port) & commandInProgress)
			;
		COMMAND(port, RxReset, 0);
		while(STATUS(port) & commandInProgress)
			;

		COMMAND(port, SelectRegisterWindow, Wdiagnostic);
		x = ins(port+MediaStatus) & ~dcConverterEnabled;
		outs(port+MediaStatus, linkBeatEnable|jabberGuardEnable|x);
		delay(10);

		if(ins(port+MediaStatus) & linkBeatDetect)
			return xcvr10BaseT;
		outs(port+MediaStatus, x);
	}

	/*
	 * Botch.
	 */
	return autoSelect;
}

static int
eepromdata(int port, int offset)
{
	COMMAND(port, SelectRegisterWindow, Wsetup);
	while(EEPROMBUSY(port))
		;
	EEPROMCMD(port, EepromReadRegister, offset);
	while(EEPROMBUSY(port))
		;
	return EEPROMDATA(port);
}

int
elnk3reset(Ether* ether)
{
	int did, i, port, rxstatus9, x, xcvr;
	Block *bp, **bpp;
	Adapter *ap;
	uchar ea[Eaddrlen];
	Ctlr *ctlr;
#ifdef notjustpcmcia
	static int scandone;

	/*
	 * Scan for adapter on PCI, EISA and finally
	 * using the little ISA configuration dance.
	 */
	if(scandone == 0){
		tcm59Xpci();
		tcm5XXeisa();
		tcm509isa();
		scandone = 1;
	}
#endif /* notjustpcmcia */

	/*
	 * Any adapter matches if no ether->port is supplied,
	 * otherwise the ports must match.
	 */
	port = 0;
	bpp = &adapter;
	for(bp = *bpp; bp; bp = bp->next){
		ap = (Adapter*)bp->rp;
		if(ether->port == 0 || ether->port == ap->port){
			port = ap->port;
			ether->irq = ap->irq;
			ether->tbdf = ap->tbdf;
			*bpp = bp->next;
			freeb(bp);
			break;
		}
		bpp = &bp->next;
	}
	if(port == 0 && (port = tcm5XXpcmcia(ether)) == 0)
		return -1;

	/*
	 * Read the DeviceID from the EEPROM, it's at offset 0x03,
	 * and do something depending on capabilities.
	 */
	switch(did = eepromdata(port, 0x03)){

	case 0x9000:
	case 0x9001:
	case 0x9050:
	case 0x9051:
		if(BUSTYPE(ether->tbdf) != BusPCI)
			goto buggery;
		goto vortex;

	case 0x5900:
	case 0x5920:
	case 0x5950:
	case 0x5951:
	case 0x5952:
	case 0x5970:
	case 0x5971:
	case 0x5972:
	vortex:
		COMMAND(port, SelectRegisterWindow, Wfifo);
		xcvr = inl(port+InternalConfig) & (autoSelect|xcvrMask);
		rxstatus9 = 0;
		break;

	buggery:
	default:
		COMMAND(port, SelectRegisterWindow, Wsetup);
		x = ins(port+AddressConfig);
		xcvr = ((x & xcvrMask9)>>14)<<20;
		if(x & autoSelect9)
			xcvr |= autoSelect;
		rxstatus9 = 1;
		break;
	}
	USED(did);

	/*
	 * Check if the adapter's station address is to be overridden.
	 * If not, read it from the EEPROM and set in ether->ea prior to loading the
	 * station address in Wstation. The EEPROM returns 16-bits at a time.
	 */
	memset(ea, 0, Eaddrlen);
	if(memcmp(ea, ether->ea, Eaddrlen) == 0){
		for(i = 0; i < Eaddrlen/2; i++){
			x = eepromdata(port, i);
			ether->ea[2*i] = x>>8;
			ether->ea[2*i+1] = x;
		}
	}

	COMMAND(port, SelectRegisterWindow, Wstation);
	for(i = 0; i < Eaddrlen; i++)
		outb(port+i, ether->ea[i]);

	/*
	 * Enable the transceiver if necessary.
	 */
	if(xcvr & autoSelect)
		xcvr = autoselect(port, rxstatus9);
	switch(xcvr){

	case xcvrMii:
		break;

	case xcvr100BaseTX:
	case xcvr100BaseFX:
		COMMAND(port, SelectRegisterWindow, Wfifo);
		x = inl(port+InternalConfig) & ~ramPartitionMask;
		outl(port+InternalConfig, x|ramPartition1to1);

		COMMAND(port, SelectRegisterWindow, Wdiagnostic);
		x = ins(port+MediaStatus) & ~(dcConverterEnabled|jabberGuardEnable);
		x |= linkBeatEnable;
		outs(port+MediaStatus, x);
		break;

	case xcvr10BaseT:
		/*
		 * Enable Link Beat and Jabber to start the
		 * transceiver.
		 */
		COMMAND(port, SelectRegisterWindow, Wdiagnostic);
		x = ins(port+MediaStatus) & ~dcConverterEnabled;
		x |= linkBeatEnable|jabberGuardEnable;
		outs(port+MediaStatus, x);
		break;

	case xcvr10Base2:
		COMMAND(port, SelectRegisterWindow, Wdiagnostic);
		x = ins(port+MediaStatus) & ~(linkBeatEnable|jabberGuardEnable);
		outs(port+MediaStatus, x);

		/*
		 * Start the DC-DC converter.
		 * Wait > 800 microseconds.
		 */
		COMMAND(port, EnableDcConverter, 0);
		delay(1);
		break;
	}

	/*
	 * Wop is the normal operating register set.
	 * The 3C59[0257] adapters allow access to more than one register window
	 * at a time, but there are situations where switching still needs to be
	 * done, so just do it.
	 * Clear out any lingering Tx status.
	 */
	COMMAND(port, SelectRegisterWindow, Wop);
	while(inb(port+TxStatus))
		outb(port+TxStatus, 0);

	/*
	 * Allocate a controller structure and start
	 * to initialise it.
	 */
	ether->ctlr = malloc(sizeof(Ctlr));
	ctlr = ether->ctlr;
	memset(ctlr, 0, sizeof(Ctlr));

	/*
	 * Set a base TxStartThresh which will be incremented
	 * if any txUnderrun errors occur and ensure no RxEarly
	 * interrupts happen.
	 */
	ctlr->txthreshold = ETHERMINTU;
	COMMAND(port, SetTxStartThresh, ETHERMINTU);
	COMMAND(port, SetRxEarlyThresh, ETHERMAXTU);

	/*
	 * Set up the software configuration.
	 */
	ether->port = port;
	ether->attach = attach;
	ether->transmit = transmit;
	ether->interrupt = interrupt;

	return 0;
}
