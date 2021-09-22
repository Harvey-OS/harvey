#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include "apic.h"
#include "io.h"

typedef struct Rdt Rdt;
struct Rdt {
	Apic*	apic;
	int	devno;
	int	intin;
	u32int	lo;

	Rdt*	next;				/* on this bus */
};

enum {						/* IOAPIC registers */
	Ioregsel	= 0x00,			/* indirect register address */
	Iowin		= 0x04,			/* indirect register data */
	Ioipa		= 0x08,			/* IRQ Pin Assertion */
	Ioeoi		= 0x10,			/* EOI */

	Ioapicid	= 0x00,			/* Identification */
	Ioapicver	= 0x01,			/* Version */
	Ioapicarb	= 0x02,			/* Arbitration */
	Ioabcfg		= 0x03,			/* Boot Coniguration */
	Ioredtbl	= 0x10,			/* Redirection Table */
};

extern int mpisabusno;

static Rdt rdtarray[Nrdt];
static int nrdtarray;
static int gsib;
static Rdt* rdtbus[Nbus];
static Rdt* rdtvecno[IdtMAX+1];

static Lock idtnolock;
static uint idtno = IdtIOAPIC;

static u32int
rtblr(u32int *addr, int sel, u32int *valp)
{
	u32int r;

	*(addr+Ioregsel) = sel;
	coherence();
	r = *(addr+Iowin);
	if(valp)
		*valp = r;
	coherence();
	return r;
}

static void
rtblget(Apic* apic, int sel, u32int* hi, u32int* lo)
{
	sel = Ioredtbl + 2*sel;
	rtblr(apic->addr, sel+1, hi);
	rtblr(apic->addr, sel, lo);
}

static void
rtblw(u32int *addr, int sel, u32int val)
{
	*(addr+Ioregsel) = sel;
	coherence();
	*(addr+Iowin) = val;
	coherence();
}

/* sel is irq */
static void
rtblput(Apic* apic, int sel, u32int hi, u32int lo)
{
	sel = Ioredtbl + 2*sel;
	rtblw(apic->addr, sel+1, hi);
	rtblw(apic->addr, sel, lo);
}

void
ioapicintrinit(int busno, int apicno, int intin, int devno, u32int lo)
{
	Rdt *rdt;
	Apic *apic;

	if(busno >= Nbus || apicno >= Napic || nrdtarray >= Nrdt)
		return;
	apic = &ioapic[apicno];
	if(!apic->useable || intin >= apic->nrdt)
		return;

	rdt = &rdtarray[nrdtarray++];
	rdt->apic = apic;
	rdt->devno = devno;
	rdt->intin = intin;
	rdt->lo = lo;
	rdt->next = rdtbus[busno];
	rdtbus[busno] = rdt;
}

void
ioapicinit(int id, uintmem pa)
{
	int i;
	u32int r;
	Apic *apic;

	/*
	 * Mark the IOAPIC useable if it has a good ID
	 * and the registers can be mapped.
	 */
	if((uint)id >= Napic)
		return;

	apic = &ioapic[id];
	if(apic->useable || (apic->addr = vmap(pa, 1024)) == nil)
		return;
	apic->useable = 1;

	/*
	 * Initialise the IOAPIC.
	 * The MultiProcessor Specification says it is the
	 * responsibility of the O/S to set the APIC ID.
	 */
	ilock(apic);
	r = rtblr(apic->addr, Ioapicver, &r);
	apic->nrdt = ((r>>16) & 0xff) + 1;
	apic->gsib = gsib;
	gsib += apic->nrdt;
	coherence();
	rtblw(apic->addr, Ioapicid, id<<24);

	/* clear apic's redirection table */
	for(i = 0; i < apic->nrdt; i++)
		rtblput(apic, i, 0, Im);
	iunlock(apic);
}

void
ioapicdump(void)
{
	int i, n;
	Rdt *rdt;
	Apic *apic;
	u32int hi, lo;

	if(!DBGFLG)
		return;

	for(i = 0; i < Napic; i++){
		apic = &ioapic[i];
		if(!apic->useable || apic->addr == 0)
			continue;
		DBG("ioapic %d addr %#p nrdt %d gsib %d\n",
			i, apic->addr, apic->nrdt, apic->gsib);
		for(n = 0; n < apic->nrdt; n++){
			ilock(apic);
			rtblget(apic, n, &hi, &lo);
			iunlock(apic);
			DBG(" rdt %2.2d %#8.8ux %#8.8ux\n", n, hi, lo);
		}
	}
	for(i = 0; i < Nbus; i++){
		if((rdt = rdtbus[i]) == nil)
			continue;
		DBG("iointr bus %d:\n", i);
		while(rdt != nil){
			DBG(" apic %lld devno %#ux (%d %d) intin %d lo %#ux\n",
				(vlong)(rdt->apic - ioapic), rdt->devno,
				rdt->devno>>2, rdt->devno & 0x03, rdt->intin,
				rdt->lo);
			rdt = rdt->next;
		}
	}
}

/* clear all ioapic interrupt redirections */
void
ioapiconline(void)			/* unused, oddly */
{
	int i;
	Apic *apic;

	for(apic = ioapic; apic < &ioapic[Napic]; apic++){
		if(!apic->useable || apic->addr == nil)
			continue;
		ilock(apic);
		for(i = 0; i < apic->nrdt; i++)
			rtblput(apic, i, 0, Im);
		iunlock(apic);
	}
	if(DBGFLG)
		ioapicdump();
}

/*
 * set v->cpu and return its apicno.
 * cpu0 is assumed to be always up and usable.
 */
static int
usecpu(Vctl *v, uint cpu)
{
	int apicno;
	Mach *mp;

	for(; ; cpu = 0) {
		if (cpu >= nelem(sys->machptr))
			continue;
		mp = sys->machptr[cpu];
		if (mp == nil || !mp->online)
			continue;
		apicno = mp->apicno;
		if(xapic[apicno].useable && xapic[apicno].addr == 0)
			break;
	}
	v->cpu = cpu;
	return apicno;
}

enum {
	Noisecore,
	Roundrobin,

	Dfpolicy = Roundrobin,
};

int usbpresent;

/*
 * Set delivery mode (lo) and destination field (hi),
 * according to interrupt routing policy.
 */
static void
ioapicintrdd(u32int* hi, u32int* lo, Vctl *v)
{
	int cpu, noise, trips;
	Apic *apic;
	Mach *mp;
	static uint df;
	static Lock dflock;

	/*
	 * The bulk of this code was written ~1995, when there was
	 * one architecture and one generation of hardware, the number
	 * of CPUs was up to 4(8) and the choices for interrupt routing
	 * were physical, or flat logical (optionally with lowest
	 * priority interrupt). Logical mode hasn't scaled well with
	 * the increasing number of packages/cores/threads, so the
	 * fall-back is to physical mode, which works across all processor
	 * generations, both AMD and Intel, using the APIC and xAPIC.
	 */
	cpu = 0;
	switch(Dfpolicy){
	default:
	case Noisecore:
		/* noise core 0 */
		break;
	case Roundrobin:
		/*
		 * Assign each interrupt to a different CPU on a round-robin.
		 * Some idea of the packages/cores/thread topology would be
		 * useful here, e.g. to not assign interrupts to more than one
		 * thread in a core. But, as usual, Intel make that an onerous
		 * task.
		 */
		noise = sys->nmach - 1;

		/* put COMs & ether0 on cpu0 to simplify rebooting. */
		if(sys->nmach == 1 || strncmp(v->name, "COM", 3) == 0 ||
		    strcmp(v->name, "ether0") == 0)
			cpu = 0;
		/* on mp, assign all noisy intr sources (usb) to a noise core */
		else if (usbpresent && sys->nmach > 1 &&
		    strncmp(v->name, "usb", 3) == 0)
			cpu = noise;
		else {
			lock(&dflock);
			trips = 0;
			for(;;){
				mp = sys->machptr[df];
				cpu = df++;
				df %= MACHMAX;
				if (++trips > 128) {
					cpu = 0;
					break;
				}
				/* this is non-usb, so skip the noise core */
				if(usbpresent && sys->nonline > 1 &&
				    cpu == noise)
					continue;
				/* don't allow pci devs on cpu0 normally */
				if(sys->nmach > 2 && cpu == 0 &&
				    BUSTYPE(v->tbdf) == BusPCI)
					continue;

				if(mp && mp->online) {
					apic = &xapic[mp->apicno];
					if(apic->useable && apic->addr == 0)
						break;
				}
			}
			unlock(&dflock);
		}
		break;
	}
	*hi = usecpu(v, cpu) << 24;
	*lo |= Pm | MTf;
	coherence();
	// print("\nioapicintrdd hi %#ux lo %#ux\n", *hi, *lo);
}

typedef struct Buggymsi Buggymsi;
struct Buggymsi {
	ushort	vid;		/* pci vendor id */
	ushort	did;		/* " device " */
};

static Buggymsi buggymsi[] = {
			/* 951 msi generates lapic errors & spurious intrs */
	0x144d, 0xa802,	/* Samsung NVMe SSD SM951/PM951 (m.2 via pcie adapter) */
	Vintel, 0x10a4,	/* intel 82571s, from intel erratum 63 */
	Vintel, 0x105e,
	Vintel, 0x107d,	/* intel 82572s, " */
	Vintel, 0x107e,
	Vintel, 0x107f,
	Vintel, 0x10b9,
};

static char *msiconf;

/*
 * use message-signalled interrupt if available.  msi and msi-x may both
 * be available (e.g. pci-e devices), but only one can be used on a given
 * device.  msi-x seems much more complicated for little benefit.
 */
static int
trymsi(Vctl *v, Pcidev *pcidev)
{
	int ptr, ptrx, pcie;
	Buggymsi *bm;
	Msi msi;

	v->ismsi = 0;
	if (msiconf == nil)
		msiconf = getconf("*msiconf");
	if (msiconf && strcmp(msiconf, "no") == 0)
		return -1;
	memset(&msi, 0, sizeof msi);
	ptr = pcigetmsi(pcidev, &msi);
	ptrx = pcigetmsixcap(pcidev);
	pcie = pcigetpciecap(pcidev);
	if (ptr == -1) {
		if (ptrx >= 0)
			print("%s: msi-x offered but not msi! using neither\n",
				v->name);
		if (pcie >= 0)
			print("%s: %T: vid %ux did %ux pci-e device NOT "
				"offering msi, but it's required for pci-e!\n",
				v->name, pcidev->tbdf, pcidev->vid, pcidev->did);
		/* else a pci device, so msi not mandatory */
		return -1;		/* sorry, no msi */
	}

	if (msiconf && strcmp(msiconf, "print") == 0) {
		print("%s: %T: vid %ux did %ux offering msi\n",
			v->name, pcidev->tbdf, pcidev->vid, pcidev->did);
		delay(20);
	}
	for (bm = buggymsi; bm < buggymsi + nelem(buggymsi); bm++)
		if (pcidev->vid == bm->vid && pcidev->did == bm->did) {
			print("%s: %T: vid %ux did %ux known to be buggy; "
				"refusing msi\n", v->name, pcidev->tbdf,
				pcidev->vid, pcidev->did);
			return -1;
		}

	v->ismsi = 1;
	msi.ctl	&= ~Msimme;		/* single-message msi */
	msi.ctl |= Msienable;
	msi.addr = (uvlong)(uintptr)Lapicphys | v->lapic<<12 | Msirhtocpuid;
	msi.data = Msitrglevel | Msitrglvlassert | Msidlvfixed | v->vno;
	pcisetmsi(pcidev, &msi);

	DBG("%s: msi enabled at vector %d lapic %d for %T\n",
		v->name, v->vno, v->lapic, pcidev->tbdf);
	delay(20);
	return v->vno;
}

static Rdt *
getrdtbus(int busno, int devno)
{
	Rdt *rdt;

	for(rdt = rdtbus[busno]; rdt != nil; rdt = rdt->next)
		if(rdt->devno == devno)
			return rdt;
	return nil;
}

int vecinuse(uint);	/* trap.c */

/* return a unique vector number; we don't share vectors in 9k */
static int
newvecno(Rdt *rdt)
{
	int vecno, firstvec, first;

	if(rdt->lo & 0xff)
		return -1;	/* assume rdt->lo is already set to vector */

	lock(&idtnolock);
	firstvec = idtno;
	first = 1;
	do {
		vecno = idtno;
		if (vecno != firstvec)
			first = 0;
		idtno = (idtno+8) % IdtMAX;
		if(idtno < IdtIOAPIC)
			idtno += IdtIOAPIC;
	} while (vecinuse(vecno) && (first || firstvec != vecno));
	unlock(&idtnolock);
	if (!first && firstvec == vecno)
		panic("newvecno: all vectors in use");
	rdt->lo |= vecno;
	rdtvecno[vecno] = rdt;
	return vecno;
}

static int
isrbusdev(Vctl *v, int *busnop, int *devnop)
{
	int irq;

	irq = v->irq;
	if(irq >= IdtLINT0 && irq <= IdtSPURIOUS){
		if(irq != IdtSPURIOUS)
			v->isr = apiceoi;
		return 0;
	} else {
		/*
		 * Legacy ISA.  Make a busno and devno using the
		 * ISA bus number and the irq.
		 */
		if(mpisabusno == -1)
			panic("no ISA bus allocated");
		*busnop = mpisabusno;
		*devnop = irq<<2;
		return -1;
	}
}

/*
 * Make a devno from BUSDNO(tbdf) and pcidev->intp.
 */
static Pcidev *
pcibusdev(Vctl *v, int *busnop, int *devnop)
{
	int busno, devno;
	Pcidev *pcidev;

	busno = BUSBNO(v->tbdf);
	if((pcidev = pcimatchtbdf(v->tbdf)) == nil)
		panic("no PCI dev for tbdf %#8.8ux", v->tbdf);
	if((devno = pcicfgr8(pcidev, PciINTP)) == 0)
		panic("no INTP for tbdf %#8.8ux", v->tbdf);
	devno = BUSDNO(v->tbdf)<<2 | (devno-1);
	DBG("ioapicintrenable: tbdf %#8.8ux busno %d devno %d\n",
		v->tbdf, busno, devno);
	*busnop = busno;
	*devnop = devno;
	return pcidev;
}

static Rdt *
ioapicrdtbusdev(Vctl *v, int *busnop, int *devnop)
{
	int busno, devno;
	Rdt *rdt;

	busno = *busnop;
	devno = *devnop;
	rdt = getrdtbus(busno, devno);
	if(rdt == nil) {
		/*
		 * First crack in the smooth exterior of the new code:
		 * some BIOS make an MPS table where the PCI devices are
		 * just defaulted to ISA.
		 * Rewrite this to be cleaner.
		 */
		if((busno = mpisabusno) >= 0) {
			devno = v->irq<<2;
			rdt = getrdtbus(busno, devno);
			DBG("isa: tbdf %#8.8ux busno %d devno %d %#p\n",
				v->tbdf, busno, devno, rdt);
			if (rdt == nil)
				print("ioapicrdtbusdev: nil getrdtbus %d %d\n",
					busno, devno);
		}
		*busnop = busno;
		*devnop = devno;
	}
	return rdt;
}

static int
in8259range(int irq)
{
	/* exclude 0, which probably indicates a missing value, not a timer */
	return irq > 0 && irq <= 15;
}

static int
novect(Vctl *v)
{
	print("ioapicintrenable: newvecno failed to assign vector for %s %T\n",
		v->name, v->tbdf);
	return -1;
}

int
ioapicintrenable(Vctl* v)
{
	Rdt *rdt;
	u32int hi, lo;
	int busno, devno, vecno;
	Pcidev *pcidev = nil;

	/*
	 * Bridge between old and unspecified new scheme,
	 * the work in progress...
	 */
	busno = devno = -1;
	if(v->tbdf == BUSUNKNOWN) {
		if (isrbusdev(v, &busno, &devno) == 0) {
			if (vecinuse(v->irq))
				print("isrbusdev vecno %d in use for %s %T\n",
					v->irq, v->name, v->tbdf);
			return v->irq;
		}
	} else if(BUSTYPE(v->tbdf) == BusPCI)
		pcidev = pcibusdev(v, &busno, &devno);
	else
		panic("ioapicintrenable: unknown tbdf %#8.8ux", v->tbdf);
	rdt = ioapicrdtbusdev(v, &busno, &devno);
//	if(rdt == nil && BUSTYPE(v->tbdf) != BusPCI)
	if(rdt == nil)
		return -1;

	/*
	 * Second crack:
	 * what to do about devices that intrenable/intrdisable frequently?
	 * 1) there is no ioapicdisable yet;
	 * 2) it would be good to reuse freed vectors.
	 * Oh bugger.
	 */
	/*
	 * This is a low-frequency event so just lock
	 * the whole IOAPIC to initialise the RDT entry
	 * rather than putting a Lock in each entry.
	 */
	lock(rdt->apic);
	vecno = newvecno(rdt); /* updates rdt->lo with vecno, if rdt->lo is 0 */
	if (vecno < 0)
		if (rdt->lo & 0xff) {
			/*
			 * ugh.  the vector was probably in use, so try
			 * assigning a new one, overriding rdt->lo.
			 */
			rdt->lo &= ~0xff;
			vecno = newvecno(rdt);
			if (vecno < 0)
				return novect(v);
		} else
			return novect(v);

	/* we trust that vecno & rdt->lo&0xff are a valid vector */
	lo = rdt->lo & ~Im;
	ioapicintrdd(&hi, &lo, v);		/* assign a cpu */
	v->vno = vecno;
	v->lapic = hi>>24;
	if (pcidev && trymsi(v, pcidev) >= 0)
		pcinointrs(pcidev);	/* msi on: no legacy pci interrupts */
	else {
		/* no msi: redirect int to this apic */
		rtblput(rdt->apic, rdt->intin, hi, lo);		/**/
		vecno = lo & 0xff;
		if (pcidev) {
			pcimsioff(v, pcidev);  /* make sure msi is really off */
			pciintrs(pcidev);
		}
	}
	unlock(rdt->apic);
	DBG("busno %d devno %d hi %#8.8ux lo %#8.8ux vecno %d\n",
		busno, devno, hi, lo, vecno);

	if (vecno >= 0) {
		v->isr = apicisr;
		v->eoi = apiceoi;
		if (!in8259range(v->irq))
			v->irq = vecno;
		if (pcidev && !in8259range(pcidev->intl))
			pcidev->intl = vecno;
	}
	/* else isr and eoi default to a nop */
	v->vno = vecno;
	return vecno;
}

/* run through the active ioapics and disable redirections to vector vno */
void
ioapicrtbloff(uint vno)
{
	int i;
	u32int hi, lo;
	Apic *apic;

	for(apic = ioapic; apic < &ioapic[Napic]; apic++)
		if(apic->useable && apic->addr != nil) {
			ilock(apic);
			for(i = 0; i < apic->nrdt; i++) {
				rtblget(apic, i, &hi, &lo);
				if ((lo & MASK(8)) == vno) {
					iprint("apic %#p vector %d was "
						"redirected to %d\n",
						apic, i, vno);
					rtblput(apic, i, 0, 0|Im);
				}
			}
			iunlock(apic);
		}
}

int
ioapicintrtbloff(uint vecno)
{
	Apic *apic;
	Rdt *rdt;

	if(vecno > IdtMAX)
		return -1;
	if((rdt = rdtvecno[vecno]) == nil){
		ioapicrtbloff(vecno);
	} else {
		apic = rdt->apic;
		ilock(apic);
		rtblput(apic, rdt->intin, 0, rdt->lo|Im);
		iunlock(apic);
	}
	return 0;
}

int
ioapicintrdisable(uint vecno)
{
	Apic *apic;
	Rdt *rdt;

	/*
	 * FOV. Oh dear. This isn't very good.
	 * Fortunately rdtvecno[vecno] is static
	 * once assigned.
	 * Must do better.
	 *
	 * What about any pending interrupts?
	 */
	if(vecno > IdtMAX){
		panic("ioapicintrdisable: vecno %d out of range", vecno);
		return -1;
	}
	if((rdt = rdtvecno[vecno]) == nil){
		panic("ioapicintrdisable: vecno %d has no rdt", vecno);
		return -1;
	}
	apic = rdt->apic;
	ilock(apic);
	rtblput(apic, rdt->intin, 0, rdt->lo|Im);
	iunlock(apic);

	return 0;
}
