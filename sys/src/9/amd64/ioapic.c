/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include "apic.h"
#include "io.h"

typedef struct Rbus Rbus;
typedef struct Rdt Rdt;

struct Rbus {
	Rbus	*next;
	int	devno;
	Rdt	*rdt;
};

struct Rdt {
	Apic	*apic;
	int	intin;
	uint32_t	lo;

	int	ref;				/* could map to multiple busses */
	int	enabled;				/* times enabled */
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

static Rdt rdtarray[Nrdt];
static int nrdtarray;
static int gsib;
static Rbus* rdtbus[Nbus];
static Rdt* rdtvecno[IdtMAX+1];

static Lock idtnolock;
static int idtno = IdtIOAPIC;

Apic	xioapic[Napic];

static uint32_t ioapicread(Apic*apic, int reg)
{
	volatile uint32_t *sel = apic->Ioapic.addr+Ioregsel;
	volatile uint32_t *data = apic->Ioapic.addr+Iowin;
	*sel = reg;
	return *data;
}
static void ioapicwrite(Apic*apic, int reg, uint32_t val)
{
	volatile uint32_t *sel = apic->Ioapic.addr+Ioregsel;
	volatile uint32_t *data = apic->Ioapic.addr+Iowin;
	*sel = reg;
	*data = val;
}

static void
rtblget(Apic* apic, int sel, uint32_t* hi, uint32_t* lo)
{
	sel = Ioredtbl + 2*sel;

	*hi = ioapicread(apic, sel+1);
	*lo = ioapicread(apic, sel);
}

static void
rtblput(Apic* apic, int sel, uint32_t hi, uint32_t lo)
{
	sel = Ioredtbl + 2*sel;

	ioapicwrite(apic, sel+1, hi);
	ioapicwrite(apic, sel, lo);
}

Rdt*
rdtlookup(Apic *apic, int intin)
{
	int i;
	Rdt *r;

	for(i = 0; i < nrdtarray; i++){
		r = rdtarray + i;
		if(apic == r->apic && intin == r->intin)
			return r;
	}
	return nil;
}

void
ioapicintrinit(int busno, int apicno, int intin, int devno, uint32_t lo)
{
	Rbus *rbus;
	Rdt *rdt;
	Apic *apic;

	if(busno >= Nbus){
		print("ioapicintrinit: botch: Busno %d >= Nbus %d\n", busno, Nbus);
		return;
	}
	if (apicno >= Napic) {
		print("ioapicintrinit: botch: acpicno %d >= Napic %d\n", apicno, Napic);
		return;
	}
	if (nrdtarray >= Nrdt){
		print("ioapicintrinit: botch: nrdtarray %d >= Nrdt %d\n", nrdtarray, Nrdt);
		return;
	}

	apic = &xioapic[apicno];
	if(!apic->useable) {
		print("ioapicintrinit: botch: apic %d not marked usable\n", apicno);
		return;
	}
	if (intin >= apic->Ioapic.nrdt){
		print("ioapicintrinit: botch: initin %d >= apic->Ioapic.nrdt %d\n", intin, apic->Ioapic.nrdt);
		return;
	}

	rdt = rdtlookup(apic, intin);
	if(rdt == nil){
		rdt = &rdtarray[nrdtarray++];
		rdt->apic = apic;
		rdt->intin = intin;
		rdt->lo = lo;
	}else{
		if(lo != rdt->lo){
			print("mutiple irq botch bus %d %d/%d/%d lo %d vs %d\n",
				busno, apicno, intin, devno, lo, rdt->lo);
			return;
		}
		DBG("dup rdt %d %d %d %d %.8x\n", busno, apicno, intin, devno, lo);
	}
	rdt->ref++;
	rbus = malloc(sizeof *rbus);
	rbus->rdt = rdt;
	rbus->devno = devno;
	rbus->next = rdtbus[busno];
	rdtbus[busno] = rbus;
}

void
ioapicinit(int id, int ibase, uintptr_t pa)
{
	Apic *apic;

	/*
	 * Mark the IOAPIC useable if it has a good ID
	 * and the registers can be mapped.
	 */
	if(id >= Napic)

		return;

	apic = &xioapic[id];
	if(apic->useable || (apic->Ioapic.addr = vmap(pa, 1024)) == nil)
		return;
	apic->useable = 1;
	apic->Ioapic.paddr = pa;

	/*
	 * Initialise the I/O APIC.
	 * The MultiProcessor Specification says it is the
	 * responsibility of the O/S to set the APIC ID.
	 */
	lock(&apic->Ioapic.l);
	apic->Ioapic.nrdt = ((ioapicread(apic, Ioapicver)>>16) & 0xff) + 1;
	if (ibase == -1) {
		apic->Ioapic.gsib = gsib;
		gsib += apic->Ioapic.nrdt;
	} else {
		apic->Ioapic.gsib = ibase;
	}

	ioapicwrite(apic, Ioapicid, id<<24);
	unlock(&apic->Ioapic.l);
}

void
ioapicdump(void)
{
	int i, n;
	Rbus *rbus;
	Rdt *rdt;
	Apic *apic;
	uint32_t hi, lo;

	if(!DBGFLG)
		return;
	for(i = 0; i < Napic; i++){
		apic = &xioapic[i];
		if(!apic->useable || apic->Ioapic.addr == 0)
			continue;
		print("ioapic %d addr %#p nrdt %d gsib %d\n",
			i, apic->Ioapic.addr, apic->Ioapic.nrdt, apic->Ioapic.gsib);
		for(n = 0; n < apic->Ioapic.nrdt; n++){
			lock(&apic->Ioapic.l);
			rtblget(apic, n, &hi, &lo);
			unlock(&apic->Ioapic.l);
			print(" rdt %2.2d %#8.8x %#8.8x\n", n, hi, lo);
		}
	}
	for(i = 0; i < Nbus; i++){
		if((rbus = rdtbus[i]) == nil)
			continue;
		print("iointr bus %d:\n", i);
		for(; rbus != nil; rbus = rbus->next){
			rdt = rbus->rdt;
			print(" apic %ld devno %#x (%d %d) intin %d lo %#x ref %d\n",
				rdt->apic-xioapic, rbus->devno, rbus->devno>>2,
				rbus->devno & 0x03, rdt->intin, rdt->lo, rdt->ref);
		}
	}
}

void
ioapiconline(void)
{
	int i;
	Apic *apic;

	for(apic = xioapic; apic < &xioapic[Napic]; apic++){
		if(!apic->useable || apic->Ioapic.addr == nil)
			continue;
		for(i = 0; i < apic->Ioapic.nrdt; i++){
			lock(&apic->Ioapic.l);
			rtblput(apic, i, 0, Im);
			unlock(&apic->Ioapic.l);
		}
	}
	ioapicdump();
}

static int dfpolicy = 0;

static void
ioapicintrdd(uint32_t* hi, uint32_t* lo)
{
	int i;
	static int df;
	static Lock dflock;

	/*
	 * Set delivery mode (lo) and destination field (hi),
	 * according to interrupt routing policy.
	 */
	/*
	 * The bulk of this code was written ~1995, when there was
	 * one architecture and one generation of hardware, the number
	 * of CPUs was up to 4(8) and the choices for interrupt routing
	 * were physical, or flat logical (optionally with lowest
	 * priority interrupt). Logical mode hasn't scaled well with
	 * the increasing number of packages/cores/threads, so the
	 * fall-back is to physical mode, which works across all processor
	 * generations, both AMD and Intel, using the APIC and xAPIC.
	 *
	 * Interrupt routing policy can be set here.
	 */
	switch(dfpolicy){
	default:				/* noise core 0 */
		*hi = sys->machptr[0]->apicno<<24;
		break;
	case 1:					/* round-robin */
		/*
		 * Assign each interrupt to a different CPU on a round-robin
		 * Some idea of the packages/cores/thread topology would be
		 * useful here, e.g. to not assign interrupts to more than one
		 * thread in a core. But, as usual, Intel make that an onerous
		 * task.
		 */
		lock(&dflock);
		for(;;){
			i = df++;
			if(df >= sys->nmach+1)
				df = 0;
			if(sys->machptr[i] == nil || !sys->machptr[i]->online)
				continue;
			i = sys->machptr[i]->apicno;
			if(xlapic[i].useable && xlapic[i].Ioapic.addr == 0)
				break;
		}
		unlock(&dflock);

		*hi = i<<24;
		break;
	}
	*lo |= Pm|MTf;
}

int
nextvec(void)
{
	uint vecno;

	lock(&idtnolock);
	vecno = idtno;
	idtno = (idtno+8) % IdtMAX;
	if(idtno < IdtIOAPIC)
		idtno += IdtIOAPIC;
	unlock(&idtnolock);

	return vecno;
}

static int
msimask(Vkey *v, int mask)
{
	Pcidev *p;

	p = pcimatchtbdf(v->tbdf);
	if(p == nil)
		return -1;
	return pcimsimask(p, mask);
}

static int
intrenablemsi(Vctl* v, Pcidev *p)
{
	uint vno, lo, hi;
	uint64_t msivec;

	vno = nextvec();

	lo = IPlow | TMedge | vno;
	ioapicintrdd(&hi, &lo);

	if(lo & Lm)
		lo |= MTlp;

	msivec = (uint64_t)hi<<32 | lo;
	if(pcimsienable(p, msivec) == -1)
		return -1;
	v->isr = apicisr;
	v->eoi = apiceoi;
	v->vno = vno;
	v->type = "msi";
	v->mask = msimask;

	DBG("msiirq: %T: enabling %.16llx %s irq %d vno %d\n", p->tbdf, msivec, v->name, v->Vkey.irq, vno);
	return vno;
}

int
disablemsi(Vctl* v, Pcidev *p)
{
	if(p == nil)
		return -1;
	return pcimsimask(p, 1);
}

int
ioapicintrenable(Vctl* v)
{
	Rbus *rbus;
	Rdt *rdt;
	uint32_t hi, lo;
	int busno, devno, vecno;

	/*
	 * Bridge between old and unspecified new scheme,
	 * the work in progress...
	 */
	if(v->Vkey.tbdf == BUSUNKNOWN){
		if(v->Vkey.irq >= IrqLINT0 && v->Vkey.irq <= MaxIrqLAPIC){
			if(v->Vkey.irq != IrqSPURIOUS)
				v->isr = apiceoi;
			v->type = "lapic";
			return v->Vkey.irq;
		}
		else{
			/*
			 * Legacy ISA.
			 * Make a busno and devno using the
			 * ISA bus number and the irq.
			 */
			extern int mpisabusno;

			if(mpisabusno == -1) {
				print("no ISA bus allocated");
				return -1;
			}
			busno = mpisabusno;
			devno = v->Vkey.irq<<2;
		}
	}
	else if(BUSTYPE(v->Vkey.tbdf) == BusPCI){
		/*
		 * PCI.
		 * Make a devno from BUSDNO(tbdf) and pcidev->intp.
		 */
		Pcidev *pcidev;

		busno = BUSBNO(v->Vkey.tbdf);
		if((pcidev = pcimatchtbdf(v->Vkey.tbdf)) == nil)
			panic("no PCI dev for tbdf %#8.8x\n", v->Vkey.tbdf);
		if((vecno = intrenablemsi(v, pcidev)) != -1)
			return vecno;
		disablemsi(v, pcidev);
		if((devno = pcicfgr8(pcidev, PciINTP)) == 0)
			panic("no INTP for tbdf %#8.8x\n", v->Vkey.tbdf);
		devno = BUSDNO(v->Vkey.tbdf)<<2|(devno-1);
		DBG("ioapicintrenable: tbdf %#8.8x busno %d devno %d\n",
			v->Vkey.tbdf, busno, devno);
	}
	else{
		SET(busno); SET(devno);
		panic("unknown tbdf %#8.8x\n", v->Vkey.tbdf);
	}

	rdt = nil;
	for(rbus = rdtbus[busno]; rbus != nil; rbus = rbus->next) {
print("IOAPIC: find it, rbus->devno %d devno %d\n", rbus->devno, devno);
		if(rbus->devno == devno){
			rdt = rbus->rdt;
			break;
		}
	}
	if(rdt == nil){
		extern int mpisabusno;

		/*
		 * First crack in the smooth exterior of the new code:
		 * some BIOS make an MPS table where the PCI devices are
		 * just defaulted to ISA.
		 * Rewrite this to be cleaner.
		 */
print("TRY 2!\n");
		if((busno = mpisabusno) == -1)
			return -1;
		devno = v->Vkey.irq<<2;
		for(rbus = rdtbus[busno]; rbus != nil; rbus = rbus->next)
			if(rbus->devno == devno){
				rdt = rbus->rdt;
				break;
			}
		DBG("isa: tbdf %#8.8x busno %d devno %d %#p\n",
			v->Vkey.tbdf, busno, devno, rdt);
	}
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
	lock(&rdt->apic->Ioapic.l);
	DBG("%T: %ld/%d/%d (%d)\n", v->Vkey.tbdf, rdt->apic - xioapic, rbus->devno, rdt->intin, devno);
	if((rdt->lo & 0xff) == 0){
		vecno = nextvec();
		rdt->lo |= vecno;
		rdtvecno[vecno] = rdt;
	}else
		DBG("%T: mutiple irq bus %d dev %d\n", v->Vkey.tbdf, busno, devno);

	rdt->enabled++;
	lo = (rdt->lo & ~Im);
	ioapicintrdd(&hi, &lo);
	rtblput(rdt->apic, rdt->intin, hi, lo);
	vecno = lo & 0xff;
	unlock(&rdt->apic->Ioapic.l);

	DBG("busno %d devno %d hi %#8.8x lo %#8.8x vecno %d\n",
		busno, devno, hi, lo, vecno);
	v->isr = apicisr;
	v->eoi = apiceoi;
	v->vno = vecno;
	v->type = "ioapic";

	return vecno;
}

int
ioapicintrdisable(int vecno)
{
	Rdt *rdt;

	/*
	 * FOV. Oh dear. This isn't very good.
	 * Fortunately rdtvecno[vecno] is static
	 * once assigned.
	 * Must do better.
	 *
	 * What about any pending interrupts?
	 */
	if(vecno < 0 || vecno > MaxVectorAPIC){
		panic("ioapicintrdisable: vecno %d out of range", vecno);
		return -1;
	}
	if((rdt = rdtvecno[vecno]) == nil){
		panic("ioapicintrdisable: vecno %d has no rdt", vecno);
		return -1;
	}

	lock(&rdt->apic->Ioapic.l);
	rdt->enabled--;
	if(rdt->enabled == 0)
		rtblput(rdt->apic, rdt->intin, 0, rdt->lo);
	unlock(&rdt->apic->Ioapic.l);

	return 0;
}
