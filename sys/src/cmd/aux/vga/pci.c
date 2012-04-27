#include <u.h>
#include <libc.h>
#include <bio.h>

#include "pci.h"
#include "vga.h"

/*
 * PCI support code.
 * There really should be a driver for this, it's not terribly safe
 * without locks or restrictions on what can be poked (e.g. Axil NX801).
 */
enum {					/* configuration mechanism #1 */
	PciADDR		= 0xCF8,	/* CONFIG_ADDRESS */
	PciDATA		= 0xCFC,	/* CONFIG_DATA */

					/* configuration mechanism #2 */
	PciCSE		= 0xCF8,	/* configuration space enable */
	PciFORWARD	= 0xCFA,	/* which bus */

	MaxFNO		= 7,
	MaxUBN		= 255,
};

static int pcicfgmode = -1;
static int pcimaxdno;
static Pcidev* pciroot;
static Pcidev* pcilist;
static Pcidev* pcitail;

static int pcicfgrw32(int, int, int, int);

static int
pciscan(int bno, Pcidev** list)
{
	ulong v;
	Pcidev *p, *head, *tail;
	int dno, fno, i, hdt, l, maxfno, maxubn, rno, sbn, tbdf, ubn;

	maxubn = bno;
	head = nil;
	tail = nil;
	for(dno = 0; dno <= pcimaxdno; dno++){
		maxfno = 0;
		for(fno = 0; fno <= maxfno; fno++){
			/*
			 * For this possible device, form the bus+device+function
			 * triplet needed to address it and try to read the vendor
			 * and device ID. If successful, allocate a device struct
			 * and start to fill it in with some useful information from
			 * the device's configuration space.
			 */
			tbdf = MKBUS(BusPCI, bno, dno, fno);
			l = pcicfgrw32(tbdf, PciVID, 0, 1);
			if(l == 0xFFFFFFFF || l == 0)
				continue;
			p = mallocz(sizeof(*p), 1);
			p->tbdf = tbdf;
			p->vid = l;
			p->did = l>>16;
			p->rid = pcicfgr8(p, PciRID);	

			if(pcilist != nil)
				pcitail->list = p;
			else
				pcilist = p;
			pcitail = p;

			p->intl = pcicfgr8(p, PciINTL);
			p->ccru = pcicfgr16(p, PciCCRu);

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
			switch(p->ccru>>8){

			case 0x01:		/* mass storage controller */
			case 0x02:		/* network controller */
			case 0x03:		/* display controller */
			case 0x04:		/* multimedia device */
			case 0x07:		/* simple communication controllers */
			case 0x08:		/* base system peripherals */
			case 0x09:		/* input devices */
			case 0x0A:		/* docking stations */
			case 0x0B:		/* processors */
			case 0x0C:		/* serial bus controllers */
				if((hdt & 0x7F) != 0)
					break;
				rno = PciBAR0 - 4;
				for(i = 0; i < nelem(p->mem); i++){
					rno += 4;
					p->mem[i].bar = pcicfgr32(p, rno);
					pcicfgw32(p, rno, -1);
					v = pcicfgr32(p, rno);
					pcicfgw32(p, rno, p->mem[i].bar);
					p->mem[i].size = -(v & ~0xF);
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
		if(p->ccru != ((0x06<<8)|0x04))
			continue;

		/*
		 * If the secondary or subordinate bus number is not initialised
		 * try to do what the PCI BIOS should have done and fill in the
		 * numbers as the tree is descended. On the way down the subordinate
		 * bus number is set to the maximum as it's not known how many
		 * buses are behind this one; the final value is set on the way
		 * back up.
		 */
		sbn = pcicfgr8(p, PciSBN);
		ubn = pcicfgr8(p, PciUBN);
		if(sbn == 0 || ubn == 0){
			sbn = maxubn+1;
			/*
			 * Make sure memory, I/O and master enables are off,
			 * set the primary, secondary and subordinate bus numbers
			 * and clear the secondary status before attempting to
			 * scan the secondary bus.
			 *
			 * Initialisation of the bridge should be done here.
			 */
			pcicfgw32(p, PciPCR, 0xFFFF0000);
			l = (MaxUBN<<16)|(sbn<<8)|bno;
			pcicfgw32(p, PciPBN, l);
			pcicfgw16(p, PciSPSR, 0xFFFF);
			maxubn = pciscan(sbn, &p->bridge);
			l = (maxubn<<16)|(sbn<<8)|bno;
			pcicfgw32(p, PciPBN, l);
		}
		else{
			maxubn = ubn;
			pciscan(sbn, &p->bridge);
		}
	}

	return maxubn;
}

static void
pcicfginit(void)
{
#ifdef kernel
	char *p;
#endif /* kernel */
	int bno;
	Pcidev **list;

	if(pcicfgmode == -1){
		/*
		 * Try to determine which PCI configuration mode is implemented.
		 * Mode2 uses a byte at 0xCF8 and another at 0xCFA; Mode1 uses
		 * a DWORD at 0xCF8 and another at 0xCFC and will pass through
		 * any non-DWORD accesses as normal I/O cycles. There shouldn't be
		 * a device behind these addresses so if Mode2 accesses fail try
		 * for Mode1 (which is preferred, Mode2 is deprecated).
		 */
		outportb(PciCSE, 0);
		if(inportb(PciCSE) == 0){
			pcicfgmode = 2;
			pcimaxdno = 15;
		}
		else{
			outportl(PciADDR, 0);
			if(inportl(PciADDR) == 0){
				pcicfgmode = 1;
				pcimaxdno = 31;
			}
		}
	
		if(pcicfgmode > 0){
			list = &pciroot;
			for(bno = 0; bno < 256; bno++){
				bno = pciscan(bno, list);
				while(*list)
					list = &(*list)->link;
			}
				
		}
	}
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

	switch(pcicfgmode){

	case 1:
		o = rno & 0x03;
		rno &= ~0x03;
		outportl(PciADDR, 0x80000000|BUSBDF(tbdf)|rno|type);
		if(read)
			x = inportb(PciDATA+o);
		else
			outportb(PciDATA+o, data);
		outportl(PciADDR, 0);
		break;

	case 2:
		outportb(PciCSE, 0x80|(BUSFNO(tbdf)<<1));
		outportb(PciFORWARD, BUSBNO(tbdf));
		if(read)
			x = inportb((0xC000|(BUSDNO(tbdf)<<8)) + rno);
		else
			outportb((0xC000|(BUSDNO(tbdf)<<8)) + rno, data);
		outportb(PciCSE, 0);
		break;
	}

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

	switch(pcicfgmode){

	case 1:
		o = rno & 0x02;
		rno &= ~0x03;
		outportl(PciADDR, 0x80000000|BUSBDF(tbdf)|rno|type);
		if(read)
			x = inportw(PciDATA+o);
		else
			outportw(PciDATA+o, data);
		outportl(PciADDR, 0);
		break;

	case 2:
		outportb(PciCSE, 0x80|(BUSFNO(tbdf)<<1));
		outportb(PciFORWARD, BUSBNO(tbdf));
		if(read)
			x = inportw((0xC000|(BUSDNO(tbdf)<<8)) + rno);
		else
			outportw((0xC000|(BUSDNO(tbdf)<<8)) + rno, data);
		outportb(PciCSE, 0);
		break;
	}

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

	switch(pcicfgmode){

	case 1:
		rno &= ~0x03;
		outportl(PciADDR, 0x80000000|BUSBDF(tbdf)|rno|type);
		if(read)
			x = inportl(PciDATA);
		else
			outportl(PciDATA, data);
		outportl(PciADDR, 0);
		break;

	case 2:
		outportb(PciCSE, 0x80|(BUSFNO(tbdf)<<1));
		outportb(PciFORWARD, BUSBNO(tbdf));
		if(read)
			x = inportl((0xC000|(BUSDNO(tbdf)<<8)) + rno);
		else
			outportl((0xC000|(BUSDNO(tbdf)<<8)) + rno, data);
		outportb(PciCSE, 0);
		break;
	}

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

	while(prev != nil) {
		if(prev->vid == vid && (did == 0 || prev->did == did))
			break;
		prev = prev->list;
	}
	return prev;
}

void
pcihinv(Pcidev* p)
{
	int i;
	Pcidev *t;

	if(pcicfgmode == -1)
		pcicfginit();


	if(p == nil) {
		p = pciroot;
		Bprint(&stdout, "bus dev type vid  did intl memory\n");
	}
	for(t = p; t != nil; t = t->link) {
		Bprint(&stdout, "%d  %2d/%d %.4ux %.4ux %.4ux %2d  ",
			BUSBNO(t->tbdf), BUSDNO(t->tbdf), BUSFNO(t->tbdf),
			t->ccru, t->vid, t->did, t->intl);

		for(i = 0; i < nelem(p->mem); i++) {
			if(t->mem[i].size == 0)
				continue;
			Bprint(&stdout, "%d:%.8lux %d ", i,
				t->mem[i].bar, t->mem[i].size);
		}
		Bprint(&stdout, "\n");
	}
	while(p != nil) {
		if(p->bridge != nil)
			pcihinv(p->bridge);
		p = p->link;
	}
}
