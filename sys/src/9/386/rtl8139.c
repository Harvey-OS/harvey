/* rtl8139.c: A RealTek RTL8129/8139 Fast Ethernet driver for Linux. */
/*
	Written 1997-1998 by Donald Becker.

	This software may be used and distributed according to the terms
	of the GNU Public License, incorporated herein by reference.
    All other rights reserved.

	This driver is for boards based on the RTL8129 and RTL8139 PCI ethernet
	chips.

	The author may be reached as becker@CESDIS.gsfc.nasa.gov, or C/O
	Center of Excellence in Space Data and Information Sciences
	   Code 930.5, Goddard Space Flight Center, Greenbelt MD 20771

	Support and updates available at
	http://cesdis.gsfc.nasa.gov/linux/drivers/rtl8139.html

	Twister-tuning code contributed by Kinston <shangh@realtek.com.tw>.
*/

static const char *version =
"rtl8139.c:v1.04 9/22/98 Donald Becker http://cesdis.gsfc.nasa.gov/linux/drivers/rtl8139.html\n";

/* A few user-configurable values. */
/* Maximum events (Rx packets, etc.) to handle at each interrupt. */
static int max_interrupt_work = 20;
#define rtl8129_debug debug
static int rtl8129_debug = 1;

/* Maximum number of multicast addresses to filter (vs. Rx-all-multicast).
   The RTL chips use a 64 element hash table based on the Ethernet CRC.  */
static int multicast_filter_limit = 32;

/* Used to pass the full-duplex flag, etc. */
#define MAX_UNITS 8		/* More are supported, limit only on options */
static int options[MAX_UNITS] = {-1, -1, -1, -1, -1, -1, -1, -1};
static int full_duplex[MAX_UNITS] = {-1, -1, -1, -1, -1, -1, -1, -1};

/* Size of the in-memory receive ring. */
#define RX_BUF_LEN_IDX	3			/* 0==8K, 1==16K, 2==32K, 3==64K */
#define RX_BUF_LEN (8192 << RX_BUF_LEN_IDX)
/* Size of the Tx bounce buffers -- must be at least (dev->mtu+14+4). */
#define TX_BUF_SIZE	1536

/* PCI Tuning Parameters
   Threshold is bytes transferred to chip before transmission starts. */
#define TX_FIFO_THRESH 256	/* In bytes, rounded down to 32 byte units. */

/* The following settings are log_2(bytes)-4:  0 == 16 bytes .. 6==1024. */
#define RX_FIFO_THRESH	4		/* Rx buffer level before first PCI xfer.  */
#define RX_DMA_BURST	4		/* Maximum PCI burst, '4' is 256 bytes */
#define TX_DMA_BURST	4		/* Calculate as 16<<val. */

/* Operational parameters that usually are not changed. */
/* Time in jiffies before concluding the transmitter is hung. */
#define TX_TIMEOUT  (4*HZ)

#include <linux/config.h>
#ifdef MODULE
#ifdef MODVERSIONS
#include <linux/modversions.h>
#endif
#include <linux/module.h>
#include <linux/version.h>
#else
#define MOD_INC_USE_COUNT
#define MOD_DEC_USE_COUNT
#endif

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/errno.h>
#include <linux/ioport.h>
#include <linux/malloc.h>
#include <linux/interrupt.h>
#include <linux/pci.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <asm/processor.h>		/* Processor type for cache alignment. */
#include <asm/bitops.h>
#include <asm/io.h>

/* Kernel compatibility defines, some common to David Hind's PCMCIA package.
   This is only in the support-all-kernels source code. */

#define RUN_AT(x) (jiffies + (x))

#include <linux/delay.h>

#if LINUX_VERSION_CODE < 0x20123
#define test_and_set_bit(val, addr) set_bit(val, addr)
#endif
#if LINUX_VERSION_CODE <= 0x20139
#define	net_device_stats enet_statistics
#else
#define NETSTATS_VER2
#endif
#if LINUX_VERSION_CODE < 0x20155  ||  defined(CARDBUS)
/* Grrrr, the PCI code changed, but did not consider CardBus... */
#include <linux/bios32.h>
#define PCI_SUPPORT_VER1
#else
#define PCI_SUPPORT_VER2
#endif
#if LINUX_VERSION_CODE < 0x20159
#define dev_free_skb(skb) dev_kfree_skb(skb, FREE_WRITE);
#else
#define dev_free_skb(skb) dev_kfree_skb(skb);
#endif

/* The I/O extent. */
#define RTL8129_TOTAL_SIZE 0x80

/*
				Theory of Operation

I. Board Compatibility

This device driver is designed for the RealTek RTL8129, the RealTek Fast
Ethernet controllers for PCI.  This chip is used on a few clone boards.


II. Board-specific settings

PCI bus devices are configured by the system at boot time, so no jumpers
need to be set on the board.  The system BIOS will assign the
PCI INTA signal to a (preferably otherwise unused) system IRQ line.
Note: Kernel versions earlier than 1.3.73 do not support shared PCI
interrupt lines.

III. Driver operation

IIIa. Rx Ring buffers

The receive unit uses a single linear ring buffer rather than the more
common (and more efficient) descriptor-based architecture.  Incoming frames
are sequentially stored into the Rx region, and the host copies them into
skbuffs.

Comment: While it is theoretically possible to process many frames in place,
any delay in Rx processing would cause us to drop frames.  More importantly,
the Linux protocol stack is not designed to operate in this manner.

IIIb. Tx operation

The RTL8129 uses a fixed set of four Tx descriptors in register space.
In a stunningly bad design choice, Tx frames must be 32 bit aligned.  Linux
aligns the IP header on word boundaries, and 14 byte ethernet header means
that almost all frames will need to be copied to an alignment buffer.

IVb. References

http://www.realtek.com.tw/cn/cn.html
http://cesdis.gsfc.nasa.gov/linux/misc/NWay.html

IVc. Errata

*/


/* This table drives the PCI probe routines.  It's mostly boilerplate in all
   of the drivers, and will likely be provided by some future kernel.
   Note the matching code -- the first table entry matchs all 56** cards but
   second only the 1234 card.
*/
enum pci_flags_bit {
	PCI_USES_IO=1, PCI_USES_MEM=2, PCI_USES_MASTER=4,
	PCI_ADDR0=0x10<<0, PCI_ADDR1=0x10<<1, PCI_ADDR2=0x10<<2, PCI_ADDR3=0x10<<3,
};
struct pci_id_info {
	const char *name;
	u16	vendor_id, device_id, device_id_mask, flags;
	int io_size;
	struct device *(*probe1)(int pci_bus, int pci_devfn, struct device *dev,
							 long ioaddr, int irq, int chip_idx, int fnd_cnt);
};

static struct device * rtl8129_probe1(int pci_bus, int pci_devfn,
									  struct device *dev, long ioaddr,
									  int irq, int chp_idx, int fnd_cnt);

static struct pci_id_info pci_tbl[] =
{{ "RealTek RTL8129 Fast Ethernet",
   0x10ec, 0x8129, 0xffff, PCI_USES_IO|PCI_USES_MASTER, 0x80, rtl8129_probe1},
 { "RealTek RTL8139 Fast Ethernet",
   0x10ec, 0x8139, 0xffff, PCI_USES_IO|PCI_USES_MASTER, 0x80, rtl8129_probe1},
 { "RealTek RTL8139 Fast Ethernet (mislabeled)",
   0x1113, 0x1211, 0xffff, PCI_USES_IO|PCI_USES_MASTER, 0x80, rtl8129_probe1},
 {0,},						/* 0 terminated list. */
};

/* The capability table matches the chip table above. */
enum {HAS_MII_XCVR=0x01, HAS_CHIP_XCVR=0x02, HAS_LNK_CHNG=0x04};
static int rtl_cap_tbl[] = {
	HAS_MII_XCVR, HAS_CHIP_XCVR|HAS_LNK_CHNG, HAS_CHIP_XCVR|HAS_LNK_CHNG,
};


/* The rest of these values should never change. */
#define NUM_TX_DESC	4			/* Number of Tx descriptor registers. */

/* Symbolic offsets to registers. */
enum RTL8129_registers {
	MAC0=0,						/* Ethernet hardware address. */
	MAR0=8,						/* Multicast filter. */
	TxStatus0=0x10,				/* Transmit status (Four 32bit registers). */
	TxAddr0=0x20,				/* Tx descriptors (also four 32bit). */
	RxBuf=0x30, RxEarlyCnt=0x34, RxEarlyStatus=0x36,
	ChipCmd=0x37, RxBufPtr=0x38, RxBufAddr=0x3A,
	IntrMask=0x3C, IntrStatus=0x3E,
	TxConfig=0x40, RxConfig=0x44,
	Timer=0x48,					/* A general-purpose counter. */
	RxMissed=0x4C,				/* 24 bits valid, write clears. */
	Cfg9346=0x50, Config0=0x51, Config1=0x52,
	FlashReg=0x54, GPPinData=0x58, GPPinDir=0x59, MII_SMI=0x5A, HltClk=0x5B,
	MultiIntr=0x5C, TxSummary=0x60,
	MII_BMCR=0x62, MII_BMSR=0x64, NWayAdvert=0x66, NWayLPAR=0x68,
	NWayExpansion=0x6A,
	/* Undocumented registers, but required for proper operation. */
	FIFOTMS=0x70,	/* FIFO Test Mode Select */
	CSCR=0x74,	/* Chip Status and Configuration Register. */
	PARA78=0x78, PARA7c=0x7c,	/* Magic transceiver parameter register. */
};

enum ChipCmdBits {
	CmdReset=0x10, CmdRxEnb=0x08, CmdTxEnb=0x04, RxBufEmpty=0x01, };

/* Interrupt register bits, using my own meaningful names. */
enum IntrStatusBits {
	PCIErr=0x8000, PCSTimeout=0x4000,
	RxFIFOOver=0x40, RxUnderrun=0x20, RxOverflow=0x10,
	TxErr=0x08, TxOK=0x04, RxErr=0x02, RxOK=0x01,
};
enum TxStatusBits {
	TxHostOwns=0x2000, TxUnderrun=0x4000, TxStatOK=0x8000,
	TxOutOfWindow=0x20000000, TxAborted=0x40000000, TxCarrierLost=0x80000000,
};
enum RxStatusBits {
	RxMulticast=0x8000, RxPhysical=0x4000, RxBroadcast=0x2000,
	RxBadSymbol=0x0020, RxRunt=0x0010, RxTooLong=0x0008, RxCRCErr=0x0004,
	RxBadAlign=0x0002, RxStatusOK=0x0001,
};

enum CSCRBits {
	CSCR_LinkOKBit=0x0400, CSCR_LinkChangeBit=0x0800,
	CSCR_LinkStatusBits=0x0f000, CSCR_LinkDownOffCmd=0x003c0,
	CSCR_LinkDownCmd=0x0f3c0,
};	

/* Twister tuning parameters from RealTek.  Completely undocumented. */
unsigned long param[4][4]={
	{0x0cb39de43,0x0cb39ce43,0x0fb38de03,0x0cb38de43},
	{0x0cb39de43,0x0cb39ce43,0x0cb39ce83,0x0cb39ce83},
	{0x0cb39de43,0x0cb39ce43,0x0cb39ce83,0x0cb39ce83},
	{0x0bb39de43,0x0bb39ce43,0x0bb39ce83,0x0bb39ce83}
};

struct rtl8129_private {
	char devname[8];			/* Used only for kernel debugging. */
	const char *product_name;
	struct device *next_module;
	int chip_id;
	int chip_revision;
	unsigned char pci_bus, pci_devfn;
#if LINUX_VERSION_CODE > 0x20139
	struct net_device_stats stats;
#else
	struct enet_statistics stats;
#endif
	struct timer_list timer;	/* Media selection timer. */
	unsigned int cur_rx;		/* Index into the Rx buffer of next Rx pkt. */
	unsigned int cur_tx, dirty_tx, tx_flag;
	/* The saved address of a sent-in-place packet/buffer, for skfree(). */
	struct sk_buff* tx_skbuff[NUM_TX_DESC];
	unsigned char *tx_buf[NUM_TX_DESC];	/* Tx bounce buffers */
	unsigned char *rx_ring;
	unsigned char *tx_bufs;				/* Tx bounce buffer region. */
	char phys[4];						/* MII device addresses. */
	unsigned int tx_full:1;				/* The Tx queue is full. */
	unsigned int full_duplex:1;			/* Full-duplex operation requested. */
	unsigned int duplex_lock:1;			/* Full-duplex operation requested. */
	unsigned int default_port:4;		/* Last dev->if_port value. */
	unsigned int media2:4;				/* Secondary monitored media port. */
	unsigned int medialock:1;			/* Don't sense media type. */
	unsigned int mediasense:1;			/* Media sensing in progress. */
};

#ifdef MODULE
#if LINUX_VERSION_CODE > 0x20115
MODULE_AUTHOR("Donald Becker <becker@cesdis.gsfc.nasa.gov>");
MODULE_DESCRIPTION("RealTek RTL8129/8139 Fast Ethernet driver");
MODULE_PARM(options, "1-" __MODULE_STRING(MAX_UNITS) "i");
MODULE_PARM(full_duplex, "1-" __MODULE_STRING(MAX_UNITS) "i");
MODULE_PARM(multicast_filter_limit, "i");
MODULE_PARM(max_interrupt_work, "i");
MODULE_PARM(debug, "i");
#endif
#endif

static int rtl8129_open(struct device *dev);
static int read_eeprom(long ioaddr, int location);
static int mdio_read(struct device *dev, int phy_id, int location);
static void mdio_write(struct device *dev, int phy_id, int location, int val);
static void rtl8129_timer(unsigned long data);
static void rtl8129_tx_timeout(struct device *dev);
static void rtl8129_init_ring(struct device *dev);
static int rtl8129_start_xmit(struct sk_buff *skb, struct device *dev);
static int rtl8129_rx(struct device *dev);
static void rtl8129_interrupt(int irq, void *dev_instance, struct pt_regs *regs);
static int rtl8129_close(struct device *dev);
static int mii_ioctl(struct device *dev, struct ifreq *rq, int cmd);
static struct enet_statistics *rtl8129_get_stats(struct device *dev);
static inline u32 ether_crc(int length, unsigned char *data);
static void set_rx_mode(struct device *dev);


/* A list of all installed RTL8129 devices, for removing the driver module. */
static struct device *root_rtl8129_dev = NULL;

/* Ideally we would detect all network cards in slot order.  That would
   be best done a central PCI probe dispatch, which wouldn't work
   well when dynamically adding drivers.  So instead we detect just the
   Rtl81*9 cards in slot order. */

int rtl8139_probe(struct device *dev)
{
	int cards_found = 0;
	int pci_index = 0;
	unsigned char pci_bus, pci_device_fn;

	if ( ! pcibios_present())
		return -ENODEV;

	for (;pci_index < 0xff; pci_index++) {
		u16 vendor, device, pci_command, new_command;
		int chip_idx, irq;
		long ioaddr;

		if (pcibios_find_class (PCI_CLASS_NETWORK_ETHERNET << 8, pci_index,
								&pci_bus, &pci_device_fn)
			!= PCIBIOS_SUCCESSFUL)
			break;
		pcibios_read_config_word(pci_bus, pci_device_fn,
								 PCI_VENDOR_ID, &vendor);
		pcibios_read_config_word(pci_bus, pci_device_fn,
								 PCI_DEVICE_ID, &device);

		for (chip_idx = 0; pci_tbl[chip_idx].vendor_id; chip_idx++)
			if (vendor == pci_tbl[chip_idx].vendor_id
				&& (device & pci_tbl[chip_idx].device_id_mask) ==
				pci_tbl[chip_idx].device_id)
				break;
		if (pci_tbl[chip_idx].vendor_id == 0) 		/* Compiled out! */
			continue;

		{
#if defined(PCI_SUPPORT_VER2)
			struct pci_dev *pdev = pci_find_slot(pci_bus, pci_device_fn);
			ioaddr = pdev->base_address[0] & ~3;
			irq = pdev->irq;
#else
			u32 pci_ioaddr;
			u8 pci_irq_line;
			pcibios_read_config_byte(pci_bus, pci_device_fn,
									 PCI_INTERRUPT_LINE, &pci_irq_line);
			pcibios_read_config_dword(pci_bus, pci_device_fn,
									  PCI_BASE_ADDRESS_0, &pci_ioaddr);
			ioaddr = pci_ioaddr & ~3;
			irq = pci_irq_line;
#endif
		}

		if ((pci_tbl[chip_idx].flags & PCI_USES_IO) &&
			check_region(ioaddr, pci_tbl[chip_idx].io_size))
			continue;

		/* Activate the card: fix for brain-damaged Win98 BIOSes. */
		pcibios_read_config_word(pci_bus, pci_device_fn,
								 PCI_COMMAND, &pci_command);
		new_command = pci_command | (pci_tbl[chip_idx].flags & 7);
		if (pci_command != new_command) {
			printk(KERN_INFO "  The PCI BIOS has not enabled the"
				   " device at %d/%d!  Updating PCI command %4.4x->%4.4x.\n",
				   pci_bus, pci_device_fn, pci_command, new_command);
			pcibios_write_config_word(pci_bus, pci_device_fn,
									  PCI_COMMAND, new_command);
		}

		dev = pci_tbl[chip_idx].probe1(pci_bus, pci_device_fn, dev, ioaddr,
									   irq, chip_idx, cards_found);

		if (dev  && (pci_tbl[chip_idx].flags & PCI_COMMAND_MASTER)) {
			u8 pci_latency;
			pcibios_read_config_byte(pci_bus, pci_device_fn,
									 PCI_LATENCY_TIMER, &pci_latency);
			if (pci_latency < 32) {
				printk(KERN_NOTICE "  PCI latency timer (CFLT) is "
					   "unreasonably low at %d.  Setting to 64 clocks.\n",
					   pci_latency);
				pcibios_write_config_byte(pci_bus, pci_device_fn,
										  PCI_LATENCY_TIMER, 64);
			}
		}
		dev = 0;
		cards_found++;
	}

	return cards_found ? 0 : -ENODEV;
}

static struct device * rtl8129_probe1(int pci_bus, int pci_devfn,
									  struct device *dev, long ioaddr,
									  int irq, int chip_idx, int found_cnt)
{
	static int did_version = 0;			/* Already printed version info. */
	struct rtl8129_private *tp;
	int i, option = found_cnt < MAX_UNITS ? options[found_cnt] : 0;

	if (rtl8129_debug > 0  &&  did_version++ == 0)
		printk(KERN_INFO "%s", version);

	dev = init_etherdev(dev, 0);

	printk(KERN_INFO "%s: %s at %#lx, IRQ %d, ",
		   dev->name, pci_tbl[chip_idx].name, ioaddr, irq);

	/* Bring the chip out of low-power mode. */
	outb(0x00, ioaddr + Config1);

	if (read_eeprom(ioaddr, 0) != 0xffff)
		for (i = 0; i < 3; i++)
			((u16 *)(dev->dev_addr))[i] = read_eeprom(ioaddr, i + 7);
	else
		for (i = 0; i < 6; i++)
			dev->dev_addr[i] = inb(ioaddr + MAC0 + i);

	for (i = 0; i < 5; i++)
		printk("%2.2x:", dev->dev_addr[i]);
	printk("%2.2x.\n", dev->dev_addr[i]);

	/* We do a request_region() to register /proc/ioports info. */
	request_region(ioaddr, pci_tbl[chip_idx].io_size, dev->name);

	dev->base_addr = ioaddr;
	dev->irq = irq;

	/* Some data structures must be quadword aligned. */
	tp = kmalloc(sizeof(*tp), GFP_KERNEL | GFP_DMA);
	memset(tp, 0, sizeof(*tp));
	dev->priv = tp;

	tp->next_module = root_rtl8129_dev;
	root_rtl8129_dev = dev;

	tp->chip_id = chip_idx;
	tp->pci_bus = pci_bus;
	tp->pci_devfn = pci_devfn;

	/* Find the connected MII xcvrs.
	   Doing this in open() would allow detecting external xcvrs later, but
	   takes too much time. */
	if (rtl_cap_tbl[chip_idx] & HAS_MII_XCVR) {
		int phy, phy_idx;
		for (phy = 0, phy_idx = 0; phy < 32 && phy_idx < sizeof(tp->phys);
			 phy++) {
			int mii_status = mdio_read(dev, phy, 1);
			if (mii_status != 0xffff  && mii_status != 0x0000) {
				tp->phys[phy_idx++] = phy;
				printk(KERN_INFO "%s: MII transceiver found at address %d.\n",
					   dev->name, phy);
			}
		}
		if (phy_idx == 0) {
			printk(KERN_INFO "%s: No MII transceivers found!  Assuming SYM "
				   "transceiver.\n",
				   dev->name);
			tp->phys[0] = -1;
		}
	} else {
			tp->phys[0] = 32;
	}

	/* Put the chip into low-power mode. */
	outb(0xC0, ioaddr + Cfg9346);
	outb(0x03, ioaddr + Config1);
	outb('H', ioaddr + HltClk);		/* 'R' would leave the clock running. */

	/* The lower four bits are the media type. */
	if (option > 0) {
		tp->full_duplex = (option & 0x200) ? 1 : 0;
		tp->default_port = option & 15;
		if (tp->default_port)
			tp->medialock = 1;
	}

	if (found_cnt < MAX_UNITS  &&  full_duplex[found_cnt] > 0)
		tp->full_duplex = full_duplex[found_cnt];

	if (tp->full_duplex) {
		printk(KERN_INFO "%s: Media type forced to Full Duplex.\n", dev->name);
		mdio_write(dev, tp->phys[0], 4, 0x141);
		tp->duplex_lock = 1;
	}

	/* The Rtl8129-specific entries in the device structure. */
	dev->open = &rtl8129_open;
	dev->hard_start_xmit = &rtl8129_start_xmit;
	dev->stop = &rtl8129_close;
	dev->get_stats = &rtl8129_get_stats;
	dev->set_multicast_list = &set_rx_mode;
	dev->do_ioctl = &mii_ioctl;

	return dev;
}

/* Serial EEPROM section. */

/*  EEPROM_Ctrl bits. */
#define EE_SHIFT_CLK	0x04	/* EEPROM shift clock. */
#define EE_CS			0x08	/* EEPROM chip select. */
#define EE_DATA_WRITE	0x02	/* EEPROM chip data in. */
#define EE_WRITE_0		0x00
#define EE_WRITE_1		0x02
#define EE_DATA_READ	0x01	/* EEPROM chip data out. */
#define EE_ENB			(0x80 | EE_CS)

/* Delay between EEPROM clock transitions.
   No extra delay is needed with 33Mhz PCI, but 66Mhz may change this.
 */

#define eeprom_delay()	inl(ee_addr)

/* The EEPROM commands include the alway-set leading bit. */
#define EE_WRITE_CMD	(5 << 6)
#define EE_READ_CMD		(6 << 6)
#define EE_ERASE_CMD	(7 << 6)

static int read_eeprom(long ioaddr, int location)
{
	int i;
	unsigned retval = 0;
	long ee_addr = ioaddr + Cfg9346;
	int read_cmd = location | EE_READ_CMD;

	outb(EE_ENB & ~EE_CS, ee_addr);
	outb(EE_ENB, ee_addr);

	/* Shift the read command bits out. */
	for (i = 10; i >= 0; i--) {
		int dataval = (read_cmd & (1 << i)) ? EE_DATA_WRITE : 0;
		outb(EE_ENB | dataval, ee_addr);
		eeprom_delay();
		outb(EE_ENB | dataval | EE_SHIFT_CLK, ee_addr);
		eeprom_delay();
	}
	outb(EE_ENB, ee_addr);
	eeprom_delay();

	for (i = 16; i > 0; i--) {
		outb(EE_ENB | EE_SHIFT_CLK, ee_addr);
		eeprom_delay();
		retval = (retval << 1) | ((inb(ee_addr) & EE_DATA_READ) ? 1 : 0);
		outb(EE_ENB, ee_addr);
		eeprom_delay();
	}

	/* Terminate the EEPROM access. */
	outb(~EE_CS, ee_addr);
	return retval;
}

/* MII serial management: mostly bogus for now. */
/* Read and write the MII management registers using software-generated
   serial MDIO protocol.
   The maximum data clock rate is 2.5 Mhz.  The minimum timing is usually
   met by back-to-back PCI I/O cycles, but we insert a delay to avoid
   "overclocking" issues. */
#define MDIO_DIR		0x80
#define MDIO_DATA_OUT	0x04
#define MDIO_DATA_IN	0x02
#define MDIO_CLK		0x01
#define MDIO_WRITE0 (MDIO_DIR)
#define MDIO_WRITE1 (MDIO_DIR | MDIO_DATA_OUT)

#define mdio_delay()	inb(mdio_addr)

static char mii_2_8139_map[8] = {MII_BMCR, MII_BMSR, 0, 0, NWayAdvert,
								 NWayLPAR, NWayExpansion, 0 };

/* Syncronize the MII management interface by shifting 32 one bits out. */
static void mdio_sync(long mdio_addr)
{
	int i;

	for (i = 32; i >= 0; i--) {
		outb(MDIO_WRITE1, mdio_addr);
		mdio_delay();
		outb(MDIO_WRITE1 | MDIO_CLK, mdio_addr);
		mdio_delay();
	}
	return;
}
static int mdio_read(struct device *dev, int phy_id, int location)
{
	long mdio_addr = dev->base_addr + MII_SMI;
	int mii_cmd = (0xf6 << 10) | (phy_id << 5) | location;
	int retval = 0;
	int i;

	if ((phy_id & 0x1f) == 0) {	/* Really a 8139.  Use internal registers. */
		return location < 8 && mii_2_8139_map[location] ?
			inw(dev->base_addr + mii_2_8139_map[location]) : 0;
	}
	mdio_sync(mdio_addr);
	/* Shift the read command bits out. */
	for (i = 15; i >= 0; i--) {
		int dataval = (mii_cmd & (1 << i)) ? MDIO_DATA_OUT : 0;

		outb(MDIO_DIR | dataval, mdio_addr);
		mdio_delay();
		outb(MDIO_DIR | dataval | MDIO_CLK, mdio_addr);
		mdio_delay();
	}

	/* Read the two transition, 16 data, and wire-idle bits. */
	for (i = 19; i > 0; i--) {
		outb(0, mdio_addr);
		mdio_delay();
		retval = (retval << 1) | ((inb(mdio_addr) & MDIO_DATA_IN) ? 1 : 0);
		outb(MDIO_CLK, mdio_addr);
		mdio_delay();
	}
	return (retval>>1) & 0xffff;
}

static void mdio_write(struct device *dev, int phy_id, int location, int value)
{
	long mdio_addr = dev->base_addr + MII_SMI;
	int mii_cmd = (0x5002 << 16) | (phy_id << 23) | (location<<18) | value;
	int i;

	if (phy_id == 32) {			/* Really a 8139.  Use internal registers. */
		if (location < 8  &&  mii_2_8139_map[location])
			outw(value, dev->base_addr + mii_2_8139_map[location]);
		return;
	}
	mdio_sync(mdio_addr);

	/* Shift the command bits out. */
	for (i = 31; i >= 0; i--) {
		int dataval = (mii_cmd & (1 << i)) ? MDIO_WRITE1 : MDIO_WRITE0;
		outb(dataval, mdio_addr);
		mdio_delay();
		outb(dataval | MDIO_CLK, mdio_addr);
		mdio_delay();
	}
	/* Clear out extra bits. */
	for (i = 2; i > 0; i--) {
		outb(0, mdio_addr);
		mdio_delay();
		outb(MDIO_CLK, mdio_addr);
		mdio_delay();
	}
	return;
}


static int
rtl8129_open(struct device *dev)
{
	struct rtl8129_private *tp = (struct rtl8129_private *)dev->priv;
	long ioaddr = dev->base_addr;
	int i;

	/* Soft reset the chip. */
	outb(CmdReset, ioaddr + ChipCmd);

	if (request_irq(dev->irq, &rtl8129_interrupt, SA_SHIRQ, dev->name, dev)) {
		return -EAGAIN;
	}

	MOD_INC_USE_COUNT;

	tp->tx_bufs = kmalloc(TX_BUF_SIZE * NUM_TX_DESC, GFP_KERNEL);
	tp->rx_ring = kmalloc(RX_BUF_LEN + 16, GFP_KERNEL);
	if (tp->tx_bufs == NULL ||  tp->rx_ring == NULL) {
		if (tp->tx_bufs)
			kfree(tp->tx_bufs);
		if (rtl8129_debug > 0)
			printk(KERN_ERR "%s: Couldn't allocate a %d byte receive ring.\n",
				   dev->name, RX_BUF_LEN);
		return -ENOMEM;
	}
	rtl8129_init_ring(dev);

	/* Check that the chip has finished the reset. */
	for (i = 1000; i > 0; i--)
		if ((inb(ioaddr + ChipCmd) & CmdReset) == 0)
			break;

	for (i = 0; i < 6; i++)
		outb(dev->dev_addr[i], ioaddr + MAC0 + i);

	/* Must enable Tx/Rx before setting transfer thresholds! */
	outb(CmdRxEnb | CmdTxEnb, ioaddr + ChipCmd);
	outl((RX_FIFO_THRESH << 13) | (RX_BUF_LEN_IDX << 11) | (RX_DMA_BURST<<8),
		 ioaddr + RxConfig);
	outl((TX_DMA_BURST<<8)|0x03000000, ioaddr + TxConfig);
	tp->tx_flag = (TX_FIFO_THRESH<<11) & 0x003f0000;

	tp->full_duplex = tp->duplex_lock;
	if (tp->phys[0] >= 0  ||  (rtl_cap_tbl[tp->chip_id] & HAS_MII_XCVR)) {
		u16 mii_reg5 = mdio_read(dev, tp->phys[0], 5);
		if (mii_reg5 == 0xffff)
			;					/* Not there */
		else if ((mii_reg5 & 0x0100) == 0x0100
				 || (mii_reg5 & 0x00C0) == 0x0040)
			tp->full_duplex = 1;
		if (rtl8129_debug > 1)
			printk(KERN_INFO"%s: Setting %s%s-duplex based on"
				   " auto-negotiated partner ability %4.4x.\n", dev->name,
				   mii_reg5 == 0 ? "" :
				   (mii_reg5 & 0x0180) ? "100mbps " : "10mbps ",
				   tp->full_duplex ? "full" : "half", mii_reg5);
	}

	outb(0xC0, ioaddr + Cfg9346);
	outb(tp->full_duplex ? 0x60 : 0x20, ioaddr + Config1);
	outb(0x00, ioaddr + Cfg9346);

	outl(virt_to_bus(tp->rx_ring), ioaddr + RxBuf);

	/* Start the chip's Tx and Rx process. */
	outl(0, ioaddr + RxMissed);
	set_rx_mode(dev);

	outb(CmdRxEnb | CmdTxEnb, ioaddr + ChipCmd);

	dev->tbusy = 0;
	dev->interrupt = 0;
	dev->start = 1;

	/* Enable all known interrupts by setting the interrupt mask. */
	outw(PCIErr | PCSTimeout | RxUnderrun | RxOverflow | RxFIFOOver
		| TxErr | TxOK | RxErr | RxOK, ioaddr + IntrMask);

	if (rtl8129_debug > 1)
		printk(KERN_DEBUG"%s: rtl8129_open() ioaddr %#lx IRQ %d"
			   " GP Pins %2.2x %s-duplex.\n",
			   dev->name, ioaddr, dev->irq, inb(ioaddr + GPPinData),
			   tp->full_duplex ? "full" : "half");

	/* Set the timer to switch to check for link beat and perhaps switch
	   to an alternate media type. */
	init_timer(&tp->timer);
	tp->timer.expires = RUN_AT((24*HZ)/10);			/* 2.4 sec. */
	tp->timer.data = (unsigned long)dev;
	tp->timer.function = &rtl8129_timer;				/* timer handler */
	add_timer(&tp->timer);

	return 0;
}

static void rtl8129_timer(unsigned long data)
{
	struct device *dev = (struct device *)data;
	struct rtl8129_private *tp = (struct rtl8129_private *)dev->priv;
	long ioaddr = dev->base_addr;
	int next_tick = 0;
	int mii_reg5 = mdio_read(dev, tp->phys[0], 5);

	if (! tp->duplex_lock  &&  mii_reg5 != 0xffff) {
		int duplex = (mii_reg5&0x0100) || (mii_reg5 & 0x01C0) == 0x0040;
		if (tp->full_duplex != duplex) {
			tp->full_duplex = duplex;
			printk(KERN_INFO "%s: Setting %s-duplex based on MII #%d link"
				   " partner ability of %4.4x.\n", dev->name,
				   tp->full_duplex ? "full" : "half", tp->phys[0], mii_reg5);
			outb(0xC0, ioaddr + Cfg9346);
			outb(tp->full_duplex ? 0x60 : 0x20, ioaddr + Config1);
			outb(0x00, ioaddr + Cfg9346);
		}
		next_tick = 60*HZ;
	}

	if (rtl8129_debug > 2) {
		if (rtl_cap_tbl[tp->chip_id] & HAS_MII_XCVR)
			printk(KERN_DEBUG"%s: Media selection tick, GP pins %2.2x.\n",
				   dev->name, inb(ioaddr + GPPinData));
		else
			printk(KERN_DEBUG"%s: Media selection tick, Link partner %4.4x.\n",
				   dev->name, inw(ioaddr + NWayLPAR));
		printk(KERN_DEBUG"%s:  Other registers are IntMask %4.4x IntStatus %4.4x"
			   " RxStatus %4.4x.\n",
			   dev->name, inw(ioaddr + IntrMask), inw(ioaddr + IntrStatus),
			   inl(ioaddr + RxEarlyStatus));
		printk(KERN_DEBUG"%s:  Chip config %2.2x %2.2x.\n",
			   dev->name, inb(ioaddr + Config0), inb(ioaddr + Config1));
	}

	if (next_tick) {
		tp->timer.expires = RUN_AT(next_tick);
		add_timer(&tp->timer);
	}
}

static void rtl8129_tx_timeout(struct device *dev)
{
	struct rtl8129_private *tp = (struct rtl8129_private *)dev->priv;
	long ioaddr = dev->base_addr;
	int mii_reg, i;

	if (rtl8129_debug > 0)
		printk(KERN_WARNING "%s: Transmit timeout, status %2.2x %4.4x "
			   "media %2.2x.\n",
			   dev->name, inb(ioaddr + ChipCmd), inw(ioaddr + IntrStatus),
			   inb(ioaddr + GPPinData));

	/* Disable interrupts by clearing the interrupt mask. */
	outw(0x0000, ioaddr + IntrMask);
	/* Emit info to figure out what went wrong. */
	printk("%s: Tx queue start entry %d  dirty entry %d.\n",
		   dev->name, tp->cur_tx, tp->dirty_tx);
	for (i = 0; i < NUM_TX_DESC; i++)
		printk(KERN_DEBUG"%s:  Tx descriptor %d is %8.8x.%s\n",
			   dev->name, i, inl(ioaddr + TxStatus0 + i*4),
			   i == tp->dirty_tx % NUM_TX_DESC ? " (queue head)" : "");
	printk(KERN_DEBUG"%s: MII #%d registers are:", dev->name, tp->phys[0]);
	for (mii_reg = 0; mii_reg < 8; mii_reg++)
		printk(" %4.4x", mdio_read(dev, tp->phys[0], mii_reg));
	printk(".\n");

	/* Soft reset the chip. */
	outb(CmdReset, ioaddr + ChipCmd);
	/* Check that the chip has finished the reset. */
	for (i = 1000; i > 0; i--)
		if ((inb(ioaddr + ChipCmd) & CmdReset) == 0)
			break;
	for (i = 0; i < 6; i++)
		outb(dev->dev_addr[i], ioaddr + MAC0 + i);

	outb(0x00, ioaddr + Cfg9346);
	tp->cur_rx = 0;
	/* Must enable Tx/Rx before setting transfer thresholds! */
	outb(CmdRxEnb | CmdTxEnb, ioaddr + ChipCmd);
	outl((RX_FIFO_THRESH << 13) | (RX_BUF_LEN_IDX << 11) | (RX_DMA_BURST<<8),
		 ioaddr + RxConfig);
	outl((TX_DMA_BURST<<8), ioaddr + TxConfig);
	set_rx_mode(dev);
	{							/* Save the unsent Tx packets. */
		struct sk_buff *saved_skb[NUM_TX_DESC], *skb;
		int j;
		for (j = 0; tp->cur_tx - tp->dirty_tx > 0 ; j++, tp->dirty_tx++)
			saved_skb[j] = tp->tx_skbuff[tp->dirty_tx % NUM_TX_DESC];
		tp->dirty_tx = tp->cur_tx = 0;

		for (i = 0; i < j; i++) {
			skb = tp->tx_skbuff[i] = saved_skb[i];
			if ((long)skb->data & 3) {		/* Must use alignment buffer. */
				memcpy(tp->tx_buf[i], skb->data, skb->len);
				outl(virt_to_bus(tp->tx_buf[i]), ioaddr + TxAddr0 + i*4);
			} else
				outl(virt_to_bus(skb->data), ioaddr + TxAddr0 + i*4);
			/* Note: the chip doesn't have auto-pad! */
			outl(tp->tx_flag | (skb->len >= ETH_ZLEN ? skb->len : ETH_ZLEN),
				 ioaddr + TxStatus0 + i*4);
		}
		tp->cur_tx = i;
		while (i < NUM_TX_DESC)
			tp->tx_skbuff[i] = 0;
		if (tp->cur_tx - tp->dirty_tx < NUM_TX_DESC) {/* Typical path */
			dev->tbusy = 0;
			tp->tx_full = 0;
		} else {
			tp->tx_full = 1;
		}
	}

	dev->trans_start = jiffies;
	tp->stats.tx_errors++;
	/* Enable all known interrupts by setting the interrupt mask. */
	outw(PCIErr | PCSTimeout | RxUnderrun | RxOverflow | RxFIFOOver
		 | TxErr | TxOK | RxErr | RxOK, ioaddr + IntrMask);
	return;
}


/* Initialize the Rx and Tx rings, along with various 'dev' bits. */
static void
rtl8129_init_ring(struct device *dev)
{
	struct rtl8129_private *tp = (struct rtl8129_private *)dev->priv;
	int i;

	tp->tx_full = 0;
	tp->cur_rx = 0;
	tp->dirty_tx = tp->cur_tx = 0;

	for (i = 0; i < NUM_TX_DESC; i++) {
		tp->tx_skbuff[i] = 0;
		tp->tx_buf[i] = &tp->tx_bufs[i*TX_BUF_SIZE];
	}
}

static int
rtl8129_start_xmit(struct sk_buff *skb, struct device *dev)
{
	struct rtl8129_private *tp = (struct rtl8129_private *)dev->priv;
	long ioaddr = dev->base_addr;
	int entry;

	/* Block a timer-based transmit from overlapping.  This could better be
	   done with atomic_swap(1, dev->tbusy), but set_bit() works as well. */
	if (test_and_set_bit(0, (void*)&dev->tbusy) != 0) {
		if (jiffies - dev->trans_start < TX_TIMEOUT)
			return 1;
		rtl8129_tx_timeout(dev);
		return 1;
	}

	/* Calculate the next Tx descriptor entry. */
	entry = tp->cur_tx % NUM_TX_DESC;

	tp->tx_skbuff[entry] = skb;
	if ((long)skb->data & 3) {			/* Must use alignment buffer. */
		memcpy(tp->tx_buf[entry], skb->data, skb->len);
		outl(virt_to_bus(tp->tx_buf[entry]), ioaddr + TxAddr0 + entry*4);
	} else
		outl(virt_to_bus(skb->data), ioaddr + TxAddr0 + entry*4);
	/* Note: the chip doesn't have auto-pad! */
	outl(tp->tx_flag | (skb->len >= ETH_ZLEN ? skb->len : ETH_ZLEN),
		 ioaddr + TxStatus0 + entry*4);

	if (++tp->cur_tx - tp->dirty_tx < NUM_TX_DESC) {/* Typical path */
		clear_bit(0, (void*)&dev->tbusy);
	} else {
		tp->tx_full = 1;
	}

	dev->trans_start = jiffies;
	if (rtl8129_debug > 4)
		printk(KERN_DEBUG"%s: Queued Tx packet at %p size %d to slot %d.\n",
			   dev->name, skb->data, (int)skb->len, entry);

	return 0;
}

/* The interrupt handler does all of the Rx thread work and cleans up
   after the Tx thread. */
static void rtl8129_interrupt(int irq, void *dev_instance, struct pt_regs *regs)
{
	struct device *dev = (struct device *)dev_instance;
	struct rtl8129_private *tp = (struct rtl8129_private *)dev->priv;
	int boguscnt = max_interrupt_work;
	int status;
	long ioaddr = dev->base_addr;

#if defined(__i386__)
	/* A lock to prevent simultaneous entry bug on Intel SMP machines. */
	if (test_and_set_bit(0, (void*)&dev->interrupt)) {
		printk(KERN_ERR"%s: SMP simultaneous entry of an interrupt handler.\n",
			   dev->name);
		dev->interrupt = 0;	/* Avoid halting machine. */
		return;
	}
#else
	if (dev->interrupt) {
		printk(KERN_ERR "%s: Re-entering the interrupt handler.\n", dev->name);
		return;
	}
	dev->interrupt = 1;
#endif

	do {
		status = inw(ioaddr + IntrStatus);
		/* Acknowledge all of the current interrupt sources ASAP. */
		outw(status, ioaddr + IntrStatus);

		if (rtl8129_debug > 4)
			printk(KERN_DEBUG"%s: interrupt  status=%#4.4x new intstat=%#4.4x.\n",
				   dev->name, status, inw(ioaddr + IntrStatus));

		if ((status & (PCIErr|PCSTimeout|RxUnderrun|RxOverflow|RxFIFOOver
					   |TxErr|TxOK|RxErr|RxOK)) == 0)
			break;

		if (status & (RxOK|RxUnderrun|RxOverflow|RxFIFOOver))/* Rx interrupt */
			rtl8129_rx(dev);

		if (status & (TxOK | TxErr)) {
			unsigned int dirty_tx;

			for (dirty_tx = tp->dirty_tx; dirty_tx < tp->cur_tx; dirty_tx++) {
				int entry = dirty_tx % NUM_TX_DESC;
				int txstatus = inl(ioaddr + TxStatus0 + entry*4);

				if ( ! (txstatus & (TxStatOK | TxUnderrun | TxAborted)))
					break;			/* It still hasn't been Txed */

				/* Note: TxCarrierLost is always asserted at 100mbps. */
				if (txstatus & (TxOutOfWindow | TxAborted)) {
					/* There was an major error, log it. */
#ifndef final_version
					if (rtl8129_debug > 1)
						printk(KERN_NOTICE"%s: Transmit error, Tx status %8.8x.\n",
							   dev->name, txstatus);
#endif
					tp->stats.tx_errors++;
					if (txstatus&TxAborted) {
						tp->stats.tx_aborted_errors++;
						outl((TX_DMA_BURST<<8)|0x03000001, ioaddr + TxConfig);
					}
					if (txstatus&TxCarrierLost) tp->stats.tx_carrier_errors++;
					if (txstatus&TxOutOfWindow) tp->stats.tx_window_errors++;
#ifdef ETHER_STATS
					if ((txstatus & 0x0f000000) == 0x0f000000)
						tp->stats.collisions16++;
#endif
				} else {
#ifdef ETHER_STATS
					/* No count for tp->stats.tx_deferred */
#endif
					if (txstatus & TxUnderrun) {
						/* Add 64 to the Tx FIFO threshold. */
						if (tp->tx_flag <  0x00300000)
							tp->tx_flag += 0x00020000;
						tp->stats.tx_fifo_errors++;
					}
					tp->stats.collisions += (txstatus >> 24) & 15;
#if LINUX_VERSION_CODE > 0x20119
					tp->stats.tx_bytes += txstatus & 0x7ff;
#endif
					tp->stats.tx_packets++;
				}

				/* Free the original skb. */
				dev_free_skb(tp->tx_skbuff[entry]);
				tp->tx_skbuff[entry] = 0;
			}

#ifndef final_version
			if (tp->cur_tx - dirty_tx > NUM_TX_DESC) {
				printk(KERN_ERR"%s: Out-of-sync dirty pointer, %d vs. %d, full=%d.\n",
					   dev->name, dirty_tx, tp->cur_tx, tp->tx_full);
				dirty_tx += NUM_TX_DESC;
			}
#endif

			if (tp->tx_full  &&  dirty_tx > tp->cur_tx - NUM_TX_DESC) {
				/* The ring is no longer full, clear tbusy. */
				tp->tx_full = 0;
				dev->tbusy = 0;
				mark_bh(NET_BH);
			}

			tp->dirty_tx = dirty_tx;
		}

		/* Check uncommon events with one test. */
		if (status & (PCIErr|PCSTimeout |RxUnderrun|RxOverflow|RxFIFOOver
					  |TxErr|RxErr)) {
			if (rtl8129_debug > 2)
				printk(KERN_NOTICE"%s: Abnormal interrupt, status %8.8x.\n",
					   dev->name, status);

			if (status == 0xffffffff)
				break;
			/* Update the error count. */
			tp->stats.rx_missed_errors += inl(ioaddr + RxMissed);
			outl(0, ioaddr + RxMissed);

			if ((status & RxUnderrun)  &&
				(rtl_cap_tbl[tp->chip_id] & HAS_LNK_CHNG)) {
				/* Really link-change on new chips. */
				int lpar = inw(ioaddr + NWayLPAR);
				int duplex = (lpar&0x0100)||(lpar & 0x01C0) == 0x0040; 
				if (tp->full_duplex != duplex) {
					tp->full_duplex = duplex;
					outb(0xC0, ioaddr + Cfg9346);
					outb(tp->full_duplex ? 0x60 : 0x20, ioaddr + Config1);
					outb(0x00, ioaddr + Cfg9346);
				}
				status &= ~RxUnderrun;
			}
			if (status & (RxUnderrun | RxOverflow | RxErr | RxFIFOOver))
				tp->stats.rx_errors++;

			if (status & (PCSTimeout)) tp->stats.rx_length_errors++;
			if (status & (RxUnderrun|RxFIFOOver)) tp->stats.rx_fifo_errors++;
			if (status & RxOverflow) {
				tp->stats.rx_over_errors++;
				tp->cur_rx = inw(ioaddr + RxBufAddr) % RX_BUF_LEN;
				outw(tp->cur_rx - 16, ioaddr + RxBufPtr);
			}
			if (status & PCIErr) {
				u32 pci_cmd_status;
				pcibios_read_config_dword(tp->pci_bus, tp->pci_devfn,
										  PCI_COMMAND, &pci_cmd_status);

				printk(KERN_ERR "%s: PCI Bus error %4.4x.\n",
					   dev->name, pci_cmd_status);
			}
		}
		if (--boguscnt < 0) {
			printk(KERN_WARNING"%s: Too much work at interrupt, "
				   "IntrStatus=0x%4.4x.\n",
				   dev->name, status);
			/* Clear all interrupt sources. */
			outw(0xffff, ioaddr + IntrStatus);
			break;
		}
	} while (1);

	if (rtl8129_debug > 3)
		printk(KERN_DEBUG"%s: exiting interrupt, intr_status=%#4.4x.\n",
			   dev->name, inl(ioaddr + IntrStatus));

#if defined(__i386__)
	clear_bit(0, (void*)&dev->interrupt);
#else
	dev->interrupt = 0;
#endif
	return;
}

/* The data sheet doesn't describe the Rx ring at all, so I'm guessing at the
   field alignments and semantics. */
static int rtl8129_rx(struct device *dev)
{
	struct rtl8129_private *tp = (struct rtl8129_private *)dev->priv;
	long ioaddr = dev->base_addr;
	unsigned char *rx_ring = tp->rx_ring;
	u16 cur_rx = tp->cur_rx;

	if (rtl8129_debug > 4)
		printk(KERN_DEBUG"%s: In rtl8129_rx(), current %4.4x BufAddr %4.4x,"
			   " free to %4.4x, Cmd %2.2x.\n",
			   dev->name, cur_rx, inw(ioaddr + RxBufAddr),
			   inw(ioaddr + RxBufPtr), inb(ioaddr + ChipCmd));

	while ((inb(ioaddr + ChipCmd) & 1) == 0) {
		int ring_offset = cur_rx % RX_BUF_LEN;
		u32 rx_status = *(u32*)(rx_ring + ring_offset);
		int rx_size = rx_status >> 16;

		if (rtl8129_debug > 4) {
			int i;
			printk(KERN_DEBUG"%s:  rtl8129_rx() status %4.4x, size %4.4x, cur %4.4x.\n",
				   dev->name, rx_status, rx_size, cur_rx);
			printk(KERN_DEBUG"%s: Frame contents ", dev->name);
			for (i = 0; i < 70; i++)
				printk(" %2.2x", rx_ring[ring_offset + i]);
			printk(".\n");
		}
		if (rx_status & RxTooLong) {
			if (rtl8129_debug > 0)
				printk(KERN_NOTICE"%s: Oversized Ethernet frame, status %4.4x!\n",
					   dev->name, rx_status);
			tp->stats.rx_length_errors++;
		} else if (rx_status &
				   (RxBadSymbol|RxRunt|RxTooLong|RxCRCErr|RxBadAlign)) {
			if (rtl8129_debug > 1)
				printk(KERN_DEBUG"%s: Ethernet frame had errors,"
					   " status %4.4x.\n", dev->name, rx_status);
			tp->stats.rx_errors++;
			if (rx_status & (RxBadSymbol|RxBadAlign))
				tp->stats.rx_frame_errors++;
			if (rx_status & (RxRunt|RxTooLong)) tp->stats.rx_length_errors++;
			if (rx_status & RxCRCErr) tp->stats.rx_crc_errors++;
			/* Reset the receiver, based on RealTek recommendation. (Bug?) */
			tp->cur_rx = 0;
			outb(CmdTxEnb, ioaddr + ChipCmd);
			outb(CmdRxEnb | CmdTxEnb, ioaddr + ChipCmd);
			outl((RX_FIFO_THRESH << 13) | (RX_BUF_LEN_IDX << 11) |
				 (RX_DMA_BURST<<8), ioaddr + RxConfig);
		} else {
			/* Malloc up new buffer, compatible with net-2e. */
			/* Omit the four octet CRC from the length. */
			struct sk_buff *skb;

			skb = dev_alloc_skb(rx_size + 2);
			if (skb == NULL) {
				printk(KERN_WARNING"%s: Memory squeeze, deferring packet.\n",
					   dev->name);
				/* We should check that some rx space is free.
				   If not, free one and mark stats->rx_dropped++. */
				tp->stats.rx_dropped++;
				break;
			}
			skb->dev = dev;
			skb_reserve(skb, 2);	/* 16 byte align the IP fields. */
			if (ring_offset+rx_size+4 > RX_BUF_LEN) {
				int semi_count = RX_BUF_LEN - ring_offset - 4;
				memcpy(skb_put(skb, semi_count), &rx_ring[ring_offset + 4],
					   semi_count);
				memcpy(skb_put(skb, rx_size-semi_count), rx_ring,
					   rx_size-semi_count);
				if (rtl8129_debug > 4) {
					int i;
					printk(KERN_DEBUG"%s:  Frame wrap @%d",
						   dev->name, semi_count);
					for (i = 0; i < 16; i++)
						printk(" %2.2x", rx_ring[i]);
					printk(".\n");
					memset(rx_ring, 0xcc, 16);
				}
			} else {
#if 1  /* USE_IP_COPYSUM */
				eth_copy_and_sum(skb, &rx_ring[ring_offset + 4],
								 rx_size, 0);
				skb_put(skb, rx_size);
#else
				memcpy(skb_put(skb, rx_size), &rx_ring[ring_offset + 4],
					   rx_size);
#endif
			}
			skb->protocol = eth_type_trans(skb, dev);
			netif_rx(skb);
#if LINUX_VERSION_CODE > 0x20119
			tp->stats.rx_bytes += rx_size;
#endif
			tp->stats.rx_packets++;
		}

		cur_rx = (cur_rx + rx_size + 4 + 3) & ~3;
		outw(cur_rx - 16, ioaddr + RxBufPtr);
	}
	if (rtl8129_debug > 4)
		printk(KERN_DEBUG"%s: Done rtl8129_rx(), current %4.4x BufAddr %4.4x,"
			   " free to %4.4x, Cmd %2.2x.\n",
			   dev->name, cur_rx, inw(ioaddr + RxBufAddr),
			   inw(ioaddr + RxBufPtr), inb(ioaddr + ChipCmd));
	tp->cur_rx = cur_rx;
	return 0;
}

static int
rtl8129_close(struct device *dev)
{
	long ioaddr = dev->base_addr;
	struct rtl8129_private *tp = (struct rtl8129_private *)dev->priv;
	int i;

	dev->start = 0;
	dev->tbusy = 1;

	if (rtl8129_debug > 1)
		printk(KERN_DEBUG"%s: Shutting down ethercard, status was 0x%4.4x.\n",
			   dev->name, inw(ioaddr + IntrStatus));

	/* Disable interrupts by clearing the interrupt mask. */
	outw(0x0000, ioaddr + IntrMask);

	/* Stop the chip's Tx and Rx DMA processes. */
	outb(0x00, ioaddr + ChipCmd);

	/* Update the error counts. */
	tp->stats.rx_missed_errors += inl(ioaddr + RxMissed);
	outl(0, ioaddr + RxMissed);

	del_timer(&tp->timer);

	free_irq(dev->irq, dev);

	for (i = 0; i < NUM_TX_DESC; i++) {
		if (tp->tx_skbuff[i])
			dev_free_skb(tp->tx_skbuff[i]);
		tp->tx_skbuff[i] = 0;
	}
	kfree(tp->rx_ring);
	kfree(tp->tx_bufs);

	/* Green! Put the chip in low-power mode. */
	outb(0xC0, ioaddr + Cfg9346);
	outb(0x03, ioaddr + Config1);
	outb('H', ioaddr + HltClk);		/* 'R' would leave the clock running. */

	MOD_DEC_USE_COUNT;

	return 0;
}

static int mii_ioctl(struct device *dev, struct ifreq *rq, int cmd)
{
	struct rtl8129_private *tp = (struct rtl8129_private *)dev->priv;
	u16 *data = (u16 *)&rq->ifr_data;

	switch(cmd) {
	case SIOCDEVPRIVATE:		/* Get the address of the PHY in use. */
		data[0] = tp->phys[0] & 0x3f;
		/* Fall Through */
	case SIOCDEVPRIVATE+1:		/* Read the specified MII register. */
		data[3] = mdio_read(dev, data[0] & 0x1f, data[1] & 0x1f);
		return 0;
	case SIOCDEVPRIVATE+2:		/* Write the specified MII register */
		if (!suser())
			return -EPERM;
		mdio_write(dev, data[0] & 0x1f, data[1] & 0x1f, data[2]);
		return 0;
	default:
		return -EOPNOTSUPP;
	}
}

static struct enet_statistics *
rtl8129_get_stats(struct device *dev)
{
	struct rtl8129_private *tp = (struct rtl8129_private *)dev->priv;
	long ioaddr = dev->base_addr;

	if (dev->start) {
		tp->stats.rx_missed_errors += inl(ioaddr + RxMissed);
		outl(0, ioaddr + RxMissed);
	}

	return &tp->stats;
}

/* Set or clear the multicast filter for this adaptor.
   This routine is not state sensitive and need not be SMP locked. */

static unsigned const ethernet_polynomial = 0x04c11db7U;
static inline u32 ether_crc(int length, unsigned char *data)
{
    int crc = -1;

    while(--length >= 0) {
		unsigned char current_octet = *data++;
		int bit;
		for (bit = 0; bit < 8; bit++, current_octet >>= 1)
			crc = (crc << 1) ^
				((crc < 0) ^ (current_octet & 1) ? ethernet_polynomial : 0);
    }
    return crc;
}

/* Bits in RxConfig. */
enum rx_mode_bits {
	AcceptErr=0x20, AcceptRunt=0x10, AcceptBroadcast=0x08,
	AcceptMulticast=0x04, AcceptMyPhys=0x02, AcceptAllPhys=0x01,
};

static void set_rx_mode(struct device *dev)
{
	long ioaddr = dev->base_addr;
	u32 mc_filter[2];		 /* Multicast hash filter */
	int i, rx_mode;

	if (rtl8129_debug > 3)
		printk(KERN_DEBUG"%s:   set_rx_mode(%4.4x) done -- Rx config %8.8x.\n",
			   dev->name, dev->flags, inl(ioaddr + RxConfig));

	/* Note: do not reorder, GCC is clever about common statements. */
	if (dev->flags & IFF_PROMISC) {
		/* Unconditionally log net taps. */
		printk(KERN_NOTICE"%s: Promiscuous mode enabled.\n", dev->name);
		rx_mode = AcceptBroadcast|AcceptMulticast|AcceptMyPhys|AcceptAllPhys;
		mc_filter[1] = mc_filter[0] = 0xffffffff;
	} else if ((dev->mc_count > multicast_filter_limit)
			   ||  (dev->flags & IFF_ALLMULTI)) {
		/* Too many to filter perfectly -- accept all multicasts. */
		rx_mode = AcceptBroadcast | AcceptMulticast | AcceptMyPhys;
		mc_filter[1] = mc_filter[0] = 0xffffffff;
	} else {
		struct dev_mc_list *mclist;
		rx_mode = AcceptBroadcast | AcceptMulticast | AcceptMyPhys;
		mc_filter[1] = mc_filter[0] = 0;
		for (i = 0, mclist = dev->mc_list; mclist && i < dev->mc_count;
			 i++, mclist = mclist->next)
			set_bit(ether_crc(ETH_ALEN, mclist->dmi_addr) >> 26, mc_filter);
	}
	/* We can safely update without stopping the chip. */
	outb(rx_mode, ioaddr + RxConfig);
	outl(mc_filter[0], ioaddr + MAR0 + 0);
	outl(mc_filter[1], ioaddr + MAR0 + 4);
	return;
}

#ifdef MODULE
int init_module(void)
{
	return rtl8139_probe(0);
}

void
cleanup_module(void)
{
	struct device *next_dev;

	/* No need to check MOD_IN_USE, as sys_delete_module() checks. */
	while (root_rtl8129_dev) {
		struct rtl8129_private *tp =
			(struct rtl8129_private *)root_rtl8129_dev->priv;
		next_dev = tp->next_module;
		unregister_netdev(root_rtl8129_dev);
		release_region(root_rtl8129_dev->base_addr,
					   pci_tbl[tp->chip_id].io_size);
		kfree(tp);
		kfree(root_rtl8129_dev);
		root_rtl8129_dev = next_dev;
	}
}

#endif  /* MODULE */

/*
 * Local variables:
 *  compile-command: "gcc -DMODULE -D__KERNEL__ -Wall -Wstrict-prototypes -O6 -c rtl8139.c `[ -f /usr/include/linux/modversions.h ] && echo -DMODVERSIONS`"
 *  SMP-compile-command: "gcc -D__SMP__ -DMODULE -D__KERNEL__ -Wall -Wstrict-prototypes -O6 -c rtl8139.c `[ -f /usr/include/linux/modversions.h ] && echo -DMODVERSIONS`"
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  tab-width: 4
 * End:
 */
