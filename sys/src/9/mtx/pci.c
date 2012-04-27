/*
 * PCI support code.
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/error.h"

#define DBG	if(0) pcilog

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

enum
{					/* configuration mechanism #1 */
	PciADDR		= 0xCF8,	/* CONFIG_ADDRESS */
	PciDATA		= 0xCFC,	/* CONFIG_DATA */

					/* configuration mechanism #2 */
	PciCSE		= 0xCF8,	/* configuration space enable */
	PciFORWARD	= 0xCFA,	/* which bus */

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
static QLock pcicfginitlock;
static int pcicfgmode = -1;
static int pcimaxbno = 7;
static int pcimaxdno;
static Pcidev* pciroot;
static Pcidev* pcilist;
static Pcidev* pcitail;

static int pcicfgrw32(int, int, int, int);
static int pcicfgrw8(int, int, int, int);

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
pcibusmap(Pcidev *root, ulong *pmema, ulong *pioa, int wrreg)
{
	Pcidev *p;
	int ntb, i, size, rno, hole;
	ulong v, mema, ioa, sioa, smema, base, limit;
	Pcisiz *table, *tptr, *mtb, *itb;
	extern void qsort(void*, long, long, int (*)(void*, void*));

	ioa = *pioa;
	mema = *pmema;

	DBG("pcibusmap wr=%d %T mem=%luX io=%luX\n", 
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
		if(p->ccrb == 0x06) {
			if(p->ccru == 0x04 && p->bridge != nil) {
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
			}
			if((pcicfgr8(p, PciHDT)&0x7f) != 0)
				continue;
		}

		for(i = 0; i <= 5; i++) {
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
			}
			else {
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
			pcicfgrw32(p->tbdf, PciPCR, p->pcr, 0);
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
		v = 0xFFFF0000 | IOen | MEMen | MASen;
		pcicfgrw32(p->tbdf, PciPCR, v, 0);

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

			p->rid = pcicfgr8(p, PciRID);
			p->ccrp = pcicfgr8(p, PciCCRp);
			p->ccru = pcicfgr8(p, PciCCRu);
			p->ccrb = pcicfgr8(p, PciCCRb);
			p->pcr = pcicfgr32(p, PciPCR);

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
			case 0x01:		/* mass storage controller */
			case 0x02:		/* network controller */
			case 0x03:		/* display controller */
			case 0x04:		/* multimedia device */
			case 0x06:		/* bridge device */
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

	qlock(&pcicfginitlock);
	ubn = pcilscan(bno, list);
	qunlock(&pcicfginitlock);
	return ubn;
}

static void
pcicfginit(void)
{
	char *p;
	int bno;
	Pcidev **list;
	ulong mema, ioa;

	qlock(&pcicfginitlock);
	if(pcicfgmode != -1)
		goto out;

	/*
	 * Try to determine which PCI configuration mode is implemented.
	 * Mode2 uses a byte at 0xCF8 and another at 0xCFA; Mode1 uses
	 * a DWORD at 0xCF8 and another at 0xCFC and will pass through
	 * any non-DWORD accesses as normal I/O cycles. There shouldn't be
	 * a device behind these addresses so if Mode2 accesses fail try
	 * for Mode1 (which is preferred, Mode2 is deprecated).
	 */
	outb(PciCSE, 0);
	if(inb(PciCSE) == 0){
		pcicfgmode = 2;
		pcimaxdno = 15;
	}
	else {
		outl(PciADDR, 0);
		if(inl(PciADDR) == 0){
			pcicfgmode = 1;
			pcimaxdno = 31;
		}
	}
	
	if(pcicfgmode < 0)
		goto out;

	fmtinstall('T', tbdffmt);

	if(p = getconf("*pcimaxbno"))
		pcimaxbno = strtoul(p, 0, 0);
	if(p = getconf("*pcimaxdno"))
		pcimaxdno = strtoul(p, 0, 0);

	list = &pciroot;
	for(bno = 0; bno <= pcimaxbno; bno++) {
		int sbno = bno;
		bno = pcilscan(bno, list);

		while(*list)
			list = &(*list)->link;

		if (sbno == 0) {
			Pcidev *pci;

			/*
			  * If we have found a PCI-to-Cardbus bridge, make sure
			  * it has no valid mappings anymore.  
			  */
			pci = pciroot;
			while (pci) {
				if (pci->ccrb == 6 && pci->ccru == 7) {
					ushort bcr;

					/* reset the cardbus */
					bcr = pcicfgr16(pci, PciBCR);
					pcicfgw16(pci, PciBCR, 0x40 | bcr);
					delay(50);
				}
				pci = pci->link;
			}
		}
	}

	if(pciroot == nil)
		goto out;

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
	mema = 0;

	pcilog("Mask sizes: mem=%lux io=%lux\n", mema, ioa);

	pcibusmap(pciroot, &mema, &ioa, 1);
	DBG("Sizes2: mem=%lux io=%lux\n", mema, ioa);

out:
	qunlock(&pcicfginitlock);
}

static int
pcicfgrw8(int tbdf, int rno, int data, int read)
{
	int o, type, x;

	if(pcicfgmode == -1)
		pcicfginit();

	if(BUSBNO(tbdf))
		type = 0x01;
	else
		type = 0x00;
	x = -1;
	if(BUSDNO(tbdf) > pcimaxdno)
		return x;

	lock(&pcicfglock);
	switch(pcicfgmode){

	case 1:
		o = rno & 0x03;
		rno &= ~0x03;
		outl(PciADDR, 0x80000000|BUSBDF(tbdf)|rno|type);
		if(read)
			x = inb(PciDATA+o);
		else
			outb(PciDATA+o, data);
		outl(PciADDR, 0);
		break;

	case 2:
		outb(PciCSE, 0x80|(BUSFNO(tbdf)<<1));
		outb(PciFORWARD, BUSBNO(tbdf));
		if(read)
			x = inb((0xC000|(BUSDNO(tbdf)<<8)) + rno);
		else
			outb((0xC000|(BUSDNO(tbdf)<<8)) + rno, data);
		outb(PciCSE, 0);
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

	if(pcicfgmode == -1)
		pcicfginit();

	if(BUSBNO(tbdf))
		type = 0x01;
	else
		type = 0x00;
	x = -1;
	if(BUSDNO(tbdf) > pcimaxdno)
		return x;

	lock(&pcicfglock);
	switch(pcicfgmode){

	case 1:
		o = rno & 0x02;
		rno &= ~0x03;
		outl(PciADDR, 0x80000000|BUSBDF(tbdf)|rno|type);
		if(read)
			x = ins(PciDATA+o);
		else
			outs(PciDATA+o, data);
		outl(PciADDR, 0);
		break;

	case 2:
		outb(PciCSE, 0x80|(BUSFNO(tbdf)<<1));
		outb(PciFORWARD, BUSBNO(tbdf));
		if(read)
			x = ins((0xC000|(BUSDNO(tbdf)<<8)) + rno);
		else
			outs((0xC000|(BUSDNO(tbdf)<<8)) + rno, data);
		outb(PciCSE, 0);
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

	if(pcicfgmode == -1)
		pcicfginit();

	if(BUSBNO(tbdf))
		type = 0x01;
	else
		type = 0x00;
	x = -1;
	if(BUSDNO(tbdf) > pcimaxdno)
		return x;

	lock(&pcicfglock);
	switch(pcicfgmode){

	case 1:
		rno &= ~0x03;
		outl(PciADDR, 0x80000000|BUSBDF(tbdf)|rno|type);
		if(read)
			x = inl(PciDATA);
		else
			outl(PciDATA, data);
		outl(PciADDR, 0);
		break;

	case 2:
		outb(PciCSE, 0x80|(BUSFNO(tbdf)<<1));
		outb(PciFORWARD, BUSBNO(tbdf));
		if(read)
			x = inl((0xC000|(BUSDNO(tbdf)<<8)) + rno);
		else
			outl((0xC000|(BUSDNO(tbdf)<<8)) + rno, data);
		outb(PciCSE, 0);
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

uchar
pciipin(Pcidev *pci, uchar pin)
{
	if (pci == nil)
		pci = pcilist;

	while (pci) {
		uchar intl;

		if (pcicfgr8(pci, PciINTP) == pin && pci->intl != 0 && pci->intl != 0xff)
			return pci->intl;

		if (pci->bridge && (intl = pciipin(pci->bridge, pin)) != 0)
			return intl;

		pci = pci->list;
	}
	return 0;
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
		if(t->ioa.bar || t->ioa.size)
			print("ioa:%.8lux %d ", t->ioa.bar, t->ioa.size);
		if(t->mema.bar || t->mema.size)
			print("mema:%.8lux %d ", t->mema.bar, t->mema.size);
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
	qlock(&pcicfginitlock);
	pcilhinv(p);
	qunlock(&pcicfginitlock);
}

void
pcireset(void)
{
	Pcidev *p;
	int pcr;

	if(pcicfgmode == -1)
		pcicfginit();

	for(p = pcilist; p != nil; p = p->list){
		pcr = pcicfgr16(p, PciPCR);
		pcr &= ~0x0004;
		pcicfgw16(p, PciPCR, pcr);
	}
}

void
pcisetbme(Pcidev* p)
{
	int pcr;

	pcr = pcicfgr16(p, PciPCR);
	pcr |= MASen;
	pcicfgw16(p, PciPCR, pcr);
}

void
pciclrbme(Pcidev* p)
{
	int pcr;

	pcr = pcicfgr16(p, PciPCR);
	pcr &= ~MASen;
	pcicfgw16(p, PciPCR, pcr);
}
