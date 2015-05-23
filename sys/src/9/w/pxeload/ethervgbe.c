/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

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
 *	- shutxbufown
 *	- promiscuous
 *	- report error
 *	- Rx/Tx Csum
 *	- Jumbo frames
 *
 * Philippe Anel, xigh@free.fr
 */

#include "u.h"
#include "lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

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
	Timeout		= 50000, // 10000000, //
	RxCount		= 16,
	TxCount		= 4,
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
	TdHi	= 0x18,
	DataBufHi	= 0x1d,

	/* Rx engine registers. */
	RdLo	= 0x38,			/* Rx descriptor base address (lo 32 bits) */
	RxCsrS		= 0x32,			/* Rx descriptor queue control/status (Set) */
	RxCsrC		= 0x36,			/* Rx descriptor queue control/status (Clear) */
		RxCsr_RunQueue	= 0x01,		/* 	- enable queue */
		RxCsr_Active	= 0x02,		/* - queue active indicator */
		RxCsr_Wakeup	= 0x04,		/* - wake up queue */
		RxCsr_Dead	= 0x08,		/* - queue dead indicator */
	Rdcsize		= 0x50,			/* Size of Rx desc ring */
	Rdindex	= 0x3c,			/* Current Rx descriptor index */
	RxResCnt	= 0x5e,			/* Rx descriptor residue count */
	Txesr = 0x22,
		Tfdbs = (1<<19),
		Tdwbs = (1<<18),
		Tdrbs = (1<<17),
		Tdstr = (1<<16),
	Rxesr	= 0x23,			/* Rx host error status */
		Rfdbs = (1<<19),
		Rdwbs = (1<<18),
		Rdrbs = (1<<17),
		Rdstr = (1<<16),
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
	TdLo	= 0x40,			/* Tx descriptor base address (lo 32 bits) */
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
	isr		= 0x24,			/* Interrupt status */
		Pprx	= (1<<0),	/* - hi prio Rx int */
		Pptx	= (1<<1),	/* - hi prio Tx int */
		Prx	= (1<<2),	/* - Rx queue completed */
		Ptx	= (1<<3),	/* - One of Tx queues completed */

		Ptx0	= (1<<4),	/* - Tx queue 0 completed */
		Ptx1	= (1<<5),	/* - Tx queue 1 completed */
		Ptx2	= (1<<6),	/* - Tx queue 2 completed */
		Ptx3	= (1<<7),	/* - Tx queue 3 completed */

		isr_Reserved8	= (1<<8),	/* - reserved */
		isr_Reserver9	= (1<<9),	/* - reserved */
		Race = (1<<10),	/* - Rx packet count overflow */
		Flon	= (1<<11),	/* - pause frame Rx */

		Ovf = (1<<12),	/* - RX FIFO overflow */
		Lste	= (1<<13),	/* - ran out of Rx descriptors */
		Lstpe	= (1<<14),	/* - running out of Rx descriptors */
		Src	= (1<<15),	/* - link status change */

		Tmr0	= (1<<16),	/* - one shot timer expired */
		Tmr1	= (1<<17),	/* - periodic timer expired */
		Pwe	= (1<<18),	/* - wake up power event */
		Phyint	= (1<<19),	/* - PHY interrupt */

		Shdn	= (1<<20),	/* - software shutxbufown complete */
		isr_MibOvflow	= (1<<21),	/* - MIB counter overflow warning */
		isr_SoftIntr	= (1<<22),	/* - software interrupt */
		isr_HoldOffReload = (1<<23),	/* - reload hold timer */

		Rxstl	= (1<<24),	/* - Rx DMA stall */
		Txstl	= (1<<25),	/* - Tx DMA stall */
		isr_Reserved26	= (1<<26),	/* - reserved */
		isr_Reserved27	= (1<<27),	/* - reserved */

		Isr0	= (1<<28),	/* - interrupt source indication */
		Isr1	= (1<<29),	/* - interrupt source indication */
		Isr2	= (1<<30),	/* - interrupt source indication */
		Isr3	= (1<<31),	/* - interrupt source indication */

	isr_Mask = Ptx0|Prx|Shdn|
			Ovf|Phyint|Src|
			Lste|Rxstl|Txstl
};

typedef struct Frag Frag;
struct Frag
{
	uint32_t	addr_lo;
	uint16_t	addr_hi;
	uint16_t	length;
};

typedef struct Rd Rd;
struct Rd
{
	uint32_t	status;
	uint32_t	control;
	Frag;
};

typedef struct Td Td;
struct Td
{
	uint32_t	status;
	uint32_t	control;
	Frag	frags[7];
};

enum
{
	Vidm	= (1<<0),	/* VLAN tag filter miss */
	Crc	= (1<<1),	/* bad CRC error */
	Fae	= (1<<3),	/* frame alignment error */
	Ce	= (1<<3),	/* bad TCP/IP checksum */
	Rl	= (1<<4),	/* Rx length error */
	Rxer	= (1<<5),	/* PCS symbol error */
	Sntag	= (1<<6),	/* RX'ed tagged SNAP pkt */
	Detag	= (1<<7),	/* VLAN tag extracted */

	RDOneFrag	= (0<<8),	/* only one fragment */
	RDFirstFrag	= (1<<8),	/* first frag in frame */
	RDLastFrag	= (2<<8),	/* last frag in frame */
	RDMidFrag	= (3<<8),	/* intermediate frag */

	Vtag	= (1<<10),	/* VLAN tag indicator */
	Phy	= (1<<11),	/* unicast frame */
	Bar	= (1<<12),	/* broadcast frame */
	Mar	= (1<<13),	/* multicast frame */
	Pft	= (1<<14),	/* perfect filter hit */
	Rxok	= (1<<15),	/* frame is good. */

	RDSizShift	= 16,		/* received frame len shift */
	RDSizMask	= 0x3FFF,	/* received frame len mask */

	RDShutxbufown	= (1<<30),	/* shutxbufown during RX */
	Own	= (1<<31),	/* own bit */

	/* ... */

	/* ... */
	Td_Control_Intr	= (1<<23),	/* Tx intr request */
	Td_Control_Normal	= (3<<24),	/* normal frame */
};

typedef struct Stats Stats;
struct Stats
{
	uint32_t	rx;
	uint32_t	tx;
	uint32_t	txe;
	uint32_t	intr;
};

enum {
	Rblen64K	= 0x00001800,	/* 64KB+16 */

	Rblen		= Rblen64K,	/* Receive Buffer Length */
	Tdbsz		= ROUNDUP(sizeof(Etherpkt), 4),
};

typedef struct Ctlr Ctlr;
struct Ctlr
{
	int	port;
	Pcidev*	pcidev;
	Ctlr	*next;
	int	id;

	int	inited;
	Lock	ilock;

	uint32_t	debugflags;
	uint32_t	debugcount;

	Mii*	mii;
	int	active;
	uint8_t	ea[6];

	Rd*	rdring;
	uint8_t*	rxbuf[RxCount];

	Lock	tlock;
	Td*	td;
	uint8_t	txpkt[TxCount][sizeof(Etherpkt)];
	uint32_t	tx_count;

	Stats	stats;
};

static Ctlr* vgbectlrhead;
static Ctlr* vgbectlrtail;

#define riob(c, r)	inb((c)->port + (r))
#define riow(c, r)	ins((c)->port + (r))
#define riol(c, r)	inl((c)->port + (r))
#define wiob(c, r, d)	outb((c)->port + (r), (d))
#define wiow(c, r, d)	outs((c)->port + (r), (d))
#define wiol(c, r, d)	outl((c)->port + (r), (d))

#define siob(c, r, b)	wiob((c), (r), riob((c), (r)) | (b))
#define siow(c, r, b)	wiow((c), (r), riob((c), r) | b)
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

	// write our addr to the Mii
	wiob(ctlr, MiiAddr, addr);
	// write our data to the Mii data reg
	wiow(ctlr, MiiData, (uint16_t) data);
	wiob(ctlr, MiiCmd, MiiCmd_write);

	// poll the mii
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
	"software shutxbufown complete",
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
vgbedumpisr(uint32_t isr)
{
	int i;
	uint32_t mask;

	for(i = 0; i < 32; i++){
		mask = 1<<i;
		if(isr & mask)
			print("vgbe: irq:  - %02d : %c %s\n", i,
			 	isr_Mask & mask ? '*' : '-', vgbeisr_info[i]);
	}
}

static void
noop(Block *)
{
}

static int
vgbenewrx(Ether* edev, int i)
{
	Rd* rd;
	uint8_t *rb;
	Ctlr* ctlr;

	ctlr = edev->ctlr;

	/*
	 * allocate a receive Block.  we're maintaining
	 * a private pool of Blocks, so we don't want freeb
	 * to actually free them, thus we set block->free.
	 */
	rb = mallocz(RxSize, 1);
	rb = (uint8_t*)ROUNDUP((uint32_t)rb, 64);
	ctlr->rxbuf[i] = rb;

	/* Initialize Rx descriptor. (TODO: 48/64 bits support ?) */
	rd = &ctlr->rdring[i];
	rd->status = htole32(Own);
	rd->control = htole32(0);

	rd->addr_lo = htole32((uint32_t)PADDR(rb));
	rd->addr_hi = htole16(0);
	rd->length = htole16(RxSize | 0x8000);

	return 0;
}

static void
toringbuf(Ether* edev, uint8_t* data, int len)
{
	RingBuf *rb;
	extern int interesting(void*);

	if(!interesting(data))
		return;

	rb = &edev->rb[edev->ri];
	if(rb->owner == Interface){
		if(len > sizeof(rb->pkt))
			len = sizeof(rb->pkt);
		rb->len = len;
		memmove(rb->pkt, data, rb->len);
		rb->owner = Host;
		edev->ri = NEXT(edev->ri, edev->nrb);
	}
	else if(debug){
		print("#l%d: toringbuf: dropping packets @ ri %d\n",
			edev->ctlrno, edev->ri);
	}
}

static void
vgberxeof(Ether* edev)
{
	Ctlr* ctlr;
	int i;
	uint32_t length, status;
	Rd* rd;
	uint8_t *rb;

	ctlr = edev->ctlr;

	if(ctlr->debugflags & DumpRx)
		print("vgbe: rx_eof\n");

	for(i = 0; i < RxCount; i++){
		rd = &ctlr->rdring[i];

		status = le32toh(rd->status);

		if(status & Own)
			continue;

		if(status & Rxok){
			if(ctlr->debugflags & DumpRx)
				print("vgbe: Receive successful!\n");
			length = status >> RDSizShift;
			length &= RDSizMask;

			if(ctlr->debugflags & DumpRx)
				print("vgbe: Rx-rd[%03d] status=%#08ulx ctl=%#08ulx len=%uld bytes\n",
					i, status, rd->control, length);

			rb = ctlr->rxbuf[i];
			ctlr->stats.rx++;
			if(ctlr->debugflags & DumpRx){
				print("&edev->rb[edev->ri] %ulx rb %ulx length %uld\n",  &edev->rb[edev->ri], rb, length);
				for(i = 0; i < length; i++)
					print("%x ", rb[i]);
				print("\n");
			}
			toringbuf(edev, rb, length);
		}else
			print("vgbe: Rx-rd[%#02x] *BAD FRAME* status=%#08ulx ctl=%#08ulx\n",
				i, status, rd->control);

		/* reset packet ... */
		rd->status = htole32(Own);
		rd->control = htole32(0);
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
	uint32_t status;

	ctlr = edev->ctlr;

	ilock(&ctlr->tlock);

	if(ctlr->debugflags & DumpTx)
		print("vgbe: tx_eof\n");

	for(count = 0, i = 0; i < TxCount; i++){
		status = le32toh(ctlr->td[i].status);
		if(status & Own)
			continue;

		/* Todo add info if it failed */
		ctlr->stats.tx++;

		count++;
	}
	ctlr->tx_count -= count;

	if(ctlr->debugflags & DumpTx)
		print("vgbe: tx_eof: done [count=%d]\n", count);

	iunlock(&ctlr->tlock);

	if(ctlr->tx_count)
		wiob(ctlr, TxCsrS, TxCsr_Wakeup);
}

static void
vgbetransmit(Ether* edev)
{
	uint8_t *pkt;
	Ctlr* ctlr;
	int i, index, start, count;
	Td* desc;
	RingBuf *rb;
	uint32_t status, length;

	ctlr = edev->ctlr;

	ilock(&ctlr->tlock);

	/* find empty slot */
	start = riow(ctlr, TxDscIdx);
	for(count = 0, i = 0; i < TxCount; i++){
		index = (i + start) % TxCount;

		desc = &ctlr->td[index];
		status = le32toh(desc->status);
		if(status & Own)
			continue;

		rb = &edev->tb[edev->ti];
		if(rb->owner != Interface)
			break;

		pkt = ctlr->txpkt[index];
		length = rb->len;
		memmove(pkt, rb->pkt, length);

		count++;

		/* Initialize Tx descriptor. */
		desc->status = htole32((length<<16)|Own);
		desc->control = htole32(Td_Control_Intr|Td_Control_Normal|((1+1)<<28));

		desc->frags[0].addr_lo = htole32((uint32_t) PCIWADDR(pkt));
		desc->frags[0].addr_hi = htole16(0);
		desc->frags[0].length = htole16(length);
		rb->owner = Host;
		edev->ti = NEXT(edev->ti, edev->ntb);
	}
	ctlr->tx_count += count;

	if(ctlr->debugflags & DumpTx)
		print("vgbe: transmit: done [count=%d]\n", count);

	iunlock(&ctlr->tlock);

	if(ctlr->tx_count)
		wiob(ctlr, TxCsrS, TxCsr_Wakeup);

	if(count == 0)
		print("vgbe: transmit: no Tx entry available\n");
}

static void
vgbeattach(Ether* edev)
{
	Ctlr* ctlr;
	Rd* rd;
	Td* td;
	int i;

	ctlr = edev->ctlr;

	// ctlr->debugflags |= DumpRx;
//	ctlr->debugflags |= DumpRx|DumpTx;

	// XXX: lock occurs in init
	lock(&ctlr->ilock);
	if(ctlr->inited){
		unlock(&ctlr->ilock);
		return;
	}

//	print("vgbe: attach\n");

	/* Allocate Rx ring.  (TODO: Alignment ?) */
	/*
		ctlr->rblen == RxCount
		ctlr->alloc = rd




	*/
	// he's making
	rd = mallocalign(RxCount* sizeof(Rd), 256, 0, 0);
	if(rd == nil){
		print("vgbe: unable to alloc Rx ring\n");
		unlock(&ctlr->ilock);
		return;
	}
	ctlr->rdring = rd;

	/* Allocate Rx blocks, initialize Rx ring. */
	for(i = 0; i < RxCount; i++)
		vgbenewrx(edev, i);

	/* Init Rx MAC. */
	wiob(ctlr, RxControl,
		RxControl_MultiCast|RxControl_BroadCast|RxControl_UniCast);
	wiob(ctlr, RxConfig, RxConfig_VlanOpt0);

	/* Load Rx ring. */
	wiol(ctlr, RdLo, (uint32_t) PCIWADDR(rd));
	wiow(ctlr, Rdcsize, RxCount - 1);
	wiow(ctlr, Rdindex, 0);
	wiow(ctlr, RxResCnt, RxCount);

	/* Allocate Tx ring. */
	td = mallocalign(TxCount* sizeof(Td), 256, 0, 0);
	if(td == nil){
		print("vgbe: unable to alloc Tx ring\n");
		unlock(&ctlr->ilock);
		return;
	}
	ctlr->td = td;

	/* Init DMAs */
	wiob(ctlr, DmaCfg0, 4);

	/* Init Tx MAC. */
	wiob(ctlr, TxControl, 0);
	wiob(ctlr, TxConfig, TxConfig_NonBlk|TxConfig_ArbPrio);

	/* Load Tx ring. */
	wiol(ctlr, TdLo, (uint32_t) PCIWADDR(td));
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
	unlock(&ctlr->ilock);

	/* Enable interrupts */
	wiol(ctlr, isr, 0xffffffff);
	wiob(ctlr, Cr3S, Cr3_IntMask);

	/* Wake up Rx queue */
	wiob(ctlr, RxCsrS, RxCsr_Wakeup);
}

static int
vgbereset(Ctlr* ctlr)
{
//	MiiPhy* phy;
	int timeo, i;

//	print("vgbe: reset: cr1s port: %x contents: %x\n", ctlr->port + Cr1S, riob(ctlr, Cr1S));

	/* Soft reset the controller. */
	wiob(ctlr, Cr1S, Cr1_reset);

	for(timeo = 0; timeo < Timeout; timeo++)
		if((riob(ctlr, Cr1S) & Cr1_reset) == 0)
			break;

	if(timeo >= Timeout){
		print("vgbe: softreset timeout\n");
		return -1;
	}

	/* Reload eeprom. */
	siob(ctlr, Eecsr, Eecsr_Autold);

	for(timeo = 0; timeo < Timeout; timeo++)
		if((riob(ctlr, Eecsr) & Eecsr_Autold) == 0)
			break;

	if(timeo >= Timeout){
		print("vgbe: eeprom reload timeout\n");
		return -1;
	}

	/* Load the MAC address. */
	for(i = 0; i < Eaddrlen; i++)
		ctlr->ea[i] = riob(ctlr, EthAddr+i);
//	print("vgbe: EA %E\n", ctlr->ea);

	/* Initialize interrupts. */
	wiol(ctlr, isr, 0xffffffff);
	wiol(ctlr, Imr, 0xffffffff);

	/* Disable interrupts. */
	wiol(ctlr, Cr3C, Cr3_IntMask);

	/* 32 bits addresses only. (TODO: 64 bits ?) */
	wiol(ctlr, TdHi, 0);
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
		return -1;
	}

	ctlr->mii->mir = vgbemiir;
	ctlr->mii->miw = vgbemiiw;
	ctlr->mii->ctlr = ctlr;

	if(mii(ctlr->mii, 1<<1) == 0){
		print("vgbe: no phy found\n");
		return -1;
	}

	return 0;
//	phy = ctlr->mii->curphy;
//	print("vgbe: phy:oui %#x\n", phy->oui);
}

void
vgbedetach(Ether* edev)
{
	vgbereset(edev->ctlr);
}



static void
vgbeinterrupt(Ureg *, void* arg)
{
	/* now we need to do something special to empty the ring buffer since we don't have the process */
	Ether* edev;
	Ctlr* ctlr;
	uint32_t status;
	uint32_t err;

//	print("vgbe: Intr: entering interrupt\n");
	edev = arg;
	ctlr = edev->ctlr;

	/* Mask interrupts. */
	wiol(ctlr, Imr, 0);

	status = riol(ctlr, isr);


	if(status == 0xffff)
		goto end;

	/* acknowledge */
	if(status)
		wiol(ctlr, isr, status);

	if((status & isr_Mask) == 0)
		goto end;

	ctlr->stats.intr++;

	if(ctlr->debugflags & DumpIntr)
		if(ctlr->debugcount){
			print("vgbe: irq: status = %#08ulx\n", status);
			vgbedumpisr(status);
			ctlr->debugcount--;
		}

	if(status & Prx)
		vgberxeof(edev);

	if(status & Ptx0)
		vgbetxeof(edev);

	if(status & Shdn){
		//print("vgbe: irq: software shutxbufown complete\n");
	}
	if(status & Ovf)
		print("vgbe: irq: RX FIFO overflow\n");

	if(status & Phyint)
		print("vgbe: irq: PHY interrupt\n");

	if(status & Src)
		print("vgbe: irq: link status change\n");

	if(status & Lste)
		print("vgbe: irq: ran out of Rx descriptors\n");

	if(status & Rxstl){
		//print("vgbe: irq: Rx DMA stall\n");
		err = riol(ctlr, Rxesr);
		//print("vgbe: Rxesr %ulx\n", err);
		if(err & Rfdbs)
			print("Rx fifo dma bus error\n");
		if(err & Rdwbs)
			print("Rd write back host bus error\n");
		if(err & Rdrbs)
			print("Rd fetch host bus error.\n");
		if(err & Rdstr)
			print("Valid RD with linked buffer size zero.\n");
		wiol(ctlr, Rxesr, 0);
		wiol(ctlr, Cr3C, Cr3_IntMask);
		return;
	}

	if(status & Txstl){
		print("vgbe: irq: Tx DMA stall\n");
		wiol(ctlr, Cr3C, Cr3_IntMask);
		return;
	}

end:
	/* Unmask interrupts. */
	wiol(ctlr, Imr, ~0);
}

static void
vgbepci(void)
{
	Pcidev *p;
	Ctlr *ctlr;
	int i, port;
	uint32_t bar;

	p = nil;
	while(p = pcimatch(p, 0, 0)){
		if(p->ccrb != 0x02 || p->ccru != 0)
			continue;

		switch((p->did<<16)|p->vid){
		default:
			continue;
		case (0x3119<<16)|0x1106:
			break;
		}

		bar = p->mem[0].bar;
		port = bar & ~0x01;
		if(ioalloc(port, p->mem[0].size, 0, "vgbe") < 0){
			print("vgbe: port %#ux in use\n", port);
			continue;
		}
		ctlr = malloc(sizeof(Ctlr));
		ctlr->port = port;
		ctlr->pcidev = p;

		if(pcigetpms(p) > 0){
			pcisetpms(p, 0);

			for(i = 0; i < 6; i++)
				pcicfgw32(p, PciBAR0+i*4, p->mem[i].bar);
			pcicfgw8(p, PciINTL, p->intl);
			pcicfgw8(p, PciLTR, p->ltr);
			pcicfgw8(p, PciCLS, p->cls);
			pcicfgw16(p, PciPCR, p->pcr);
		}

		if(vgbereset(ctlr)){
			iofree(port);
			free(ctlr);
			continue;
		}
		pcisetbme(p);

		if(vgbectlrhead != nil)
			vgbectlrtail->next = ctlr;
		else
			vgbectlrhead = ctlr;
		vgbectlrtail = ctlr;
	}
}

int
vgbepnp(Ether* edev)
{
	Ctlr *ctlr;

	if(vgbectlrhead == nil)
		vgbepci();

	/*
	 * Any adapter matches if no edev->port is supplied,
	 * otherwise the ports must match.
	 */
	for(ctlr = vgbectlrhead; ctlr != nil; ctlr = ctlr->next){
		if(ctlr->active)
			continue;
		if(edev->port == 0 || edev->port == ctlr->port){
			ctlr->active = 1;
			break;
		}
	}
	if(ctlr == nil)
		return -1;

	edev->ctlr = ctlr;
	edev->port = ctlr->port;
	edev->irq = ctlr->pcidev->intl;
	edev->tbdf = ctlr->pcidev->tbdf;

	memmove(edev->ea, ctlr->ea, Eaddrlen);

	/*
	 * Linkage to the generic ethernet driver.
	 */
	edev->attach = vgbeattach;
	edev->transmit = vgbetransmit;
	edev->interrupt = vgbeinterrupt;
	edev->detach = vgbedetach;

	return 0;
}
