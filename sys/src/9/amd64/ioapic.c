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
#include "acpi.h"

typedef struct Rbus Rbus;
typedef struct Rdt Rdt;

/* this cross-dependency from acpi to ioapic is from akaros, and
 * kind of breaks the clean model we had before, where table
 * parsing and hardware were completely separate. We'll try to
 * clean it up later.
 */
extern Atable *apics; 		/* APIC info */
extern int mpisabusno;

struct Rbus {
	Rbus	*next;
	int	devno;
	Rdt	*rdt;
};

struct Rdt {
	Apic	*apic;
	int	intin;
	uint32_t	lo;
	uint32_t	hi;

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

static int map_polarity[4] = {
	-1, IPhigh, -1, IPlow
};

static int map_edge_level[4] = {
	-1, TMedge, -1, TMlevel
};

/* TODO: use the slice library for this. */
typedef struct {
	Vctl v;
	uint32_t lo;
	int valid;
} Vinfo;
static Vinfo todo[1<<13];

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
		print("dup rdt %d %d %d %d %.8x\n", busno, apicno, intin, devno, lo);
	}
	rdt->ref++;
	rbus = malloc(sizeof *rbus);
	rbus->rdt = rdt;
	rbus->devno = devno;
	rbus->next = rdtbus[busno];
	rdtbus[busno] = rbus;
	print("%s: success\n", __func__);
}

static int acpi_irq2ioapic(int irq)
{
	int ioapic_idx = 0;
	Apic *apic;
	/* with acpi, the ioapics map a global interrupt space.  each covers a
	 * window of the space from [ibase, ibase + nrdt). */
	for (apic = xioapic; apic < &xioapic[Napic]; apic++, ioapic_idx++) {
		/* addr check is just for sanity */
		if (!apic->useable || !apic->Ioapic.addr)
			continue;
		if ((apic->Ioapic.gsib <= irq) && (irq < apic->Ioapic.gsib + apic->Ioapic.nrdt))
			return ioapic_idx;
	}
	return -1;
}

/* Build an RDT route, like we would have had from the MP tables had they been
 * parsed, via ACPI.
 *
 * This only really deals with the ISA IRQs and maybe PCI ones that happen to
 * have an override.  FWIW, on qemu the PCI NIC shows up as an ACPI intovr.
 *
 * From Brendan http://f.osdev.org/viewtopic.php?f=1&t=25951:
 *
 * 		Before parsing the MADT you should begin by assuming that redirection
 * 		entries 0 to 15 are used for ISA IRQs 0 to 15. The MADT's "Interrupt
 * 		Source Override Structures" will tell you when this initial/default
 * 		assumption is wrong. For example, the MADT might tell you that ISA IRQ 9
 * 		is connected to IO APIC 44 and is level triggered; and (in this case)
 * 		it'd be silly to assume that ISA IRQ 9 is also connected to IO APIC
 * 		input 9 just because IO APIC input 9 is not listed.
 *
 *		For PCI IRQs, the MADT tells you nothing and you can't assume anything
 *		at all. Sadly, you have to interpret the ACPI AML to determine how PCI
 *		IRQs are connected to IO APIC inputs (or find some other work-around;
 *		like implementing a motherboard driver for each different motherboard,
 *		or some complex auto-detection scheme, or just configure PCI devices to
 *		use MSI instead). */
static int acpi_make_rdt(Vctl *v, int tbdf, int irq, int busno, int devno)
{
	Atable *at;
	Apicst *st, *lst;
	uint32_t lo = 0;
	int pol, edge_level, ioapic_nr, gsi_irq;
print("acpi_make_rdt(0x%x %d %d 0x%x)\n", tbdf, irq, busno, devno);
//die("acpi.make.rdt)\n");

	at = apics;
	st = nil;
	for (int i = 0; i < at->nchildren; i++) {
		lst = at->children[i]->tbl;
		if (lst->type == ASintovr) {
			if (lst->intovr.irq == irq) {
				st = lst;
				break;
			}
		}
	}
	if (st) {
		pol = map_polarity[st->intovr.flags & AFpmask];
		if (pol < 0) {
			print("ACPI override had bad polarity\n");
			return -1;
		}
		edge_level = map_edge_level[(st->intovr.flags & AFlevel) >> 2];
		if (edge_level < 0) {
			print("ACPI override had bad edge/level\n");
			return -1;
		}
		lo = pol | edge_level;
		gsi_irq = st->intovr.intr;
	} else {
		if (BUSTYPE(tbdf) == BusISA) {
			lo = IPhigh | TMedge;
			gsi_irq = irq;
		} else {
			/* Need to query ACPI at some point to handle this */
			print("Non-ISA IRQ %d not found in MADT, aborting\n", irq);
			todo[(tbdf>>11)&0x1fff].v = *v;
			todo[(tbdf>>11)&0x1fff].lo = lo;
			todo[(tbdf>>11)&0x1fff].valid = 1;
			print("Set TOOD[0x%x] to valid\n", (tbdf>>11)&0x1fff);
			return -1;
		}
	}
	ioapic_nr = acpi_irq2ioapic(gsi_irq);
	if (ioapic_nr < 0) {
		print("Could not find an IOAPIC for global irq %d!\n", gsi_irq);
		return -1;
	}
	ioapicintrinit(busno, ioapic_nr, gsi_irq - xioapic[ioapic_nr].Ioapic.gsib,
	               devno, lo);
	return 0;
}

void
ioapicinit(int id, int ibase, uintptr_t pa)
{
	Apic *apic;

	/*
	 * Mark the IOAPIC useable if it has a good ID
	 * and the registers can be mapped.
	 */
	if(id >= Napic) {
		print("NOT setting ioapic %d useable; id must be < %d\n", id, Napic);
		return;
	}

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

/*
	if(!DBGFLG)
		return;
 */
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

/* From Akaros, not sure we want this but for now ... */
static int ioapic_exists(void)
{
	/* not foolproof, if we called this before parsing */
	for (int i = 0; i < Napic; i++)
		if (xioapic[i].useable)
			return 1;
	return 0;
}

Rdt *rbus_get_rdt(int busno, int devno)
{
	Rbus *rbus;
	for (rbus = rdtbus[busno]; rbus != nil; rbus = rbus->next) {
		if (rbus->devno == devno)
			return rbus->rdt;
	}
	return 0;
}

/* Attempts to init a bus interrupt, initializes Vctl, and returns the IDT
 * vector to use (-1 on error).  If routable, the IRQ will route to core 0.  The
 * IRQ will be masked, if possible.  Call Vctl->unmask() when you're ready.
 *
 * This will determine the type of bus the device is on (LAPIC, IOAPIC, PIC,
 * etc), and set the appropriate fields in isr_h.  If applicable, it'll also
 * allocate an IDT vector, such as for an IOAPIC, and route the IOAPIC entries
 * appropriately.
 *
 * Callers init Vctl->dev_irq and ->tbdf.  tbdf encodes the bus type and the
 * classic PCI bus:dev:func.  dev_irq may be ignored based on the bus type (e.g.
 * PCI, esp MSI).
 *
 * In plan9, this was ioapicintrenable(), which also unmasked.  We don't have a
 * deinit/disable method that would tear down the route yet.  All the plan9 one
 * did was dec enabled and mask the entry. */
int bus_irq_setup(Vctl *v)
{
	//Rbus *rbus;
	Rdt *rdt;
	int busno = -1, devno = -1, vno;
	Pcidev *p;

       	if (!ioapic_exists()) {
		panic("%s: no ioapics?", __func__);
		switch (BUSTYPE(v->Vkey.tbdf)) {
			//case BusLAPIC:
			//case BusIPI:
			//break;
		default:
			//irq_h->check_spurious = pic_check_spurious;
			//v->eoi = pic_send_eoi;
			//irq_h->mask = pic_mask_irq;
			//irq_h->unmask = pic_unmask_irq;
			//irq_h->route_irq = 0;
			//irq_h->type = "pic";
			/* PIC devices have vector = irq + 32 */
			return -1; //irq_h->dev_irq + IdtPIC;
		}
	}
	switch (BUSTYPE(v->Vkey.tbdf)) {
	case BusLAPIC:
		/* nxm used to set the initial 'isr' method (i think equiv to our
		 * check_spurious) to apiceoi for non-spurious lapic vectors.  in
		 * effect, i think they were sending the EOI early, and their eoi
		 * method was 0.  we're not doing that (unless we have to). */
		//v->check_spurious = lapic_check_spurious;
		v->eoi = nil; //apiceoi;
		v->isr = apiceoi;
		//v->mask = lapic_mask_irq;
		//v->unmask = lapic_unmask_irq;
		//v->route_irq = 0;
		v->type = "lapic";
		/* For the LAPIC, irq == vector */
		return v->Vkey.irq;
	case BusIPI:
		/* similar to LAPIC, but we don't actually have LVT entries */
		//v->check_spurious = lapic_check_spurious;
		v->eoi = apiceoi;
		//v->mask = 0;
		//v->unmask = 0;
		//v->route_irq = 0;
		v->type = "IPI";
		return v->Vkey.irq;
	case BusISA:
		if (mpisabusno == -1)
			panic("No ISA bus allocated");
		busno = mpisabusno;
		/* need to track the irq in devno in PCI interrupt assignment entry
		 * format (see mp.c or MP spec D.3). */
		devno = v->Vkey.irq << 2;
		break;
	case BusPCI:
		p = pcimatchtbdf(v->Vkey.tbdf);
		if (!p) {
			print("No PCI dev for tbdf %p!", v->Vkey.tbdf);
			return -1;
		}
		if ((vno = intrenablemsi(v, p))!= -1)
			return vno;
		busno = BUSBNO(v->Vkey.tbdf);
		devno = pcicfgr8(p, PciINTP);

		/* this might not be a big deal - some PCI devices have no INTP.  if
		 * so, change our devno - 1 below. */
		if (devno == 0)
			panic("no INTP for tbdf %p", v->Vkey.tbdf);
		/* remember, devno is the device shifted with irq pin in bits 0-1.
		 * we subtract 1, since the PCI intp maps 1 -> INTA, 2 -> INTB, etc,
		 * and the MP spec uses 0 -> INTA, 1 -> INTB, etc. */
		devno = BUSDNO(v->Vkey.tbdf) << 2 | (devno - 1);
		break;
	default:
		panic("Unknown bus type, TBDF %p", v->Vkey.tbdf);
	}
	/* busno and devno are set, regardless of the bustype, enough to find rdt.
	 * these may differ from the values in tbdf. */
	rdt = rbus_get_rdt(busno, devno);
	if (!rdt) {
		/* second chance.  if we didn't find the item the first time, then (if
		 * it exists at all), it wasn't in the MP tables (or we had no tables).
		 * So maybe we can figure it out via ACPI. */
		acpi_make_rdt(v, v->Vkey.tbdf, v->Vkey.irq, busno, devno);
		rdt = rbus_get_rdt(busno, devno);
	}
	if (!rdt) {
		print("Unable to build IOAPIC route for irq %d\n", v->Vkey.irq);
		return -1;
	}
	/*
	 * what to do about devices that intrenable/intrdisable frequently?
	 * 1) there is no ioapicdisable yet;
	 * 2) it would be good to reuse freed vectors.
	 * Oh bugger.
	 * brho: plus the diff btw mask/unmask and enable/disable is unclear
	 */
	/*
	 * This is a low-frequency event so just lock
	 * the whole IOAPIC to initialise the RDT entry
	 * rather than putting a Lock in each entry.
	 */
	lock(&rdt->apic->Ioapic.l);
	/* if a destination has already been picked, we store it in the lo.  this
	 * stays around regardless of enabled/disabled, since we don't reap vectors
	 * yet.  nor do we really mess with enabled... */
	if ((rdt->lo & 0xff) == 0) {
		vno = nextvec();
		rdt->lo |= vno;
		rdtvecno[vno] = rdt;
	} else {
		print("%p: mutiple irq bus %d dev %d\n", v->Vkey.tbdf, busno, devno);
	}
	rdt->enabled++;
	rdt->hi = 0;			/* route to 0 by default */
	rdt->lo |= Pm | MTf;
	rtblput(rdt->apic, rdt->intin, rdt->hi, rdt->lo);
	vno = rdt->lo & 0xff;
	unlock(&rdt->apic->Ioapic.l);

	v->type = "ioapic";

	v->eoi = apiceoi;
	v->vno = vno;
	v->mask = msimask;
	return vno;
}

int acpiirq(uint32_t tbdf, int gsi)
{
	Proc *up = externup();
	int ioapic_nr;
	int busno = BUSBNO(tbdf);
	int pin, devno;
	int ix = (BUSBNO(tbdf) << 5) | BUSDNO(tbdf);
	Pcidev *pcidev;
	Vctl *v;
	/* for now we know it's PCI, just ignore what they told us. */
	tbdf = MKBUS(BusPCI, busno, BUSDNO(tbdf), 0);
	if((pcidev = pcimatchtbdf(tbdf)) == nil)
		error("No such device (any more?)");
	if((pin = pcicfgr8(pcidev, PciINTP)) == 0)
		error("no INTP for that device, which is impossible");

	print("ix is %x\n", ix);
	if (!todo[ix].valid)
		error("Invalid tbdf");
	v = malloc(sizeof(*v));
	if (waserror()) {
		print("well, that went badly\n");
		free(v);
		nexterror();
	}
	*v = todo[ix].v;
	devno = BUSDNO(v->Vkey.tbdf)<<2|(pin-1);
	print("acpiirq: tbdf %#8.8x busno %d devno %d\n",
	    v->Vkey.tbdf, busno, devno);

	ioapic_nr = acpi_irq2ioapic(gsi);
	print("ioapic_nr for gsi %d is %d\n", gsi, ioapic_nr);
	if (ioapic_nr < 0) {
		error("Could not find an IOAPIC for global irq!\n");
	}
	ioapicintrinit(busno, ioapic_nr, gsi - xioapic[ioapic_nr].Ioapic.gsib,
	               devno, todo[ix].lo);
	print("ioapicinrinit seems to have worked\n");
	poperror();

	todo[ix].valid = 0;
	ioapicdump();
	return 0;
}

