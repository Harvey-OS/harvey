/*
 * from 9front
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "pci.h"

#define PCIWIN 0x0600000000ULL

/* bcmstb PCIe controller registers */
enum{
	RC_CFG_VENDOR_VENDOR_SPECIFIC_REG1	= 0x0188/4,
	RC_CFG_PRIV1_ID_VAL3			= 0x043c/4,
	RC_DL_MDIO_ADDR				= 0x1100/4,
	RC_DL_MDIO_WR_DATA			= 0x1104/4,
	RC_DL_MDIO_RD_DATA			= 0x1108/4,
	MISC_MISC_CTRL				= 0x4008/4,
	MISC_CPU_2_PCIE_MEM_WIN0_LO		= 0x400c/4,
	MISC_CPU_2_PCIE_MEM_WIN0_HI		= 0x4010/4,
	MISC_RC_BAR1_CONFIG_LO			= 0x402c/4,
	MISC_RC_BAR2_CONFIG_LO			= 0x4034/4,
	MISC_RC_BAR2_CONFIG_HI			= 0x4038/4,
	MISC_RC_BAR3_CONFIG_LO			= 0x403c/4,
	MISC_MSI_BAR_CONFIG_LO			= 0x4044/4,
	MISC_MSI_BAR_CONFIG_HI			= 0x4048/4,
	MISC_MSI_DATA_CONFIG			= 0x404c/4,
	MISC_EOI_CTRL				= 0x4060/4,
	MISC_PCIE_CTRL				= 0x4064/4,
	MISC_PCIE_STATUS			= 0x4068/4,
	MISC_REVISION				= 0x406c/4,
	MISC_CPU_2_PCIE_MEM_WIN0_BASE_LIMIT	= 0x4070/4,
	MISC_CPU_2_PCIE_MEM_WIN0_BASE_HI	= 0x4080/4,
	MISC_CPU_2_PCIE_MEM_WIN0_LIMIT_HI	= 0x4084/4,
	MISC_HARD_PCIE_HARD_DEBUG		= 0x4204/4,

	INTR2_CPU_BASE				= 0x4300/4,
	MSI_INTR2_BASE				= 0x4500/4,
		INTR_STATUS = 0,
		INTR_SET,
		INTR_CLR,
		INTR_MASK_STATUS,
		INTR_MASK_SET,
		INTR_MASK_CLR,

	EXT_CFG_INDEX				= 0x9000/4,
	RGR1_SW_INIT_1				= 0x9210/4,
	EXT_CFG_DATA				= 0x8000/4,

};

#define MSI_TARGET_ADDR		0xFFFFFFFFCULL

static u32int *regs = (u32int*)(VIRTIO + 0x500000);

static Lock pcicfglock;
static int pcimaxbno = 0;
static int pcimaxdno = 0;
static Pcidev* pciroot;
static Pcidev* pcilist;
static Pcidev* pcitail;

typedef struct Pcisiz Pcisiz;
struct Pcisiz
{
	Pcidev*	dev;
	int	siz;
	int	bar;
};

enum
{
	MaxFNO		= 7,
	MaxUBN		= 255,
};

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

static void pcicfginit(void);

static void*
cfgaddr(int tbdf, int rno)
{
	if(BUSBNO(tbdf) == 0 && BUSDNO(tbdf) == 0)
		return (uchar*)regs + rno;
	regs[EXT_CFG_INDEX] = BUSBNO(tbdf) << 20 | BUSDNO(tbdf) << 15 | BUSFNO(tbdf) << 12;
	coherence();
	return ((uchar*)&regs[EXT_CFG_DATA]) + rno;
}

static int
pcicfgrw32(int tbdf, int rno, int data, int read)
{
	int x = -1;
	u32int *p;

	ilock(&pcicfglock);
	if((p = cfgaddr(tbdf, rno & ~3)) != nil){
		if(read)
			x = *p;
		else
			*p = data;
	}
	iunlock(&pcicfglock);
	return x;
}
static int
pcicfgrw16(int tbdf, int rno, int data, int read)
{
	int x = -1;
	u16int *p;

	ilock(&pcicfglock);
	if((p = cfgaddr(tbdf, rno & ~1)) != nil){
		if(read)
			x = *p;
		else
			*p = data;
	}
	iunlock(&pcicfglock);
	return x;
}
static int
pcicfgrw8(int tbdf, int rno, int data, int read)
{
	int x = -1;
	u8int *p;

	ilock(&pcicfglock);
	if((p = cfgaddr(tbdf, rno)) != nil){
		if(read)
			x = *p;
		else
			*p = data;
	}
	iunlock(&pcicfglock);
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
pcicfgr16(Pcidev* pcidev, int rno)
{
	return pcicfgrw16(pcidev->tbdf, rno, 0, 1);
}
void
pcicfgw16(Pcidev* pcidev, int rno, int data)
{
	pcicfgrw16(pcidev->tbdf, rno, data, 0);
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

Pcidev*
pcimatch(Pcidev* prev, int vid, int did)
{
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

	for(pcidev = pcilist; pcidev != nil; pcidev = pcidev->list) {
		if(pcidev->tbdf == tbdf)
			break;
	}
	return pcidev;
}

static u32int
pcibarsize(Pcidev *p, int rno)
{
	u32int v, size;

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
	ulong m;

	m = BI2BY*sizeof(v);
	for(m = 1<<(m-1); m != 0; m >>= 1) {
		if(m & v)
			break;
	}

	m--;
	if((v & m) == 0)
		return v;

	v |= m;
	return v+1;
}

static void
pcibusmap(Pcidev *root, uintpci *pmema, uintpci *pioa, int wrreg)
{
	Pcidev *p;
	int ntb, i, size, rno, hole;
	uintpci v, mema, ioa, sioa, smema, base, limit;
	Pcisiz *table, *tptr, *mtb, *itb;

	ioa = *pioa;
	mema = *pmema;

	ntb = 0;
	for(p = root; p != nil; p = p->link)
		ntb++;

	ntb *= (PciCIS-PciBAR0)/4;
	table = malloc(2*ntb*sizeof(Pcisiz));
	if(table == nil)
		panic("pcibusmap: can't allocate memory");
	itb = table;
	mtb = table+ntb;

	/*
	 * Build a table of sizes
	 */
	for(p = root; p != nil; p = p->link) {
		if(p->ccrb == 0x06) {
			if(p->ccru != 0x04 || p->bridge == nil)
				continue;

			sioa = ioa;
			smema = mema;
			pcibusmap(p->bridge, &smema, &sioa, 0);

			hole = pcimask(smema-mema);
			if(hole < (1<<20))
				hole = 1<<20;
			p->mema.size = hole;

			hole = pcimask(sioa-ioa);
			if(hole < (1<<12))
				hole = 1<<12;

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

		for(i = 0; i <= 5; i++) {
			rno = PciBAR0 + i*4;
			v = pcicfgrw32(p->tbdf, rno, 0, 1);
			size = pcibarsize(p, rno);
			if(size == 0)
				continue;

			p->mem[i].size = size;
			if(v & 1) {
				itb->dev = p;
				itb->bar = i;
				itb->siz = size;
				itb++;
			}
			else {
				mtb->dev = p;
				mtb->bar = i;
				mtb->siz = size;
				mtb++;

				if((v & 7) == 4)
					i++;
			}
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
			if(wrreg){
				rno = PciBAR0+(tptr->bar*4);
				if((mema >> 32) != 0){
					pcicfgrw32(p->tbdf, rno, mema|4, 0);
					pcicfgrw32(p->tbdf, rno+4, mema >> 32, 0);
				} else {
					pcicfgrw32(p->tbdf, rno, mema, 0);
				}
			}
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
			if(p->cls == 0){
				p->cls = 64;
				pcicfgw8(p, PciCLS, p->cls);
			}
			pcicfgrw8(p->tbdf, PciLTR, 64, 0);
			p->pcr |= MASen;
			pcicfgrw16(p->tbdf, PciPCR, p->pcr, 0);
			continue;
		}

		if(p == pciroot){
			base = p->mema.bar;
			limit = base+p->mema.size-1;
			regs[MISC_CPU_2_PCIE_MEM_WIN0_LO] = base;
			regs[MISC_CPU_2_PCIE_MEM_WIN0_HI] = base >> 32;
			base >>= 20, limit >>= 20;
			regs[MISC_CPU_2_PCIE_MEM_WIN0_BASE_LIMIT] = (base & 0xFFF) << 4 | (limit & 0xFFF) << 20;
			regs[MISC_CPU_2_PCIE_MEM_WIN0_BASE_HI] = base >> 12;
			regs[MISC_CPU_2_PCIE_MEM_WIN0_LIMIT_HI] = limit >> 12;
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
		pcicfgrw32(p->tbdf, PciPCR, 0xFFFF0000|p->pcr, 0);

		sioa = p->ioa.bar;
		smema = p->mema.bar;
		pcibusmap(p->bridge, &smema, &sioa, 1);
	}
}

static int
pcilscan(int bno, Pcidev** list, Pcidev *parent)
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
			case 0x00:		/* prehistoric */
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
			case 0x0D:		/* wireless controllers */
			case 0x0E:		/* intelligent I/O controllers */
			case 0x0F:		/* sattelite communication controllers */
			case 0x10:		/* encryption/decryption controllers */
			case 0x11:		/* signal processing controllers */
				if((hdt & 0x7F) != 0)
					break;
				rno = PciBAR0;
				for(i = 0; i <= 5; i++) {
					p->mem[i].bar = pcicfgr32(p, rno);
					p->mem[i].size = pcibarsize(p, rno);
					if((p->mem[i].bar & 7) == 4 && i < 5){
						rno += 4;
						p->mem[i].bar |= (uintpci)pcicfgr32(p, rno) << 32;
						i++;
					}
					rno += 4;
				}
				break;

			case 0x05:		/* memory controller */
			case 0x06:		/* bridge device */
			default:
				break;
			}

			p->parent = parent;
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
			maxubn = pcilscan(sbn, &p->bridge, p);
			l = (maxubn<<16)|(sbn<<8)|bno;

			pcicfgw32(p, PciPBN, l);
		}
		else {
			if(ubn > maxubn)
				maxubn = ubn;
			pcilscan(sbn, &p->bridge, p);
		}
	}

	return maxubn;
}

static void
pcicfginit(void)
{
	uintpci mema, ioa;

	fmtinstall('T', tbdffmt);

	pcilscan(0, &pciroot, nil);

	/*
	 * Work out how big the top bus is
	 */
	ioa = 0;
	mema = 0;
	pcibusmap(pciroot, &mema, &ioa, 0);

	/*
	 * Align the windows and map it
	 */
	ioa = 0;
	mema = PCIWIN;
	pcibusmap(pciroot, &mema, &ioa, 1);
}

static void
pcilhinv(Pcidev* p)
{
	int i;
	Pcidev *t;

	if(p == nil) {
		p = pciroot;
		print("pci dev type     vid  did intl memory\n");
	}
	for(t = p; t != nil; t = t->link) {
		print("%d  %2d/%d %.2ux %.2ux %.2ux %.4ux %.4ux %3d  ",
			BUSBNO(t->tbdf), BUSDNO(t->tbdf), BUSFNO(t->tbdf),
			t->ccrb, t->ccru, t->ccrp, t->vid, t->did, t->intl);

		for(i = 0; i < nelem(p->mem); i++) {
			if(t->mem[i].size == 0)
				continue;
			print("%d:%llux %d ", i,
				(uvlong)t->mem[i].bar, t->mem[i].size);
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

static void
pcihinv(Pcidev* p)
{
	pcilhinv(p);
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
enumcaps(Pcidev *p, int (*fmatch)(Pcidev*, int, int, int), int arg)
{
	int i, r, cap, off;

	/* status register bit 4 has capabilities */
	if((pcicfgr16(p, PciPSR) & 1<<4) == 0)
		return -1;      
	switch(pcicfgr8(p, PciHDT) & 0x7F){
	default:
		return -1;
	case 0:                         /* etc */
	case 1:                         /* pci to pci bridge */
		off = 0x34;
		break;
	case 2:                         /* cardbus bridge */
		off = 0x14;
		break;
	}
	for(i = 48; i--;){
		off = pcicfgr8(p, off);
		if(off < 0x40 || (off & 3))
			break;
		off &= ~3;
		cap = pcicfgr8(p, off);
		if(cap == 0xff)
			break;
		r = (*fmatch)(p, cap, off, arg);
		if(r < 0)
			break;
		if(r == 0)
			return off;
		off++;
	}
	return -1;
}

static int
matchcap(Pcidev *, int cap, int, int arg)
{
	return cap != arg;
}

static int
matchhtcap(Pcidev *p, int cap, int off, int arg)
{
	int mask;

	if(cap != PciCapHTC)
		return 1;
	if(arg == 0x00 || arg == 0x20)
		mask = 0xE0;
	else
		mask = 0xF8;
	cap = pcicfgr8(p, off+3);
	return (cap & mask) != arg;
}

int
pcicap(Pcidev *p, int cap)
{
	return enumcaps(p, matchcap, cap);
}

int
pcinextcap(Pcidev *pci, int offset)
{
	if(offset == 0) {
		if((pcicfgr16(pci, PciPSR) & (1<<4)) == 0)
			return 0; /* no capabilities */
		offset = PciCAP-1;
	}
	return pcicfgr8(pci, offset+1) & ~3;
}

int
pcihtcap(Pcidev *p, int cap)
{
	return enumcaps(p, matchhtcap, cap);
}

static int
pcigetpmrb(Pcidev* p)
{
        if(p->pmrb != 0)
                return p->pmrb;
        return p->pmrb = pcicap(p, PciCapPMG);
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

void
pcienable(Pcidev *p)
{
	uint pcr;
	int i;

	if(p == nil)
		return;

	pcienable(p->parent);

	switch(pcisetpms(p, 0)){
	case 1:
		print("pcienable %T: wakeup from D1\n", p->tbdf);
		break;
	case 2:
		print("pcienable %T: wakeup from D2\n", p->tbdf);
		if(p->bridge != nil)
			delay(100);	/* B2: minimum delay 50ms */
		else
			delay(1);	/* D2: minimum delay 200µs */
		break;
	case 3:
		print("pcienable %T: wakeup from D3\n", p->tbdf);
		delay(100);		/* D3: minimum delay 50ms */

		/* restore registers */
		for(i = 0; i < 6; i++)
			pcicfgw32(p, PciBAR0+i*4, p->mem[i].bar);
		pcicfgw8(p, PciINTL, p->intl);
		pcicfgw8(p, PciLTR, p->ltr);
		pcicfgw8(p, PciCLS, p->cls);
		pcicfgw16(p, PciPCR, p->pcr);
		break;
	}

	if(p->bridge != nil)
		pcr = IOen|MEMen|MASen;
	else {
		pcr = 0;
		for(i = 0; i < 6; i++){
			if(p->mem[i].size == 0)
				continue;
			if(p->mem[i].bar & 1)
				pcr |= IOen;
			else
				pcr |= MEMen;
		}
	}

	if((p->pcr & pcr) != pcr){
		print("pcienable %T: pcr %ux->%ux\n", p->tbdf, p->pcr, p->pcr|pcr);
		p->pcr |= pcr;
		pcicfgrw32(p->tbdf, PciPCR, 0xFFFF0000|p->pcr, 0);
	}
}

void
pcidisable(Pcidev *p)
{
	if(p == nil)
		return;
	pciclrbme(p);
}

enum {
	MSICtrl = 0x02, /* message control register (16 bit) */
	MSIAddr = 0x04, /* message address register (64 bit) */
	MSIData32 = 0x08, /* message data register for 32 bit MSI (16 bit) */
	MSIData64 = 0x0C, /* message data register for 64 bit MSI (16 bit) */
};

typedef struct Pciisr Pciisr;
struct Pciisr {
	void	(*f)(Ureg*, void*);
	void	*a;
	Pcidev	*p;
};

static Pciisr pciisr[32];
static Lock pciisrlk;

void
pciintrenable(int tbdf, void (*f)(Ureg*, void*), void *a)
{
	int cap, ok64;
	u32int dat;
	u64int adr;
	Pcidev *p;
	Pciisr *isr;

	if((p = pcimatchtbdf(tbdf)) == nil){
		print("pciintrenable: %T: unknown device\n", tbdf);
		return;
	}
	if((cap = pcicap(p, PciCapMSI)) < 0){
		print("pciintrenable: %T: no MSI cap\n", tbdf);
		return;
	}

	lock(&pciisrlk);
	for(isr = pciisr; isr < &pciisr[nelem(pciisr)]; isr++){
		if(isr->p == p){
			isr->p = nil;
			regs[MSI_INTR2_BASE + INTR_MASK_SET] = 1 << (isr-pciisr);
			break;
		}
	}
	for(isr = pciisr; isr < &pciisr[nelem(pciisr)]; isr++){
		if(isr->p == nil){
			isr->p = p;
			isr->a = a;
			isr->f = f;
			regs[MSI_INTR2_BASE + INTR_CLR] = 1 << (isr-pciisr);
			regs[MSI_INTR2_BASE + INTR_MASK_CLR] = 1 << (isr-pciisr);
			break;
		}
	}
	unlock(&pciisrlk);

	if(isr >= &pciisr[nelem(pciisr)]){
		print("pciintrenable: %T: out of isr slots\n", tbdf);
		return;
	}

	adr = MSI_TARGET_ADDR;
	ok64 = (pcicfgr16(p, cap + MSICtrl) & (1<<7)) != 0;
	pcicfgw32(p, cap + MSIAddr, adr);
	if(ok64) pcicfgw32(p, cap + MSIAddr + 4, adr>>32);
	dat = regs[MISC_MSI_DATA_CONFIG];
	dat = ((dat >> 16) & (dat & 0xFFFF)) | (isr-pciisr);
	pcicfgw16(p, cap + (ok64 ? MSIData64 : MSIData32), dat);
	pcicfgw16(p, cap + MSICtrl, 1);
}

void
pciintrdisable(int tbdf, void (*f)(Ureg*, void*), void *a)
{
	Pciisr *isr;

	lock(&pciisrlk);
	for(isr = pciisr; isr < &pciisr[nelem(pciisr)]; isr++){
		if(isr->p != nil && isr->p->tbdf == tbdf && isr->f == f && isr->a == a){
			regs[MSI_INTR2_BASE + INTR_MASK_SET] = 1 << (isr-pciisr);
			isr->p = nil;
			isr->f = nil;
			isr->a = nil;
			break;
		}
	}
	unlock(&pciisrlk);
}

static void
pciinterrupt(Ureg *ureg, void*)
{
	Pciisr *isr;
	u32int sts;

	sts = regs[MSI_INTR2_BASE + INTR_STATUS];
	if(sts == 0)
		return;
	regs[MSI_INTR2_BASE + INTR_CLR] = sts;
	for(isr = pciisr; sts != 0 && isr < &pciisr[nelem(pciisr)]; isr++, sts>>=1){
		if((sts & 1) != 0 && isr->f != nil)
			(*isr->f)(ureg, isr->a);
	}
	regs[MISC_EOI_CTRL] = 1;
}

void
pcilink(void)
{
	int log2dmasize = 30;	// 1GB

	regs[RGR1_SW_INIT_1] |= 3;
	delay(200);
	regs[RGR1_SW_INIT_1] &= ~2;
	regs[MISC_PCIE_CTRL] &= ~5;
	delay(200);

	regs[MISC_HARD_PCIE_HARD_DEBUG] &= ~0x08000000;
	delay(200);

	regs[MSI_INTR2_BASE + INTR_CLR] = -1;
	regs[MSI_INTR2_BASE + INTR_MASK_SET] = -1;

	regs[MISC_CPU_2_PCIE_MEM_WIN0_LO] = 0;
	regs[MISC_CPU_2_PCIE_MEM_WIN0_HI] = 0;
	regs[MISC_CPU_2_PCIE_MEM_WIN0_BASE_LIMIT] = 0;
	regs[MISC_CPU_2_PCIE_MEM_WIN0_BASE_HI] = 0;
	regs[MISC_CPU_2_PCIE_MEM_WIN0_LIMIT_HI] = 0;

	// SCB_ACCESS_EN, CFG_READ_UR_MODE, MAX_BURST_SIZE_128, SCB0SIZE
	regs[MISC_MISC_CTRL] = 1<<12 | 1<<13 | 0<<20 | (log2dmasize-15)<<27;

	regs[MISC_RC_BAR2_CONFIG_LO] = (log2dmasize-15);
	regs[MISC_RC_BAR2_CONFIG_HI] = 0;

	regs[MISC_RC_BAR1_CONFIG_LO] = 0;
	regs[MISC_RC_BAR3_CONFIG_LO] = 0;

	regs[MISC_MSI_BAR_CONFIG_LO] = MSI_TARGET_ADDR | 1;
	regs[MISC_MSI_BAR_CONFIG_HI] = MSI_TARGET_ADDR>>32;
	regs[MISC_MSI_DATA_CONFIG] = 0xFFF86540;
	intrenable(IRQpci, pciinterrupt, nil, BUSUNKNOWN, "pci");

	// force to GEN2
	regs[(0xAC + 12)/4] = (regs[(0xAC + 12)/4] & ~15) | 2;	// linkcap
	regs[(0xAC + 48)/4] = (regs[(0xAC + 48)/4] & ~15) | 2;	// linkctl2

	regs[RGR1_SW_INIT_1] &= ~1;
	delay(500);

	if((regs[MISC_PCIE_STATUS] & 0x30) != 0x30){
		print("pcireset: phy link is down\n");
		return;
	}

	regs[RC_CFG_PRIV1_ID_VAL3] = 0x060400;
	regs[RC_CFG_VENDOR_VENDOR_SPECIFIC_REG1] &= ~0xC;
	regs[MISC_HARD_PCIE_HARD_DEBUG] |= 2;

	pcicfginit();
	pcihinv(nil);
}
