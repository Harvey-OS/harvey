/* 
     cardbus and pcmcia (grmph) support.
*/
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"
#include "io.h"

#define MAP(x,o)	(Rmap + (x)*0x8 + o)

enum {
	TI_vid = 0x104c,
	TI_1131_did = 0xAC15,
	TI_1250_did = 0xAC16,
	TI_1450_did = 0xAC1B,
	TI_1251A_did = 0xAC1D,
	TI_1420_did = 0xAC51,

	Ricoh_vid = 0x1180,
	Ricoh_475_did = 0x0475,
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

typedef struct Variant Variant;
struct Variant {
	ushort	vid;
	ushort	did;
	char		*name;
};

static Variant variant[] = {
{	Ricoh_vid,	Ricoh_475_did,	"Ricoh 475 PCI/Cardbus bridge",	},
{	Ricoh_vid,	Ricoh_476_did,	"Ricoh 476 PCI/Cardbus bridge",	},
{	Ricoh_vid,	Ricoh_478_did,	"Ricoh 478 PCI/Cardbus bridge",	},
{	TI_vid,		TI_1131_did,	"TI PCI-1131 Cardbus Controller", },
{	TI_vid,		TI_1250_did,	"TI PCI-1250 Cardbus Controller", },
{	TI_vid,		TI_1450_did,	"TI PCI-1450 Cardbus Controller", },
{	TI_vid,		TI_1251A_did,	"TI PCI-1251A Cardbus Controller", },
{	TI_vid,		TI_1420_did,	"TI PCI-1420 Cardbus Controller", },
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

	PciPMC = 0xa4,

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
typedef struct Cisdat Cisdat;
struct Cisdat {
	uchar		*cisbase;
	int			cispos;
	int			cisskip;
	int			cislen;
};

typedef struct Pcminfo Pcminfo;
struct Pcminfo {
	char			verstr[512];		/* Version string */
	PCMmap		mmap[4];		/* maps, last is always for the kernel */
	ulong		conf_addr;		/* Config address */
	uchar		conf_present;	/* Config register present */
	int			nctab;			/* In use configuration tables */
	PCMconftab	ctab[8];		/* Configuration tables */
	PCMconftab	*defctab;		/* Default conftab */

	int			port;			/* Actual port usage */
	int			irq;			/* Actual IRQ usage */
};

typedef struct Cardbus Cardbus;
struct Cardbus {
	Lock;
	Variant		*variant;		/* Which CardBus chipset */
	Pcidev		*pci;			/* The bridge itself */
	ulong		*regs;			/* Cardbus registers */
	int			ltype;			/* Legacy type */
	int			lindex;		/* Legacy port index address */
	int			ldata;			/* Legacy port data address */
	int			lbase;			/* Base register for this socket */

	int			state;			/* Current state of card */
	int			type;			/* Type of card */
	Pcminfo		linfo;			/* PCMCIA slot info */

	int			special;		/* card is allocated to a driver */

	int			refs;			/* Number of refs to slot */
	Lock		refslock;		/* inc/dev ref lock */
};

static int managerstarted;

enum {
	Mshift=	12,
	Mgran=	(1<<Mshift),	/* granularity of maps */
	Mmask=	~(Mgran-1),	/* mask for address bits important to the chip */
};

static Cardbus cbslots[Nslots];
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

enum
{
	CMdown,
	CMpower,
};

static Cmdtab pccardctlmsg[] =
{
	CMdown,		"down",	2,
	CMpower,	"power",	1,
};

static void cbint(Ureg *, void *);
static int powerup(Cardbus *);
static void configure(Cardbus *);
static void managecard(Cardbus *);
static void cardmanager(void *);
static void eject(Cardbus *);
static void interrupt(Ureg *, void *);
static void powerdown(Cardbus *cb);
static void unconfigure(Cardbus *cb);

static void i82365probe(Cardbus *cb, int lindex, int ldata);
static void i82365configure(Cardbus *cb);
static PCMmap *isamap(Cardbus *cb, ulong offset, int len, int attr);
static void isaunmap(PCMmap* m);
static uchar rdreg(Cardbus *cb, int index);
static void wrreg(Cardbus *cb, int index, uchar val);
static int readc(Cisdat *cis, uchar *x);
static void tvers1(Cardbus *cb, Cisdat *cis, int );
static void tcfig(Cardbus *cb, Cisdat *cis, int );
static void tentry(Cardbus *cb, Cisdat *cis, int );
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
engine(Cardbus *cb, int message)
{
	//print("engine(%d): %s(%s)\n", 
	//	 (int)(cb - cbslots), states[cb->state], messages[message]);
	switch (cb->state) {
	case SlotEmpty:

		switch (message) {
		case CardDetected:
			cb->state = SlotFull;
			powerup(cb);
			break;
		case CardEjected:
			break;
		default:
			//print("#Y%d: Invalid message %s in SlotEmpty state\n",
			//	(int)(cb - cbslots), messages[message]);
			break;
		}
		break;

	case SlotFull:

		switch (message) {
		case CardPowered:
			cb->state = SlotPowered;
			configure(cb);
			break;
		case CardEjected:
			cb->state = SlotEmpty;
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
			cb->state = SlotConfigured;
			break;
		case CardEjected:
			cb->state = SlotEmpty;
			unconfigure(cb);
			powerdown(cb);
			break;
		default:
			//print("#Y%d: Invalid message %s in SlotPowered state\n",
			//	(int)(cb - cbslots), messages[message]);
			break;
		}
		break;

	case SlotConfigured:

		switch (message) {
		case CardEjected:
			cb->state = SlotEmpty;
			unconfigure(cb);
			powerdown(cb);
			break;
		default:
			//print("#Y%d: Invalid message %s in SlotConfigured state\n",
			//	(int)(cb - cbslots), messages[message]);
			break;
		}
		break;
	}
}

static void
qengine(Cardbus *cb, int message)
{
	lock(cb);
	engine(cb, message);
	unlock(cb);
}

typedef struct Events Events;
struct Events {
	Cardbus	*cb;
	int	message;
};

static Lock levents;
static Events events[NUMEVENTS];
static Rendez revents;
static int nevents;

static void
iengine(Cardbus *cb, int message)
{
	if (nevents >= NUMEVENTS) {
		print("#Y: Too many events queued, discarding request\n");
		return;
	}
	ilock(&levents);
	events[nevents].cb = cb;
	events[nevents].message = message;
	nevents++;
	iunlock(&levents);
	wakeup(&revents);
}

static int
eventoccured(void)
{
	return nevents > 0;
}

static void
processevents(void *)
{
	while (1) {
		int message;
		Cardbus *cb;

		sleep(&revents, (int (*)(void *))eventoccured, nil);

		cb = nil;
		message = 0;
		ilock(&levents);
		if (nevents > 0) {
			cb = events[0].cb;
			message = events[0].message;
			nevents--;
			if (nevents > 0)
				memmove(events, &events[1], nevents * sizeof(Events));
		}
		iunlock(&levents);

		if (cb)
			qengine(cb, message);
	}
}

static void
cbinterrupt(Ureg *, void *)
{
	int i;

	for (i = 0; i != nslots; i++) {
		Cardbus *cb = &cbslots[i];
		ulong event, state;

		event= cb->regs[SocketEvent];
		state = cb->regs[SocketState];
		rdreg(cb, Rcsc);	/* Ack the interrupt */

		//print("interrupt: slot %d, event %.8lX, state %.8lX, (%s)\n", 
		//	(int)(cb - cbslots), event, state, states[cb->state]);

		if (event & SE_CCD) {
			cb->regs[SocketEvent] |= SE_CCD;	/* Ack interrupt */
			if (state & SE_CCD) {
				if (cb->state != SlotEmpty) {
					print("#Y: take cardejected interrupt\n");
					iengine(cb, CardEjected);
				}
			}
			else
				iengine(cb, CardDetected);
		}

		if (event & SE_POWER) {
			cb->regs[SocketEvent] |= SE_POWER;	/* Ack interrupt */
			iengine(cb, CardPowered);
		}
	}
}

void
devpccardlink(void)
{
	static int initialized;
	Pcidev *pci;
	int i;
	uchar intl;
	char *p;

	if (initialized) 
		return;
	initialized = 1;

	if((p=getconf("pccard0")) && strncmp(p, "disabled", 8)==0)
		return;

	if(_pcmspecial)
		return;

	/* Allocate legacy space */
	if (ioalloc(LegacyAddr, 2, 0, "i82365.0") < 0)
		print("#Y: WARNING: Cannot allocate legacy ports\n");

	/* Find all CardBus controllers */
	pci = nil;
	intl = (uchar)-1;
	while ((pci = pcimatch(pci, 0, 0)) != nil) {
		ulong baddr;
		Cardbus *cb;
		int slot;
		uchar pin;

		for (i = 0; i != nelem(variant); i++)
			if (pci->vid == variant[i].vid && pci->did == variant[i].did)
				break;
		if (i == nelem(variant))
			continue;

		/* initialize this slot */
		slot = nslots++;
		cb = &cbslots[slot];

		cb->pci = pci;
		cb->variant = &variant[i];
		
		if (pci->vid != TI_vid) {
			// Gross hack, needs a fix.  Inherit the mappings from 9load
			// for the TIs (pb)
			pcicfgw32(pci, PciCBMBR0, 0xffffffff);
			pcicfgw32(pci, PciCBMLR0, 0);
			pcicfgw32(pci, PciCBMBR1, 0xffffffff);
			pcicfgw32(pci, PciCBMLR1, 0);
			pcicfgw32(pci, PciCBIBR0, 0xffffffff);
			pcicfgw32(pci, PciCBILR0, 0);
			pcicfgw32(pci, PciCBIBR1, 0xffffffff);
			pcicfgw32(pci, PciCBILR1, 0);
		}

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
			else if (pci->did == TI_1420_did) {
				// Disable Vcc protection
				pcicfgw32(cb->pci, 0x80, 
					pcicfgr32(cb->pci, 0x80) | (1 << 21));
			}
			
			pcicfgw16(cb->pci, PciPMC, pcicfgr16(cb->pci, PciPMC) & ~3);
		}

		if (intl != -1 && intl != pci->intl)
			intrenable(pci->intl, cbinterrupt, cb, pci->tbdf, "cardbus");
		intl = pci->intl;

		if ((baddr = pcicfgr32(cb->pci, PciBAR0)) == 0) {
			int align = (pci->did == Ricoh_478_did)? 0x10000: 0x1000;

			baddr = upamalloc(baddr, align, align);
			pcicfgw32(cb->pci, PciBAR0, baddr);
			cb->regs = (ulong *)KADDR(baddr);
		}
		else
			cb->regs = (ulong *)KADDR(upamalloc(baddr, 4096, 0));
		cb->state = SlotEmpty;

		/* Don't really know what to do with this... */
		i82365probe(cb, LegacyAddr, LegacyAddr + 1);

		print("#Y%ld: %s, %.8ulX intl %d\n", cb - cbslots, 
			 variant[i].name, baddr, pci->intl);
	}

	if (nslots == 0){
		iofree(LegacyAddr);
		return;
	}

	_pcmspecial = pccard_pcmspecial;
	_pcmspecialclose = pccard_pcmspecialclose;

	for (i = 0; i != nslots; i++) {
		Cardbus *cb = &cbslots[i];

		if ((cb->regs[SocketState] & SE_CCD) == 0)
			engine(cb, CardDetected);
	}

	delay(500);			/* Allow time for power up */

	for (i = 0; i != nslots; i++) {
		Cardbus *cb = &cbslots[i];

		if (cb->regs[SocketState] & SE_POWER)
			engine(cb, CardPowered);

		/* Ack and enable interrupts on all events */
		// cb->regs[SocketEvent] = cb->regs[SocketEvent];
		cb->regs[SocketMask] |= 0xF;	
		wrreg(cb, Rcscic, 0xC);
	}
}

static int
powerup(Cardbus *cb)
{
	ulong state;
	ushort bcr;

	state = cb->regs[SocketState];
	if (state & SS_PC16) {
	
		// print("#Y%ld: Probed a PC16 card, powering up card\n", cb - cbslots);
		cb->type = PC16;
		memset(&cb->linfo, 0, sizeof(Pcminfo));

		/* power up and unreset, wait's are empirical (???) */
		wrreg(cb, Rpc, Fautopower|Foutena|Fcardena);
		delay(300);
		wrreg(cb, Rigc, 0);
		delay(100);
		wrreg(cb, Rigc, Fnotreset);
		delay(500);

		return 1;
	}

	if (state & SS_CCD)
		return 0;

	if (state & SS_NOTCARD) {
		print("#Y%ld: Not a card inserted\n", cb - cbslots);
		return 0;
	}

	if ((state & SS_3V) == 0 && (state & SS_5V) == 0) {
		print("#Y%ld: Unsupported voltage, powering down card!\n", 
			cb - cbslots);
		cb->regs[SocketControl] = 0;
		return 0;
	}

	//print("#Y%ld: card %spowered at %d volt\n", cb - cbslots, 
	//	(state & SS_POWER)? "": "not ", 
	//	(state & SS_3V)? 3: (state & SS_5V)? 5: -1);

	/* Power up the card
	 * and make sure the secondary bus is not in reset.
	 */
	cb->regs[SocketControl] = (state & SS_5V)? SC_5V: SC_3V;
	delay(50);
	bcr = pcicfgr16(cb->pci, PciBCR);
	bcr &= ~0x40;
	pcicfgw16(cb->pci, PciBCR, bcr);
	delay(100);

	cb->type = (state & SS_PC16)? PC16: PC32;
	return 1;
}

static void
powerdown(Cardbus *cb)
{
	ushort bcr;

	if (cb->type == PC16) {

		wrreg(cb, Rpc, 0);	/* turn off card power */
		wrreg(cb, Rwe, 0);	/* no windows */

		cb->type = -1;
		return;
	}

	bcr = pcicfgr16(cb->pci, PciBCR);
	bcr |= 0x40;
	pcicfgw16(cb->pci, PciBCR, bcr);
	cb->regs[SocketControl] = 0;
	cb->type = -1;
}

static void
configure(Cardbus *cb)
{
	int i;
	Pcidev *pci;

	//print("configuring slot %d (%s)\n", (int)(cb - cbslots), states[cb->state]);
	if (cb->state == SlotConfigured)
		return;
	engine(cb, CardConfigured);

	delay(50);					/* Emperically established */

	if (cb->type == PC16) {
		i82365configure(cb);
		return;
	}

	/* Scan the CardBus for new PCI devices */
	pciscan(pcicfgr8(cb->pci, PciSBN), &cb->pci->bridge);
	pci = cb->pci->bridge;
	while (pci) {
		ulong size, bar;
		int memindex, ioindex;

		pcicfgw16(pci, PciPCR, 
				pcicfgr16(pci, PciPCR) & ~(PciPCR_IO|PciPCR_MEM));

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
				pcicfgw16(cb->pci, PciCBIBR0 + ioindex * 8, bar);
				pcicfgw16(cb->pci, PciCBILR0 + ioindex * 8, 
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
			pcicfgw32(cb->pci, PciCBMBR0 + memindex * 8, bar);
			pcicfgw32(cb->pci, PciCBMLR0 + memindex * 8, 
					  bar + pci->mem[i].size - 1);

			if (pci->mem[i].bar & 0x80)
				/* Enable prefetch */
				pcicfgw16(cb->pci, PciBCR, 
						 pcicfgr16(cb->pci, PciBCR) | 
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
				pcicfgw32(cb->pci, PciCBMBR0 + memindex * 8,
						 pci->rom.bar);
				pcicfgw32(cb->pci, PciCBMLR0 + memindex * 8, 
						 pci->rom.bar + pci->rom.size - 1);
			}
		}

		/* Set the basic PCI registers for the device */
		pci->pcr = pcicfgr16(pci, PciPCR) | PciPCR_IO|PciPCR_MEM|PciPCR_Master;
		pci->cls = 8;
		pci->ltr = 64;
		pcicfgw16(pci, PciPCR, pci->pcr);
		pcicfgw8(pci, PciCLS, pci->cls);
		pcicfgw8(pci, PciLTR, pci->ltr);

		if (pcicfgr8(pci, PciINTP)) {
			pci->intl = pcicfgr8(cb->pci, PciINTL);
			pcicfgw8(pci, PciINTL, pci->intl);

			/* Route interrupts to INTA#/B# */
			pcicfgw16(cb->pci, PciBCR, 
					  pcicfgr16(cb->pci, PciBCR) & ~(1 << 7));
		}
			
		pci = pci->list;
	}
}

static void
unconfigure(Cardbus *cb)
{
	Pcidev *pci;
	int i, ioindex, memindex;

	if (cb->type == PC16) {
		print("#Y%d: Don't know how to unconfigure a PC16 card\n",
			 (int)(cb - cbslots));

		memset(&cb->linfo, 0, sizeof(Pcminfo));
		return;
	}

	pci = cb->pci->bridge;
	if (pci == nil) 
		return;		/* Not configured */
	cb->pci->bridge = nil;		

	memindex = ioindex = 0;
	while (pci) {
		Pcidev *_pci;

		for (i = 0; i != Nbars; i++) {
			if (pci->mem[i].size == 0) continue;
			if (pci->mem[i].bar & 1) {
				iofree(pci->mem[i].bar & ~1);
				pcicfgw16(cb->pci, PciCBIBR0 + ioindex * 8, 
						 (ushort)-1);
				pcicfgw16(cb->pci, PciCBILR0 + ioindex * 8, 0);
				ioindex++;
				continue;
			}

			upafree(pci->mem[i].bar & ~0xF, pci->mem[i].size);
			pcicfgw32(cb->pci, PciCBMBR0 + memindex * 8, 
				          (ulong)-1);
			pcicfgw32(cb->pci, PciCBMLR0 + memindex * 8, 0);
			pcicfgw16(cb->pci, PciBCR, 
					 pcicfgr16(cb->pci, PciBCR) & 
							       ~(1 << (8 + memindex)));
			memindex++;
		}

		if (pci->rom.bar && memindex < 2) {
			upafree(pci->rom.bar & ~0xF, pci->rom.size);
			pcicfgw32(cb->pci, PciCBMBR0 + memindex * 8, 
					  (ulong)-1);
			pcicfgw32(cb->pci, PciCBMLR0 + memindex * 8, 0);
			memindex++;
		}

		_pci = pci->list;
		free(_pci);
		pci = _pci;
	}
}

static void
i82365configure(Cardbus *cb)
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
	Pcminfo *pi;
	Cardbus *cb;
	uchar x, we, *p;

	cb = nil;
	for (i = 0; i != nslots; i++) {
		cb = &cbslots[i];

		lock(cb);
		if (cb->state == SlotConfigured &&
		    cb->type == PC16 && 
		    !cb->special &&
		    strstr(cb->linfo.verstr, idstr)) 
			break;
		unlock(cb);
	}

	if (i == nslots) {
		// print("#Y: %s not found\n", idstr);
		return -1;
	}

	pi = &cb->linfo;

	/*
 	  *  configure the PCMslot for IO.  We assume very heavily that we can read
 	  *  configuration info from the CIS.  If not, we won't set up correctly.
 	  */
	irq = isa->irq;
	if(irq == 2)
		irq = 9;

	et = &pi->ctab[pi->nctab];
	ct = nil;
	for(i = 0; i < isa->nopt; i++){
		int index;
		char *cp;

		if(strncmp(isa->opt[i], "index=", 6))
			continue;
		index = strtol(&isa->opt[i][6], &cp, 0);
		if(cp == &isa->opt[i][6] || index >= pi->nctab) {
			unlock(cb);
			print("#Y%d: Cannot find index %d in conf table\n", 
				 (int)(cb - cbslots), index);
			return -1;
		}
		ct = &pi->ctab[index];
	}

	if(ct == nil){
		PCMconftab *t;

		/* assume default is right */
		if(pi->defctab)
			ct = pi->defctab;
		else
			ct = pi->ctab;
	
		/* try for best match */
		if(ct->nio == 0
		|| ct->io[0].start != isa->port || ((1<<irq) & ct->irqs) == 0){
			for(t = pi->ctab; t < et; t++)
				if(t->nio
				&& t->io[0].start == isa->port
				&& ((1<<irq) & t->irqs)){
					ct = t;
					break;
				}
		}
		if(ct->nio == 0 || ((1<<irq) & ct->irqs) == 0){
			for(t = pi->ctab; t < et; t++)
				if(t->nio && ((1<<irq) & t->irqs)){
					ct = t;
					break;
				}
		}
		if(ct->nio == 0){
			for(t = pi->ctab; t < et; t++)
				if(t->nio){
					ct = t;
					break;
				}
		}
	}

	if(ct == et || ct->nio == 0) {
		unlock(cb);
		print("#Y%d: No configuration?\n", (int)(cb - cbslots));
		return -1;
	}
	if(isa->port == 0 && ct->io[0].start == 0) {
		unlock(cb);
		print("#Y%d: No part or start address\n", (int)(cb - cbslots));
		return -1;
	}

	cb->special = 1;	/* taken */

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
	if(pi->conf_present & (1<<Rconfig)){
		PCMmap *m;

		/*  Reset adapter */
		m = isamap(cb, pi->conf_addr + Rconfig, 1, 1);
		p = KADDR(m->isa + pi->conf_addr + Rconfig - m->ca);

		/* set configuration and interrupt type */
		x = ct->index;
		if(ct->irqtype & 0x20)
			x |= Clevel;
		*p = x;
		delay(5);

		isaunmap(m);
	}

	pi->port = isa->port;
	pi->irq = isa->irq;
	unlock(cb);

	print("#Y%d: %s irq %d, port %lX\n", (int)(cb - cbslots), pi->verstr, isa->irq, isa->port);
	return (int)(cb - cbslots);
}

static void
pccard_pcmspecialclose(int slotno)
{
	Cardbus *cb = &cbslots[slotno];

	wrreg(cb, Rwe, 0);	/* no windows */
	cb->special = 0;
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
	Cardbus *cb = &cbslots[slotno];

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

static Chan*
pccardattach(char *spec)
{
	if (!managerstarted) {
		managerstarted = 1;
		kproc("cardbus", processevents, nil);
	}
	return devattach('Y', spec);
}

enum
{
	Qdir,
	Qctl,

	Nents = 1,
};

#define SLOTNO(c)	((ulong)((c->qid.path>>8)&0xff))
#define TYPE(c)	((ulong)(c->qid.path&0xff))
#define QID(s,t)	(((s)<<8)|(t))

static int
pccardgen(Chan *c, char*, Dirtab *, int , int i, Dir *dp)
{
	int slotno;
	Qid qid;
	long len;
	int entry;

	if(i == DEVDOTDOT){
		mkqid(&qid, Qdir, 0, QTDIR);
		devdir(c, qid, "#Y", 0, eve, 0555, dp);
		return 1;
	}

	len = 0;
	if(i >= Nents * nslots) return -1;
	slotno = i / Nents;
	entry = i % Nents;
	if (entry == 0) {
		qid.path = QID(slotno, Qctl);
		snprint(up->genbuf, sizeof up->genbuf, "cb%dctl", slotno);
	}
	else {
		/* Entries for memory regions.  I'll implement them when 
		     needed. (pb) */
	}
	qid.vers = 0;	
	qid.type = QTFILE;
	devdir(c, qid, up->genbuf, len, eve, 0660, dp);
	return 1;
}

static Walkqid*
pccardwalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, 0, 0, pccardgen);
}

static int
pccardstat(Chan *c, uchar *db, int n)
{
	return devstat(c, db, n, 0, 0, pccardgen);
}

static void
increfp(Cardbus *cb)
{
	lock(&cb->refslock);
	cb->refs++;
	unlock(&cb->refslock);
}

static void
decrefp(Cardbus *cb)
{
	lock(&cb->refslock);
	cb->refs--;
	unlock(&cb->refslock);
}

static Chan*
pccardopen(Chan *c, int omode)
{
	if (c->qid.type & QTDIR){
		if(omode != OREAD)
			error(Eperm);
	} else
		increfp(&cbslots[SLOTNO(c)]);
	c->mode = openmode(omode);
	c->flag |= COPEN;
	c->offset = 0;
	return c;
}

static void
pccardclose(Chan *c)
{
	if(c->flag & COPEN)
		if((c->qid.type & QTDIR) == 0)
			decrefp(&cbslots[SLOTNO(c)]);
}

static long
pccardread(Chan *c, void *a, long n, vlong offset)
{
	Cardbus *cb;
	char *buf, *p, *e;
	int i;

	switch(TYPE(c)){
	case Qdir:
		return devdirread(c, a, n, 0, 0, pccardgen);

	case Qctl:
		buf = p = malloc(READSTR);
		buf[0] = 0;
		e = p + READSTR;
	
		cb = &cbslots[SLOTNO(c)];
		lock(cb);
		p = seprint(p, e, "slot %ld: %s; ", cb - cbslots, states[cb->state]);

		switch (cb->type) {
		case -1:
			seprint(p, e, "\n");
			break;

		case PC32:
			if (cb->pci->bridge) {
				Pcidev *pci = cb->pci->bridge;
				int i;

				while (pci) {
					p = seprint(p, e, "%.4uX %.4uX; irq %d\n", 
							  pci->vid, pci->did, pci->intl);
					for (i = 0; i != Nbars; i++)
						if (pci->mem[i].size)
							p = seprint(p, e, 
									  "\tmem[%d] %.8ulX (%.8uX)\n",
									  i, pci->mem[i].bar, 
									  pci->mem[i].size);
					if (pci->rom.size)
						p = seprint(p, e, "\tROM %.8ulX (%.8uX)\n",
								  pci->rom.bar, pci->rom.size);
					pci = pci->list;
				}
			}
			break;

		case PC16:
			if (cb->state == SlotConfigured) {
				Pcminfo *pi = &cb->linfo;

				p = seprint(p, e, "%s port %X; irq %d;\n",
						  pi->verstr, pi->port,
						  pi->irq);
				for (i = 0; i != pi->nctab; i++) {
					PCMconftab *ct;
					int j;

					ct = &pi->ctab[i];
					p = seprint(p, e, 
						"\tconfiguration[%d] irqs %.4uX; vpp %d, %d; %s\n",
							  i, ct->irqs, ct->vpp1, ct->vpp2,
							  (ct == pi->defctab)? "(default);": "");
					for (j = 0; j != ct->nio; j++)
						if (ct->io[j].len > 0)
							p = seprint(p, e, "\t\tio[%d] %.8ulX %uld\n",
									  j, ct->io[j].start, ct->io[j].len);
				}
			}
			break;
		}
		unlock(cb);
			
		n = readstr(offset, a, n, buf);
		free(buf);
		return n;
	}
	return 0;
}

static long
pccardwrite(Chan *c, void *v, long n, vlong)
{
	Rune r;
	ulong n0;
	char *device;
	Cmdbuf *cbf;
	Cmdtab *ct;
	Cardbus *cb;

	n0 = n;
	switch(TYPE(c)){
	case Qctl:
		cb = &cbslots[SLOTNO(c)];

		cbf = parsecmd(v, n);
		if(waserror()){
			free(cbf);
			nexterror();
		}
		ct = lookupcmd(cbf, pccardctlmsg, nelem(pccardctlmsg));
		switch(ct->index){
		case CMdown:
			device = cbf->f[1];
			device += chartorune(&r, device);
			if ((n = devno(r, 1)) >= 0 && devtab[n]->config)
				devtab[n]->config(0, device, nil);
			qengine(cb, CardEjected);
			break;
		case CMpower:
			if ((cb->regs[SocketState] & SS_CCD) == 0)
				qengine(cb, CardDetected);
			break;
		}
		poperror();
		free(cbf);
		break;
	}
	return n0 - n;
}

Dev pccarddevtab = {
	'Y',
	"cardbus",

	devreset,
	devinit,	
	devshutdown,
	pccardattach,
	pccardwalk,
	pccardstat,
	pccardopen,
	devcreate,
	pccardclose,
	pccardread,
	devbread,
	pccardwrite,
	devbwrite,
	devremove,
	devwstat,
};

static PCMmap *
isamap(Cardbus *cb, ulong offset, int len, int attr)
{
	uchar we, bit;
	PCMmap *m, *nm;
	Pcminfo *pi;
	int i;
	ulong e;

	pi = &cb->linfo;

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
	for(m = pi->mmap; m < &pi->mmap[nelem(pi->mmap)]; m++){
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
	i = m - pi->mmap;
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
rdreg(Cardbus *cb, int index)
{
	outb(cb->lindex, cb->lbase + index);
	return inb(cb->ldata);
}

static void
wrreg(Cardbus *cb, int index, uchar val)
{
	outb(cb->lindex, cb->lbase + index);
	outb(cb->ldata, val);
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
tcfig(Cardbus *cb, Cisdat *cis, int )
{
	uchar size, rasize, rmsize;
	uchar last;
	Pcminfo *pi;

	if(readc(cis, &size) != 1)
		return;
	rasize = (size&0x3) + 1;
	rmsize = ((size>>2)&0xf) + 1;
	if(readc(cis, &last) != 1)
		return;

	pi = &cb->linfo;
	pi->conf_addr = getlong(cis, rasize);
	pi->conf_present = getlong(cis, rmsize);
}

static void
tvers1(Cardbus *cb, Cisdat *cis, int )
{
	uchar c, major, minor, last;
	int  i;
	Pcminfo *pi;

	pi = &cb->linfo;
	if(readc(cis, &major) != 1)
		return;
	if(readc(cis, &minor) != 1)
		return;
	last = 0;
	for(i = 0; i < sizeof(pi->verstr) - 1; i++){
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
		pi->verstr[i] = c;
		last = c;
	}
	pi->verstr[i] = 0;
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
tentry(Cardbus *cb, Cisdat *cis, int )
{
	uchar c, i, feature;
	PCMconftab *ct;
	Pcminfo *pi;

	pi = &cb->linfo;
	if(pi->nctab >= nelem(pi->ctab))
		return;
	if(readc(cis, &c) != 1)
		return;
	ct = &pi->ctab[pi->nctab++];

	/* copy from last default config */
	if(pi->defctab)
		*ct = *pi->defctab;

	ct->index = c & 0x3f;

	/* is this the new default? */
	if(c & 0x40)
		pi->defctab = ct;

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
i82365probe(Cardbus *cb, int lindex, int ldata)
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

	cb->lindex = lindex;
	cb->ldata = ldata;
	cb->ltype = Ti82365;
	cb->lbase = (int)(cb - cbslots) * 0x40;

	switch(id){
	case 0x82:
	case 0x83:
	case 0x84:
		/* could be a cirrus */
		outb(cb->lindex, Rchipinfo + (dev<<7));
		outb(cb->ldata, 0);
		c = inb(cb->ldata);
		if((c & 0xc0) != 0xc0)
			break;
		c = inb(cb->ldata);
		if((c & 0xc0) != 0x00)
			break;
		if(c & 0x20){
			cb->ltype = Tpd6720;
		} else {
			cb->ltype = Tpd6710;
		}
		break;
	}

	/* if it's not a Cirrus, it could be a Vadem... */
	if(cb->ltype == Ti82365){
		/* unlock the Vadem extended regs */
		outb(cb->lindex, 0x0E + (dev<<7));
		outb(cb->lindex, 0x37 + (dev<<7));

		/* make the id register show the Vadem id */
		outb(cb->lindex, 0x3A + (dev<<7));
		c = inb(cb->ldata);
		outb(cb->ldata, c|0xC0);
		outb(cb->lindex, Rid + (dev<<7));
		c = inb(cb->ldata);
		if(c & 0x08)
			cb->ltype = Tvg46x;

		/* go back to Intel compatible id */
		outb(cb->lindex, 0x3A + (dev<<7));
		c = inb(cb->ldata);
		outb(cb->ldata, c & ~0xC0);
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

