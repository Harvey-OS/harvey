/*
 * PCI support code.
 * Needs a massive rewrite.
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

#define DBG	if(0) pcilog

typedef struct Pci Pci;

struct
{
	char	output[PCICONSSIZE];
	int	ptr;
}PCICONS;

int
pcilog(char *fmt, ...)
{
	int n;
	va_list arg;
	char buf[PRINTSIZE];

	va_start(arg, fmt);
	n = vseprint(buf, buf+sizeof(buf), fmt, arg) - buf;
	va_end(arg);

	memmove(PCICONS.output+PCICONS.ptr, buf, n);
	PCICONS.ptr += n;
	return n;
}

enum
{
	MaxFNO		= 7,
	MaxUBN		= 255,
};

enum
{					/* command register */
	IOen		= (1<<0),
	MEMen		= (1<<1),
	MASen		= (1<<2),
	MemWrInv	= (1<<4),
	PErrEn		= (1<<6),
	SErrEn		= (1<<8),
};

typedef struct {
	ulong	cap;
	ulong	ctl;
} Capctl;
typedef struct {
	Capctl	dev;
	Capctl	link;
	Capctl	slot;
} Devlinkslot;

/* capability list id 0x10 is pci-e */
struct Pci {
	/* pci-compatible config */
	/* what io.h calls type 0 & type 1 pre-defined header */
	ulong	id;
	ulong	cs;
	ulong	revclass;
	ulong	misc;	/* cache line size, latency timer, header type, bist */
	ulong	bar[2];		/* always 0 on tegra 2 */

	/* types 1 & 2 pre-defined header */
	ulong	bus;
	ulong	ioaddrs;
	ulong	memaddrs;
	ulong	prefmem;
	ulong	prefbasehi;
	ulong	preflimhi;
	/* type 2 pre-defined header only */
	ulong	ioaddrhi;
	ulong	cfgcapoff;	/* offset in cfg. space to cap. list (0x40) */
	ulong	rom;
	ulong	intr;		/* PciINT[LP] */
	/* subsystem capability regs */
	ulong	subsysid;
	ulong	subsyscap;
	/* */

	Capctl	pwrmgmt;

	/* msi */
	ulong	msictlcap;
	ulong	msimsgaddr[2];	/* little-endian */
	ulong	msimsgdata;

	/* pci-e cap. */
	uchar	_pad0[0x80-0x60];
	ulong	pciecap;
	Devlinkslot port0;
	ulong	rootctl;
	ulong	rootsts;
	Devlinkslot port1;

	/* 0xbc */
	
};

enum {
	/* offsets from soc.pci */
	Port0		= 0,
	Port1		= 0x1000,
	Pads		= 0x3000,
	Afi		= 0x3800,
	Aficfg		= Afi + 0xac,
	Cfgspace	= 0x4000,
	Ecfgspace	= 0x104000,

	/* cs bits */
	Iospace		= 1<<0,
	Memspace	= 1<<1,
	Busmaster	= 1<<2,

	/* Aficfg bits */
	Fpcion		= 1<<0,
};

struct Pcictlr {
	union {
		uchar	_padpci[0x1000];
		Pci;
	} ports[2];
	uchar	_padpads[0x1000];
	uchar	pads[0x800];
	uchar	afi[0x800];
	ulong	cfg[0x1000];
	ulong	extcfg[0x1000];
};

static Lock pcicfglock;
static Lock pcicfginitlock;
static int pcicfgmode = -1;
static int pcimaxbno = 1;  /* was 7; only 2 pci buses; touching 3rd hangs */
static int pcimaxdno;
static Pcidev* pciroot;
static Pcidev* pcilist;
static Pcidev* pcitail;

static int pcicfgrw8(int, int, int, int);
static int pcicfgrw16(int, int, int, int);
static int pcicfgrw32(int, int, int, int);

static char* bustypes[] = {
	"CBUSI",
	"CBUSII",
	"EISA",
	"FUTURE",
	"INTERN",
	"ISA",
	"MBI",
	"MBII",
	"MCA",
	"MPI",
	"MPSA",
	"NUBUS",
	"PCI",
	"PCMCIA",
	"TC",
	"VL",
	"VME",
	"XPRESS",
};

static int
tbdffmt(Fmt* fmt)
{
	char *p;
	int l, r;
	uint type, tbdf;

	if((p = malloc(READSTR)) == nil)
		return fmtstrcpy(fmt, "(tbdfconv)");

	switch(fmt->r){
	case 'T':
		tbdf = va_arg(fmt->args, int);
		if(tbdf == BUSUNKNOWN)
			snprint(p, READSTR, "unknown");
		else{
			type = BUSTYPE(tbdf);
			if(type < nelem(bustypes))
				l = snprint(p, READSTR, bustypes[type]);
			else
				l = snprint(p, READSTR, "%d", type);
			snprint(p+l, READSTR-l, ".%d.%d.%d",
				BUSBNO(tbdf), BUSDNO(tbdf), BUSFNO(tbdf));
		}
		break;

	default:
		snprint(p, READSTR, "(tbdfconv)");
		break;
	}
	r = fmtstrcpy(fmt, p);
	free(p);

	return r;
}

ulong
pcibarsize(Pcidev *p, int rno)
{
	ulong v, size;

	v = pcicfgrw32(p->tbdf, rno, 0, 1);
	pcicfgrw32(p->tbdf, rno, 0xFFFFFFF0, 0);
	size = pcicfgrw32(p->tbdf, rno, 0, 1);
	if(v & 1)
		size |= 0xFFFF0000;
	pcicfgrw32(p->tbdf, rno, v, 0);

	return -(size & ~0x0F);
}

static int
pcilscan(int bno, Pcidev** list)
{
	Pcidev *p, *head, *tail;
	int dno, fno, i, hdt, l, maxfno, maxubn, rno, sbn, tbdf, ubn;

	maxubn = bno;
	head = nil;
	tail = nil;
	for(dno = 0; dno <= pcimaxdno; dno++){
		maxfno = 0;
		for(fno = 0; fno <= maxfno; fno++){
			/*
			 * For this possible device, form the
			 * bus+device+function triplet needed to address it
			 * and try to read the vendor and device ID.
			 * If successful, allocate a device struct and
			 * start to fill it in with some useful information
			 * from the device's configuration space.
			 */
			tbdf = MKBUS(BusPCI, bno, dno, fno);
			l = pcicfgrw32(tbdf, PciVID, 0, 1);
			if(l == 0xFFFFFFFF || l == 0)
				continue;
			p = malloc(sizeof(*p));
			if(p == nil)
				panic("pcilscan: no memory");
			p->tbdf = tbdf;
			p->vid = l;
			p->did = l>>16;

			if(pcilist != nil)
				pcitail->list = p;
			else
				pcilist = p;
			pcitail = p;

			p->pcr = pcicfgr16(p, PciPCR);
			p->rid = pcicfgr8(p, PciRID);
			p->ccrp = pcicfgr8(p, PciCCRp);
			p->ccru = pcicfgr8(p, PciCCRu);
			p->ccrb = pcicfgr8(p, PciCCRb);
			p->cls = pcicfgr8(p, PciCLS);
			p->ltr = pcicfgr8(p, PciLTR);

			p->intl = pcicfgr8(p, PciINTL);

			/*
			 * If the device is a multi-function device adjust the
			 * loop count so all possible functions are checked.
			 */
			hdt = pcicfgr8(p, PciHDT);
			if(hdt & 0x80)
				maxfno = MaxFNO;

			/*
			 * If appropriate, read the base address registers
			 * and work out the sizes.
			 */
			switch(p->ccrb) {
			case 0x03:		/* display controller */
				/* fall through */
			case 0x01:		/* mass storage controller */
			case 0x02:		/* network controller */
			case 0x04:		/* multimedia device */
			case 0x07:		/* simple comm. controllers */
			case 0x08:		/* base system peripherals */
			case 0x09:		/* input devices */
			case 0x0A:		/* docking stations */
			case 0x0B:		/* processors */
			case 0x0C:		/* serial bus controllers */
				if((hdt & 0x7F) != 0)
					break;
				rno = PciBAR0 - 4;
				for(i = 0; i < nelem(p->mem); i++) {
					rno += 4;
					p->mem[i].bar = pcicfgr32(p, rno);
					p->mem[i].size = pcibarsize(p, rno);
				}
				break;

			case 0x00:
			case 0x05:		/* memory controller */
			case 0x06:		/* bridge device */
			default:
				break;
			}

			if(head != nil)
				tail->link = p;
			else
				head = p;
			tail = p;
		}
	}

	*list = head;
	for(p = head; p != nil; p = p->link){
		/*
		 * Find PCI-PCI bridges and recursively descend the tree.
		 */
		if(p->ccrb != 0x06 || p->ccru != 0x04)
			continue;

		/*
		 * If the secondary or subordinate bus number is not
		 * initialised try to do what the PCI BIOS should have
		 * done and fill in the numbers as the tree is descended.
		 * On the way down the subordinate bus number is set to
		 * the maximum as it's not known how many buses are behind
		 * this one; the final value is set on the way back up.
		 */
		sbn = pcicfgr8(p, PciSBN);
		ubn = pcicfgr8(p, PciUBN);

		if(sbn == 0 || ubn == 0) {
			sbn = maxubn+1;
			/*
			 * Make sure memory, I/O and master enables are
			 * off, set the primary, secondary and subordinate
			 * bus numbers and clear the secondary status before
			 * attempting to scan the secondary bus.
			 *
			 * Initialisation of the bridge should be done here.
			 */
			pcicfgw32(p, PciPCR, 0xFFFF0000);
			l = (MaxUBN<<16)|(sbn<<8)|bno;
			pcicfgw32(p, PciPBN, l);
			pcicfgw16(p, PciSPSR, 0xFFFF);
			maxubn = pcilscan(sbn, &p->bridge);
			l = (maxubn<<16)|(sbn<<8)|bno;

			pcicfgw32(p, PciPBN, l);
		}
		else {
			if(ubn > maxubn)
				maxubn = ubn;
			pcilscan(sbn, &p->bridge);
		}
	}

	return maxubn;
}

extern void rtl8169interrupt(Ureg*, void* arg);

/* not used yet */
static void
pciintr(Ureg *ureg, void *p)
{
	rtl8169interrupt(ureg, p);		/* HACK */
}

static void
pcicfginit(void)
{
	char *p;
	Pci *pci = (Pci *)soc.pci;
	Pcidev **list;
	int bno, n;

	lock(&pcicfginitlock);
	if(pcicfgmode != -1) {
		unlock(&pcicfginitlock);
		return;
	}

	/*
	 * TrimSlice # pci 0 1
	 * Scanning PCI devices on bus 0 1
	 * BusDevFun  VendorId   DeviceId   Device Class       Sub-Class
	 * _____________________________________________________________
	 * 00.00.00   0x10de     0x0bf0     Bridge device           0x04
	 * 01.00.00   0x10ec     0x8168     Network controller      0x00
	 *
	 * thus pci bus 0 has a bridge with, perhaps, an ide/sata ctlr behind,
	 * and pci bus 1 has the realtek 8169 on it:
	 *
	 * TrimSlice # pci 1 long
	 * Scanning PCI devices on bus 1
	 *
	 * Found PCI device 01.00.00:
	 *   vendor ID =                   0x10ec
	 *   device ID =                   0x8168
	 *   command register =            0x0007
	 *   status register =             0x0010
	 *   revision ID =                 0x03
	 *   class code =                  0x02 (Network controller)
	 *   sub class code =              0x00
	 *   programming interface =       0x00
	 *   cache line =                  0x08
	 *   base address 0 =              0x80400001		config
	 *   base address 1 =              0x00000000		(ext. config)
	 *   base address 2 =              0xa000000c		"downstream"
	 *   base address 3 =              0x00000000		(prefetchable)
	 *   base address 4 =              0xa000400c		not "
	 *   base address 5 =              0x00000000		(unused)
	 */
	n = pci->id >> 16;
	if (((pci->id & MASK(16)) != Vnvidia || (n != 0xbf0 && n != 0xbf1)) &&
	     (pci->id & MASK(16)) != Vrealtek) {
		print("no pci controller at %#p\n", pci);
		unlock(&pcicfginitlock);
		return;
	}
	if (0)
		iprint("pci: %#p: nvidia, rev %#ux class %#6.6lux misc %#8.8lux\n",
			pci, (uchar)pci->revclass, pci->revclass >> 8,
			pci->misc);

	pci->cs &= Iospace;
	pci->cs |= Memspace | Busmaster;
	coherence();

	pcicfgmode = 1;
//	pcimaxdno = 31;
	pcimaxdno = 15;			/* for trimslice */

	fmtinstall('T', tbdffmt);

	if(p = getconf("*pcimaxbno")){
		n = strtoul(p, 0, 0);
		if(n < pcimaxbno)
			pcimaxbno = n;
	}
	if(p = getconf("*pcimaxdno")){
		n = strtoul(p, 0, 0);
		if(n < pcimaxdno)
			pcimaxdno = n;
	}

	list = &pciroot;
	/* was bno = 0; trimslice needs to start at 1 */
	for(bno = 1; bno <= pcimaxbno; bno++) {
		bno = pcilscan(bno, list);
		while(*list)
			list = &(*list)->link;
	}
	unlock(&pcicfginitlock);

	if(getconf("*pcihinv"))
		pcihinv(nil);
}

enum {
	Afiintrcode	= 0xb8,
};

void
pcieintrdone(void)				/* dismiss pci-e intr */
{
	ulong *afi;

	afi = (ulong *)(soc.pci + Afi);
	afi[Afiintrcode/sizeof *afi] = 0;	/* magic */
	coherence();
}

/*
 * whole config space for tbdf should be at (return address - rno).
 */
static void *
tegracfgaddr(int tbdf, int rno)
{
	uintptr addr;

	addr = soc.pci + (rno < 256? Cfgspace: Ecfgspace) + BUSBDF(tbdf) + rno;
//	if (BUSBNO(tbdf) == 1)
//		addr += Port1;
	return (void *)addr;
}

static int
pcicfgrw8(int tbdf, int rno, int data, int read)
{
	int x;
	void *addr;

	if(pcicfgmode == -1)
		pcicfginit();

	x = -1;
	if(BUSDNO(tbdf) > pcimaxdno)
		return x;

	addr = tegracfgaddr(tbdf, rno);

	lock(&pcicfglock);
	if(read)
		x = *(uchar *)addr;
	else
		*(uchar *)addr = data;
	unlock(&pcicfglock);

	return x;
}

int
pcicfgr8(Pcidev* pcidev, int rno)
{
	return pcicfgrw8(pcidev->tbdf, rno, 0, 1);
}

void
pcicfgw8(Pcidev* pcidev, int rno, int data)
{
	pcicfgrw8(pcidev->tbdf, rno, data, 0);
}

static int
pcicfgrw16(int tbdf, int rno, int data, int read)
{
	int x;
	void *addr;

	if(pcicfgmode == -1)
		pcicfginit();

	x = -1;
	if(BUSDNO(tbdf) > pcimaxdno)
		return x;

	addr = tegracfgaddr(tbdf, rno);

	lock(&pcicfglock);
	if(read)
		x = *(ushort *)addr;
	else
		*(ushort *)addr = data;
	unlock(&pcicfglock);

	return x;
}

int
pcicfgr16(Pcidev* pcidev, int rno)
{
	return pcicfgrw16(pcidev->tbdf, rno, 0, 1);
}

void
pcicfgw16(Pcidev* pcidev, int rno, int data)
{
	pcicfgrw16(pcidev->tbdf, rno, data, 0);
}

static int
pcicfgrw32(int tbdf, int rno, int data, int read)
{
	int x;
	vlong v;
	void *addr;

	if(pcicfgmode == -1)
		pcicfginit();

	x = -1;
	if(BUSDNO(tbdf) > pcimaxdno)
		return x;

	addr = tegracfgaddr(tbdf, rno);
	v = probeaddr((uintptr)addr);
	if (v < 0)
		return -1;

	lock(&pcicfglock);
	if(read)
		x = *(ulong *)addr;
	else
		*(ulong *)addr = data;
	unlock(&pcicfglock);

	return x;
}

int
pcicfgr32(Pcidev* pcidev, int rno)
{
	return pcicfgrw32(pcidev->tbdf, rno, 0, 1);
}

void
pcicfgw32(Pcidev* pcidev, int rno, int data)
{
	pcicfgrw32(pcidev->tbdf, rno, data, 0);
}

Pcidev*
pcimatch(Pcidev* prev, int vid, int did)
{
	if(pcicfgmode == -1)
		pcicfginit();

	if(prev == nil)
		prev = pcilist;
	else
		prev = prev->list;

	while(prev != nil){
		if((vid == 0 || prev->vid == vid)
		&& (did == 0 || prev->did == did))
			break;
		prev = prev->list;
	}
	return prev;
}

Pcidev*
pcimatchtbdf(int tbdf)
{
	Pcidev *pcidev;

	if(pcicfgmode == -1)
		pcicfginit();

	for(pcidev = pcilist; pcidev != nil; pcidev = pcidev->list) {
		if(pcidev->tbdf == tbdf)
			break;
	}
	return pcidev;
}

static void
pcilhinv(Pcidev* p)
{
	int i;
	Pcidev *t;

	if(p == nil) {
		putstrn(PCICONS.output, PCICONS.ptr);
		p = pciroot;
		print("bus dev type vid  did intl memory\n");
	}
	for(t = p; t != nil; t = t->link) {
		print("%d  %2d/%d %.2ux %.2ux %.2ux %.4ux %.4ux %3d  ",
			BUSBNO(t->tbdf), BUSDNO(t->tbdf), BUSFNO(t->tbdf),
			t->ccrb, t->ccru, t->ccrp, t->vid, t->did, t->intl);

		for(i = 0; i < nelem(p->mem); i++) {
			if(t->mem[i].size == 0)
				continue;
			print("%d:%.8lux %d ", i,
				t->mem[i].bar, t->mem[i].size);
		}
		if(t->bridge)
			print("->%d", BUSBNO(t->bridge->tbdf));
		print("\n");
	}
	while(p != nil) {
		if(p->bridge != nil)
			pcilhinv(p->bridge);
		p = p->link;
	}
}

void
pcihinv(Pcidev* p)
{
	if(pcicfgmode == -1)
		pcicfginit();
	lock(&pcicfginitlock);
	pcilhinv(p);
	unlock(&pcicfginitlock);
}

void
pcireset(void)
{
	Pcidev *p;

	if(pcicfgmode == -1)
		pcicfginit();

	for(p = pcilist; p != nil; p = p->list) {
		/* don't mess with the bridges */
		if(p->ccrb == 0x06)
			continue;
		pciclrbme(p);
	}
}

void
pcisetioe(Pcidev* p)
{
	p->pcr |= IOen;
	pcicfgw16(p, PciPCR, p->pcr);
}

void
pciclrioe(Pcidev* p)
{
	p->pcr &= ~IOen;
	pcicfgw16(p, PciPCR, p->pcr);
}

void
pcisetbme(Pcidev* p)
{
	p->pcr |= MASen;
	pcicfgw16(p, PciPCR, p->pcr);
}

void
pciclrbme(Pcidev* p)
{
	p->pcr &= ~MASen;
	pcicfgw16(p, PciPCR, p->pcr);
}

void
pcisetmwi(Pcidev* p)
{
	p->pcr |= MemWrInv;
	pcicfgw16(p, PciPCR, p->pcr);
}

void
pciclrmwi(Pcidev* p)
{
	p->pcr &= ~MemWrInv;
	pcicfgw16(p, PciPCR, p->pcr);
}

static int
pcigetpmrb(Pcidev* p)
{
	int ptr;

	if(p->pmrb != 0)
		return p->pmrb;
	p->pmrb = -1;

	/*
	 * If there are no extended capabilities implemented,
	 * (bit 4 in the status register) assume there's no standard
	 * power management method.
	 * Find the capabilities pointer based on PCI header type.
	 */
	if(!(pcicfgr16(p, PciPSR) & 0x0010))
		return -1;
	switch(pcicfgr8(p, PciHDT)){
	default:
		return -1;
	case 0:					/* all other */
	case 1:					/* PCI to PCI bridge */
		ptr = 0x34;
		break;
	case 2:					/* CardBus bridge */
		ptr = 0x14;
		break;
	}
	ptr = pcicfgr32(p, ptr);

	while(ptr != 0){
		/*
		 * Check for validity.
		 * Can't be in standard header and must be double
		 * word aligned.
		 */
		if(ptr < 0x40 || (ptr & ~0xFC))
			return -1;
		if(pcicfgr8(p, ptr) == 0x01){
			p->pmrb = ptr;
			return ptr;
		}

		ptr = pcicfgr8(p, ptr+1);
	}

	return -1;
}

int
pcigetpms(Pcidev* p)
{
	int pmcsr, ptr;

	if((ptr = pcigetpmrb(p)) == -1)
		return -1;

	/*
	 * Power Management Register Block:
	 *  offset 0:	Capability ID
	 *	   1:	next item pointer
	 *	   2:	capabilities
	 *	   4:	control/status
	 *	   6:	bridge support extensions
	 *	   7:	data
	 */
	pmcsr = pcicfgr16(p, ptr+4);

	return pmcsr & 0x0003;
}

int
pcisetpms(Pcidev* p, int state)
{
	int ostate, pmc, pmcsr, ptr;

	if((ptr = pcigetpmrb(p)) == -1)
		return -1;

	pmc = pcicfgr16(p, ptr+2);
	pmcsr = pcicfgr16(p, ptr+4);
	ostate = pmcsr & 0x0003;
	pmcsr &= ~0x0003;

	switch(state){
	default:
		return -1;
	case 0:
		break;
	case 1:
		if(!(pmc & 0x0200))
			return -1;
		break;
	case 2:
		if(!(pmc & 0x0400))
			return -1;
		break;
	case 3:
		break;
	}
	pmcsr |= state;
	pcicfgw16(p, ptr+4, pmcsr);

	return ostate;
}
