/*
     cardbus and pcmcia (grmph) support.
*/
#include "u.h"
#include "lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "error.h"
#include "io.h"

#define ioalloc(addr, len, align, name)	(addr)
#define iofree(addr)
extern int pciscan(int, Pcidev **);
extern ulong pcibarsize(Pcidev *, int);

int (*_pcmspecial)(char *, ISAConf *);
void (*_pcmspecialclose)(int);

int
pcmspecial(char *idstr, ISAConf *isa)
{
	return (_pcmspecial  != nil)? _pcmspecial(idstr, isa): -1;
}

void
pcmspecialclose(int a)
{
	if (_pcmspecialclose != nil)
		_pcmspecialclose(a);
}

static ulong
ioreserve(ulong, int size, int align, char *)
{
	static ulong isaend = 0xfd00;
	ulong ioaddr;

	if (align)
		isaend = ((isaend + align - 1) / align) * align;
	ioaddr = isaend;
	isaend += size;
	return ioaddr;
}

#define MAP(x,o)	(Rmap + (x)*0x8 + o)

enum {
	TI_vid = 0x104c,
	TI_1131_did = 0xAC15,
	TI_1250_did = 0xAC16,
	TI_1450_did = 0xAC1B,
	TI_1251A_did = 0xAC1D,

	Ricoh_vid = 0x1180,
	Ricoh_476_did = 0x0476,
	Ricoh_478_did = 0x0478,

	Nslots = 4,		/* Maximum number of CardBus slots to use */

	K = 1024,
	M = K * K,

	LegacyAddr = 0x3e0,
	NUMEVENTS = 10,

	TI1131xSC = 0x80,		// system control
		TI122X_SC_INTRTIE	= 1 << 29,
	TI12xxIM = 0x8c,		// 
	TI1131xCC = 0x91,		// card control
		TI113X_CC_RIENB = 1 << 7,
		TI113X_CC_ZVENABLE = 1 << 6,
		TI113X_CC_PCI_IRQ_ENA = 1 << 5,
		TI113X_CC_PCI_IREQ = 1 << 4,
		TI113X_CC_PCI_CSC = 1 << 3,
		TI113X_CC_SPKROUTEN = 1 << 1,
		TI113X_CC_IFG = 1 << 0,
	TI1131xDC = 0x92,		// device control
};

typedef struct {
	ushort	r_vid;
	ushort	r_did;
	char		*r_name;
} variant_t;

static variant_t variant[] = {
{	Ricoh_vid,	Ricoh_476_did,	"Ricoh 476 PCI/Cardbus bridge",	},
{	Ricoh_vid,	Ricoh_478_did,	"Ricoh 478 PCI/Cardbus bridge",	},
{	TI_vid,		TI_1131_did,		"TI PCI-1131 Cardbus Controller",	},
{	TI_vid,		TI_1250_did,		"TI PCI-1250 Cardbus Controller",	},
{	TI_vid,		TI_1450_did,		"TI PCI-1450 Cardbus Controller",	},
{	TI_vid,		TI_1251A_did,		"TI PCI-1251A Cardbus Controller",	},
};

/* Cardbus registers */
enum {
	SocketEvent = 0,
		SE_CCD = 3 << 1,
		SE_POWER = 1 << 3,
	SocketMask = 1,
	SocketState = 2,
		SS_CCD = 3 << 1,
		SS_POWER = 1 << 3,
		SS_PC16 = 1 << 4,
		SS_CBC = 1 << 5,
		SS_NOTCARD = 1 << 7,
		SS_BADVCC = 1 << 9,
		SS_5V = 1 << 10,
		SS_3V = 1 << 11,
	SocketForce = 3,
	SocketControl = 4,
		SC_5V = 0x22,
		SC_3V = 0x33,
};

enum {
	PciPCR_IO = 1 << 0,
	PciPCR_MEM = 1 << 1,
	PciPCR_Master = 1 << 2,

	Nbars = 6,
	Ncmd = 10,
	CBIRQ = 9,

	PC16,
	PC32,
};

enum {
	Ti82365,
	Tpd6710,
	Tpd6720,
	Tvg46x,
};

static char *chipname[] = {
[Ti82365]		"Intel 82365SL",
[Tpd6710]	"Cirrus Logic PD6710",
[Tpd6720]	"Cirrus Logic PD6720",
[Tvg46x]		"Vadem VG-46x",
};

/*
 *  Intel 82365SL PCIC controller for the PCMCIA or
 *  Cirrus Logic PD6710/PD6720 which is mostly register compatible
 */
enum
{
	/*
	 *  registers indices
	 */
	Rid=		0x0,		/* identification and revision */
	Ris=		0x1,		/* interface status */
	Rpc=	 	0x2,		/* power control */
	 Foutena=	 (1<<7),	/*  output enable */
	 Fautopower=	 (1<<5),	/*  automatic power switching */
	 Fcardena=	 (1<<4),	/*  PC card enable */
	Rigc= 		0x3,		/* interrupt and general control */
	 Fiocard=	 (1<<5),	/*  I/O card (vs memory) */
	 Fnotreset=	 (1<<6),	/*  reset if not set */	
	 FSMIena=	 (1<<4),	/*  enable change interrupt on SMI */ 
	Rcsc= 		0x4,		/* card status change */
	Rcscic= 	0x5,		/* card status change interrupt config */
	 Fchangeena=	 (1<<3),	/*  card changed */
	 Fbwarnena=	 (1<<1),	/*  card battery warning */
	 Fbdeadena=	 (1<<0),	/*  card battery dead */
	Rwe= 		0x6,		/* address window enable */
	 Fmem16=	 (1<<5),	/*  use A23-A12 to decode address */
	Rio= 		0x7,		/* I/O control */
	 Fwidth16=	 (1<<0),	/*  16 bit data width */
	 Fiocs16=	 (1<<1),	/*  IOCS16 determines data width */
	 Fzerows=	 (1<<2),	/*  zero wait state */
	 Ftiming=	 (1<<3),	/*  timing register to use */
	Riobtm0lo=	0x8,		/* I/O address 0 start low byte */
	Riobtm0hi=	0x9,		/* I/O address 0 start high byte */
	Riotop0lo=	0xa,		/* I/O address 0 stop low byte */
	Riotop0hi=	0xb,		/* I/O address 0 stop high byte */
	Riobtm1lo=	0xc,		/* I/O address 1 start low byte */
	Riobtm1hi=	0xd,		/* I/O address 1 start high byte */
	Riotop1lo=	0xe,		/* I/O address 1 stop low byte */
	Riotop1hi=	0xf,		/* I/O address 1 stop high byte */
	Rmap=		0x10,		/* map 0 */

	/*
	 *  CL-PD67xx extension registers
	 */
	Rmisc1=		0x16,		/* misc control 1 */
	 F5Vdetect=	 (1<<0),
	 Fvcc3V=	 (1<<1),
	 Fpmint=	 (1<<2),
	 Fpsirq=	 (1<<3),
	 Fspeaker=	 (1<<4),
	 Finpack=	 (1<<7),
	Rfifo=		0x17,		/* fifo control */
	 Fflush=	 (1<<7),	/*  flush fifo */
	Rmisc2=		0x1E,		/* misc control 2 */
	 Flowpow=	 (1<<1),	/*  low power mode */
	Rchipinfo=	0x1F,		/* chip information */
	Ratactl=	0x26,		/* ATA control */

	/*
	 *  offsets into the system memory address maps
	 */
	Mbtmlo=		0x0,		/* System mem addr mapping start low byte */
	Mbtmhi=		0x1,		/* System mem addr mapping start high byte */
	 F16bit=	 (1<<7),	/*  16-bit wide data path */
	Mtoplo=		0x2,		/* System mem addr mapping stop low byte */
	Mtophi=		0x3,		/* System mem addr mapping stop high byte */
	 Ftimer1=	 (1<<6),	/*  timer set 1 */
	Mofflo=		0x4,		/* Card memory offset address low byte */
	Moffhi=		0x5,		/* Card memory offset address high byte */
	 Fregactive=	 (1<<6),	/*  attribute memory */

	/*
	 *  configuration registers - they start at an offset in attribute
	 *  memory found in the CIS.
	 */
	Rconfig=	0,
	 Creset=	 (1<<7),	/*  reset device */
	 Clevel=	 (1<<6),	/*  level sensitive interrupt line */
};

/*
 *  read and crack the card information structure enough to set
 *  important parameters like power
 */
/* cis memory walking */
typedef struct Cisdat {
	uchar		*cisbase;
	int			cispos;
	int			cisskip;
	int			cislen;
} Cisdat;

/* configuration table entry */
typedef struct PCMconftab	PCMconftab;
struct PCMconftab
{
	int	index;
	ushort	irqs;		/* legal irqs */
	uchar	irqtype;
	uchar	bit16;		/* true for 16 bit access */
	struct {
		ulong	start;
		ulong	len;
	} io[16];
	int	nio;
	uchar	vpp1;
	uchar	vpp2;
	uchar	memwait;
	ulong	maxwait;
	ulong	readywait;
	ulong	otherwait;
};

typedef struct {
	char			pi_verstr[512];		/* Version string */
	PCMmap		pi_mmap[4];		/* maps, last is always for the kernel */
	ulong		pi_conf_addr;		/* Config address */
	uchar		pi_conf_present;	/* Config register present */
	int			pi_nctab;			/* In use configuration tables */
	PCMconftab	pi_ctab[8];		/* Configuration tables */
	PCMconftab	*pi_defctab;		/* Default conftab */

	int			pi_port;			/* Actual port usage */
	int			pi_irq;			/* Actual IRQ usage */
} pcminfo_t;

#define qlock(i)	{/* nothing to do */;}
#define qunlock(i)	{/* nothing to do */;}
typedef struct QLock { int r; } QLock;

typedef struct {
	QLock;
	variant_t		*cb_variant;		/* Which CardBus chipset */
	Pcidev		*cb_pci;			/* The bridge itself */
	ulong		*cb_regs;			/* Cardbus registers */
	int			cb_ltype;			/* Legacy type */
	int			cb_lindex;		/* Legacy port index address */
	int			cb_ldata;			/* Legacy port data address */
	int			cb_lbase;			/* Base register for this socket */

	int			cb_state;			/* Current state of card */
	int			cb_type;			/* Type of card */
	pcminfo_t	cb_linfo;			/* PCMCIA slot info */

	int			cb_refs;			/* Number of refs to slot */
	QLock		cb_refslock;		/* inc/dev ref lock */
} cb_t;

static int managerstarted;

enum {
	Mshift=	12,
	Mgran=	(1<<Mshift),	/* granularity of maps */
	Mmask=	~(Mgran-1),	/* mask for address bits important to the chip */
};

static cb_t cbslots[Nslots];
static int nslots;

static ulong exponent[8] = { 
	1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 
};

static ulong vmant[16] = {
	10, 12, 13, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 70, 80, 90,
};

static ulong mantissa[16] = { 
	0, 10, 12, 13, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 70, 80, 
};

static char Enocard[] = "No card in slot";

static void cbint(Ureg *, void *);
static int powerup(cb_t *);
static void configure(cb_t *);
static void managecard(cb_t *);
static void cardmanager(void *);
static void eject(cb_t *);
static void interrupt(Ureg *, void *);
static void powerdown(cb_t *cb);
static void unconfigure(cb_t *cb);

static void i82365probe(cb_t *cb, int lindex, int ldata);
static void i82365configure(cb_t *cb);
static PCMmap *isamap(cb_t *cb, ulong offset, int len, int attr);
static void isaunmap(PCMmap* m);
static uchar rdreg(cb_t *cb, int index);
static void wrreg(cb_t *cb, int index, uchar val);
static int readc(Cisdat *cis, uchar *x);
static void tvers1(cb_t *cb, Cisdat *cis, int );
static void tcfig(cb_t *cb, Cisdat *cis, int );
static void tentry(cb_t *cb, Cisdat *cis, int );
static int vcode(int volt);
static int pccard_pcmspecial(char *idstr, ISAConf *isa);
static void pccard_pcmspecialclose(int slotno);

enum {
	CardDetected,
	CardPowered,
	CardEjected,
	CardConfigured,
};

static char *messages[] = {
[CardDetected]		"CardDetected",
[CardPowered]		"CardPowered",
[CardEjected]		"CardEjected",
[CardConfigured]	"CardConfigured",
};

enum {
	SlotEmpty,
	SlotFull,
	SlotPowered,
	SlotConfigured,
};

static char *states[] = {
[SlotEmpty]		"SlotEmpty",
[SlotFull]			"SlotFull",
[SlotPowered]		"SlotPowered",
[SlotConfigured]	"SlotConfigured",
};

static void
engine(cb_t *cb, int message)
{
	// print("engine(%d): %s(%s)\n", 
	//	 (int)(cb - cbslots), states[cb->cb_state], messages[message]);
	switch (cb->cb_state) {
	case SlotEmpty:

		switch (message) {
		case CardDetected:
			cb->cb_state = SlotFull;
			powerup(cb);
			break;
		case CardEjected:
			break;
		default:
			print("#Y%d: Invalid message %s in SlotEmpty state\n",
				(int)(cb - cbslots), messages[message]);
			break;
		}
		break;

	case SlotFull:

		switch (message) {
		case CardPowered:
			cb->cb_state = SlotPowered;
			configure(cb);
			break;
		case CardEjected:
			cb->cb_state = SlotEmpty;
			powerdown(cb);
			break;
		default:
			//print("#Y%d: Invalid message %s in SlotFull state\n",
			//	(int)(cb - cbslots), messages[message]);
			break;
		}
		break;

	case SlotPowered:

		switch (message) {
		case CardConfigured:
			cb->cb_state = SlotConfigured;
			break;
		case CardEjected:
			cb->cb_state = SlotEmpty;
			unconfigure(cb);
			powerdown(cb);
			break;
		default:
			print("#Y%d: Invalid message %s in SlotPowered state\n",
				(int)(cb - cbslots), messages[message]);
			break;
		}
		break;

	case SlotConfigured:

		switch (message) {
		case CardEjected:
			cb->cb_state = SlotEmpty;
			unconfigure(cb);
			powerdown(cb);
			break;
		default:
			print("#Y%d: Invalid message %s in SlotConfigured state\n",
				(int)(cb - cbslots), messages[message]);
			break;
		}
		break;
	}
}

static void
qengine(cb_t *cb, int message)
{
	qlock(cb);
	engine(cb, message);
	qunlock(cb);
}

typedef struct {
	cb_t	*e_cb;
	int	e_message;
} events_t;

static Lock levents;
static events_t events[NUMEVENTS];
// static Rendez revents;
static int nevents;

//static void
//iengine(cb_t *cb, int message)
//{
//	if (nevents >= NUMEVENTS) {
//		print("#Y: Too many events queued, discarding request\n");
//		return;
//	}
//	ilock(&levents);
//	events[nevents].e_cb = cb;
//	events[nevents].e_message = message;
//	nevents++;
//	iunlock(&levents);
//	wakeup(&revents);
//}

static int
eventoccured(void)
{
	return nevents > 0;
}

// static void
// processevents(void *)
// {
//	while (1) {
//		int message;
//		cb_t *cb;
//
//		sleep(&revents, (int (*)(void *))eventoccured, nil);
//
//		cb = nil;
//		message = 0;
//		ilock(&levents);
//		if (nevents > 0) {
//			cb = events[0].e_cb;
//			message = events[0].e_message;
//			nevents--;
//			if (nevents > 0)
//				memmove(events, &events[1], nevents * sizeof(events_t));
//		}
//		iunlock(&levents);
//
//		if (cb)
//			qengine(cb, message);
//	}
// }

// static void
// interrupt(Ureg *, void *)
// {
// 	int i;
// 
// 	for (i = 0; i != nslots; i++) {
// 		cb_t *cb = &cbslots[i];
// 		ulong event, state;
// 
// 		event= cb->cb_regs[SocketEvent];
// 		state = cb->cb_regs[SocketState];
// 		rdreg(cb, Rcsc);	/* Ack the interrupt */
// 
// 		print("interrupt: slot %d, event %.8lX, state %.8lX, (%s)\n", 
// 			(int)(cb - cbslots), event, state, states[cb->cb_state]);
// 
// 		if (event & SE_CCD) {
// 			cb->cb_regs[SocketEvent] |= SE_CCD;	/* Ack interrupt */
// 			if (state & SE_CCD) {
// 				if (cb->cb_state != SlotEmpty) {
// 					print("#Y: take cardejected interrupt\n");
// 					iengine(cb, CardEjected);
// 				}
// 			}
// 			else
// 				iengine(cb, CardDetected);
// 		}
// 
// 		if (event & SE_POWER) {
// 			cb->cb_regs[SocketEvent] |= SE_POWER;	/* Ack interrupt */
// 			iengine(cb, CardPowered);
// 		}
// 	}
// }

void
devpccardlink(void)
{
	static int initialized;
	Pcidev *pci;
	int i;
//	uchar intl;

	if (initialized) 
		return;
	initialized = 1;

	if (!getconf("pccard0"))
		return;

	if (_pcmspecial) {
		print("#Y: CardBus and PCMCIA at the same time?\n");
		return;
	}

	_pcmspecial = pccard_pcmspecial;
	_pcmspecialclose = pccard_pcmspecialclose;


	/* Allocate legacy space */
	if (ioalloc(LegacyAddr, 2, 0, "i82365.0") < 0)
		print("#Y: WARNING: Cannot allocate legacy ports\n");

	/* Find all CardBus controllers */
	pci = nil;
//	intl = (uchar)-1;
	while ((pci = pcimatch(pci, 0, 0)) != nil) {
		ulong baddr;
		uchar pin;
		cb_t *cb;
		int slot;

		for (i = 0; i != nelem(variant); i++)
			if (pci->vid == variant[i].r_vid && pci->did == variant[i].r_did)
				break;
		if (i == nelem(variant))
			continue;

		/* initialize this slot */
		slot = nslots++;
		cb = &cbslots[slot];

		cb->cb_pci = pci;
		cb->cb_variant = &variant[i];
		
		// Don't you love standards!
		if (pci->vid == TI_vid) {
			if (pci->did <= TI_1131_did) {
				uchar cc;

				cc = pcicfgr8(pci, TI1131xCC);
				cc &= ~(TI113X_CC_PCI_IRQ_ENA |
						TI113X_CC_PCI_IREQ | 
						TI113X_CC_PCI_CSC |
						TI113X_CC_ZVENABLE);
				cc |= TI113X_CC_PCI_IRQ_ENA | 
						TI113X_CC_PCI_IREQ | 
						TI113X_CC_SPKROUTEN;
				pcicfgw8(pci, TI1131xCC, cc);

				// PCI interrupts only
				pcicfgw8(pci, TI1131xDC, 
						pcicfgr8(pci, TI1131xDC) & ~6);

				// CSC ints to PCI bus.
				wrreg(cb, Rigc, rdreg(cb, Rigc) | 0x10);
			}
			else if (pci->did == TI_1250_did) {
				print("No support yet for the TI_1250_did, prod pb\n");
			}
		}

//		if (intl != -1 && intl != pci->intl)
//			intrenable(pci->intl, interrupt, cb, pci->tbdf, "cardbus");
//		intl = pci->intl;

		// Set up PCI bus numbers if needed.
		if (pcicfgr8(pci, PciSBN) == 0) {
			static int busbase = 0x20;

			pcicfgw8(pci, PciSBN, busbase);
			pcicfgw8(pci, PciUBN, busbase + 2);
			busbase += 3;
		}

		// Patch up intl if needed.
		if ((pin = pcicfgr8(pci, PciINTP)) != 0 && 
		    (pci->intl == 0xff || pci->intl == 0)) {
			pci->intl = pciipin(nil, pin);
			pcicfgw8(pci, PciINTL, pci->intl);

			if (pci->intl == 0xff || pci->intl == 0)
				print("#Y%d: No interrupt?\n", (int)(cb - cbslots));
		}
	
		if ((baddr = pcicfgr32(cb->cb_pci, PciBAR0)) == 0) {
			int align = (pci->did == Ricoh_478_did)? 0x10000: 0x1000;

			baddr = upamalloc(baddr, align, align);
			pcicfgw32(cb->cb_pci, PciBAR0, baddr);
			cb->cb_regs = (ulong *)KADDR(baddr);
		}
		else
			cb->cb_regs = (ulong *)KADDR(upamalloc(baddr, 4096, 0));
		cb->cb_state = SlotEmpty;

		/* Don't really know what to do with this... */
		i82365probe(cb, LegacyAddr, LegacyAddr + 1);

		print("#Y%ld: %s, %.8ulX intl %d\n", cb - cbslots, 
			 variant[i].r_name, baddr, pci->intl);
	}

	if (nslots == 0)
		return;

	for (i = 0; i != nslots; i++) {
		cb_t *cb = &cbslots[i];

		if ((cb->cb_regs[SocketState] & SE_CCD) == 0)
			engine(cb, CardDetected);
	}

	delay(500);			/* Allow time for power up */

	for (i = 0; i != nslots; i++) {
		cb_t *cb = &cbslots[i];

		if (cb->cb_regs[SocketState] & SE_POWER)
			engine(cb, CardPowered);

		/* Enable interrupt on all events */
//		cb->cb_regs[SocketMask] |= 0xF;	
//		wrreg(cb, Rcscic, 0xC);
	}
}

static int
powerup(cb_t *cb)
{
	ulong state;
	ushort bcr;

	if ((state = cb->cb_regs[SocketState]) & SS_PC16) {
	
		// print("#Y%ld: Probed a PC16 card, powering up card\n", cb - cbslots);
		cb->cb_type = PC16;
		memset(&cb->cb_linfo, 0, sizeof(pcminfo_t));

		/* power up and unreset, wait's are empirical (???) */
		wrreg(cb, Rpc, Fautopower|Foutena|Fcardena);
		delay(300);
		wrreg(cb, Rigc, 0);
		delay(100);
		wrreg(cb, Rigc, Fnotreset);

		return 1;
	}

	if (cb->cb_regs[SocketState] & SS_CCD)
		return 0;

	if ((state & SS_CBC) == 0 || (state & SS_NOTCARD)) {
		print("#Y%ld: No cardbus card inserted\n", cb - cbslots);
		return 0;
	}

	if (state & SS_BADVCC) {
		print("#Y%ld: Bad VCC request to card, powering down card!\n", 
			 cb - cbslots);
		cb->cb_regs[SocketControl] = 0;
		return 0;
	}

	if ((state & SS_3V) == 0 && (state & SS_5V) == 0) {
		print("#Y%ld: Unsupported voltage, powering down card!\n", 
			cb - cbslots);
		cb->cb_regs[SocketControl] = 0;
		return 0;
	}

	print("#Y%ld: card %spowered at %d volt\n", cb - cbslots, 
		(state & SS_POWER)? "": "not ", 
		(state & SS_3V)? 3: (state & SS_5V)? 5: -1);

	/* Power up the card
	 * and make sure the secondary bus is not in reset.
	 */
	cb->cb_regs[SocketControl] = (state & SS_5V)? SC_5V: SC_3V;
	delay(50);
	bcr = pcicfgr16(cb->cb_pci, PciBCR);
	bcr &= ~0x40;
	pcicfgw16(cb->cb_pci, PciBCR, bcr);
	delay(100);

	cb->cb_type = PC32;

	return 1;
}

static void
powerdown(cb_t *cb)
{
	ushort bcr;

	if (cb->cb_type == PC16) {

		wrreg(cb, Rpc, 0);	/* turn off card power */
		wrreg(cb, Rwe, 0);	/* no windows */

		cb->cb_type = -1;
		return;
	}

	bcr = pcicfgr16(cb->cb_pci, PciBCR);
	bcr |= 0x40;
	pcicfgw16(cb->cb_pci, PciBCR, bcr);
	cb->cb_regs[SocketControl] = 0;
	cb->cb_type = -1;
}

static void
configure(cb_t *cb)
{
	int i;
	Pcidev *pci;

	// print("configuring slot %d (%s)\n", (int)(cb - cbslots), states[cb->cb_state]);
	if (cb->cb_state == SlotConfigured)
		return;
	engine(cb, CardConfigured);

	delay(50);					/* Emperically established */

	if (cb->cb_type == PC16) {
		i82365configure(cb);
		return;
	}

	/* Scan the CardBus for new PCI devices */
	pciscan(pcicfgr8(cb->cb_pci, PciSBN), &cb->cb_pci->bridge);
	pci = cb->cb_pci->bridge;
	while (pci) {
		ulong size, bar;
		int memindex, ioindex;

		/* Treat the found device as an ordinary PCI card.  It seems that the 
		     CIS is not always present in CardBus cards.  XXX, need to support 
		     multifunction cards */
		memindex = ioindex = 0;
		for (i = 0; i != Nbars; i++) {

			if (pci->mem[i].size == 0) continue;
			if (pci->mem[i].bar & 1) {

				// Allocate I/O space
				if (ioindex > 1) {
					print("#Y%ld: WARNING: Can only configure 2 I/O slots\n", cb - cbslots);
					continue;
				}
				bar = ioreserve(-1, pci->mem[i].size, 0, "cardbus");
				pci->mem[i].bar = bar | 1;
				pcicfgw32(pci, PciBAR0 + i * sizeof(ulong), 
					          pci->mem[i].bar);
				pcicfgw16(cb->cb_pci, PciCBIBR0 + ioindex * 8, bar);
				pcicfgw16(cb->cb_pci, PciCBILR0 + ioindex * 8, 
						 bar + pci->mem[i].size - 1);
				//print("ioindex[%d] %.8uX (%d)\n", 
				//	ioindex, bar, pci->mem[i].size);
				ioindex++;
				continue;
			}

			// Allocating memory space
			if (memindex > 1) {
				print("#Y%ld: WARNING: Can only configure 2 memory slots\n", cb - cbslots);
				continue;
			}

			bar = upamalloc(0, pci->mem[i].size, BY2PG);
			pci->mem[i].bar = bar | (pci->mem[i].bar & 0x80);
			pcicfgw32(pci, PciBAR0 + i * sizeof(ulong), pci->mem[i].bar);
			pcicfgw32(cb->cb_pci, PciCBMBR0 + memindex * 8, bar);
			pcicfgw32(cb->cb_pci, PciCBMLR0 + memindex * 8, 
					  bar + pci->mem[i].size - 1);

			if (pci->mem[i].bar & 0x80)
				/* Enable prefetch */
				pcicfgw16(cb->cb_pci, PciBCR, 
						 pcicfgr16(cb->cb_pci, PciBCR) | 
							          (1 << (8 + memindex)));

			//print("memindex[%d] %.8uX (%d)\n", 
			//	  memindex, bar, pci->mem[i].size);
			memindex++;
		}

		if ((size = pcibarsize(pci, PciEBAR0)) > 0) {

			if (memindex > 1)
				print("#Y%ld: WARNING: Too many memory spaces, not mapping ROM space\n",
					cb - cbslots);
			else {
				pci->rom.bar = upamalloc(0, size, BY2PG);
				pci->rom.size = size;

				pcicfgw32(pci, PciEBAR0, pci->rom.bar);
				pcicfgw32(cb->cb_pci, PciCBMBR0 + memindex * 8,
						 pci->rom.bar);
				pcicfgw32(cb->cb_pci, PciCBMLR0 + memindex * 8, 
						 pci->rom.bar + pci->rom.size - 1);
			}
		}

		/* Set the basic PCI registers for the device */
		pcicfgw16(pci, PciPCR, 
				 pcicfgr16(pci, PciPCR) | 
				 	PciPCR_IO|PciPCR_MEM|PciPCR_Master);
		pcicfgw8(pci, PciCLS, 8);
		pcicfgw8(pci, PciLTR, 64);

		if (pcicfgr8(pci, PciINTP)) {
			pci->intl = pcicfgr8(cb->cb_pci, PciINTL);
			pcicfgw8(pci, PciINTL, pci->intl);

			/* Route interrupts to INTA#/B# */
			pcicfgw16(cb->cb_pci, PciBCR, 
					  pcicfgr16(cb->cb_pci, PciBCR) & ~(1 << 7));
		}
			
		pci = pci->list;
	}
}

static void
unconfigure(cb_t *cb)
{
	Pcidev *pci;
	int i, ioindex, memindex;

	if (cb->cb_type == PC16) {
		print("#Y%d: Don't know how to unconfigure a PC16 card\n",
			 (int)(cb - cbslots));

		memset(&cb->cb_linfo, 0, sizeof(pcminfo_t));
		return;
	}

	pci = cb->cb_pci->bridge;
	if (pci == nil) 
		return;		/* Not configured */
	cb->cb_pci->bridge = nil;		

	memindex = ioindex = 0;
	while (pci) {
		Pcidev *_pci;

		for (i = 0; i != Nbars; i++) {
			if (pci->mem[i].size == 0) continue;
			if (pci->mem[i].bar & 1) {
				iofree(pci->mem[i].bar & ~1);
				pcicfgw16(cb->cb_pci, PciCBIBR0 + ioindex * 8, 
						 (ushort)-1);
				pcicfgw16(cb->cb_pci, PciCBILR0 + ioindex * 8, 0);
				ioindex++;
				continue;
			}

			upafree(pci->mem[i].bar & ~0xF, pci->mem[i].size);
			pcicfgw32(cb->cb_pci, PciCBMBR0 + memindex * 8, 
				          (ulong)-1);
			pcicfgw32(cb->cb_pci, PciCBMLR0 + memindex * 8, 0);
			pcicfgw16(cb->cb_pci, PciBCR, 
					 pcicfgr16(cb->cb_pci, PciBCR) & 
							       ~(1 << (8 + memindex)));
			memindex++;
		}

		if (pci->rom.bar && memindex < 2) {
			upafree(pci->rom.bar & ~0xF, pci->rom.size);
			pcicfgw32(cb->cb_pci, PciCBMBR0 + memindex * 8, 
					  (ulong)-1);
			pcicfgw32(cb->cb_pci, PciCBMLR0 + memindex * 8, 0);
			memindex++;
		}

		_pci = pci->list;
		free(_pci);
		pci = _pci;
	}
}

static void
i82365configure(cb_t *cb)
{
	int this;
	Cisdat cis;
	PCMmap *m;
	uchar type, link;

	/*
	 * Read all tuples in attribute space.
	 */
	m = isamap(cb, 0, 0, 1);
	if(m == 0)
		return;

	cis.cisbase = KADDR(m->isa);
	cis.cispos = 0;
	cis.cisskip = 2;
	cis.cislen = m->len;

	/* loop through all the tuples */
	for(;;){
		this = cis.cispos;
		if(readc(&cis, &type) != 1)
			break;
		if(type == 0xFF)
			break;
		if(readc(&cis, &link) != 1)
			break;

		switch(type){
		default:
			break;
		case 0x15:
			tvers1(cb, &cis, type);
			break;
		case 0x1A:
			tcfig(cb, &cis, type);
			break;
		case 0x1B:
			tentry(cb, &cis, type);
			break;
		}

		if(link == 0xFF)
			break;
		cis.cispos = this + (2+link);
	}
	isaunmap(m);
}

/*
 *  look for a card whose version contains 'idstr'
 */
static int
pccard_pcmspecial(char *idstr, ISAConf *isa)
{
	int i, irq;
	PCMconftab *ct, *et;
	pcminfo_t *pi;
	cb_t *cb;
	uchar x, we, *p;

	cb = nil;
	for (i = 0; i != nslots; i++) {
		cb = &cbslots[i];

		qlock(cb);
		if (cb->cb_state == SlotConfigured &&
		    cb->cb_type == PC16 && 
		    strstr(cb->cb_linfo.pi_verstr, idstr)) 
			break;
		qunlock(cb);
	}

	if (i == nslots) {
		// print("#Y: %s not found\n", idstr);
		return -1;
	}

	pi = &cb->cb_linfo;

	/*
 	  *  configure the PCMslot for IO.  We assume very heavily that we can read
 	  *  configuration info from the CIS.  If not, we won't set up correctly.
 	  */
	irq = isa->irq;
	if(irq == 2)
		irq = 9;

	et = &pi->pi_ctab[pi->pi_nctab];
	ct = nil;
	for(i = 0; i < isa->nopt; i++){
		int index;
		char *cp;

		if(strncmp(isa->opt[i], "index=", 6))
			continue;
		index = strtol(&isa->opt[i][6], &cp, 0);
		if(cp == &isa->opt[i][6] || index >= pi->pi_nctab) {
			qunlock(cb);
			print("#Y%d: Cannot find index %d in conf table\n", 
				 (int)(cb - cbslots), index);
			return -1;
		}
		ct = &pi->pi_ctab[index];
	}

	if(ct == nil){
		PCMconftab *t;

		/* assume default is right */
		if(pi->pi_defctab)
			ct = pi->pi_defctab;
		else
			ct = pi->pi_ctab;
	
		/* try for best match */
		if(ct->nio == 0
		|| ct->io[0].start != isa->port || ((1<<irq) & ct->irqs) == 0){
			for(t = pi->pi_ctab; t < et; t++)
				if(t->nio
				&& t->io[0].start == isa->port
				&& ((1<<irq) & t->irqs)){
					ct = t;
					break;
				}
		}
		if(ct->nio == 0 || ((1<<irq) & ct->irqs) == 0){
			for(t = pi->pi_ctab; t < et; t++)
				if(t->nio && ((1<<irq) & t->irqs)){
					ct = t;
					break;
				}
		}
		if(ct->nio == 0){
			for(t = pi->pi_ctab; t < et; t++)
				if(t->nio){
					ct = t;
					break;
				}
		}
	}

	if(ct == et || ct->nio == 0) {
		qunlock(cb);
		print("#Y%d: No configuration?\n", (int)(cb - cbslots));
		return -1;
	}
	if(isa->port == 0 && ct->io[0].start == 0) {
		qunlock(cb);
		print("#Y%d: No part or start address\n", (int)(cb - cbslots));
		return -1;
	}

	/* route interrupts */
	isa->irq = irq;
	wrreg(cb, Rigc, irq | Fnotreset | Fiocard);

	/* set power and enable device */
	x = vcode(ct->vpp1);
	wrreg(cb, Rpc, x|Fautopower|Foutena|Fcardena);

	/* 16-bit data path */
	if(ct->bit16)
		x = Ftiming|Fiocs16|Fwidth16;
	else
		x = Ftiming;
	if(ct->nio == 2 && ct->io[1].start)
		x |= x<<4;
	wrreg(cb, Rio, x);

	/*
	 * enable io port map 0
	 * the 'top' register value includes the last valid address
	 */
	if(isa->port == 0)
		isa->port = ct->io[0].start;
	we = rdreg(cb, Rwe);
	wrreg(cb, Riobtm0lo, isa->port);
	wrreg(cb, Riobtm0hi, isa->port>>8);
	i = isa->port+ct->io[0].len-1;
	wrreg(cb, Riotop0lo, i);
	wrreg(cb, Riotop0hi, i>>8);
	we |= 1<<6;
	if(ct->nio == 2 && ct->io[1].start){
		wrreg(cb, Riobtm1lo, ct->io[1].start);
		wrreg(cb, Riobtm1hi, ct->io[1].start>>8);
		i = ct->io[1].start+ct->io[1].len-1;
		wrreg(cb, Riotop1lo, i);
		wrreg(cb, Riotop1hi, i>>8);
		we |= 1<<7;
	}
	wrreg(cb, Rwe, we);

	/* only touch Rconfig if it is present */
	if(pi->pi_conf_present & (1<<Rconfig)){
		PCMmap *m;

		/*  Reset adapter */
		m = isamap(cb, pi->pi_conf_addr + Rconfig, 1, 1);
		p = KADDR(m->isa + pi->pi_conf_addr + Rconfig - m->ca);

		/* set configuration and interrupt type */
		x = ct->index;
		if((ct->irqtype & 0x20) && ((ct->irqtype & 0x40)==0 || isa->irq>7))
			x |= Clevel;
		*p = x;
		delay(5);

		isaunmap(m);
	}

	pi->pi_port = isa->port;
	pi->pi_irq = isa->irq;
	qunlock(cb);

	print("#Y%d: %s irq %ld, port %lX\n", (int)(cb - cbslots), pi->pi_verstr, isa->irq, isa->port);
	return (int)(cb - cbslots);
}

static void
pccard_pcmspecialclose(int slotno)
{
	cb_t *cb = &cbslots[slotno];

	wrreg(cb, Rwe, 0);	/* no windows */
}

static int
xcistuple(int slotno, int tuple, int subtuple, void *v, int nv, int attr)
{
	PCMmap *m;
	Cisdat cis;
	int i, l;
	uchar *p;
	uchar type, link, n, c;
	int this, subtype;
	cb_t *cb = &cbslots[slotno];

	m = isamap(cb, 0, 0, attr);
	if(m == 0)
		return -1;

	cis.cisbase = KADDR(m->isa);
	cis.cispos = 0;
	cis.cisskip = attr ? 2 : 1;
	cis.cislen = m->len;

	/* loop through all the tuples */
	for(i = 0; i < 1000; i++){
		this = cis.cispos;
		if(readc(&cis, &type) != 1)
			break;
		if(type == 0xFF)
			break;
		if(readc(&cis, &link) != 1)
			break;
		if(link == 0xFF)
			break;

		n = link;
		if (link > 1 && subtuple != -1) {
			if (readc(&cis, &c) != 1)
				break;
			subtype = c;
			n--;
		} else
			subtype = -1;

		if(type == tuple && subtype == subtuple) {
			p = v;
			for(l=0; l<nv && l<n; l++)
				if(readc(&cis, p++) != 1)
					break;
			isaunmap(m);
			return nv;
		}
		cis.cispos = this + (2+link);
	}
	isaunmap(m);
	return -1;
}

// static Chan*
// pccardattach(char *spec)
// {
// 	if (!managerstarted) {
// 		managerstarted = 1;
// 		kproc("cardbus", processevents, nil);
// 	}
// 	return devattach('Y', spec);
// }
//
//enum
//{
//	Qdir,
//	Qctl,
//
//	Nents = 1,
//};
//
//#define SLOTNO(c)	((ulong)((c->qid.path>>8)&0xff))
//#define TYPE(c)	((ulong)(c->qid.path&0xff))
//#define QID(s,t)	(((s)<<8)|(t))
//
//static int
//pccardgen(Chan *c, char*, Dirtab *, int , int i, Dir *dp)
//{
//	int slotno;
//	Qid qid;
//	long len;
//	int entry;
//
//	if(i == DEVDOTDOT){
//		mkqid(&qid, Qdir, 0, QTDIR);
//		devdir(c, qid, "#Y", 0, eve, 0555, dp);
//		return 1;
//	}
//
//	len = 0;
//	if(i >= Nents * nslots) return -1;
//	slotno = i / Nents;
//	entry = i % Nents;
//	if (entry == 0) {
//		qid.path = QID(slotno, Qctl);
//		snprint(up->genbuf, sizeof up->genbuf, "cb%dctl", slotno);
//	}
//	else {
//		/* Entries for memory regions.  I'll implement them when 
//		     needed. (pb) */
//	}
//	qid.vers = 0;	
//	qid.type = QTFILE;
//	devdir(c, qid, up->genbuf, len, eve, 0660, dp);
//	return 1;
//}
//
//static Walkqid*
//pccardwalk(Chan *c, Chan *nc, char **name, int nname)
//{
//	return devwalk(c, nc, name, nname, 0, 0, pccardgen);
//}
//
//static int
//pccardstat(Chan *c, uchar *db, int n)
//{
//	return devstat(c, db, n, 0, 0, pccardgen);
//}
//
//static void
//increfp(cb_t *cb)
//{
//	qlock(&cb->cb_refslock);
//	cb->cb_refs++;
//	qunlock(&cb->cb_refslock);
//}
//
//static void
//decrefp(cb_t *cb)
//{
//	qlock(&cb->cb_refslock);
//	cb->cb_refs--;
//	qunlock(&cb->cb_refslock);
//}
//
//static Chan*
//pccardopen(Chan *c, int omode)
//{
//	if (c->qid.type & QTDIR){
//		if(omode != OREAD)
//			error(Eperm);
//	} else
//		increfp(&cbslots[SLOTNO(c)]);
//	c->mode = openmode(omode);
//	c->flag |= COPEN;
//	c->offset = 0;
//	return c;
//}
//
//static void
//pccardclose(Chan *c)
//{
//	if(c->flag & COPEN)
//		if((c->qid.type & QTDIR) == 0)
//			decrefp(&cbslots[SLOTNO(c)]);
//}
//
//static long
//pccardread(Chan *c, void *a, long n, vlong offset)
//{
//	cb_t *cb;
//	char *buf, *p, *e;
//
//	switch(TYPE(c)){
//	case Qdir:
//		return devdirread(c, a, n, 0, 0, pccardgen);
//
//	case Qctl:
//		buf = p = malloc(READSTR);
//		buf[0] = 0;
//		e = p + READSTR;
//	
//		cb = &cbslots[SLOTNO(c)];
//		qlock(cb);
//		p = seprint(p, e, "slot %ld: %s; ", cb - cbslots, states[cb->cb_state]);
//
//		switch (cb->cb_type) {
//		case -1:
//			seprint(p, e, "\n");
//			break;
//
//		case PC32:
//			if (cb->cb_pci->bridge) {
//				Pcidev *pci = cb->cb_pci->bridge;
//				int i;
//
//				while (pci) {
//					p = seprint(p, e, "%.4uX %.4uX; irq %d\n", 
//							  pci->vid, pci->did, pci->intl);
//					for (i = 0; i != Nbars; i++)
//						if (pci->mem[i].size)
//							p = seprint(p, e, 
//									  "\tmem[%d] %.8uX (%.8uX)\n",
//									  i, pci->mem[i].bar, 
//									  pci->mem[i].size);
//					if (pci->rom.size)
//						p = seprint(p, e, "\tROM %.8uX (%.8uX)\n", i, 
//								  pci->rom.bar, pci->rom.size);
//					pci = pci->list;
//				}
//			}
//			break;
//
//		case PC16:
//			if (cb->cb_state == SlotConfigured) {
//				pcminfo_t *pi = &cb->cb_linfo;
//
//				p = seprint(p, e, "%s port %X; irq %d;\n",
//						  pi->pi_verstr, pi->pi_port,
//						  pi->pi_irq);
//				for (n = 0; n != pi->pi_nctab; n++) {
//					PCMconftab *ct;
//					int i;
//
//					ct = &pi->pi_ctab[n];
//					p = seprint(p, e, 
//						"\tconfiguration[%d] irqs %.4X; vpp %d, %d; %s\n",
//							  n, ct->irqs, ct->vpp1, ct->vpp2,
//							  (ct == pi->pi_defctab)? "(default);": "");
//					for (i = 0; i != ct->nio; i++)
//						if (ct->io[i].len > 0)
//							p = seprint(p, e, "\t\tio[%d] %.8lX %d\n",
//									  i, ct->io[i].start, ct->io[i].len);
//				}
//			}
//			break;
//		}
//		qunlock(cb);
//			
//		n = readstr(offset, a, n, buf);
//		free(buf);
//		return n;
//	}
//	return 0;
//}
//
//static long
//pccardwrite(Chan *c, void *v, long n, vlong)
//{
//	Rune r;
//	ulong n0;
//	int i, nf;
//	char buf[255], *field[Ncmd], *device;
//	cb_t *cb;
//
//	n0 = n;
//	switch(TYPE(c)){
//	case Qctl:
//		cb = &cbslots[SLOTNO(c)];
//		if(n > sizeof(buf)-1) n = sizeof(buf)-1;
//		memmove(buf, v, n);
//		buf[n] = '\0';
//
//		nf = getfields(buf, field, Ncmd, 1, " \t\n");
//		for (i = 0; i != nf; i++) {
//			if (!strcmp(field[i], "down")) {
//
//				if (i + 1 < nf && *field[i + 1] == '#') {
//					device = field[++i];
//					device += chartorune(&r, device);
//					if ((n = devno(r, 1)) >= 0 && devtab[n]->config)
//						devtab[n]->config(0, device, nil);
//				}
//				qengine(cb, CardEjected);
//			}
//			else if (!strcmp(field[i], "power")) {
//				if ((cb->cb_regs[SocketState] & SS_CCD) == 0)
//					qengine(cb, CardDetected);
//			}
//			else
//				error(Ebadarg);
//		}
//		break;
//	}
//	return n0 - n;
//}
//
//Dev pccarddevtab = {
//	'Y',
//	"cardbus",
//
//	devreset,
//	devinit,
//	pccardattach,
//	pccardwalk,
//	pccardstat,
//	pccardopen,
//	devcreate,
//	pccardclose,
//	pccardread,
//	devbread,
//	pccardwrite,
//	devbwrite,
//	devremove,
//	devwstat,
//};

static PCMmap *
isamap(cb_t *cb, ulong offset, int len, int attr)
{
	uchar we, bit;
	PCMmap *m, *nm;
	pcminfo_t *pi;
	int i;
	ulong e;

	pi = &cb->cb_linfo;

	/* convert offset to granularity */
	if(len <= 0)
		len = 1;
	e = ROUND(offset+len, Mgran);
	offset &= Mmask;
	len = e - offset;

	/* look for a map that covers the right area */
	we = rdreg(cb, Rwe);
	bit = 1;
	nm = 0;
	for(m = pi->pi_mmap; m < &pi->pi_mmap[nelem(pi->pi_mmap)]; m++){
		if((we & bit))
		if(m->attr == attr)
		if(offset >= m->ca && e <= m->cea){

			m->ref++;
			return m;
		}
		bit <<= 1;
		if(nm == 0 && m->ref == 0)
			nm = m;
	}
	m = nm;
	if(m == 0)
		return 0;

	/* if isa space isn't big enough, free it and get more */
	if(m->len < len){
		if(m->isa){
			umbfree(m->isa, m->len);
			m->len = 0;
		}
		m->isa = PADDR(umbmalloc(0, len, Mgran));
		if(m->isa == 0){
			print("isamap: out of isa space\n");
			return 0;
		}
		m->len = len;
	}

	/* set up new map */
	m->ca = offset;
	m->cea = m->ca + m->len;
	m->attr = attr;
	i = m - pi->pi_mmap;
	bit = 1<<i;
	wrreg(cb, Rwe, we & ~bit);		/* disable map before changing it */
	wrreg(cb, MAP(i, Mbtmlo), m->isa>>Mshift);
	wrreg(cb, MAP(i, Mbtmhi), (m->isa>>(Mshift+8)) | F16bit);
	wrreg(cb, MAP(i, Mtoplo), (m->isa+m->len-1)>>Mshift);
	wrreg(cb, MAP(i, Mtophi), ((m->isa+m->len-1)>>(Mshift+8)));
	offset -= m->isa;
	offset &= (1<<25)-1;
	offset >>= Mshift;
	wrreg(cb, MAP(i, Mofflo), offset);
	wrreg(cb, MAP(i, Moffhi), (offset>>8) | (attr ? Fregactive : 0));
	wrreg(cb, Rwe, we | bit);		/* enable map */
	m->ref = 1;

	return m;
}

static void
isaunmap(PCMmap* m)
{
	m->ref--;
}

/*
 *  reading and writing card registers
 */
static uchar
rdreg(cb_t *cb, int index)
{
	outb(cb->cb_lindex, cb->cb_lbase + index);
	return inb(cb->cb_ldata);
}

static void
wrreg(cb_t *cb, int index, uchar val)
{
	outb(cb->cb_lindex, cb->cb_lbase + index);
	outb(cb->cb_ldata, val);
}

static int
readc(Cisdat *cis, uchar *x)
{
	if(cis->cispos >= cis->cislen)
		return 0;
	*x = cis->cisbase[cis->cisskip*cis->cispos];
	cis->cispos++;
	return 1;
}

static ulong
getlong(Cisdat *cis, int size)
{
	uchar c;
	int i;
	ulong x;

	x = 0;
	for(i = 0; i < size; i++){
		if(readc(cis, &c) != 1)
			break;
		x |= c<<(i*8);
	}
	return x;
}

static void
tcfig(cb_t *cb, Cisdat *cis, int )
{
	uchar size, rasize, rmsize;
	uchar last;
	pcminfo_t *pi;

	if(readc(cis, &size) != 1)
		return;
	rasize = (size&0x3) + 1;
	rmsize = ((size>>2)&0xf) + 1;
	if(readc(cis, &last) != 1)
		return;

	pi = &cb->cb_linfo;
	pi->pi_conf_addr = getlong(cis, rasize);
	pi->pi_conf_present = getlong(cis, rmsize);
}

static void
tvers1(cb_t *cb, Cisdat *cis, int )
{
	uchar c, major, minor, last;
	int  i;
	pcminfo_t *pi;

	pi = &cb->cb_linfo;
	if(readc(cis, &major) != 1)
		return;
	if(readc(cis, &minor) != 1)
		return;
	last = 0;
	for(i = 0; i < sizeof(pi->pi_verstr) - 1; i++){
		if(readc(cis, &c) != 1)
			return;
		if(c == 0)
			c = ';';
		if(c == '\n')
			c = ';';
		if(c == 0xff)
			break;
		if(c == ';' && last == ';')
			continue;
		pi->pi_verstr[i] = c;
		last = c;
	}
	pi->pi_verstr[i] = 0;
}

static ulong
microvolt(Cisdat *cis)
{
	uchar c;
	ulong microvolts;
	ulong exp;

	if(readc(cis, &c) != 1)
		return 0;
	exp = exponent[c&0x7];
	microvolts = vmant[(c>>3)&0xf]*exp;
	while(c & 0x80){
		if(readc(cis, &c) != 1)
			return 0;
		switch(c){
		case 0x7d:
			break;		/* high impedence when sleeping */
		case 0x7e:
		case 0x7f:
			microvolts = 0;	/* no connection */
			break;
		default:
			exp /= 10;
			microvolts += exp*(c&0x7f);
		}
	}
	return microvolts;
}

static ulong
nanoamps(Cisdat *cis)
{
	uchar c;
	ulong nanoamps;

	if(readc(cis, &c) != 1)
		return 0;
	nanoamps = exponent[c&0x7]*vmant[(c>>3)&0xf];
	while(c & 0x80){
		if(readc(cis, &c) != 1)
			return 0;
		if(c == 0x7d || c == 0x7e || c == 0x7f)
			nanoamps = 0;
	}
	return nanoamps;
}

/*
 * only nominal voltage (feature 1) is important for config,
 * other features must read card to stay in sync.
 */
static ulong
power(Cisdat *cis)
{
	uchar feature;
	ulong mv;

	mv = 0;
	if(readc(cis, &feature) != 1)
		return 0;
	if(feature & 1)
		mv = microvolt(cis);
	if(feature & 2)
		microvolt(cis);
	if(feature & 4)
		microvolt(cis);
	if(feature & 8)
		nanoamps(cis);
	if(feature & 0x10)
		nanoamps(cis);
	if(feature & 0x20)
		nanoamps(cis);
	if(feature & 0x40)
		nanoamps(cis);
	return mv/1000000;
}

static ulong
ttiming(Cisdat *cis, int scale)
{
	uchar unscaled;
	ulong nanosecs;

	if(readc(cis, &unscaled) != 1)
		return 0;
	nanosecs = (mantissa[(unscaled>>3)&0xf]*exponent[unscaled&7])/10;
	nanosecs = nanosecs * exponent[scale];
	return nanosecs;
}

static void
timing(Cisdat *cis, PCMconftab *ct)
{
	uchar c, i;

	if(readc(cis, &c) != 1)
		return;
	i = c&0x3;
	if(i != 3)
		ct->maxwait = ttiming(cis, i);		/* max wait */
	i = (c>>2)&0x7;
	if(i != 7)
		ct->readywait = ttiming(cis, i);		/* max ready/busy wait */
	i = (c>>5)&0x7;
	if(i != 7)
		ct->otherwait = ttiming(cis, i);		/* reserved wait */
}

static void
iospaces(Cisdat *cis, PCMconftab *ct)
{
	uchar c;
	int i, nio;

	ct->nio = 0;
	if(readc(cis, &c) != 1)
		return;

	ct->bit16 = ((c>>5)&3) >= 2;
	if(!(c & 0x80)){
		ct->io[0].start = 0;
		ct->io[0].len = 1<<(c&0x1f);
		ct->nio = 1;
		return;
	}

	if(readc(cis, &c) != 1)
		return;

	/*
	 * For each of the range descriptions read the
	 * start address and the length (value is length-1).
	 */
	nio = (c&0xf)+1;
	for(i = 0; i < nio; i++){
		ct->io[i].start = getlong(cis, (c>>4)&0x3);
		ct->io[i].len = getlong(cis, (c>>6)&0x3)+1;
	}
	ct->nio = nio;
}

static void
irq(Cisdat *cis, PCMconftab *ct)
{
	uchar c;

	if(readc(cis, &c) != 1)
		return;
	ct->irqtype = c & 0xe0;
	if(c & 0x10)
		ct->irqs = getlong(cis, 2);
	else
		ct->irqs = 1<<(c&0xf);
	ct->irqs &= 0xDEB8;		/* levels available to card */
}

static void
memspace(Cisdat *cis, int asize, int lsize, int host)
{
	ulong haddress, address, len;

	len = getlong(cis, lsize)*256;
	address = getlong(cis, asize)*256;
	USED(len, address);
	if(host){
		haddress = getlong(cis, asize)*256;
		USED(haddress);
	}
}

static void
tentry(cb_t *cb, Cisdat *cis, int )
{
	uchar c, i, feature;
	PCMconftab *ct;
	pcminfo_t *pi;

	pi = &cb->cb_linfo;
	if(pi->pi_nctab >= nelem(pi->pi_ctab))
		return;
	if(readc(cis, &c) != 1)
		return;
	ct = &pi->pi_ctab[pi->pi_nctab++];

	/* copy from last default config */
	if(pi->pi_defctab)
		*ct = *pi->pi_defctab;

	ct->index = c & 0x3f;

	/* is this the new default? */
	if(c & 0x40)
		pi->pi_defctab = ct;

	/* memory wait specified? */
	if(c & 0x80){
		if(readc(cis, &i) != 1)
			return;
		if(i&0x80)
			ct->memwait = 1;
	}

	if(readc(cis, &feature) != 1)
		return;
	switch(feature&0x3){
	case 1:
		ct->vpp1 = ct->vpp2 = power(cis);
		break;
	case 2:
		power(cis);
		ct->vpp1 = ct->vpp2 = power(cis);
		break;
	case 3:
		power(cis);
		ct->vpp1 = power(cis);
		ct->vpp2 = power(cis);
		break;
	default:
		break;
	}
	if(feature&0x4)
		timing(cis, ct);
	if(feature&0x8)
		iospaces(cis, ct);
	if(feature&0x10)
		irq(cis, ct);
	switch((feature>>5)&0x3){
	case 1:
		memspace(cis, 0, 2, 0);
		break;
	case 2:
		memspace(cis, 2, 2, 0);
		break;
	case 3:
		if(readc(cis, &c) != 1)
			return;
		for(i = 0; i <= (c&0x7); i++)
			memspace(cis, (c>>5)&0x3, (c>>3)&0x3, c&0x80);
		break;
	}
}

static void
i82365probe(cb_t *cb, int lindex, int ldata)
{
	uchar c, id;
	int dev = 0;	/* According to the Ricoh spec 00->3F _and_ 80->BF seem
				     to be the same socket A (ditto for B). */

	outb(lindex, Rid + (dev<<7));
	id = inb(ldata);
	if((id & 0xf0) != 0x80)
		return;		/* not a memory & I/O card */
	if((id & 0x0f) == 0x00)
		return;		/* no revision number, not possible */

	cb->cb_lindex = lindex;
	cb->cb_ldata = ldata;
	cb->cb_ltype = Ti82365;
	cb->cb_lbase = (int)(cb - cbslots) * 0x40;

	switch(id){
	case 0x82:
	case 0x83:
	case 0x84:
		/* could be a cirrus */
		outb(cb->cb_lindex, Rchipinfo + (dev<<7));
		outb(cb->cb_ldata, 0);
		c = inb(cb->cb_ldata);
		if((c & 0xc0) != 0xc0)
			break;
		c = inb(cb->cb_ldata);
		if((c & 0xc0) != 0x00)
			break;
		if(c & 0x20){
			cb->cb_ltype = Tpd6720;
		} else {
			cb->cb_ltype = Tpd6710;
		}
		break;
	}

	/* if it's not a Cirrus, it could be a Vadem... */
	if(cb->cb_ltype == Ti82365){
		/* unlock the Vadem extended regs */
		outb(cb->cb_lindex, 0x0E + (dev<<7));
		outb(cb->cb_lindex, 0x37 + (dev<<7));

		/* make the id register show the Vadem id */
		outb(cb->cb_lindex, 0x3A + (dev<<7));
		c = inb(cb->cb_ldata);
		outb(cb->cb_ldata, c|0xC0);
		outb(cb->cb_lindex, Rid + (dev<<7));
		c = inb(cb->cb_ldata);
		if(c & 0x08)
			cb->cb_ltype = Tvg46x;

		/* go back to Intel compatible id */
		outb(cb->cb_lindex, 0x3A + (dev<<7));
		c = inb(cb->cb_ldata);
		outb(cb->cb_ldata, c & ~0xC0);
	}
}

static int
vcode(int volt)
{
	switch(volt){
	case 5:
		return 1;
	case 12:
		return 2;
	default:
		return 0;
	}
}

