/*
 * PCI support code.
 * To do:
 *	initialise bridge mappings if the PCI BIOS didn't.
 */
#include "u.h"
#include "lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "error.h"

enum {					/* configuration mechanism #1 */
	PciADDR		= 0xCF8,	/* CONFIG_ADDRESS */
	PciDATA		= 0xCFC,	/* CONFIG_DATA */

					/* configuration mechanism #2 */
	PciCSE		= 0xCF8,	/* configuration space enable */
	PciFORWARD	= 0xCFA,	/* which bus */

	MaxFNO		= 7,
	MaxUBN		= 255,
};

static Lock pcicfglock;
static Lock pcicfginitlock;
static int pcicfgmode = -1;
static int pcimaxbno = 7;
static int pcimaxdno;
static Pcidev* pciroot;
static Pcidev* pcilist;
static Pcidev* pcitail;

static int pcicfgrw32(int, int, int, int);
static int pcicfgrw8(int, int, int, int);

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

int
pciscan(int bno, Pcidev** list)
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
			switch(p->ccrb){

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
				if((hdt & 0x7F) != 0)
					break;
				rno = PciBAR0 - 4;
				for(i = 0; i < nelem(p->mem); i++){
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
		 * Find PCI-PCI and PCI-Cardbus bridges and recursively descend the tree.
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
		ubn = pcicfgr8(p, PciUBN);
		sbn = pcicfgr8(p, PciSBN);

		if(sbn == 0 || ubn == 0){
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

static uchar 
pIIx_link(Pcidev *router, uchar link)
{
	uchar pirq;

	/* link should be 0x60, 0x61, 0x62, 0x63 */
	pirq = pcicfgr8(router, link);
	return (pirq < 16)? pirq: 0;
}

static void 
pIIx_init(Pcidev *router, uchar link, uchar irq)
{
	pcicfgw8(router, link, irq);
}

static uchar 
via_link(Pcidev *router, uchar link)
{
	uchar pirq;

	/* link should be 1, 2, 3, 5 */
	pirq = (link < 6)? pcicfgr8(router, 0x55 + (link>>1)): 0;

	return (link & 1)? (pirq >> 4): (pirq & 15);
}

static void 
via_init(Pcidev *router, uchar link, uchar irq)
{
	uchar pirq;

	pirq = pcicfgr8(router, 0x55 + (link >> 1));
	pirq &= (link & 1)? 0x0f: 0xf0;
	pirq |= (link & 1)? (irq << 4): (irq & 15);
	pcicfgw8(router, 0x55 + (link>>1), pirq);
}

static uchar 
opti_link(Pcidev *router, uchar link)
{
	uchar pirq = 0;

	/* link should be 0x02, 0x12, 0x22, 0x32 */
	if ((link & 0xcf) == 0x02)
		pirq = pcicfgr8(router, 0xb8 + (link >> 5));
	return (link & 0x10)? (pirq >> 4): (pirq & 15);
}

static void 
opti_init(Pcidev *router, uchar link, uchar irq)
{
	uchar pirq;

	pirq = pcicfgr8(router, 0xb8 + (link >> 5));
    	pirq &= (link & 0x10)? 0x0f : 0xf0;
    	pirq |= (link & 0x10)? (irq << 4): (irq & 15);
	pcicfgw8(router, 0xb8 + (link >> 5), pirq);
}

static uchar 
ali_link(Pcidev *router, uchar link)
{
	/* No, you're not dreaming */
	static const uchar map[] = { 0, 9, 3, 10, 4, 5, 7, 6, 1, 11, 0, 12, 0, 14, 0, 15 };
	uchar pirq;

	/* link should be 0x01..0x08 */
	pirq = pcicfgr8(router, 0x48 + ((link-1)>>1));
	return (link & 1)? map[pirq&15]: map[pirq>>4];
}

static void 
ali_init(Pcidev *router, uchar link, uchar irq)
{
	/* Inverse of map in ali_link */
	static const uchar map[] = { 0, 8, 0, 2, 4, 5, 7, 6, 0, 1, 3, 9, 11, 0, 13, 15 };
	uchar pirq;

	pirq = pcicfgr8(router, 0x48 + ((link-1)>>1));
	pirq &= (link & 1)? 0x0f: 0xf0;
	pirq |= (link & 1)? (map[irq] << 4): (map[irq] & 15);
	pcicfgw8(router, 0x48 + ((link-1)>>1), pirq);
}

static uchar 
cyrix_link(Pcidev *router, uchar link)
{
	uchar pirq;

	/* link should be 1, 2, 3, 4 */
	pirq = pcicfgr8(router, 0x5c + ((link-1)>>1));
	return ((link & 1)? pirq >> 4: pirq & 15);
}

static void 
cyrix_init(Pcidev *router, uchar link, uchar irq)
{
	uchar pirq;

	pirq = pcicfgr8(router, 0x5c + (link>>1));
	pirq &= (link & 1)? 0x0f: 0xf0;
	pirq |= (link & 1)? (irq << 4): (irq & 15);
	pcicfgw8(router, 0x5c + (link>>1), pirq);
}

enum {
	Intel = 0x8086,		
		Intel_82371FB_0 = 0x122e,
		Intel_82371MX_0 = 0x1234,
		Intel_82371SB_0 = 0x7000,
		Intel_82371AB_0 = 0x7110,
		Intel_82443MX_1 = 0x7198,
		Intel_82801AA_0 = 0x2410,
		Intel_82801AB_0 = 0x2420,
		Intel_82801BA_0 = 0x2440,
		Intel_82801BAM_0 = 0x244c,
		Intel_82801CAM_0 = 0x248c,
		Intel_82801DBM_0 = 0x24cc,
		Intel_82801EB_0 = 0x24d0,
		Intel_82801FB_0 = 0x2640,
	Viatech = 0x1106,
		Via_82C586_0 = 0x0586,
		Via_82C596 = 0x0596,
		Via_82C686 = 0x0686,
	Opti = 0x1045,
		Opti_82C700 = 0xc700,
	Al = 0x10b9,
		Al_M1533 = 0x1533,
	SI = 0x1039,
		SI_503 = 0x0008,
		SI_496 = 0x0496,
	Cyrix = 0x1078,
		Cyrix_5530_Legacy = 0x0100,
};

typedef struct {
	ushort	sb_vid, sb_did;
	uchar	(*sb_translate)(Pcidev *, uchar);
	void	(*sb_initialize)(Pcidev *, uchar, uchar);	
} bridge_t;

static bridge_t southbridges[] = {
{	Intel, Intel_82371FB_0,		pIIx_link,	pIIx_init },
{	Intel, Intel_82371MX_0,		pIIx_link,	pIIx_init },
{	Intel, Intel_82371SB_0,		pIIx_link,	pIIx_init },
{	Intel, Intel_82371AB_0,		pIIx_link,	pIIx_init },
{	Intel, Intel_82443MX_1,		pIIx_link,	pIIx_init },
{	Intel, Intel_82801AA_0,		pIIx_link,	pIIx_init },
{	Intel, Intel_82801AB_0,		pIIx_link,	pIIx_init },
{	Intel, Intel_82801BA_0,		pIIx_link,	pIIx_init },
{	Intel, Intel_82801BAM_0,	pIIx_link,	pIIx_init },
{	Intel, Intel_82801CAM_0,	pIIx_link,	pIIx_init },
{	Intel, Intel_82801DBM_0,	pIIx_link,	pIIx_init },
{	Intel, Intel_82801EB_0,		pIIx_link,	pIIx_init },
{	Intel, Intel_82801FB_0,		pIIx_link,	pIIx_init },
{	Viatech, Via_82C586_0,		via_link,	via_init },
{	Viatech, Via_82C596,		via_link,	via_init },
{	Viatech, Via_82C686,		via_link,	via_init },
{	Opti, Opti_82C700,		opti_link,	opti_init },
{	Al, Al_M1533,			ali_link,	ali_init },
{	SI, SI_503,			pIIx_link,	pIIx_init },
{	SI, SI_496,			pIIx_link,	pIIx_init },
{	Cyrix, Cyrix_5530_Legacy,	cyrix_link,	cyrix_init }
};

typedef struct {
	uchar	e_bus;			// Pci bus number
	uchar	e_dev;			// Pci device number
	uchar	e_maps[12];		// Avoid structs!  Link and mask.
	uchar	e_slot;			// Add-in/built-in slot
	uchar	e_reserved;
} slot_t;

typedef struct {
	uchar	rt_signature[4];	// Routing table signature
	uchar	rt_version[2];		// Version number
	uchar	rt_size[2];			// Total table size
	uchar	rt_bus;			// Interrupt router bus number
	uchar	rt_devfn;			// Router's devfunc
	uchar	rt_pciirqs[2];		// Exclusive PCI irqs
	uchar	rt_compat[4];		// Compatible PCI interrupt router
	uchar	rt_miniport[4];		// Miniport data
	uchar	rt_reserved[11];
	uchar	rt_checksum;
} router_t;

static ushort pciirqs;			// Exclusive PCI irqs
static bridge_t *southbridge;	// Which southbridge to use.

static void
pcirouting(void)
{
	uchar *p, pin, irq;
	ulong tbdf, vdid;
	ushort vid, did;
	router_t *r;
	slot_t *e;
	int size, i, fn;
	Pcidev *sbpci, *pci;

	// Peek in the BIOS
	for (p = (uchar *)KADDR(0xf0000); p < (uchar *)KADDR(0xfffff); p += 16)
		if (p[0] == '$' && p[1] == 'P' && p[2] == 'I' && p[3] == 'R')
			break;

	if (p >= (uchar *)KADDR(0xfffff))
		return;

	r = (router_t *)p;

	// print("PCI interrupt routing table version %d.%d at %.6uX\n",
	// 	r->rt_version[0], r->rt_version[1], (ulong)r & 0xfffff);

	tbdf = (BusPCI << 24)|(r->rt_bus << 16)|(r->rt_devfn << 8);
	vdid = pcicfgrw32(tbdf, PciVID, 0, 1);
	vid = vdid;
	did = vdid >> 16;

	for (i = 0; i != nelem(southbridges); i++)
		if (vid == southbridges[i].sb_vid && did == southbridges[i].sb_did)
			break;

	if (i == nelem(southbridges)) {
		print("pcirouting: South bridge %.4uX, %.4uX not found\n", vid, did);
		return;
	}
	southbridge = &southbridges[i];

	if ((sbpci = pcimatch(nil, vid, did)) == nil) {
		print("pcirouting: Cannot match south bridge %.4uX, %.4uX\n",
			  vid, did);
		return;
	}
	
	pciirqs = (r->rt_pciirqs[1] << 8)|r->rt_pciirqs[0];

	size = (r->rt_size[1] << 8)|r->rt_size[0];
	for (e = (slot_t *)&r[1]; (uchar *)e < p + size; e++) {
		// print("%.2uX/%.2uX %.2uX: ", e->e_bus, e->e_dev, e->e_slot);
		// for (i = 0; i != 4; i++) {
		// 	uchar *m = &e->e_maps[i * 3];
		// 	print("[%d] %.2uX %.4uX ",
		// 		i, m[0], (m[2] << 8)|m[1]);
		// }
		// print("\n");

		for (fn = 0; fn != 8; fn++) {
			uchar *m;

			// Retrieve the did and vid through the devfn before
			// obtaining the Pcidev structure.
			tbdf = (BusPCI << 24)|(e->e_bus << 16)|((e->e_dev | fn) << 8);
			vdid = pcicfgrw32(tbdf, PciVID, 0, 1);
			if (vdid == 0xFFFFFFFF || vdid == 0) 
				continue;

			vid = vdid;
			did = vdid >> 16;
	
			pci = nil;
			while ((pci = pcimatch(pci, vid, did)) != nil) {

				if (pci->intl != 0 && pci->intl != 0xFF)
					continue;

				pin = pcicfgr8(pci, PciINTP);
				if (pin == 0 || pin == 0xff) 
					continue;
	
				m = &e->e_maps[(pin - 1) * 3];
				irq = southbridge->sb_translate(sbpci, m[0]);
				if (irq) {
					print("pcirouting: %.4uX/%.4uX at pin %d irq %d\n", 
						  vid, did, pin, irq);
					pcicfgw8(pci, PciINTL, irq);
					pci->intl = irq;
				}
			}
		}
	}
}

static void
pcicfginit(void)
{
	char *p;
	int bno, n;
	Pcidev **list;

	lock(&pcicfginitlock);
	if(pcicfgmode != -1)
		goto out;

	/*
	 * Try to determine which PCI configuration mode is implemented.
	 * Mode2 uses a byte at 0xCF8 and another at 0xCFA; Mode1 uses
	 * a DWORD at 0xCF8 and another at 0xCFC and will pass through
	 * any non-DWORD accesses as normal I/O cycles. There shouldn't be
	 * a device behind these addresses so if Mode1 accesses fail try
	 * for Mode2 (Mode2 is deprecated).
	 */

	/*
	 * Bits [30:24] of PciADDR must be 0,
	 * according to the spec.
	 */
	n = inl(PciADDR);
	if(!(n & 0x7FF00000)){
		outl(PciADDR, 0x80000000);
		outb(PciADDR+3, 0);
		if(inl(PciADDR) & 0x80000000){
			pcicfgmode = 1;
			pcimaxdno = 31;
		}
	}
	outl(PciADDR, n);

	if(pcicfgmode < 0){
		/*
		 * The 'key' part of PciCSE should be 0.
		 */
		n = inb(PciCSE);
		if(!(n & 0xF0)){
			outb(PciCSE, 0x0E);
			if(inb(PciCSE) == 0x0E){
				pcicfgmode = 2;
				pcimaxdno = 15;
			}
		}
		outb(PciCSE, n);
	}
	
	if(pcicfgmode < 0)
		goto out;


	if(p = getconf("*pcimaxbno"))
		pcimaxbno = strtoul(p, 0, 0);
	if(p = getconf("*pcimaxdno"))
		pcimaxdno = strtoul(p, 0, 0);

	list = &pciroot;
	for(bno = 0; bno <= pcimaxbno; bno++) {
		bno = pciscan(bno, list);
		while(*list)
			list = &(*list)->link;
	}

	pcirouting();

out:
	unlock(&pcicfginitlock);

	if(getconf("*pcihinv"))
		pcihinv(nil);
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

	while(prev != nil) {
		if((vid == 0 || prev->vid == vid)
		&& (did == 0 || prev->did == did))
			break;
		prev = prev->list;
	}
	return prev;
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

static ushort
pciimask(Pcidev *pci)
{
	ushort imask;

	imask = 0;
	while (pci) {
		if (pcicfgr8(pci, PciINTP) && pci->intl < 16)
			imask |= 1 << pci->intl;

		if (pci->bridge)
			imask |= pciimask(pci->bridge);

		pci = pci->list;
	}
	return imask;
}

uchar
pciintl(Pcidev *pci)
{
	ushort imask;
	int i;

	if (pci == nil)
		pci = pcilist;

	imask = pciimask(pci) | 1;
	for (i = 0; i != 16; i++)
		if ((imask & (1 << i)) == 0)
			return i;
	return 0;
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
		print("%d  %2d/%d %.2ux %.2ux %.2ux %.4ux %.4ux %3d  ",
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
	pcr |= 0x0004;
	pcicfgw16(p, PciPCR, pcr);
}

