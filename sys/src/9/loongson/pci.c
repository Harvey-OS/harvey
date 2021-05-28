/*
 *	PCI support code.
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

typedef struct Pci Pci;

struct Pci {
	ulong	id;
	ulong	cs;
	ulong	revclass;
	ulong	misc;	/* cache line size, latency timer, header type, bist */
	ulong	base[6];	/* base addr regs */
	ulong	unused[5];
	ulong	intr;
	ulong	mask[6];
	ulong	trans[6];
};

enum {
	/* cs bits */
	CIoEn		= (1<<0),
	CMemEn		= (1<<1),
	CMasEn		= (1<<2),
	CSpcEn		= (1<<4),
	CParEn		= (1<<6),
	CSErrEn		= (1<<8),

	SMasTgtAb	= (1<<24),	/* master target abort */
	SMasAb		= (1<<25),	/* master abort */
};

enum {
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

static Lock pcicfglock;
static Lock pcicfginitlock;
static int pcicfgmode = -1;
static int pcimaxbno = 0;
static int pcimaxdno;
static Pcidev *pciroot;
static Pcidev *pcilist;
static Pcidev *pcitail;
static Pci *pci = (Pci*)PCICFG;

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
	/* dno from 5 due to its address mode */
	for(dno = 5; dno <= pcimaxdno; dno++){
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
			p->intp = pcicfgr8(p, PciINTP);

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

static void
pcicfginit(void)
{
	char *p;
	int n, bno;
	Pcidev **list;

	lock(&pcicfginitlock);
	if(pcicfgmode != -1) {
		unlock(&pcicfginitlock);
		return;
	}

	pcicfgmode = 1;
	pcimaxdno = 19;

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
	for(bno = 0; bno <= pcimaxbno; bno++){
		bno = pcilscan(bno, list);
		while(*list)
			list = &(*list)->link;
	}
	unlock(&pcicfginitlock);
}

/* map the devince's cfg space and calculate the address */
static void*
pcidevcfgaddr(int tbdf, int rno)
{
	ulong addr;
	ulong b, d, f, type;

	b = BUSBNO(tbdf);
	d = BUSDNO(tbdf);
	f = BUSFNO(tbdf);
    if(b == 0) {
        /* Type 0 configuration on onboard PCI bus */
        addr = (1<<(d+11))|(f<<8)|rno;
        type = 0x00000;
    } else {
        /* Type 1 configuration on offboard PCI bus */
        addr = (b<<16)|(d<<11)|(f<<8)|rno;
        type = 0x10000;
    }

    /* clear aborts */
    pci->cs |= SMasAb | SMasTgtAb;

	/* config map cfg reg to map the device's cfg space */
    *Pcimapcfg = (addr>>16)|type;

	return (void*)(PCIDEVCFG+(addr&0xffff));
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

	lock(&pcicfglock);
	addr = pcidevcfgaddr(tbdf, rno);
	if(read)
		x = *(uchar*)addr;
	else
		*(uchar*)addr = data;
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

	lock(&pcicfglock);
	addr = pcidevcfgaddr(tbdf, rno);
	if(read)
		x = *(ushort*)addr;
	else
		*(ushort*)addr = data;
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
	void *addr;

	if(pcicfgmode == -1)
		pcicfginit();

	x = -1;
	if(BUSDNO(tbdf) > pcimaxdno)
		return x;

	lock(&pcicfglock);
	addr = pcidevcfgaddr(tbdf, rno);
	if(read)
		x = *(ulong*)addr;
	else
		*(ulong*)addr = data;
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

int
pcisubirq(int tbdf)
{
	return Pciintrbase + pcicfgrw8(tbdf, PciINTP, 0, 1);
}
