/*
 * PCI support code.
 * To do:
 *	initialise bridge mappings if the PCI BIOS didn't.
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/error.h"

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
static int pcimaxdno;
static Pcidev* pciroot;
static Pcidev* pcilist;
static Pcidev* pcitail;

static int pcicfgrw32(int, int, int, int);

uchar	*vgabios;

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
/* optional safety checks:
			if(l == pcicfgrw32(tbdf, PciPCR, 0, 1))
				continue;
			if(l != pcicfgrw32(tbdf, PciVID, 0, 1))
				continue;
			if(l == pcicfgrw32(tbdf, PciPCR, 0, 1))
				continue;
*/
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
			switch(p->ccrb){

			case 0x03:		/* display controller */
				if(vgabios == nil) {
					v = pcicfgr32(p, PciROM);
					pcicfgw32(p, PciROM, v|1);	/* enable decode */
					vgabios = kmapv(((uvlong)0x88<<32LL)|(v&~0xffff), 0x10000);
					// print("VGA BIOS %lux -> %lux\n", v, vgabios);
				}
				/* fall through */
			case 0x01:		/* mass storage controller */
			case 0x02:		/* network controller */
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
		if(p->ccrb != 0x06 || p->ccru != 0x04)
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
	char *p;

	lock(&pcicfginitlock);
	if(pcicfgmode == -1){
		pcicfgmode = 0;
		pcimaxdno = 15;		/* was 20; what is correct value??? */
		if(p = getconf("*pcimaxdno"))
			pcimaxdno = strtoul(p, 0, 0);
		pciscan(0, &pciroot);
	}
	unlock(&pcicfginitlock);
}

static int
pcicfgrw8(int tbdf, int rno, int data, int read)
{
	int x;
	uchar *p;

	if(pcicfgmode == -1)
		pcicfginit();
	x = -1;
	if(BUSDNO(tbdf) > pcimaxdno)
		return x;

	p = (uchar*)arch->pcicfg(tbdf, rno);
	if(read)
		x = *p;
	else
		*p = data;

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
	ushort *p;

	if(pcicfgmode == -1)
		pcicfginit();
	x = -1;
	if(BUSDNO(tbdf) > pcimaxdno)
		return x;

	p = (ushort*)arch->pcicfg(tbdf, rno);
	if(read)
		x = *p;
	else
		*p = data;

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
	ulong *p;

	if(pcicfgmode == -1)
		pcicfginit();
	x = -1;
	if(BUSDNO(tbdf) > pcimaxdno)
		return x;

	p = (ulong*)arch->pcicfg(tbdf, rno);
	if(read)
		x = *p;
	else
		*p = data;

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
pcihinv(Pcidev* p)
{
	int i;
	Pcidev *t;

	if(pcicfgmode == -1)
		pcicfginit();
	if(p == nil) {
		p = pciroot;
		print("bus dev type vid  did intl memory\n");
	}
	for(t = p; t != nil; t = t->link) {
		print("%d  %2d/%d %.2ux %.2ux %.2ux %.4ux %.4ux %2d  ",
			BUSBNO(t->tbdf), BUSDNO(t->tbdf), BUSFNO(t->tbdf),
			t->ccrb, t->ccru, t->ccrp, t->vid, t->did, t->intl);

		for(i = 0; i < nelem(p->mem); i++) {
			if(t->mem[i].size == 0)
				continue;
			print("%d:%.8lux %d ", i,
				t->mem[i].bar, t->mem[i].size);
		}
		print("\n");
	}
	while(p != nil) {
		if(p->bridge != nil)
			pcihinv(p->bridge);
		p = p->link;
	}
}

void
pcireset(void)
{
	Pcidev *p;
	int pcr;

	if(pcicfgmode == -1)
		pcicfginit();

	for(p = pcilist; p != nil; p = p->list){
		pcr = pcicfgr16(p, PciPSR);
		pcicfgw16(p, PciPSR, pcr & ~0x04);
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
