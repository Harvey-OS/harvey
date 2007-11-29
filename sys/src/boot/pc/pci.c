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
		 * Find PCI-PCI and PCI-Cardbus bridges
		 * and recursively descend the tree.
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
			pciscan(sbn, &p->bridge);
		}
	}

	return maxubn;
}

static uchar
null_link(Pcidev *, uchar )
{
	return 0;
}

static void
null_init(Pcidev *, uchar , uchar )
{
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

typedef struct {
	ushort	sb_vid, sb_did;
	uchar	(*sb_translate)(Pcidev *, uchar);
	void	(*sb_initialize)(Pcidev *, uchar, uchar);
} bridge_t;

static bridge_t southbridges[] = {
	{ 0x8086, 0x122e, pIIx_link, pIIx_init },	// Intel 82371FB
	{ 0x8086, 0x1234, pIIx_link, pIIx_init },	// Intel 82371MX
	{ 0x8086, 0x7000, pIIx_link, pIIx_init },	// Intel 82371SB
	{ 0x8086, 0x7110, pIIx_link, pIIx_init },	// Intel 82371AB
	{ 0x8086, 0x7198, pIIx_link, pIIx_init },	// Intel 82443MX (fn 1)
	{ 0x8086, 0x2410, pIIx_link, pIIx_init },	// Intel 82801AA
	{ 0x8086, 0x2420, pIIx_link, pIIx_init },	// Intel 82801AB
	{ 0x8086, 0x2440, pIIx_link, pIIx_init },	// Intel 82801BA
	{ 0x8086, 0x244c, pIIx_link, pIIx_init },	// Intel 82801BAM
	{ 0x8086, 0x2480, pIIx_link, pIIx_init },	// Intel 82801CA
	{ 0x8086, 0x248c, pIIx_link, pIIx_init },	// Intel 82801CAM
	{ 0x8086, 0x24c0, pIIx_link, pIIx_init },	// Intel 82801DBL
	{ 0x8086, 0x24cc, pIIx_link, pIIx_init },	// Intel 82801DBM
	{ 0x8086, 0x24d0, pIIx_link, pIIx_init },	// Intel 82801EB
	{ 0x8086, 0x2640, pIIx_link, pIIx_init },	// Intel 82801FB
	{ 0x8086, 0x27b8, pIIx_link, pIIx_init },	// Intel 82801GB
	{ 0x8086, 0x27b9, pIIx_link, pIIx_init },	// Intel 82801GBM
	{ 0x1106, 0x0586, via_link, via_init },		// Viatech 82C586
	{ 0x1106, 0x0596, via_link, via_init },		// Viatech 82C596
	{ 0x1106, 0x0686, via_link, via_init },		// Viatech 82C686
	{ 0x1106, 0x3227, via_link, via_init },		// Viatech VT8237
	{ 0x1045, 0xc700, opti_link, opti_init },	// Opti 82C700
	{ 0x10b9, 0x1533, ali_link, ali_init },		// Al M1533
	{ 0x1039, 0x0008, pIIx_link, pIIx_init },	// SI 503
	{ 0x1039, 0x0496, pIIx_link, pIIx_init },	// SI 496
	{ 0x1078, 0x0100, cyrix_link, cyrix_init },	// Cyrix 5530 Legacy

	{ 0x1002, 0x4377, nil, nil },		// ATI Radeon Xpress 200M
	{ 0x1002, 0x4372, nil, nil },		// ATI SB400
	{ 0x1022, 0x746B, nil, nil },		// AMD 8111
	{ 0x10DE, 0x00D1, nil, nil },		// NVIDIA nForce 3
	{ 0x10DE, 0x00E0, nil, nil },		// NVIDIA nForce 3 250 Series
	{ 0x1166, 0x0200, nil, nil },		// ServerWorks ServerSet III LE
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
