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
	int	bustype;
	int	devno;
	Rdt	*rdt;
};

struct Rdt {
	Apic*	apic;
	int	intin;
	u32int	lo;

	int	ref;				/* could map to multiple busses */
	int	enabled;			/* times enabled */
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

static void
rtblget(Apic* apic, int sel, u32int* hi, u32int* lo)
{
	u32int r;

	sel = Ioredtbl + 2*sel;

	*(apic->addr+Ioregsel) = sel+1;
	r = *(apic->addr+Iowin);
	if(hi)
		*hi = r;
	*(apic->addr+Ioregsel) = sel;
	r = *(apic->addr+Iowin);
	if(lo)
		*lo = r;
}

static void
rtblput(Apic* apic, int sel, u32int hi, u32int lo)
{
	sel = Ioredtbl + 2*sel;

	*(apic->addr+Ioregsel) = sel+1;
	*(apic->addr+Iowin) = hi;
	*(apic->addr+Ioregsel) = sel;
	*(apic->addr+Iowin) = lo;
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
ioapicintrinit(int bustype, int busno, int apicno, int intin, int devno, u32int lo)
{
	Rbus *rbus;
	Rdt *rdt;
	Apic *apic;

	if(busno >= Nbus || apicno >= Napic || nrdtarray >= Nrdt)
		return;
	apic = &ioapic[apicno];
	if(!apic->useable || intin >= apic->nrdt)
		return;

	rdt = rdtlookup(apic, intin);
	if(rdt == nil){
		if(nrdtarray == nelem(rdtarray)){
			print("ioapic: intrinit: rdtarray too small\n");
			return;
		}
		rdt = &rdtarray[nrdtarray++];
		rdt->apic = apic;
		rdt->intin = intin;
		rdt->lo = lo;
	}else{
		if(lo != rdt->lo){
			if(bustype == BusISA && intin < 16 && lo == (Im|IPhigh|TMedge)){
				DBG("override: isa %d %.8ux\n", intin, rdt->lo);
				return;	/* expected; default was overridden*/
			}
			print("multiple irq botch type %d bus %d %d/%d/%d lo %.8ux vs %.8ux\n",
				bustype, busno, apicno, intin, devno, lo, rdt->lo);
			return;
		}
		DBG("dup rdt %d %d %d %d %.8ux\n", busno, apicno, intin, devno, lo);
	}
	rdt->ref++;
	rbus = malloc(sizeof(*rbus));
	rbus->rdt = rdt;
	rbus->bustype = bustype;
	rbus->devno = devno;
	rbus->next = rdtbus[busno];
	rdtbus[busno] = rbus;
}

void
ioapicinit(int id, uintmem pa)
{
	Apic *apic;

	/*
	 * Mark the IOAPIC useable if it has a good ID
	 * and the registers can be mapped.
	 */
	if(id >= Napic)
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
	lock(apic);
	*(apic->addr+Ioregsel) = Ioapicver;
	apic->nrdt = ((*(apic->addr+Iowin)>>16) & 0xff) + 1;
	apic->gsib = gsib;
	gsib += apic->nrdt;

	*(apic->addr+Ioregsel) = Ioapicid;
	*(apic->addr+Iowin) = id<<24;
	unlock(apic);
}

void
ioapicdump(void)
{
	int i, n;
	Rbus *rbus;
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
			lock(apic);
			rtblget(apic, n, &hi, &lo);
			unlock(apic);
			DBG(" rdt %2.2d %#8.8ux %#8.8ux\n", n, hi, lo);
		}
	}
	for(i = 0; i < Nbus; i++){
		if((rbus = rdtbus[i]) == nil)
			continue;
		DBG("iointr bus %d:\n", i);
		while(rbus != nil){
			rdt = rbus->rdt;
			DBG(" apic %ld devno %#ux (%d %d) intin %d lo %#ux ref %d\n",
				rdt->apic-ioapic, rbus->devno, rbus->devno>>2,
				rbus->devno & 0x03, rdt->intin, rdt->lo, rdt->ref);
			rbus = rbus->next;
		}
	}
}

void
ioapiconline(void)
{
	int i;
	Apic *apic;

	for(apic = ioapic; apic < &ioapic[Napic]; apic++){
		if(!apic->useable || apic->addr == nil)
			continue;
		for(i = 0; i < apic->nrdt; i++){
			lock(apic);
			rtblput(apic, i, 0, Im);
			unlock(apic);
		}
	}
	if(DBGFLG)
		ioapicdump();
}

static int dfpolicy = 0;

static void
ioapicintrdd(u32int* hi, u32int* lo)
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
			if(df >= MACHMAX+1)
				df = 0;
			if(sys->machptr[i] == nil || !sys->machptr[i]->online)
				continue;
			i = sys->machptr[i]->apicno;
			if(xapic[i].useable && xapic[i].addr == 0)
				break;
		}
		unlock(&dflock);

		*hi = i<<24;
		break;
	}
	*lo |= Pm|MTf;
}

int
ioapicintrenable(Vctl* v)
{
	Rbus *rbus;
	Rdt *rdt;
	u32int hi, lo;
	int bustype, busno, devno, vecno;

	/*
	 * Bridge between old and unspecified new scheme,
	 * the work in progress...
	 */
	if(v->tbdf == BUSUNKNOWN){
		if(v->irq >= IdtLINT0 && v->irq <= IdtSPURIOUS){
			if(v->irq != IdtSPURIOUS)
				v->isr = apiceoi;
			v->type = "lapic";
			return v->irq;
		}
		else{
			/*
			 * Legacy ISA.
			 * Make a busno and devno using the
			 * ISA bus number and the irq.
			 */
			extern int mpisabusno;

			if(mpisabusno == -1)
				panic("no ISA bus allocated");
			busno = mpisabusno;
			devno = v->irq;
			bustype = BusISA;
		}
	}
	else if((bustype = BUSTYPE(v->tbdf)) == BusPCI){
		/*
		 * PCI.
		 * Make a devno from BUSDNO(tbdf) and pcidev->intp.
		 */
		Pcidev *pcidev;

		busno = BUSBNO(v->tbdf);
		if((pcidev = pcimatchtbdf(v->tbdf)) == nil)
			panic("no PCI dev for tbdf %#8.8ux\n", v->tbdf);
		if((devno = pcicfgr8(pcidev, PciINTP)) == 0)
			panic("no INTP for tbdf %#8.8ux\n", v->tbdf);
		devno = BUSDNO(v->tbdf)<<2|(devno-1);
		DBG("ioapicintrenable: tbdf %#8.8ux busno %d devno %d\n",
			v->tbdf, busno, devno);
	}
	else{
		SET(busno, devno);
		panic("unknown tbdf %#8.8ux\n", v->tbdf);
	}

	rdt = nil;
	for(rbus = rdtbus[busno]; rbus != nil; rbus = rbus->next)
		if(rbus->devno == devno && rbus->bustype == bustype){
			rdt = rbus->rdt;
			break;
		}
	if(rdt == nil){
		extern int mpisabusno;

		/*
		 * First crack in the smooth exterior of the new code:
		 * some BIOS make an MPS table where the PCI devices are
		 * just defaulted to ISA.
		 * Rewrite this to be cleaner.
		 */
		if((busno = mpisabusno) == -1)
			return -1;
		devno = v->irq<<2;
		for(rbus = rdtbus[busno]; rbus != nil; rbus = rbus->next){
			if(rbus->devno == devno){
				rdt = rbus->rdt;
				break;
			}
		}
		DBG("isa: tbdf %#8.8ux busno %d devno %d %#p\n",
			v->tbdf, busno, devno, rdt);
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
	lock(rdt->apic);
	if((rdt->lo & 0xff) == 0){
		lock(&idtnolock);
		vecno = idtno;
		idtno = (idtno+8) % IdtMAX;
		if(idtno < IdtIOAPIC)
			idtno += IdtIOAPIC;
		unlock(&idtnolock);

		rdt->lo |= vecno;
		rdtvecno[vecno] = rdt;
	}

	rdt->enabled++;
	lo = (rdt->lo & ~Im);
	ioapicintrdd(&hi, &lo);
	rtblput(rdt->apic, rdt->intin, hi, lo);
	vecno = lo & 0xff;
	unlock(rdt->apic);

	DBG("busno %d devno %d hi %#8.8ux lo %#8.8ux vecno %d\n",
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
	if(vecno < 0 || vecno > IdtMAX){
		panic("ioapicintrdisable: vecno %d out of range", vecno);
		return -1;
	}
	if((rdt = rdtvecno[vecno]) == nil){
		panic("ioapicintrdisable: vecno %d has no rdt", vecno);
		return -1;
	}

	lock(rdt->apic);
	rdt->enabled--;
	if(rdt->enabled == 0)
		rtblput(rdt->apic, rdt->intin, 0, rdt->lo);
	unlock(rdt->apic);

	return 0;
}
