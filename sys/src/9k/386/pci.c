/*
 * PCI bus support code.  Handles PCI-Express by mostly ignoring extensions
 * to PCI, notably PCI-E extended config space.
 * has more than a few magic numbers.
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include "io.h"
#include "apic.h"

struct
{
	char	output[16384];
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

enum {					/* config space header type bits */
	Hdtmulti	= 1<<7,		/* flag: multi-function device */
};

enum
{					/* configuration mechanism #1 */
	PciADDR		= 0xCF8,	/* CONFIG_ADDRESS I/O port */
	PciDATA		= 0xCFC,	/* CONFIG_DATA I/O port */

	MaxFNO		= 7,
	MaxUBN		= 255,
};

enum
{					/* command register (pcr) */
	IOen		= (1<<0),
	MEMen		= (1<<1),
	MASen		= (1<<2),	/* bus master (dma) enable */
	MemWrInv	= (1<<4),
	PErrEn		= (1<<6),
	SErrEn		= (1<<8),
	IntrDis		= (1<<10),	/* legacy interrupt disable */
};

static Lock pcicfglock;
static Lock pcicfginitlock;
static int pcicfgmode = -1;
static int pcimaxbno = 7;
static int pcimaxdno;
static Pcidev* pciroot;
static Pcidev* pcilist;
static Pcidev* pcitail;
static int nobios, nopcirouting;

static int pcicfgrw32(int, int, int, int);
static int pcicfgrw16(int, int, int, int);
static int pcicfgrw8(int, int, int, int);

static void pcilhinv(Pcidev* p);

/*
 * these bus names are from the Intel MP spec.
 * the others (not listed) are essentially all obsolete.
 */
static char* bustypes[] = {		/* indexed by Buses; see io.h */
	"INTERN", "ISA", "PCI",
};

#pragma	varargck	type	"T"	int

static int
tbdffmt(Fmt* fmt)
{
	char *p;
	int l, r, type, tbdf;

	if((p = malloc(READSTR)) == nil)
		return fmtstrcpy(fmt, "(tbdfconv)");

	switch(fmt->r){
	case 'T':
		tbdf = va_arg(fmt->args, int);
		type = BUSTYPE(tbdf);
		if(type < nelem(bustypes))
			l = snprint(p, READSTR, bustypes[type]);
		else
			l = snprint(p, READSTR, "%d", type);
		snprint(p+l, READSTR-l, ".%d.%d.%d",
			BUSBNO(tbdf), BUSDNO(tbdf), BUSFNO(tbdf));
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
pcisizcmp(void *a, void *b)
{
	Pcisiz *aa, *bb;

	aa = a;
	bb = b;
	return aa->siz - bb->siz;
}

static ulong
pcimask(ulong v)
{
	ulong mask;

	mask = BI2BY*sizeof(v);
	for(mask = 1<<(mask-1); mask != 0; mask >>= 1)
		if(mask & v)
			break;
	mask--;
	if((v & mask) == 0)
		return v;

	v |= mask;
	return v+1;
}

static void
pcibusmap(Pcidev *root, ulong *pmema, ulong *pioa, int wrreg)
{
	Pcidev *p;
	int ntb, i, size, rno, hole;
	ulong v, mema, ioa, sioa, smema, base, limit;
	Pcisiz *table, *tptr, *mtb, *itb;
	extern void qsort(void*, long, long, int (*)(void*, void*));

	if(!nobios)
		return;

	ioa = *pioa;
	mema = *pmema;

	DBG("pcibusmap wr=%d %T mem=%#lux io=%#lux\n",
		wrreg, root->tbdf, mema, ioa);

	ntb = 0;
	for(p = root; p != nil; p = p->link)
		ntb++;

	ntb *= (PciCIS-PciBAR0)/4;
	table = malloc(2*ntb*sizeof(Pcisiz));
	itb = table;
	mtb = table+ntb;

	/*
	 * Build a table of sizes
	 */
	for(p = root; p != nil; p = p->link) {
		if(p->ccrb == Pcibcbridge) {
			if(p->ccru != Pciscbrpci || p->bridge == nil) {
//				DBG("pci: ignored bridge %T\n", p->tbdf);
				continue;
			}

			sioa = ioa;
			smema = mema;
			pcibusmap(p->bridge, &smema, &sioa, 0);

			hole = pcimask(smema-mema);
			if(hole < MB)
				hole = MB;
			p->mema.size = hole;

			hole = pcimask(sioa-ioa);
			if(hole < 4*KB)
				hole = 4*KB;

			p->ioa.size = hole;

			itb->dev = p;
			itb->bar = -1;
			itb->siz = p->ioa.size;
			itb++;

			mtb->dev = p;
			mtb->bar = -1;
			mtb->siz = p->mema.size;
			mtb++;
			continue;
		}

		for(i = 0; i < nelem(p->mem); i++) {
			rno = PciBAR0 + i*4;
			v = pcicfgrw32(p->tbdf, rno, 0, 1);
			size = pcibarsize(p, rno);
			if(size == 0)
				continue;

			if(v & 1) {
				itb->dev = p;
				itb->bar = i;
				itb->siz = size;
				itb++;
			} else {
				mtb->dev = p;
				mtb->bar = i;
				mtb->siz = size;
				mtb++;
			}

			p->mem[i].size = size;
		}
	}

	/*
	 * Sort both tables IO smallest first, Memory largest
	 */
	qsort(table, itb-table, sizeof(Pcisiz), pcisizcmp);
	tptr = table+ntb;
	qsort(tptr, mtb-tptr, sizeof(Pcisiz), pcisizcmp);

	/*
	 * Allocate IO address space on this bus
	 */
	for(tptr = table; tptr < itb; tptr++) {
		hole = tptr->siz;
		if(tptr->bar == -1)
			hole = 1<<12;
		ioa = (ioa+hole-1) & ~(hole-1);

		p = tptr->dev;
		if(tptr->bar == -1)
			p->ioa.bar = ioa;
		else {
			p->pcr |= IOen;
			p->mem[tptr->bar].bar = ioa|1;
			if(wrreg)
				pcicfgrw32(p->tbdf, PciBAR0+(tptr->bar*4), ioa|1, 0);
		}

		ioa += tptr->siz;
	}

	/*
	 * Allocate Memory address space on this bus
	 */
	for(tptr = table+ntb; tptr < mtb; tptr++) {
		hole = tptr->siz;
		if(tptr->bar == -1)
			hole = 1<<20;
		mema = (mema+hole-1) & ~(hole-1);

		p = tptr->dev;
		if(tptr->bar == -1)
			p->mema.bar = mema;
		else {
			p->pcr |= MEMen;
			p->mem[tptr->bar].bar = mema;
			if(wrreg)
				pcicfgrw32(p->tbdf, PciBAR0+(tptr->bar*4), mema, 0);
		}
		mema += tptr->siz;
	}

	*pmema = mema;
	*pioa = ioa;
	free(table);

	if(wrreg == 0)
		return;

	/*
	 * Finally set all the bridge addresses & registers
	 */
	for(p = root; p != nil; p = p->link) {
		if(p->bridge == nil) {
			pcicfgrw8(p->tbdf, PciLTR, 64, 0);

			p->pcr |= MASen;
			pcicfgrw16(p->tbdf, PciPCR, p->pcr, 0);
			continue;
		}

		base = p->ioa.bar;
		limit = base+p->ioa.size-1;
		v = pcicfgrw32(p->tbdf, PciIBR, 0, 1);
		v = (v&0xFFFF0000)|(limit & 0xF000)|((base & 0xF000)>>8);
		pcicfgrw32(p->tbdf, PciIBR, v, 0);
		v = (limit & 0xFFFF0000)|(base>>16);
		pcicfgrw32(p->tbdf, PciIUBR, v, 0);

		base = p->mema.bar;
		limit = base+p->mema.size-1;
		v = (limit & 0xFFF00000)|((base & 0xFFF00000)>>16);
		pcicfgrw32(p->tbdf, PciMBR, v, 0);

		/*
		 * Disable memory prefetch
		 */
		pcicfgrw32(p->tbdf, PciPMBR, 0x0000FFFF, 0);
		pcicfgrw8(p->tbdf, PciLTR, 64, 0);

		/*
		 * Enable the bridge
		 */
		p->pcr |= IOen|MEMen|MASen;
		pcicfgrw32(p->tbdf, PciPCR, 0xFFFF0000|p->pcr , 0);

		sioa = p->ioa.bar;
		smema = p->mema.bar;
		pcibusmap(p->bridge, &smema, &sioa, 1);
	}
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
			if(0 && p->intl == 0)
				print("pciscan: warning: %T has irq 0\n", tbdf);

			/*
			 * If the device is a multi-function device adjust the
			 * loop count so all possible functions are checked.
			 */
			hdt = pcicfgr8(p, PciHDT);
			if(hdt & Hdtmulti)
				maxfno = MaxFNO;

			/*
			 * If appropriate, read the base address registers
			 * and work out the sizes.
			 */
			switch(p->ccrb) {
			case 0x01:		/* mass storage controller */
			case 0x02:		/* network controller */
			case 0x03:		/* display controller */
			case 0x04:		/* multimedia device */
			case 0x07:		/* simple comm. controllers */
			case 0x08:		/* base system peripherals */
			case 0x09:		/* input devices */
			case 0x0A:		/* docking stations */
			case 0x0B:		/* processors */
			case 0x0C:		/* serial bus controllers */
				if((hdt & ~Hdtmulti) != 0)
					break;
				rno = PciBAR0 - 4;
				for(i = 0; i < nelem(p->mem); i++) {
					rno += 4;
					p->mem[i].bar = pcicfgr32(p, rno);
					p->mem[i].size = pcibarsize(p, rno);
				}
				pciclrbme(p);
				pcinointrs(p);
				pcimsioff(nil, p);
				break;

			case Pcibcpci1:
			case Pcibcmem:
			case Pcibcbridge:
			case 255: /* unassigned (e.g., vmware comms interface) */
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
		if(p->ccrb != Pcibcbridge || p->ccru != Pciscbrpci)
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

		if(sbn == 0 || ubn == 0 || nobios) {
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
		} else {
			/*
			 * You can't go back.
			 * This shouldn't be possible, but the
			 * Iwill DK8-HTX seems to have subordinate
			 * bus numbers which get smaller on the
			 * way down. Need to look more closely at
			 * this.
			 */
			if(ubn > maxubn)
				maxubn = ubn;
			pcilscan(sbn, &p->bridge);
		}
	}
	return maxubn;
}

int
pciscan(int bno, Pcidev **list)
{
	int ubn;

	lock(&pcicfginitlock);
	ubn = pcilscan(bno, list);
	unlock(&pcicfginitlock);
	return ubn;
}

static uchar
pIIxget(Pcidev *router, uchar link)
{
	uchar pirq;

	/* link should be 0x60, 0x61, 0x62, 0x63 */
	pirq = pcicfgr8(router, link);
	return (pirq < 16)? pirq: 0;
}

static void
pIIxset(Pcidev *router, uchar link, uchar irq)
{
	pcicfgw8(router, link, irq);
}

static uchar
viaget(Pcidev *router, uchar link)
{
	uchar pirq;

	/* link should be 1, 2, 3, 5 */
	pirq = (link < 6)? pcicfgr8(router, 0x55 + (link>>1)): 0;

	return (link & 1)? (pirq >> 4): (pirq & 15);
}

static void
viaset(Pcidev *router, uchar link, uchar irq)
{
	uchar pirq;

	pirq = pcicfgr8(router, 0x55 + (link >> 1));
	pirq &= (link & 1)? 0x0f: 0xf0;
	pirq |= (link & 1)? (irq << 4): (irq & 15);
	pcicfgw8(router, 0x55 + (link>>1), pirq);
}

static uchar
optiget(Pcidev *router, uchar link)
{
	uchar pirq = 0;

	/* link should be 0x02, 0x12, 0x22, 0x32 */
	if ((link & 0xcf) == 0x02)
		pirq = pcicfgr8(router, 0xb8 + (link >> 5));
	return (link & 0x10)? (pirq >> 4): (pirq & 15);
}

static void
optiset(Pcidev *router, uchar link, uchar irq)
{
	uchar pirq;

	pirq = pcicfgr8(router, 0xb8 + (link >> 5));
	pirq &= (link & 0x10)? 0x0f : 0xf0;
	pirq |= (link & 0x10)? (irq << 4): (irq & 15);
	pcicfgw8(router, 0xb8 + (link >> 5), pirq);
}

static uchar
aliget(Pcidev *router, uchar link)
{
	/* No, you're not dreaming */
	static const uchar map[] = { 0, 9, 3, 10, 4, 5, 7, 6, 1, 11, 0, 12, 0, 14, 0, 15 };
	uchar pirq;

	/* link should be 0x01..0x08 */
	pirq = pcicfgr8(router, 0x48 + ((link-1)>>1));
	return (link & 1)? map[pirq&15]: map[pirq>>4];
}

static void
aliset(Pcidev *router, uchar link, uchar irq)
{
	/* Inverse of map in aliget */
	static const uchar map[] = { 0, 8, 0, 2, 4, 5, 7, 6, 0, 1, 3, 9, 11, 0, 13, 15 };
	uchar pirq;

	pirq = pcicfgr8(router, 0x48 + ((link-1)>>1));
	pirq &= (link & 1)? 0x0f: 0xf0;
	pirq |= (link & 1)? (map[irq] << 4): (map[irq] & 15);
	pcicfgw8(router, 0x48 + ((link-1)>>1), pirq);
}

static uchar
cyrixget(Pcidev *router, uchar link)
{
	uchar pirq;

	/* link should be 1, 2, 3, 4 */
	pirq = pcicfgr8(router, 0x5c + ((link-1)>>1));
	return ((link & 1)? pirq >> 4: pirq & 15);
}

static void
cyrixset(Pcidev *router, uchar link, uchar irq)
{
	uchar pirq;

	pirq = pcicfgr8(router, 0x5c + (link>>1));
	pirq &= (link & 1)? 0x0f: 0xf0;
	pirq |= (link & 1)? (irq << 4): (irq & 15);
	pcicfgw8(router, 0x5c + (link>>1), pirq);
}

typedef struct Bridge Bridge;
struct Bridge
{
	ushort	vid;
	ushort	did;
	uchar	(*get)(Pcidev *, uchar);
	void	(*set)(Pcidev *, uchar, uchar);
};

static Bridge southbridges[] = {
	{ 0x8086, 0x122e, pIIxget, pIIxset },	/* Intel 82371FB */
	{ 0x8086, 0x1234, pIIxget, pIIxset },	/* Intel 82371MX */
	{ 0x8086, 0x7000, pIIxget, pIIxset },	/* Intel 82371SB */
	{ 0x8086, 0x7110, pIIxget, pIIxset },	/* Intel 82371AB */
	{ 0x8086, 0x7198, pIIxget, pIIxset },	/* Intel 82443MX (fn 1) */
	{ 0x8086, 0x2410, pIIxget, pIIxset },	/* Intel 82801AA */
	{ 0x8086, 0x2420, pIIxget, pIIxset },	/* Intel 82801AB */
	{ 0x8086, 0x2440, pIIxget, pIIxset },	/* Intel 82801BA */
	{ 0x8086, 0x2448, pIIxget, pIIxset },	/* Intel 82801BAM/CAM/DBM */
	{ 0x8086, 0x244c, pIIxget, pIIxset },	/* Intel 82801BAM */
	{ 0x8086, 0x244e, pIIxget, pIIxset },	/* Intel 82801 */
	{ 0x8086, 0x2480, pIIxget, pIIxset },	/* Intel 82801CA */
	{ 0x8086, 0x248c, pIIxget, pIIxset },	/* Intel 82801CAM */
	{ 0x8086, 0x24c0, pIIxget, pIIxset },	/* Intel 82801DBL */
	{ 0x8086, 0x24cc, pIIxget, pIIxset },	/* Intel 82801DBM */
	{ 0x8086, 0x24d0, pIIxget, pIIxset },	/* Intel 82801EB */
	{ 0x8086, 0x25a1, pIIxget, pIIxset },	/* Intel 6300ESB */
	{ 0x8086, 0x2640, pIIxget, pIIxset },	/* Intel 82801FB */
	{ 0x8086, 0x2641, pIIxget, pIIxset },	/* Intel 82801FBM */
	{ 0x8086, 0x27b8, pIIxget, pIIxset },	/* Intel 82801GB */
	{ 0x8086, 0x27b9, pIIxget, pIIxset },	/* Intel 82801GBM */
	{ 0x8086, 0x27bd, pIIxget, pIIxset },	/* Intel 82801GB/GR */
	{ 0x8086, 0x2916, pIIxget, pIIxset },	/* Intel 82801IR */
	{ 0x8086, 0x3a16, pIIxget, pIIxset },	/* Intel 82801JIR */
	{ 0x8086, 0x3a40, pIIxget, pIIxset },	/* Intel 82801JI */
	{ 0x8086, 0x3a42, pIIxget, pIIxset },	/* Intel 82801JI */
	{ 0x8086, 0x3a48, pIIxget, pIIxset },	/* Intel 82801JI */
	{ 0x8086, 0x1c02, pIIxget, pIIxset },	/* Intel 6 Series/C200 */
	{ 0x8086, 0x1c44, pIIxget, pIIxset },	/* Intel 6 Series/Z68 Express */
	{ 0x8086, 0x1e53, pIIxget, pIIxset },	/* Intel 7 Series/C216 */
	{ 0x8086, 0x8186, pIIxget, pIIxset },	/* Intel Atom E6xx LPC */
	{ 0x8086, 0x8c56, pIIxget, pIIxset },	/* Intel C226 (H81?) LPC */
	{ 0x8086, 0x9cc3, pIIxget, pIIxset },	/* Intel NUC */
	{ 0x8086, 0xa149, pIIxget, pIIxset },	/* Intel C236 Chipset LPC/eSPI */

	{ 0x1106, 0x0586, viaget, viaset },	/* Viatech 82C586 */
	{ 0x1106, 0x0596, viaget, viaset },	/* Viatech 82C596 */
	{ 0x1106, 0x0686, viaget, viaset },	/* Viatech 82C686 */
	{ 0x1106, 0x3227, viaget, viaset },	/* Viatech VT8237 */
	{ 0x1045, 0xc700, optiget, optiset },	/* Opti 82C700 */
	{ 0x10b9, 0x1533, aliget, aliset },	/* Al M1533 */
	{ 0x1039, 0x0008, pIIxget, pIIxset },	/* SI 503 */
	{ 0x1039, 0x0496, pIIxget, pIIxset },	/* SI 496 */
	{ 0x1078, 0x0100, cyrixget, cyrixset },	/* Cyrix 5530 Legacy */

	{ 0x1022, 0x746B, nil, nil },		/* AMD 8111 */
	{ 0x1022, 0x1410, nil, nil },		/* AMD SOC (for coreboot) */
	{ 0x1022, 0x780b, nil, nil },		/* AMD FCH SMBus */

	{ 0x10DE, 0x00D1, nil, nil },		/* NVIDIA nForce 3 */
	{ 0x10DE, 0x00E0, nil, nil },		/* NVIDIA nForce 3 250 Series */
	{ 0x10DE, 0x00E1, nil, nil },		/* NVIDIA nForce 3 250 Series */
	{ 0x1166, 0x0200, nil, nil },		/* ServerWorks ServerSet III LE */
	{ 0x1002, 0x4377, nil, nil },		/* ATI Radeon Xpress 200M */
	{ 0x1002, 0x4372, nil, nil },		/* ATI SB400 */
	{ 0x1002, 0x4384, nil, nil },		/* ATI SB600 (for coreboot) */
};

typedef struct Slot Slot;
struct Slot {
	uchar	bus;			// Pci bus number
	uchar	dev;			// Pci device number
	uchar	maps[12];		// Avoid structs!  Link and mask.
	uchar	slot;			// Add-in/built-in slot
	uchar	reserved;
};

typedef struct Router Router;
struct Router {
	uchar	signature[4];		// Routing table signature
	uchar	version[2];		// Version number
	uchar	size[2];		// Total table size
	uchar	bus;			// Interrupt router bus number
	uchar	devfn;			// Router's devfunc
	uchar	pciirqs[2];		// Exclusive PCI irqs
	uchar	compat[4];		// Compatible PCI interrupt router
	uchar	miniport[4];		// Miniport data
	uchar	reserved[11];
	uchar	checksum;
};

static ushort pciirqs;			// Exclusive PCI irqs
static Bridge *southbridge;		// Which southbridge to use.

static void
pcirouting(void)
{
	Slot *e;
	Router *r;
	int size, i, fn, tbdf;
	Pcidev *sbpci, *pci;
	uchar *p, pin, irq, link, *map;

	// Search for PCI interrupt routing table in BIOS
	for(p = (uchar *)KADDR(0xf0000); p < (uchar *)KADDR(0xfffff); p += 16)
		if(p[0] == '$' && p[1] == 'P' && p[2] == 'I' && p[3] == 'R')
			break;

	if(p >= (uchar *)KADDR(0xfffff))
		return;

	r = (Router *)p;

	DBG("PCI interrupt routing table v%d.%d at %#p\n",
		r->version[0], r->version[1], r);

	tbdf = (BusPCI << 24)|(r->bus << 16)|(r->devfn << 8);
	sbpci = pcimatchtbdf(tbdf);
	if(sbpci == nil) {
		print("pcirouting: Cannot find south bridge %T\n", tbdf);
		return;
	}

	for(i = 0; i != nelem(southbridges); i++)
		if(sbpci->vid == southbridges[i].vid &&
		   sbpci->did == southbridges[i].did)
			break;

	if(i == nelem(southbridges)) {
		print("pcirouting: ignoring south bridge %T %.4ux/%.4ux\n",
			tbdf, sbpci->vid, sbpci->did);
		return;
	}
	southbridge = &southbridges[i];
	if(southbridge->get == nil || southbridge->set == nil)
		return;

	pciirqs = (r->pciirqs[1] << 8)|r->pciirqs[0];

	size = (r->size[1] << 8)|r->size[0];
	for(e = (Slot *)&r[1]; (uchar *)e < p + size; e++) {
		// print("%.2ux/%.2ux %.2ux: ", e->bus, e->dev, e->slot);
		// for (i = 0; i != 4; i++) {
		// 	uchar *m = &e->maps[i * 3];
		// 	print("[%d] %.2ux %.4ux ",
		// 		i, m[0], (m[2] << 8)|m[1]);
		// }
		// print("\n");

		for(fn = 0; fn != 8; fn++) {
			tbdf = BusPCI<<24 | e->bus<<16 | (e->dev | fn)<<8;
			pci = pcimatchtbdf(tbdf);
			if(pci == nil)
				continue;
			pin = pcicfgr8(pci, PciINTP);
			if(pin == 0 || pin == 0xff)
				continue;

			map = &e->maps[(pin - 1) * 3];
			link = map[0];
			irq = southbridge->get(sbpci, link);
			if(irq == 0 || irq == pci->intl)
				continue;
			if(pci->intl != 0 && pci->intl != 0xFF) {
				print("pcirouting: BIOS workaround: %T at "
					"pin %d link %d irq %d -> %d\n",
					tbdf, pin, link, irq, pci->intl);
				southbridge->set(sbpci, link, pci->intl);
				continue;
			}
			print("pcirouting: %T at pin %d link %d irq %d\n",
				tbdf, pin, link, irq);
			pcicfgw8(pci, PciINTL, irq);
			pci->intl = irq;
			if(0 && pci->intl == 0)
				print("pcirouting: warning: %T has irq 0\n",
					tbdf);
		}
	}
}

static void pcireservemem(void);

static void
nobiosinit(void)
{
	ulong mema, ioa;

	/*
	 * Work out how big the top bus is
	 */
	mema = 0;
	ioa = 0;
	pcibusmap(pciroot, &mema, &ioa, 0);
	DBG("Sizes: mem=%8.8lux size=%8.8lux io=%8.8lux\n",
		mema, pcimask(mema), ioa);

	/*
	 * Align the windows and map it
	 */
	ioa = 0x1000;
	mema = 0x90000000;

	pcilog("Mask sizes: mem=%lux io=%lux\n", mema, ioa);

	pcibusmap(pciroot, &mema, &ioa, 1);
	DBG("Sizes2: mem=%lux io=%lux\n", mema, ioa);
}

static void
pcicfginit(void)
{
	char *p;
	Pcidev *pci;
	Pcidev **list;
	int bno, n;

	lock(&pcicfginitlock);
	if(pcicfgmode != -1) {		/* already configured? */
		unlock(&pcicfginitlock);
		return;
	}

	if(getconf("*nobios"))
		nobios = 1;
	if(getconf("*nopcirouting"))
		nopcirouting = 1;

	/*
	 * Assume Configuration Mechanism One. Method Two was deprecated
	 * a long time ago and was only for backwards compaibility with the
	 * Intel Saturn and Mercury chip sets (1993-4). Thank you, QEMU.
	 */
	pcicfgmode = 1;
	pcimaxdno = 31;

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
	for(bno = 0; bno <= pcimaxbno; bno++) {
		int sbno = bno;

		bno = pcilscan(bno, list);
		while(*list)
			list = &(*list)->link;

		if (sbno == 0)
			/*
			 * If we have found a PCI-to-Cardbus bridge, make sure
			 * it has no valid mappings anymore.
			 */
			for(pci = pciroot; pci != nil; pci = pci->link)
				if (pci->ccrb == Pcibcbridge &&
				    pci->ccru == Pciscbrcard) {
					ushort bcr;

					/* reset the cardbus */
					bcr = pcicfgr16(pci, PciBCR);
					pcicfgw16(pci, PciBCR, 0x40 | bcr);
					delay(50);
				}
	}

	if(pciroot) {
		if(nobios)
			nobiosinit();
		else {
			if (!nopcirouting)
				pcirouting();
			pcireservemem();
		}
		if(getconf("*pcihinv"))
			pcilhinv(nil);
	}
	unlock(&pcicfginitlock);
}

static void
pcireservemem(void)
{
	int i;
	Pcidev *p;
	Pcibar *bar;

	/*
	 * mark all the physical address space claimed by pci devices
	 * as in use, so that it's not given out elsewhere.
	 * beware the type and size of 'bar'.
	 */
	for(p=pciroot; p; p=p->list)
		for(i=0; i<nelem(p->mem); i++) {
			bar = &p->mem[i];
			if(bar->bar && (bar->bar&1) == 0)
				asmmapinit(bar->bar & ~0x0F, bar->size, AsmDEV);
		}
}

static ulong
mkaddr(int tbdf, uint rno, int type)
{
	if (rno >= (1<<12)) {
		print("pci reg %#ux of %T out of range; sorry.\n", rno, tbdf);
		return -1;
	}
	/* the extra 4 bits of rno only work on some amd systems */
	return 1ul<<31 | (rno & (MASK(4)<<8))<<16 | BUSBDF(tbdf) |
		(rno & MASK(8) & ~3) | type;
}

static int
pcicfgrw8(int tbdf, int rno, int data, int read)
{
	int o, type, x;
	ulong addr;

	if(pcicfgmode == -1)
		pcicfginit();

	x = -1;
	if(BUSDNO(tbdf) > pcimaxdno)
		return x;
	type = BUSBNO(tbdf) != 0;
	addr = mkaddr(tbdf, rno, type);
	if (addr == -1)
		return x;

	lock(&pcicfglock);
	switch(pcicfgmode){
	case 1:
		o = rno & 0x03;
		outl(PciADDR, addr);
		if(read)
			x = inb(PciDATA+o);
		else
			outb(PciDATA+o, data);
		outl(PciADDR, 0);
		break;
	}
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
	int o, type, x;
	ulong addr;

	if(pcicfgmode == -1)
		pcicfginit();

	x = -1;
	if(BUSDNO(tbdf) > pcimaxdno)
		return x;
	type = BUSBNO(tbdf) != 0;
	addr = mkaddr(tbdf, rno, type);
	if (addr == -1)
		return x;

	lock(&pcicfglock);
	switch(pcicfgmode){
	case 1:
		o = rno & 0x02;
		outl(PciADDR, addr);
		if(read)
			x = ins(PciDATA+o);
		else
			outs(PciDATA+o, data);
		outl(PciADDR, 0);
		break;
	}
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
	int type, x;
	ulong addr;

	if(pcicfgmode == -1)
		pcicfginit();

	x = -1;
	if(BUSDNO(tbdf) > pcimaxdno)
		return x;
	type = BUSBNO(tbdf) != 0;
	addr = mkaddr(tbdf, rno, type);
	if (addr == -1)
		return x;

	lock(&pcicfglock);
	switch(pcicfgmode){
	case 1:
		outl(PciADDR, addr);
		if(read)
			x = inl(PciDATA);
		else
			outl(PciDATA, data);
		outl(PciADDR, 0);
		break;
	}
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

int
pcisetcfgbit(Pcidev *p, int reg, ulong bit, char *onmsg)
{
	ulong bits, obit;

	bits = pcicfgr32(p, reg);
	obit = bits & bit;
	if (obit)
		print(onmsg);
	else
		pcicfgw32(p, reg, bits | bit);
	return obit;
}

int
pciclrcfgbit(Pcidev *p, int reg, ulong bit, char *offmsg)
{
	ulong bits, obit;

	bits = pcicfgr32(p, reg);
	obit = bits & bit;
	if (obit)
		pcicfgw32(p, reg, bits & ~bit);
	else
		print(offmsg);
	return obit;
}

Pcidev*
pcimatch(Pcidev* prev, int vid, int did)
{
	if(pcicfgmode == -1)
		pcicfginit();

	prev = prev == nil? pcilist: prev->list;
	for(; prev != nil; prev = prev->list)
		if((vid == 0 || prev->vid == vid)
		&& (did == 0 || prev->did == did))
			break;
	return prev;
}

Pcidev*
pcimatchtbdf(int tbdf)
{
	Pcidev *pcidev;

	if(pcicfgmode == -1)
		pcicfginit();

	for(pcidev = pcilist; pcidev != nil; pcidev = pcidev->list)
		if(pcidev->tbdf == tbdf)
			break;
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

		for(i = 0; i < nelem(p->mem); i++)
			if(t->mem[i].size != 0)
				print("%d:%.8lux %d ", i,
					t->mem[i].bar, t->mem[i].size);
		if(t->ioa.bar || t->ioa.size)
			print("ioa:%.8lux %d ", t->ioa.bar, t->ioa.size);
		if(t->mema.bar || t->mema.size)
			print("mema:%.8lux %d ", t->mema.bar, t->mema.size);
		if(t->bridge)
			print("->%d", BUSBNO(t->bridge->tbdf));
		print("\n");
	}
	for(; p != nil; p = p->link)
		if(p->bridge != nil)
			pcilhinv(p->bridge);
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

	for(p = pcilist; p != nil; p = p->list)
		if(p->ccrb != Pcibcbridge) { /* don't mess with the bridges */
			pciclrbme(p);
			pcinointrs(p);
			pcimsioff(nil, p);
		}
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

/* return offset in config space for cap; -1 if not found */
static int
pcigetcap(Pcidev* p, int cap)
{
	int ptr;

	if(!(pcicfgr16(p, PciPSR) & Pcicap))	/* no capabilities? */
		return -1;
	/*
	 * Find the capabilities pointer based on PCI header type.
	 */
	switch(pcicfgr8(p, PciHDT) & ~Hdtmulti){
	default:
		return -1;
	case 0:					/* standard */
	case 1:					/* PCI to PCI bridge */
		ptr = PciCP;
		break;
	case 2:					/* CardBus bridge */
		ptr = 0x14;
		break;
	}
	for (ptr = pcicfgr32(p, ptr); ptr != 0; ptr = pcicfgr8(p, ptr+1)) {
		/*
		 * Check for validity.
		 * Can't be in standard header and must be double-word aligned.
		 */
		if(ptr < 0x40 || ptr & ((sizeof(ulong)-1)))
			break;
		else if(pcicfgr8(p, ptr) == cap)
			return ptr;
	}
	return -1;
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
	 * assume there's no standard power management method.
	 * Find the capabilities pointer based on PCI header type.
	 */
	if(!(pcicfgr16(p, PciPSR) & Pcicap))	/* no capabilities? */
		return -1;
	switch(pcicfgr8(p, PciHDT) & ~Hdtmulti){
	default:
		return -1;
	case 0:					/* standard */
	case 1:					/* PCI to PCI bridge */
		ptr = PciCP;
		break;
	case 2:					/* CardBus bridge */
		ptr = 0x14;
		break;
	}
	for (ptr = pcicfgr32(p, ptr); ptr != 0; ptr = pcicfgr8(p, ptr+1)) {
		/*
		 * Check for validity.
		 * Can't be in standard header and must be double-word aligned.
		 */
		if(ptr < 0x40 || ptr & ((sizeof(ulong)-1)))
			 break;
		if(pcicfgr8(p, ptr) == 0x01){
			p->pmrb = ptr;		/* cache result */
			return ptr;
		}
	}
	return -1;
}

/*
 * Power Management Register Block:
 *  offset 0:	Capability ID
 *	   1:	next item pointer
 *	   2:	capabilities
 *	   4:	control/status
 *	   6:	bridge support extensions
 *	   7:	data
 */

enum {
	Pmc	= 2,
	Pmcsr	= 4,

	Haved1	= 1<<9,			/* in pmc */
	Haved2  = 1<<10,

	Pwrstate = MASK(2),		/* in Pmcsr, power state bits: D0-D3 */
	Pme_en	= 1<<8,
};

int
pcigetpms(Pcidev* p)
{
	int ptr;

	if((ptr = pcigetpmrb(p)) == -1)
		return -1;
	return pcicfgr16(p, ptr+Pmcsr) & Pwrstate;
}

int
pcisetpms(Pcidev* p, int state)
{
	int ostate, pmc, ptr;
	ushort pmcsr;

	if((ptr = pcigetpmrb(p)) == -1)
		return -1;

	pmc = pcicfgr16(p, ptr+Pmc);
	pmcsr = pcicfgr16(p, ptr+Pmcsr);
	ostate = pmcsr & Pwrstate;
	pmcsr &= ~Pwrstate;

	switch(state){
	default:
		return -1;
	case 0:
		pmcsr &= ~Pme_en;
		break;
	case 1:
		if(!(pmc & Haved1))
			return -1;
		break;
	case 2:
		if(!(pmc & Haved2))
			return -1;
		break;
	case 3:
		break;
	}
	pmcsr |= state;
	pcicfgw16(p, ptr+Pmcsr, pmcsr);

	return ostate;
}

static int
pcigetmsicap(Pcidev* p)
{
	if(p->msiptr == 0)
		p->msiptr = pcigetcap(p, Pcicapmsi);
	return p->msiptr;
}

int
pcigetmsi(Pcidev *p, Msi *msi)
{
	int ptr;

	if((ptr = pcigetmsicap(p)) == -1)
		return -1;
	msi->ctl = pcicfgr16(p, ptr+2);
	msi->addr = pcicfgr32(p, ptr+4);
	if (msi->ctl & Msiaddr64) {
		msi->addr |= (uvlong)pcicfgr32(p, ptr+8) << 32;
		msi->data = pcicfgr16(p, ptr+0xc);
	} else
		msi->data = pcicfgr16(p, ptr+8);
	return 0;
}

int
pcisetmsi(Pcidev *p, Msi *msi)
{
	int ptr;

	if((ptr = pcigetmsicap(p)) == -1)
		return -1;

	if (msi->ctl & Msienable)
		p->pcr |= IntrDis;  /* no legacy interrupts allowed with msi */
	else
		p->pcr &= ~IntrDis;
	pcicfgw16(p, PciPCR, p->pcr);

	pcicfgw32(p, ptr+4, msi->addr);
	if (msi->ctl & Msiaddr64) {
		pcicfgw32(p, ptr+8, msi->addr>>32);
		pcicfgw16(p, ptr+0xc, msi->data);
	} else
		pcicfgw16(p, ptr+8, msi->data);
	pcicfgw16(p, ptr+2, msi->ctl);		/* write ctl last */
	return 0;
}

int
pcigetmsixcap(Pcidev* p)
{
	if(p->msixptr == 0)
		p->msixptr = pcigetcap(p, Pcicapmsix);
	return p->msixptr;
}

int
pcigetpciecap(Pcidev* p)
{
	if(p->pcieptr == 0)
		p->pcieptr = pcigetcap(p, Pcicappcie);
	return p->pcieptr;
}

void
pcinointrs(Pcidev *p)
{
	p->pcr |= IntrDis;
	pcicfgw16(p, PciPCR, p->pcr);
}

void
pciintrs(Pcidev *p)
{
	p->pcr &= ~IntrDis;
	pcicfgw16(p, PciPCR, p->pcr);
}

/* v may be nil */
void
pcimsioff(Vctl *v, Pcidev *pcidev)
{
	int ptr;
	Msi msi;

	memset(&msi, 0, sizeof msi);
	ptr = pcigetmsi(pcidev, &msi);
	if (ptr == -1)
		return;			/* no msi */
	if (v)
		v->ismsi = 0;
	if (!(msi.ctl & Msienable))
		return;			/* already off */
	msi.ctl &= ~Msienable;
	msi.addr = (uintptr)Lapicphys;
	msi.data = 0;
	pcisetmsi(pcidev, &msi);

//	print("%s: msi disabled for %T\n", (v? v->name: ""), pcidev->tbdf);
}
