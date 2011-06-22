/*
 * VIA Velocity gigabit ethernet.
 * Register info has been stolen from FreeBSD driver.
 *
 * Has been tested on:
 *	- VIA8237 (ABIT AV8): 100Mpbs Full duplex only.
 *	  It works enough to run replica/pull, vncv, ...
 *
 * To do:
 *	- 64/48 bits
 *	- autonegotiation
 *	- thresholds
 *	- dynamic ring sizing ??
 *	- link status change
 *	- shutdown
 *	- promiscuous
 *	- report error
 *	- Rx/Tx Csum
 *	- Jumbo frames
 *
 * Philippe Anel, xigh@free.fr
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
#include "ethermii.h"

#define DEBUG

enum
{
	DumpIntr	= (1<<0),
	DumpRx		= (1<<1),
	DumpTx		= (1<<2),
};

#define htole16(x) (x)
#define htole32(x) (x)
#define le32toh(x) (x)

enum
{
	Timeout		= 50000,
	RxCount		= 256,
	TxCount		= 256,
	RxSize		= 2048,

	EthAddr		= 0x00,

	/* Command registers. */
	Cr0S		= 0x08,			/* Global command 0 (Set) */
	Cr0C		= 0x0c,			/* Global command 0 (Clear) */
		Cr0_Start	= 0x01,		/* - start MAC */
		Cr0_Stop	= 0x02,		/* - stop MAC */
		Cr0_EnableRx	= 0x04,		/* - turn on Rx engine */
		Cr0_EnableTx	= 0x08,		/* - turn on Tx engine */

	Cr1S		= 0x09,			/* Global command 1 (Set) */
	Cr1C		= 0x0d,			/* Global command 1 (Clear) */
		Cr1_NoPool	= 0x08,		/* - disable Rx/Tx desc pool */
		Cr1_reset	= 0x80,		/* - software reset */

	Cr2S		= 0x0a,			/* Global command 2 (Set) */
		Cr2_XonEnable	= 0x80,		/* - 802.3x XON/XOFF flow control */

	Cr3S		= 0x0b,			/* Global command 3 (Set) */
	Cr3C		= 0x0f,			/* Global command 3 (Set) */
		Cr3_IntMask	= 0x02,		/* - Mask all interrupts */

	/* Eeprom registers. */
	Eecsr		= 0x93,			/* EEPROM control/status */
		Eecsr_Autold	= 0x20,		/* - trigger reload from EEPROM */

	/* Mii registers. */
	MiiStatus	= 0x6D,			/* MII port status */
		MiiStatus_idle	= 0x80,		/* - idle */

	MiiCmd		= 0x70,			/* MII command */
		MiiCmd_write	= 0x20,		/* - write */
		MiiCmd_read	= 0x40,		/* - read */
		MiiCmd_auto	= 0x80,		/* - enable autopolling */

	MiiAddr		= 0x71,			/* MII address */
	MiiData		= 0x72,			/* MII data */

	/* 64 bits related registers. */
	TxDescHi	= 0x18,
	DataBufHi	= 0x1d,

	/* Rx engine registers. */
	RxDescLo	= 0x38,			/* Rx descriptor base address (lo 32 bits) */
	RxCsrS		= 0x32,			/* Rx descriptor queue control/status (Set) */
	RxCsrC		= 0x36,			/* Rx descriptor queue control/status (Clear) */
		RxCsr_RunQueue	= 0x01,		/* 	- enable queue */
		RxCsr_Active	= 0x02,		/* - queue active indicator */
		RxCsr_Wakeup	= 0x04,		/* - wake up queue */
		RxCsr_Dead	= 0x08,		/* - queue dead indicator */
	RxNum		= 0x50,			/* Size of Rx desc ring */
	RxDscIdx	= 0x3c,			/* Current Rx descriptor index */
	RxResCnt	= 0x5e,			/* Rx descriptor residue count */
	RxHostErr	= 0x23,			/* Rx host error status */
	RxTimer		= 0x3e,			/* Rx queue timer pend */
	RxControl	= 0x06,			/* MAC Rx control */
		RxControl_BadFrame = 0x01,	/* - accept CRC error frames */
		RxControl_Runt = 0x02,		/* - accept runts */
		RxControl_MultiCast = 0x04,	/* - accept multicasts */
		RxControl_BroadCast = 0x08,	/* - accept broadcasts */
		RxControl_Promisc = 0x10,	/* - promisc mode */
		RxControl_Giant = 0x20,		/* - accept VLAN tagged frames */
		RxControl_UniCast = 0x40,	/* - use perfect filtering */
		RxControl_SymbolErr = 0x80,	/* - accept symbol err packet */
	RxConfig	= 0x7e,			/* MAC Rx config */
		RxConfig_VlanFilter = 0x01,	/* - filter VLAN ID mismatches */
		RxConfig_VlanOpt0 = (0<<1),	/* - TX: no tag insert, RX: all, no extr */
		RxConfig_VlanOpt1 = (1<<1),	/* - TX: no tag insert, RX: tagged pkts, no extr */
		RxConfig_VlanOpt2 = (2<<1),	/* - TX: tag insert, RX: all, extract tags */
		RxConfig_VlanOpt3 = (3<<1),	/* - TX: tag insert, RX: tagged pkts, with extr */
		RxConfig_FifoLowWat = 0x08,	/* - RX FIFO low watermark (7QW/15QW) */
		RxConfig_FifoTh128 = (0<<4),	/* - RX FIFO threshold 128 bytes */
		RxConfig_FifoTh512 = (1<<4),	/* - RX FIFO threshold 512 bytes */
		RxConfig_FifoTh1024 = (2<<4),	/* - RX FIFO threshold 1024 bytes */
		RxConfig_FifoThFwd = (3<<4),	/* - RX FIFO threshold ??? */
		RxConfig_ArbPrio = 0x80,	/* - arbitration priority */

	/* Tx engine registers. */
	TxDescLo	= 0x40,			/* Tx descriptor base address (lo 32 bits) */
	TxCsrS		= 0x30,			/* Tx descriptor queue control/status (Set) */
	TxCsrC		= 0x38,			/* Tx descriptor queue control/status (Clear) */
		TxCsr_RunQueue	= 0x01,		/* 	- enable queue */
		TxCsr_Active	= 0x02,		/* - queue active indicator */
		TxCsr_Wakeup	= 0x04,		/* - wake up queue */
		TxCsr_Dead	= 0x08,		/* - queue dead indicator */
	TxNum		= 0x52,			/* Size of Tx desc ring */
	TxDscIdx	= 0x54,			/* Current Tx descriptor index */
	TxHostErr	= 0x22,			/* Tx host error status */
	TxTimer		= 0x3f,			/* Tx queue timer pend */
	TxControl	= 0x07,			/* MAC Rx control */
		TxControl_LC_Off = (0<<0),	/* - loopback control off */
		TxControl_LC_Mac = (1<<0),	/* - loopback control MAC internal */
		TxControl_LC_Ext = (2<<0),	/* - loopback control external */
		TxControl_Coll16 = (0<<2),	/* - one set of 16 retries */
		TxControl_Coll32 = (1<<2),	/* - two sets of 16 retries */
		TxControl_Coll48 = (2<<2),	/* - three sets of 16 retries */
		TxControl_CollInf = (3<<2),	/* - retry forever */

	TxConfig	= 0x7f,			/* MAC Tx config */
		TxConfig_SnapOpt = 0x01,	/* - 1 == insert VLAN tag at 13th byte, */
										/*	  0 == insert VLAN tag after SNAP header (21st byte) */
		TxConfig_NonBlk	= 0x02,		/* - priority TX/non-blocking mode */
		TxConfig_Blk64	= (0<<3),	/* - non-blocking threshold 64 packets */
		TxConfig_Blk32	= (1<<3),	/* - non-blocking threshold 32 packets */
		TxConfig_Blk128	= (2<<3),	/* - non-blocking threshold 128 packets */
		TxConfig_Blk8	= (3<<3),	/* - non-blocking threshold 8 packets */
		TxConfig_ArbPrio	= 0x80,	/* - arbitration priority */

	/* Timer registers. */
	Timer0		= 0x74,			/* single-shot timer */
	Timer1		= 0x76,			/* periodic timer */

	/* Chip config registers. */
	ChipCfgA	= 0x78,			/* chip config A */
	ChipCfgB	= 0x79,			/* chip config B */
	ChipCfgC	= 0x7a,			/* chip config C */
	ChipCfgD	= 0x7b,			/* chip config D */

	/* DMA config registers. */
	DmaCfg0		= 0x7C,			/* DMA config 0 */
	DmaCfg1		= 0x7D,			/* DMA config 1 */

	/* Interrupt registers. */
	IntCtl		= 0x20,			/* Interrupt control */
	Imr		= 0x28,			/* Interrupt mask */
	Isr		= 0x24,			/* Interrupt status */
		Isr_RxHiPrio	= (1<<0),	/* - hi prio Rx int */
		Isr_TxHiPrio	= (1<<1),	/* - hi prio Tx int */
		Isr_RxComplete	= (1<<2),	/* - Rx queue completed */
		Isr_TxComplete	= (1<<3),	/* - One of Tx queues completed */

		Isr_TxComplete0	= (1<<4),	/* - Tx queue 0 completed */
		Isr_TxComplete1	= (1<<5),	/* - Tx queue 1 completed */
		Isr_TxComplete2	= (1<<6),	/* - Tx queue 2 completed */
		Isr_TxComplete3	= (1<<7),	/* - Tx queue 3 completed */

		Isr_Reserved8	= (1<<8),	/* - reserved */
		Isr_Reserver9	= (1<<9),	/* - reserved */
		Isr_RxCountOvflow = (1<<10),	/* - Rx packet count overflow */
		Isr_RxPause	= (1<<11),	/* - pause frame Rx */

		Isr_RxFifoOvflow = (1<<12),	/* - RX FIFO overflow */
		Isr_RxNoDesc	= (1<<13),	/* - ran out of Rx descriptors */
		Isr_RxNoDescWar	= (1<<14),	/* - running out of Rx descriptors */
		Isr_LinkStatus	= (1<<15),	/* - link status change */

		Isr_Timer0	= (1<<16),	/* - one shot timer expired */
		Isr_Timer1	= (1<<17),	/* - periodic timer expired */
		Isr_Power	= (1<<18),	/* - wake up power event */
		Isr_PhyIntr	= (1<<19),	/* - PHY interrupt */

		Isr_Stopped	= (1<<20),	/* - software shutdown complete */
		Isr_MibOvflow	= (1<<21),	/* - MIB counter overflow warning */
		Isr_SoftIntr	= (1<<22),	/* - software interrupt */
		Isr_HoldOffReload = (1<<23),	/* - reload hold timer */

		Isr_RxDmaStall	= (1<<24),	/* - Rx DMA stall */
		Isr_TxDmaStall	= (1<<25),	/* - Tx DMA stall */
		Isr_Reserved26	= (1<<26),	/* - reserved */
		Isr_Reserved27	= (1<<27),	/* - reserved */

		Isr_Source0	= (1<<28),	/* - interrupt source indication */
		Isr_Source1	= (1<<29),	/* - interrupt source indication */
		Isr_Source2	= (1<<30),	/* - interrupt source indication */
		Isr_Source3	= (1<<31),	/* - interrupt source indication */

	Isr_Mask = Isr_TxComplete0|Isr_RxComplete|Isr_Stopped|
			Isr_RxFifoOvflow|Isr_PhyIntr|Isr_LinkStatus|
			Isr_RxNoDesc|Isr_RxDmaStall|Isr_TxDmaStall
};

typedef struct Frag Frag;
struct Frag
{
	ulong	addr_lo;
	ushort	addr_hi;
	ushort	length;
};

typedef struct RxDesc RxDesc;
struct RxDesc
{
	ulong	status;
	ulong	control;
	Frag;
};

typedef struct TxDesc TxDesc;
struct TxDesc
{
	ulong	status;
	ulong	control;
	Frag	frags[7];
};

enum
{
	RxDesc_Status_VidMiss	= (1<<0),	/* VLAN tag filter miss */
	RxDesc_Status_CrcErr	= (1<<1),	/* bad CRC error */
	RxDesc_Status_FrAlErr	= (1<<3),	/* frame alignment error */
	RxDesc_Status_CsumErr	= (1<<3),	/* bad TCP/IP checksum */
	RxDesc_Status_RxLenErr	= (1<<4),	/* Rx length error */
	RxDesc_Status_SymErr	= (1<<5),	/* PCS symbol error */
	RxDesc_Status_SnTag	= (1<<6),	/* RX'ed tagged SNAP pkt */
	RxDesc_Status_DeTag	= (1<<7),	/* VLAN tag extracted */

	RxDesc_Status_OneFrag	= (0<<8),	/* only one fragment */
	RxDesc_Status_FirstFrag	= (1<<8),	/* first frag in frame */
	RxDesc_Status_LastFrag	= (2<<8),	/* last frag in frame */
	RxDesc_Status_MidFrag	= (3<<8),	/* intermediate frag */

	RxDesc_Status_Vtag	= (1<<10),	/* VLAN tag indicator */
	RxDesc_Status_UniCast	= (1<<11),	/* unicast frame */
	RxDesc_Status_BroadCast	= (1<<12),	/* broadcast frame */
	RxDesc_Status_MultiCast	= (1<<13),	/* multicast frame */
	RxDesc_Status_Perfect	= (1<<14),	/* perfect filter hit */
	RxDesc_Status_Goodframe	= (1<<15),	/* frame is good. */

	RxDesc_Status_SizShift	= 16,		/* received frame len shift */
	RxDesc_Status_SizMask	= 0x3FFF,	/* received frame len mask */

	RxDesc_Status_Shutdown	= (1<<30),	/* shutdown during RX */
	RxDesc_Status_Own	= (1<<31),	/* own bit */

	/* ... */
	TxDesc_Status_Own	= (1<<31),	/* own bit */

	/* ... */
	TxDesc_Control_Intr	= (1<<23),	/* Tx intr request */
	TxDesc_Control_Normal	= (3<<24),	/* normal frame */
};

typedef struct Stats Stats;
struct Stats
{
	ulong	rx;
	ulong	tx;
	ulong	txe;
	ulong	intr;
};

typedef struct Ctlr Ctlr;
struct Ctlr
{
	Ctlr*	link;
	Pcidev*	pdev;
	int	port;

	int	inited;
	Lock	init_lock;

	ulong	debugflags;
	ulong	debugcount;

	Mii*	mii;
	int	active;
	uchar	ea[6];

	RxDesc*	rx_ring;
	Block*	rx_blocks[RxCount];

	Lock	tx_lock;
	TxDesc*	tx_ring;
	Block*	tx_blocks[TxCount];
	ulong	tx_count;

	Stats	stats;
};

static Ctlr* vgbehead;
static Ctlr* vgbetail;

#define riob(c, r)	inb(c->port + r)
#define riow(c, r)	ins(c->port + r)
#define riol(c, r)	inl(c->port + r)
#define wiob(c, r, d)	outb(c->port + r, d)
#define wiow(c, r, d)	outs(c->port + r, d)
#define wiol(c, r, d)	outl(c->port + r, d)

#define siob(c, r, b)	wiob(c, r, riob(c, r) | b)
#define siow(c, r, b)	wiow(c, r, riob(c, r) | b)
#define siol(c, r, b)	wiol(c, r, riob(c, r) | b)
#define ciob(c, r, b)	wiob(c, r, riob(c, r) & ~b)
#define ciow(c, r, b)	wiow(c, r, riob(c, r) & ~b)
#define ciol(c, r, b)	wiol(c, r, riob(c, r) & ~b)

static int
vgbemiiw(Mii* mii, int phy, int addr, int data)
{
	Ctlr* ctlr;
	int i;

	if(phy != 1)
		return -1;

	ctlr = mii->ctlr;

	wiob(ctlr, MiiAddr, addr);
	wiow(ctlr, MiiData, (ushort) data);
	wiob(ctlr, MiiCmd, MiiCmd_write);

	for(i = 0; i < Timeout; i++)
		if((riob(ctlr, MiiCmd) & MiiCmd_write) == 0)
			break;

	if(i >= Timeout){
		print("vgbe: miiw timeout\n");
		return -1;
	}

	return 0;
}

static int
vgbemiir(Mii* mii, int phy, int addr)
{
	Ctlr* ctlr;
	int i;

	if(phy != 1)
		return -1;

	ctlr = mii->ctlr;

	wiob(ctlr, MiiAddr, addr);
	wiob(ctlr, MiiCmd, MiiCmd_read);

	for(i = 0; i < Timeout; i++)
		if((riob(ctlr, MiiCmd) & MiiCmd_read) == 0)
			break;

	if(i >= Timeout){
		print("vgbe: miir timeout\n");
		return -1;
	}

	return riow(ctlr, MiiData);
}

static long
vgbeifstat(Ether* edev, void* a, long n, ulong offset)
{
	char* p;
	Ctlr* ctlr;
	int l;

	ctlr = edev->ctlr;

	p = malloc(READSTR);
	if(p == nil)
		error(Enomem);
	l = 0;
	l += snprint(p+l, READSTR-l, "tx: %uld\n", ctlr->stats.tx);
	l += snprint(p+l, READSTR-l, "tx [errs]: %uld\n", ctlr->stats.txe);
	l += snprint(p+l, READSTR-l, "rx: %uld\n", ctlr->stats.rx);
	l += snprint(p+l, READSTR-l, "intr: %uld\n", ctlr->stats.intr);
	snprint(p+l, READSTR-l, "\n");

	n = readstr(offset, a, n, p);
	free(p);

	return n;
}

static char* vgbeisr_info[] = {
	"hi prio Rx int",
	"hi prio Tx int",
	"Rx queue completed",
	"One of Tx queues completed",
	"Tx queue 0 completed",
	"Tx queue 1 completed",
	"Tx queue 2 completed",
	"Tx queue 3 completed",
	"reserved",
	"reserved",
	"Rx packet count overflow",
	"pause frame Rx'ed",
	"RX FIFO overflow",
	"ran out of Rx descriptors",
	"running out of Rx descriptors",
	"link status change",
	"one shot timer expired",
	"periodic timer expired",
	"wake up power event",
	"PHY interrupt",
	"software shutdown complete",
	"MIB counter overflow warning",
	"software interrupt",
	"reload hold timer",
	"Rx DMA stall",
	"Tx DMA stall",
	"reserved",
	"reserved",
	"interrupt source indication 0",
	"interrupt source indication 1",
	"interrupt source indication 2",
	"interrupt source indication 3",
};

static void
vgbedumpisr(ulong isr)
{
	int i;

	for(i = 0; i < 32; i++){
		ulong mask;

		mask = 1<<i;
		if(isr & mask)
			print("vgbe: irq:  - %02d : %c %s\n", i,
			 	Isr_Mask & mask ? '*' : '-', vgbeisr_info[i]);
	}
}

static void
noop(Block *)
{
}

static int
vgbenewrx(Ctlr* ctlr, int i)
{
	Block* block;
	RxDesc* desc;

	/*
	 * allocate a receive Block.  we're maintaining
	 * a private pool of Blocks, so we don't want freeb
	 * to actually free them, thus we set block->free.
	 */
	block = allocb(RxSize);
	block->free = noop;

	/* Remember that block. */
	ctlr->rx_blocks[i] = block;

	/* Initialize Rx descriptor. (TODO: 48/64 bits support ?) */
	desc = &ctlr->rx_ring[i];
	desc->status = htole32(RxDesc_Status_Own);
	desc->control = htole32(0);

	desc->addr_lo = htole32((ulong)PCIWADDR(block->rp));
	desc->addr_hi = htole16(0);
	desc->length = htole16(RxSize | 0x8000);

	return 0;
}

static void
vgberxeof(Ether* edev)
{
	Ctlr* ctlr;
	int i;
	Block* block;
	ulong length, status;
	RxDesc* desc;

	ctlr = edev->ctlr;

	if(ctlr->debugflags & DumpRx)
		print("vgbe: rx_eof\n");

	for(i = 0; i < RxCount; i++){
		/* Remember that block. */
		desc = &ctlr->rx_ring[i];

		status = le32toh(desc->status);

		if(status & RxDesc_Status_Own)
			continue;

		if(status & RxDesc_Status_Goodframe){
			length = status >> RxDesc_Status_SizShift;
			length &= RxDesc_Status_SizMask;

			if(ctlr->debugflags & DumpRx)
				print("vgbe: Rx-desc[%03d] status=%#08ulx ctl=%#08ulx len=%uld bytes\n",
					i, status, desc->control, length);

			block = ctlr->rx_blocks[i];
			block->wp = block->rp + length;

			ctlr->stats.rx++;
			etheriq(edev, block, 1);
		}
		else
			print("vgbe: Rx-desc[%#02x] *BAD FRAME* status=%#08ulx ctl=%#08ulx\n",
				i, status, desc->control);

		/* reset packet ... */
		desc->status = htole32(RxDesc_Status_Own);
		desc->control = htole32(0);
	}

	if(ctlr->debugflags & DumpRx)
		print("vgbe: rx_eof: done\n");

	wiow(ctlr, RxResCnt, RxCount);
	wiob(ctlr, RxCsrS, RxCsr_Wakeup);
}

static void
vgbetxeof(Ether* edev)
{
	Ctlr* ctlr;
	int i, count;
	Block* block;
	ulong status;

	ctlr = edev->ctlr;

	ilock(&ctlr->tx_lock);

	if(ctlr->debugflags & DumpTx)
		print("vgbe: tx_eof\n");

	for(count = 0, i = 0; i < TxCount; i++){
		block = ctlr->tx_blocks[i];
		if(block == nil)
			continue;

		status = le32toh(ctlr->tx_ring[i].status);
		if(status & TxDesc_Status_Own)
			continue;

		/* Todo add info if it failed */
		ctlr->stats.tx++;

		if(ctlr->debugflags & DumpTx)
			print("vgbe: Block[%03d]:%#p has been sent\n", i, block);

		count++;
		ctlr->tx_blocks[i] = nil;
		freeb(block);

		if(ctlr->debugflags & DumpTx)
			print("vgbe: Block[%03d]:%#p has been freed\n", i, block);
	}
	ctlr->tx_count -= count;

	if(ctlr->debugflags & DumpTx)
		print("vgbe: tx_eof: done [count=%d]\n", count);

	iunlock(&ctlr->tx_lock);

	if(ctlr->tx_count)
		wiob(ctlr, TxCsrS, TxCsr_Wakeup);
}

static void
vgbeinterrupt(Ureg *, void* arg)
{
	Ether* edev;
	Ctlr* ctlr;
	ulong status;

	edev = (Ether *) arg;
	if(edev == nil)
		return;

	ctlr = edev->ctlr;
	if(ctlr == nil)
		return;

	/* Mask interrupts. */
	wiol(ctlr, Imr, 0);

	status = riol(ctlr, Isr);
	if(status == 0xffff)
		goto end;

	/* acknowledge */
	if(status)
		wiol(ctlr, Isr, status);

	if((status & Isr_Mask) == 0)
		goto end;

	ctlr->stats.intr++;

	if(ctlr->debugflags & DumpIntr)
		if(ctlr->debugcount){
			print("vgbe: irq: status = %#08ulx\n", status);
			vgbedumpisr(status);
			ctlr->debugcount--;
		}

	if(status & Isr_RxComplete)
		vgberxeof(edev);

	if(status & Isr_TxComplete0)
		vgbetxeof(edev);

	if(status & Isr_Stopped)
		print("vgbe: irq: software shutdown complete\n");

	if(status & Isr_RxFifoOvflow)
		print("vgbe: irq: RX FIFO overflow\n");

	if(status & Isr_PhyIntr)
		print("vgbe: irq: PHY interrupt\n");

	if(status & Isr_LinkStatus)
		print("vgbe: irq: link status change\n");

	if(status & Isr_RxNoDesc)
		print("vgbe: irq: ran out of Rx descriptors\n");

	if(status & Isr_RxDmaStall){
		print("vgbe: irq: Rx DMA stall\n");
		wiol(ctlr, Cr3C, Cr3_IntMask);
		return;
	}

	if(status & Isr_TxDmaStall){
		print("vgbe: irq: Tx DMA stall\n");
		wiol(ctlr, Cr3C, Cr3_IntMask);
		return;
	}

end:
	/* Unmask interrupts. */
	wiol(ctlr, Imr, ~0);
}

static void
vgbetransmit(Ether* edev)
{
	Block* block;
	Ctlr* ctlr;
	int i, index, start, count;
	TxDesc* desc;
	ulong status, length;

	ctlr = edev->ctlr;

	ilock(&ctlr->tx_lock);

	start = riow(ctlr, TxDscIdx);

	if(ctlr->debugflags & DumpTx)
		print("vgbe: transmit (start=%d)\n", start);

	/* find empty slot */
	for(count = 0, i = 0; i < TxCount; i++){
		index = (i + start) % TxCount;

		if(ctlr->tx_blocks[index])
			continue;

		desc = &ctlr->tx_ring[index];

		status = le32toh(desc->status);
		if(status & TxDesc_Status_Own)
			continue;

		block = qget(edev->oq);
		if(block == nil)
			break;

		count++;

		length = BLEN(block);

		if(ctlr->debugflags & DumpTx)
			print("vgbe: Tx-Desc[%03d] Block:%#p, addr=%#08ulx, len:%ld\n", index, block,
				PCIWADDR(block->rp), length);

		ctlr->tx_blocks[index] = block;

		/* Initialize Tx descriptor. */
		desc->status = htole32((length<<16)|TxDesc_Status_Own);
		desc->control = htole32(TxDesc_Control_Intr|TxDesc_Control_Normal|((1+1)<<28));

		desc->frags[0].addr_lo = htole32((ulong) PCIWADDR(block->rp));
		desc->frags[0].addr_hi = htole16(0);
		desc->frags[0].length = htole16(length);
	}
	ctlr->tx_count += count;

	if(ctlr->debugflags & DumpTx)
		print("vgbe: transmit: done [count=%d]\n", count);

	iunlock(&ctlr->tx_lock);

	if(ctlr->tx_count)
		wiob(ctlr, TxCsrS, TxCsr_Wakeup);

	if(count == 0)
		print("vgbe: transmit: no Tx entry available\n");
}

static void
vgbeattach(Ether* edev)
{
	Ctlr* ctlr;
	RxDesc* rxdesc;
	TxDesc* txdesc;
	int i;

	ctlr = edev->ctlr;

	lock(&ctlr->init_lock);
	if(ctlr->inited){
		unlock(&ctlr->init_lock);
		return;
	}

//	print("vgbe: attach\n");

	/* Allocate Rx ring.  (TODO: Alignment ?) */
	rxdesc = mallocalign(RxCount* sizeof(RxDesc), 256, 0, 0);
	if(rxdesc == nil){
		print("vgbe: unable to alloc Rx ring\n");
		unlock(&ctlr->init_lock);
		return;
	}
	ctlr->rx_ring = rxdesc;

	/* Allocate Rx blocks, initialize Rx ring. */
	for(i = 0; i < RxCount; i++)
		vgbenewrx(ctlr, i);

	/* Init Rx MAC. */
	wiob(ctlr, RxControl,
		RxControl_MultiCast|RxControl_BroadCast|RxControl_UniCast);
	wiob(ctlr, RxConfig, RxConfig_VlanOpt0);

	/* Load Rx ring. */
	wiol(ctlr, RxDescLo, (ulong) PCIWADDR(rxdesc));
	wiow(ctlr, RxNum, RxCount - 1);
	wiow(ctlr, RxDscIdx, 0);
	wiow(ctlr, RxResCnt, RxCount);

	/* Allocate Tx ring. */
	txdesc = mallocalign(TxCount* sizeof(TxDesc), 256, 0, 0);
	if(txdesc == nil){
		print("vgbe: unable to alloc Tx ring\n");
		unlock(&ctlr->init_lock);
		return;
	}
	ctlr->tx_ring = txdesc;

	/* Init DMAs */
	wiob(ctlr, DmaCfg0, 4);

	/* Init Tx MAC. */
	wiob(ctlr, TxControl, 0);
	wiob(ctlr, TxConfig, TxConfig_NonBlk|TxConfig_ArbPrio);

	/* Load Tx ring. */
	wiol(ctlr, TxDescLo, (ulong) PCIWADDR(txdesc));
	wiow(ctlr, TxNum, TxCount - 1);
	wiow(ctlr, TxDscIdx, 0);

	/* Enable Xon/Xoff */
	wiob(ctlr, Cr2S, 0xb|Cr2_XonEnable);

	/* Enable Rx queue */
	wiob(ctlr, RxCsrS, RxCsr_RunQueue);

	/* Enable Tx queue */
	wiob(ctlr, TxCsrS, TxCsr_RunQueue);

	/* Done */
	ctlr->inited = 1;
	unlock(&ctlr->init_lock);

	/* Enable interrupts */
	wiol(ctlr, Isr, 0xffffffff);
	wiob(ctlr, Cr3S, Cr3_IntMask);

	/* Wake up Rx queue */
	wiob(ctlr, RxCsrS, RxCsr_Wakeup);
}

static void
vgbereset(Ctlr* ctlr)
{
//	MiiPhy* phy;
	int timeo, i;

//	print("vgbe: reset\n");

	/* Soft reset the controller. */
	wiob(ctlr, Cr1S, Cr1_reset);

	for(timeo = 0; timeo < Timeout; timeo++)
		if((riob(ctlr, Cr1S) & Cr1_reset) == 0)
			break;

	if(timeo >= Timeout){
		print("vgbe: softreset timeout\n");
		return;
	}

	/* Reload eeprom. */
	siob(ctlr, Eecsr, Eecsr_Autold);

	for(timeo = 0; timeo < Timeout; timeo++)
		if((riob(ctlr, Eecsr) & Eecsr_Autold) == 0)
			break;

	if(timeo >= Timeout){
		print("vgbe: eeprom reload timeout\n");
		return;
	}

	/* Load the MAC address. */
	for(i = 0; i < Eaddrlen; i++)
		ctlr->ea[i] = riob(ctlr, EthAddr+i);

	/* Initialize interrupts. */
	wiol(ctlr, Isr, 0xffffffff);
	wiol(ctlr, Imr, 0xffffffff);

	/* Disable interrupts. */
	wiol(ctlr, Cr3C, Cr3_IntMask);

	/* 32 bits addresses only. (TODO: 64 bits ?) */
	wiol(ctlr, TxDescHi, 0);
	wiow(ctlr, DataBufHi, 0);

	/* Enable MAC (turning off Rx/Tx engines for the moment). */
	wiob(ctlr, Cr0C, Cr0_Stop|Cr0_EnableRx|Cr0_EnableTx);
	wiob(ctlr, Cr0S, Cr0_Start);

	/* Initialize Rx engine. */
	wiow(ctlr, RxCsrC, RxCsr_RunQueue);

	/* Initialize Tx engine. */
	wiow(ctlr, TxCsrC, TxCsr_RunQueue);

	/* Enable Rx/Tx engines. */
	wiob(ctlr, Cr0S, Cr0_EnableRx|Cr0_EnableTx);

	/* Initialize link management. */
	ctlr->mii = malloc(sizeof(Mii));
	if(ctlr->mii == nil){
		print("vgbe: unable to alloc Mii\n");
		return;
	}

	ctlr->mii->mir = vgbemiir;
	ctlr->mii->miw = vgbemiiw;
	ctlr->mii->ctlr = ctlr;

	if(mii(ctlr->mii, 1<<1) == 0){
		print("vgbe: no phy found\n");
		return;
	}

//	phy = ctlr->mii->curphy;
//	print("vgbe: phy:oui %#x\n", phy->oui);
}

static void
vgbepci(void)
{
	Pcidev* pdev;

//	print("vgbe: pci\n");

	pdev = nil;
	while(pdev = pcimatch(pdev, 0, 0)){
		Ctlr* ctlr;
		int port, size;

		if(pdev->ccrb != 0x02 || pdev->ccru != 0)
			continue;

		switch((pdev->did<<16) | pdev->vid){
		default:
			continue;

		case (0x3119<<16)|0x1106:	/* VIA Velocity (VT6122) */
			break;
		}

		if((pdev->pcr & 1) == 0){
			print("vgbe: io not enabled [pcr=%#lux]\n", (ulong)pdev->pcr);
			continue;
		}

		pcisetbme(pdev);
		pcisetpms(pdev, 0);

		port = pdev->mem[0].bar;
		size = pdev->mem[0].size;

		if((port & 1) == 0){
			print("vgbe: bar[0]=%#x is not io\n", port);
			continue;
		}

		if(port > 0xff00){
			print("vgbe: invalid port %#ux\n", port);
			continue;
		}

		port &= 0xfffe;

		if(size != 256){
			print("vgbe: invalid io size: %d\n", size);
			continue;
		}

		if(ioalloc(port, size, 0, "vge") < 0){
			print("vgbe: port %#ux already in use\n", port);
			continue;
		}

		ctlr = malloc(sizeof(Ctlr));
		if(ctlr == nil){
			print("vgbe: unable to alloc Ctlr\n");
			iofree(port);
			continue;
		}

		ctlr->pdev = pdev;
		ctlr->port = port;
		ctlr->inited = 0;

		if(vgbehead != nil)
			vgbetail->link = ctlr;
		else
			vgbehead = ctlr;
		vgbetail = ctlr;
	}
}

static long
vgbectl(Ether* edev, void* buf, long n)
{
	Cmdbuf* cb;
	Ctlr* ctlr;
	ulong index;
	char* rptr;
	RxDesc* rd;
	TxDesc* td;
	uchar* p;

	ctlr = edev->ctlr;

	cb = parsecmd(buf, n);
	if(waserror()){
		free(cb);
		nexterror();
	}

	if(cistrcmp(cb->f[0], "reset") == 0){
		vgbereset(ctlr);
		wiob(ctlr, Cr3S, Cr3_IntMask);
		wiob(ctlr, RxCsrS, RxCsr_RunQueue);
		wiob(ctlr, RxCsrS, RxCsr_Wakeup);
	}
	else if(cistrcmp(cb->f[0], "dumpintr") == 0){
		if(cb->nf < 2)
			error(Ecmdargs);

		if(cistrcmp(cb->f[1], "on") == 0){
			ctlr->debugflags |= DumpIntr;
			ctlr->debugcount = ~0;
		}
		else if(cistrcmp(cb->f[1], "off") == 0)
			ctlr->debugflags &= ~DumpIntr;
		else{
			ulong count;
			char* rptr;

			count = strtoul(cb->f[1], &rptr, 0);
			if(rptr == cb->f[1])
				error("invalid control request");

			ctlr->debugflags |= DumpIntr;
			ctlr->debugcount = count;

			print("vgbe: debugcount set to %uld\n", count);
		}
	}
	else if(cistrcmp(cb->f[0], "dumprx") == 0){
		if(cb->nf < 2)
			error(Ecmdargs);

		if(cistrcmp(cb->f[1], "on") == 0)
			ctlr->debugflags |= DumpRx;
		else if(cistrcmp(cb->f[1], "off") == 0)
			ctlr->debugflags &= ~DumpRx;
		else{
			index = strtoul(cb->f[1], &rptr, 0);
			if((rptr == cb->f[1]) || (index >= RxCount))
				error("invalid control request");

			rd = &ctlr->rx_ring[index];
			print("vgbe: DumpRx[%03uld] status=%#08ulx ctl=%#08ulx len=%#04ux bytes\n",
				index, rd->status, rd->control, rd->length);
		}
	}
	else if(cistrcmp(cb->f[0], "dumptx") == 0){
		if(cb->nf < 2)
			error(Ecmdargs);

		if(cistrcmp(cb->f[1], "on") == 0)
			ctlr->debugflags |= DumpTx;
		else if(cistrcmp(cb->f[1], "off") == 0)
			ctlr->debugflags &= ~DumpTx;
		else{
			index = strtoul(cb->f[1], &rptr, 0);
			if((rptr == cb->f[1]) || (index >= TxCount))
				error("invalid control request");

			td = &ctlr->tx_ring[index];
			print("vgbe: DumpTx[%03uld] status=%#08ulx ctl=%#08ulx len=%#04ux bytes",
				index, td->status, td->control, td->frags[0].length);

			p = (uchar*)td;
			for(index = 0; index < sizeof(TxDesc); index++){
				if((index % 16) == 0)
					print("\nvgbe: ");
				else
					print(" ");
				print("%#02x", p[index]);
			}
		}
	}
	else if(cistrcmp(cb->f[0], "dumpall") == 0){
		if(cb->nf < 2)
			error(Ecmdargs);

		if(cistrcmp(cb->f[1], "on") == 0){
			ctlr->debugflags = ~0;
			ctlr->debugcount = ~0;
		}
		else if(cistrcmp(cb->f[1], "off") == 0)
			ctlr->debugflags = 0;
		else error("invalid control request");
	}
	else
		error(Ebadctl);

	free(cb);
	poperror();

	return n;
}

static void
vgbepromiscuous(void* arg, int on)
{
	USED(arg, on);
}

/* multicast already on, don't need to do anything */
static void
vgbemulticast(void*, uchar*, int)
{
}

static void
vgbeshutdown(Ether* ether)
{
	vgbereset(ether->ctlr);
}

static int
vgbepnp(Ether* edev)
{
	Ctlr* ctlr;

//	print("vgbe: pnp\n");

	if(vgbehead == nil)
		vgbepci();

	for(ctlr = vgbehead; ctlr != nil; ctlr = ctlr->link){
		if(ctlr->active)
			continue;

		if(edev->port == 0 || edev->port == ctlr->port){
			ctlr->active = 1;
			break;
		}
	}

	if(ctlr == nil)
		return -1;

	vgbereset(ctlr);

	edev->ctlr = ctlr;
	edev->port = ctlr->port;
	edev->irq = ctlr->pdev->intl;
	edev->tbdf = ctlr->pdev->tbdf;
	edev->mbps = 1000;
	memmove(edev->ea, ctlr->ea, Eaddrlen);
	edev->attach = vgbeattach;
	edev->transmit = vgbetransmit;
	edev->interrupt = vgbeinterrupt;
	edev->ifstat = vgbeifstat;
//	edev->promiscuous = vgbepromiscuous;
	edev->multicast = vgbemulticast;
	edev->shutdown = vgbeshutdown;
	edev->ctl = vgbectl;

	edev->arg = edev;
	return 0;
}

void
ethervgbelink(void)
{
	addethercard("vgbe", vgbepnp);
}
