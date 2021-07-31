#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "ureg.h"

#include "mp.h"
#include "apbootstrap.h"

#define dprint(...)	if(mpdebug) print(__VA_ARGS__); else USED(mpdebug)

/* from mpacpi.c */
Apic *bootapic;

int mpdebug;
void (*mpacpifunc)(void);

static PCMP* mppcmp;
static Bus* mpbus;
static Bus* mpbuslast;
static int mpisabus = -1;
static int mpeisabus = -1;
extern int i8259elcr;			/* mask of level-triggered interrupts */
/* static */ Apic mpapic[MaxAPICNO+1];
/* static */ int machno2apicno[MaxAPICNO+1];	/* inverse map: machno -> APIC ID */
/* static */ Apic ioapic[MaxAPICNO+1];
static Ref mpvnoref;			/* unique vector assignment */
static int mpmachno = 1;
static Lock mpphysidlock;
static int mpphysid;

static char* buses[] = {
	"CBUSI ",
	"CBUSII",
	"EISA  ",
	"FUTURE",
	"INTERN",
	"ISA   ",
	"MBI   ",
	"MBII  ",
	"MCA   ",
	"MPI   ",
	"MPSA  ",
	"NUBUS ",
	"PCI   ",
	"PCMCIA",
	"TC    ",
	"VL    ",
	"VME   ",
	"XPRESS",
	0,
};

static Apic*
mkprocessor(PCMPprocessor* p)
{
	int apicno;
	Apic *apic;

	apicno = p->apicno;
	if(!(p->flags & PcmpEN) || apicno > MaxAPICNO)
		return 0;

	apic = &mpapic[apicno];
	apic->type = PcmpPROCESSOR;
	apic->apicno = apicno;
	apic->flags = p->flags;
	apic->lintr[0] = ApicIMASK;
	apic->lintr[1] = ApicIMASK;

	if(p->flags & PcmpBP){
		machno2apicno[0] = apicno;
		apic->machno = 0;
	}
	else{
		machno2apicno[mpmachno] = apicno;
		apic->machno = mpmachno;
		mpmachno++;
	}

	return apic;
}

static Bus*
mkbus(PCMPbus* p)
{
	Bus *bus;
	int i;

	for(i = 0; buses[i]; i++){
		if(strncmp(buses[i], p->string, sizeof(p->string)) == 0)
			break;
	}
	if(buses[i] == 0)
		return 0;

	bus = xalloc(sizeof(Bus));
	if(mpbus)
		mpbuslast->next = bus;
	else
		mpbus = bus;
	mpbuslast = bus;

	bus->type = i;
	bus->busno = p->busno;
	if(bus->type == BusEISA){
		bus->po = PcmpLOW;
		bus->el = PcmpLEVEL;
		if(mpeisabus != -1)
			print("mkbus: more than one EISA bus\n");
		mpeisabus = bus->busno;
	}
	else if(bus->type == BusPCI){
		bus->po = PcmpLOW;
		bus->el = PcmpLEVEL;
	}
	else if(bus->type == BusISA){
		bus->po = PcmpHIGH;
		bus->el = PcmpEDGE;
		if(mpisabus != -1)
			print("mkbus: more than one ISA bus\n");
		mpisabus = bus->busno;
	}
	else{
		bus->po = PcmpHIGH;
		bus->el = PcmpEDGE;
	}

	return bus;
}

static Bus*
mpgetbus(int busno)
{
	Bus *bus;

	for(bus = mpbus; bus; bus = bus->next){
		if(bus->busno == busno)
			return bus;
	}
	print("mpgetbus: can't find bus %d\n", busno);

	return 0;
}

static Apic*
mkioapic(PCMPioapic* p)
{
	void *va;
	int apicno;
	Apic *apic;

	apicno = p->apicno;
	if(!(p->flags & PcmpEN) || apicno > MaxAPICNO)
		return 0;

	/*
	 * Map the I/O APIC.
	 */
	if((va = vmap(p->addr, 1024)) == nil)
		return 0;

	apic = &ioapic[apicno];
	apic->type = PcmpIOAPIC;
	apic->apicno = apicno;
	apic->addr = va;
	apic->paddr = p->addr;
	apic->flags = p->flags;

	return apic;
}

static Aintr*
mkiointr(PCMPintr* p)
{
	Bus *bus;
	Aintr *aintr;
	PCMPintr* pcmpintr;

	/*
	 * According to the MultiProcessor Specification, a destination
	 * I/O APIC of 0xFF means the signal is routed to all I/O APICs.
	 * It's unclear how that can possibly be correct so treat it as
	 * an error for now.
	 */
	if(p->apicno == 0xFF)
		return 0;
	if((bus = mpgetbus(p->busno)) == 0)
		return 0;

	aintr = xalloc(sizeof(Aintr));
	aintr->intr = p;

	if(0)
		dprint("mkiointr: type %d intr type %d flags %#o "
			"bus %d irq %d apicno %d intin %d\n",
			p->type, p->intr, p->flags,
			p->busno, p->irq, p->apicno, p->intin);
	/*
	 * Hack for Intel SR1520ML motherboard, which BIOS describes
	 * the i82575 dual ethernet controllers incorrectly.
	 */
	if(mppcmp && memcmp(mppcmp->product, "INTEL   X38MLST     ", 20) == 0){
		if(p->busno == 1 && p->intin == 16 && p->irq == 1){
			pcmpintr = malloc(sizeof(PCMPintr));
			if(pcmpintr == nil)
				panic("mkiointr: no memory");
			memmove(pcmpintr, p, sizeof(PCMPintr));
			print("mkiointr: %20.20s bus %d intin %d irq %d\n",
				(char*)mppcmp->product,
				pcmpintr->busno, pcmpintr->intin,
				pcmpintr->irq);
			pcmpintr->intin = 17;
			aintr->intr = pcmpintr;
		}
	}
	if ((unsigned)p->apicno >= nelem(mpapic))
		panic("mkiointr: apic %d out of range", p->apicno);
	aintr->apic = &ioapic[p->apicno];
	aintr->next = bus->aintr;
	bus->aintr = aintr;

	return aintr;
}

static int
mpintrinit(Bus* bus, PCMPintr* intr, int vno, int /*irq*/)
{
	int el, po, v;

	/*
	 * Parse an I/O or Local APIC interrupt table entry and
	 * return the encoded vector.
	 */
	v = vno;

	po = intr->flags & PcmpPOMASK;
	el = intr->flags & PcmpELMASK;

	switch(intr->intr){

	default:				/* PcmpINT */
		v |= ApicFIXED;			/* no-op */
		break;

	case PcmpNMI:
		v |= ApicNMI;
		po = PcmpHIGH;
		el = PcmpEDGE;
		break;

	case PcmpSMI:
		v |= ApicSMI;
		break;

	case PcmpExtINT:
		v |= ApicExtINT;
		/*
		 * The AMI Goliath doesn't boot successfully with it's LINTR0
		 * entry which decodes to low+level. The PPro manual says ExtINT
		 * should be level, whereas the Pentium is edge. Setting the
		 * Goliath to edge+high seems to cure the problem. Other PPro
		 * MP tables (e.g. ASUS P/I-P65UP5 have a entry which decodes
		 * to edge+high, so who knows.
		 * Perhaps it would be best just to not set an ExtINT entry at
		 * all, it shouldn't be needed for SMP mode.
		 */
		po = PcmpHIGH;
		el = PcmpEDGE;
		break;
	}

	/*
	 */
	if(bus->type == BusEISA && !po && !el /*&& !(i8259elcr & (1<<irq))*/){
		po = PcmpHIGH;
		el = PcmpEDGE;
	}
	if(!po)
		po = bus->po;
	if(po == PcmpLOW)
		v |= ApicLOW;
	else if(po != PcmpHIGH){
		print("mpintrinit: bad polarity 0x%uX\n", po);
		return ApicIMASK;
	}

	if(!el)
		el = bus->el;
	if(el == PcmpLEVEL)
		v |= ApicLEVEL;
	else if(el != PcmpEDGE){
		print("mpintrinit: bad trigger 0x%uX\n", el);
		return ApicIMASK;
	}

	return v;
}

static int
mklintr(PCMPintr* p)
{
	Apic *apic;
	Bus *bus;
	int intin, v;

	/*
	 * The offsets of vectors for LINT[01] are known to be
	 * 0 and 1 from the local APIC vector space at VectorLAPIC.
	 */
	if((bus = mpgetbus(p->busno)) == 0)
		return 0;
	intin = p->intin;

	/*
	 * Pentium Pros have problems if LINT[01] are set to ExtINT
	 * so just bag it, SMP mode shouldn't need ExtINT anyway.
	 */
	if(p->intr == PcmpExtINT || p->intr == PcmpNMI)
		v = ApicIMASK;
	else
		v = mpintrinit(bus, p, VectorLAPIC+intin, p->irq);

	if(p->apicno == 0xFF){
		for(apic = mpapic; apic <= &mpapic[MaxAPICNO]; apic++){
			if((apic->flags & PcmpEN)
			&& apic->type == PcmpPROCESSOR)
				apic->lintr[intin] = v;
		}
	}
	else{
		if ((unsigned)p->apicno >= nelem(mpapic))
			panic("mklintr: ioapic %d out of range", p->apicno);
		apic = &mpapic[p->apicno];
		if((apic->flags & PcmpEN) && apic->type == PcmpPROCESSOR)
			apic->lintr[intin] = v;
	}

	return v;
}

static void
checkmtrr(void)
{
	int i, vcnt;
	Mach *mach0;

	/*
	 * If there are MTRR registers, snarf them for validation.
	 */
	if(!(m->cpuiddx & Mtrr))
		return;

	rdmsr(0x0FE, &m->mtrrcap);
	rdmsr(0x2FF, &m->mtrrdef);
	if(m->mtrrcap & 0x0100){
		rdmsr(0x250, &m->mtrrfix[0]);
		rdmsr(0x258, &m->mtrrfix[1]);
		rdmsr(0x259, &m->mtrrfix[2]);
		for(i = 0; i < 8; i++)
			rdmsr(0x268+i, &m->mtrrfix[(i+3)]);
	}
	vcnt = m->mtrrcap & 0x00FF;
	if(vcnt > nelem(m->mtrrvar))
		vcnt = nelem(m->mtrrvar);
	for(i = 0; i < vcnt; i++)
		rdmsr(0x200+i, &m->mtrrvar[i]);

	/*
	 * If not the bootstrap processor, compare.
	 */
	if(m->machno == 0)
		return;

	mach0 = MACHP(0);
	if(mach0->mtrrcap != m->mtrrcap)
		print("mtrrcap%d: %lluX %lluX\n",
			m->machno, mach0->mtrrcap, m->mtrrcap);
	if(mach0->mtrrdef != m->mtrrdef)
		print("mtrrdef%d: %lluX %lluX\n",
			m->machno, mach0->mtrrdef, m->mtrrdef);
	for(i = 0; i < 11; i++){
		if(mach0->mtrrfix[i] != m->mtrrfix[i])
			print("mtrrfix%d: i%d: %lluX %lluX\n",
				m->machno, i, mach0->mtrrfix[i], m->mtrrfix[i]);
	}
	for(i = 0; i < vcnt; i++){
		if(mach0->mtrrvar[i] != m->mtrrvar[i])
			print("mtrrvar%d: i%d: %lluX %lluX\n",
				m->machno, i, mach0->mtrrvar[i], m->mtrrvar[i]);
	}
}

static void
squidboy(Apic* apic)
{
//	iprint("Hello Squidboy\n");

	machinit();
	mmuinit();

	cpuidentify();
	cpuidprint();
	checkmtrr();

	apic->online = 1;
	coherence();

	lapicinit(apic);
	lapiconline();
	syncclock();
	timersinit();

	fpoff();

	lock(&active);
	active.machs |= 1<<m->machno;
	unlock(&active);

	while(!active.thunderbirdsarego)
		microdelay(100);

	schedinit();
}

static void
mpstartap(Apic* apic)
{
	ulong *apbootp, *pdb, *pte;
	Mach *mach, *mach0;
	int i, machno;
	uchar *p;

	mach0 = MACHP(0);

	/*
	 * Initialise the AP page-tables and Mach structure. The page-tables
	 * are the same as for the bootstrap processor with the exception of
	 * the PTE for the Mach structure.
	 * Xspanalloc will panic if an allocation can't be made.
	 */
	p = xspanalloc(4*BY2PG, BY2PG, 0);
	pdb = (ulong*)p;
	memmove(pdb, mach0->pdb, BY2PG);
	p += BY2PG;

	if((pte = mmuwalk(pdb, MACHADDR, 1, 0)) == nil)
		return;
	memmove(p, KADDR(PPN(*pte)), BY2PG);
	*pte = PADDR(p)|PTEWRITE|PTEVALID;
	if(mach0->havepge)
		*pte |= PTEGLOBAL;
	p += BY2PG;

	mach = (Mach*)p;
	if((pte = mmuwalk(pdb, MACHADDR, 2, 0)) == nil)
		return;
	*pte = PADDR(mach)|PTEWRITE|PTEVALID;
	if(mach0->havepge)
		*pte |= PTEGLOBAL;
	p += BY2PG;

	machno = apic->machno;
	MACHP(machno) = mach;
	mach->machno = machno;
	mach->pdb = pdb;
	mach->gdt = (Segdesc*)p;	/* filled by mmuinit */

	/*
	 * Tell the AP where its kernel vector and pdb are.
	 * The offsets are known in the AP bootstrap code.
	 */
	apbootp = (ulong*)(APBOOTSTRAP+0x08);
	*apbootp++ = (ulong)squidboy;	/* assembler jumps here eventually */
	*apbootp++ = PADDR(pdb);
	*apbootp = (ulong)apic;

	/*
	 * Universal Startup Algorithm.
	 */
	p = KADDR(0x467);		/* warm-reset vector */
	*p++ = PADDR(APBOOTSTRAP);
	*p++ = PADDR(APBOOTSTRAP)>>8;
	i = (PADDR(APBOOTSTRAP) & ~0xFFFF)/16;
	/* code assumes i==0 */
	if(i != 0)
		print("mp: bad APBOOTSTRAP\n");
	*p++ = i;
	*p = i>>8;

	coherence();

	nvramwrite(0x0F, 0x0A);	/* shutdown code: warm reset upon init ipi */
	lapicstartap(apic, PADDR(APBOOTSTRAP));
	for(i = 0; i < 1000; i++){
		if(apic->online)
			break;
		delay(10);
	}
	nvramwrite(0x0F, 0x00);
}

static void
trympacpi(void)
{
	if (mpacpifunc != nil) {
		print("mpinit: scanning acpi madt for extra cpus\n");
		(*mpacpifunc)();
	}
}

void
mpinit(void)
{
	int ncpu, cpuson;
	char *cp;
	PCMP *pcmp;
	uchar *e, *p;
	Apic *apic, *bpapic;
	void *va;

	mpdebug = getconf("*debugmp") != nil;
	i8259init();
	syncclock();

	bpapic = nil;
	cpuson = 0;

	if(_mp_ == 0) {
		/*
		 * We can easily get processor info from ACPI, but
		 * interrupt routing, etc. would require interpreting AML.
		 */
		print("mpinit: no mp table found, assuming uniprocessor\n");
		archrevert();
		return;
	}
	pcmp = KADDR(_mp_->physaddr);

	/*
	 * Map the local APIC.
	 */
	if((va = vmap(pcmp->lapicbase, 1024)) == nil)
		return;
	mppcmp = pcmp;
	print("LAPIC: %#lux %#lux\n", pcmp->lapicbase, (ulong)va);

	/*
	 * Run through the table saving information needed for starting
	 * application processors and initialising any I/O APICs. The table
	 * is guaranteed to be in order such that only one pass is necessary.
	 */
	p = ((uchar*)pcmp)+sizeof(PCMP);
	e = ((uchar*)pcmp)+pcmp->length;
	while(p < e) switch(*p){

	default:
		print("mpinit: unknown PCMP type 0x%uX (e-p 0x%luX)\n",
			*p, e-p);
		while(p < e){
			print("%uX ", *p);
			p++;
		}
		break;

	case PcmpPROCESSOR:
		if(apic = mkprocessor((PCMPprocessor*)p)){
			/*
			 * Must take a note of bootstrap processor APIC
			 * now as it will be needed in order to start the
			 * application processors later and there's no
			 * guarantee that the bootstrap processor appears
			 * first in the table before the others.
			 */
			apic->addr = va;
			apic->paddr = pcmp->lapicbase;
			if(apic->flags & PcmpBP)
				bpapic = apic;
			cpuson++;
		}
		p += sizeof(PCMPprocessor);
		continue;

	case PcmpBUS:
		mkbus((PCMPbus*)p);
		p += sizeof(PCMPbus);
		continue;

	case PcmpIOAPIC:
		if(apic = mkioapic((PCMPioapic*)p))
			ioapicinit(apic, ((PCMPioapic*)p)->apicno);
		p += sizeof(PCMPioapic);
		continue;

	case PcmpIOINTR:
		mkiointr((PCMPintr*)p);
		p += sizeof(PCMPintr);
		continue;

	case PcmpLINTR:
		mklintr((PCMPintr*)p);
		p += sizeof(PCMPintr);
		continue;
	}

	dprint("mpinit: mp table describes %d cpus\n", cpuson);

	/* For now, always scan ACPI's MADT for processors that MP missed. */
	trympacpi();

	if (bpapic == nil)
		bpapic = bootapic;

	/*
	 * No bootstrap processor, no need to go further.
	 */
	if(bpapic == 0)
		return;
	bpapic->online = 1;

	lapicinit(bpapic);

	/*
	 * These interrupts are local to the processor
	 * and do not appear in the I/O APIC so it is OK
	 * to set them now.
	 */
	intrenable(IrqTIMER, lapicclock, 0, BUSUNKNOWN, "clock");
	intrenable(IrqERROR, lapicerror, 0, BUSUNKNOWN, "lapicerror");
	intrenable(IrqSPURIOUS, lapicspurious, 0, BUSUNKNOWN, "lapicspurious");
	lapiconline();

	checkmtrr();

	/*
	 * Initialise the application processors.
	 */
	if(cp = getconf("*ncpu")){
		ncpu = strtol(cp, 0, 0);
		if(ncpu < 1)
			ncpu = 1;
		else if(ncpu > MAXMACH)
			ncpu = MAXMACH;
	}
	else
		ncpu = MAXMACH;
	memmove((void*)APBOOTSTRAP, apbootstrap, sizeof(apbootstrap));
	for(apic = mpapic; apic <= &mpapic[MaxAPICNO]; apic++){
		if(ncpu <= 1)
			break;
		if((apic->flags & (PcmpBP|PcmpEN)) == PcmpEN
		&& apic->type == PcmpPROCESSOR){
			mpstartap(apic);
			conf.nmach++;
			ncpu--;
		}
	}

	/*
	 *  we don't really know the number of processors till
	 *  here.
	 *
	 *  set conf.copymode here if nmach > 1.
	 *  Should look for an ExtINT line and enable it.
	 */
	if(X86FAMILY(m->cpuidax) == 3 || conf.nmach > 1)
		conf.copymode = 1;
}

static int
mpintrcpu(void)
{
	int i;

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
	 * Currently, just assign each interrupt to a different CPU on
	 * a round-robin basis. Some idea of the packages/cores/thread
	 * topology would be useful here, e.g. to not assign interrupts
	 * to more than one thread in a core, or to use a "noise" core.
	 * But, as usual, Intel make that an onerous task. 
	 */

	/*
	 * temporary workaround for many-core intel (non-amd) systems:
	 * always use cpu 0.  (TODO)
	 */
	if(strncmp(m->cpuidid, "AuthenticAMD", 12) != 0 && conf.nmach > 8)
		return 0;

	lock(&mpphysidlock);
	for(;;){
		i = mpphysid++;
		if(mpphysid >= MaxAPICNO+1)
			mpphysid = 0;
		if(mpapic[i].online)
			break;
	}
	unlock(&mpphysidlock);

	return mpapic[i].apicno;
}

static int
mpintrenablex(Vctl* v, int tbdf)
{
	Bus *bus;
	Aintr *aintr;
	Apic *apic;
	Pcidev *pcidev;
	int bno, dno, hi, irq, lo, n, type, vno;
	char *typenm;

	/*
	 * Find the bus.
	 */
	type = BUSTYPE(tbdf);
	bno = BUSBNO(tbdf);
	dno = BUSDNO(tbdf);
	if(type == BusISA)
		bno = mpisabus;
	vno = -1;
	for(bus = mpbus; bus != nil; bus = bus->next){
		if(bus->type != type)
			continue;
		if(bus->busno == bno)
			break;
	}
	if(bus == nil){
		typenm = type < 0 || type >= nelem(buses)? "": buses[type];
		print("mpintrenablex: can't find bus type %d (%s) for irq %d "
			"%s busno %d\n", type, typenm, v->irq, v->name, bno);
		return -1;
	}

	/*
	 * For PCI devices the interrupt pin (INT[ABCD]) and device
	 * number are encoded into the entry irq field, so create something
	 * to match on. The interrupt pin used by the device has to be
	 * obtained from the PCI config space.
	 */
	if(bus->type == BusPCI){
		pcidev = pcimatchtbdf(tbdf);
		if(pcidev != nil && (n = pcicfgr8(pcidev, PciINTP)) != 0)
			irq = (dno<<2)|(n-1);
		else
			irq = -1;
		//print("pcidev %#uX: irq %#uX v->irq %#uX\n", tbdf, irq, v->irq);
	}
	else
		irq = v->irq;

	/*
	 * Find a matching interrupt entry from the list of interrupts
	 * attached to this bus.
	 */
	for(aintr = bus->aintr; aintr; aintr = aintr->next){
		if(aintr->intr->irq != irq)
			continue;
		if (0) {
			PCMPintr* p = aintr->intr;

	   	 	print("mpintrenablex: bus %d intin %d irq %d\n",
				p->busno, p->intin, p->irq);
		}
		/*
		 * Check if already enabled. Multifunction devices may share
		 * INT[A-D]# so, if already enabled, check the polarity matches
		 * and the trigger is level.
		 *
		 * Should check the devices differ only in the function number,
		 * but that can wait for the planned enable/disable rewrite.
		 * The RDT read here is safe for now as currently interrupts
		 * are never disabled once enabled.
		 */
		apic = aintr->apic;
		ioapicrdtr(apic, aintr->intr->intin, 0, &lo);
		if(!(lo & ApicIMASK)){
			vno = lo & 0xFF;
//print("%s vector %d (!imask)\n", v->name, vno);
			n = mpintrinit(bus, aintr->intr, vno, v->irq);
			n |= ApicPHYSICAL;		/* no-op */
			lo &= ~(ApicRemoteIRR|ApicDELIVS);
			if(n != lo || !(n & ApicLEVEL)){
				print("mpintrenable: multiple botch irq%d, tbdf %uX, lo %8.8uX, n %8.8uX\n",
					v->irq, tbdf, lo, n);
				return -1;
			}
			break;
		}

		/*
		 * With the APIC a unique vector can be assigned to each
		 * request to enable an interrupt. There are two reasons this
		 * is a good idea:
		 * 1) to prevent lost interrupts, no more than 2 interrupts
		 *    should be assigned per block of 16 vectors (there is an
		 *    in-service entry and a holding entry for each priority
		 *    level and there is one priority level per block of 16
		 *    interrupts).
		 * 2) each input pin on the IOAPIC will receive a different
		 *    vector regardless of whether the devices on that pin use
		 *    the same IRQ as devices on another pin.
		 */
		vno = VectorAPIC + (incref(&mpvnoref)-1)*8;
//print("%s vector %d (imask)\n", v->name, vno);
		if(vno > MaxVectorAPIC){
			print("mpintrenable: vno %d, irq %d, tbdf %uX\n",
				vno, v->irq, tbdf);
			return -1;
		}

		hi = mpintrcpu()<<24;
		lo = mpintrinit(bus, aintr->intr, vno, v->irq);
		//print("lo 0x%uX: busno %d intr %d vno %d irq %d elcr 0x%uX\n",
		//	lo, bus->busno, aintr->intr->irq, vno,
		//	v->irq, i8259elcr);
		if(lo & ApicIMASK)
			return -1;
		lo |= ApicPHYSICAL;			/* no-op */

		if((apic->flags & PcmpEN) && apic->type == PcmpIOAPIC)
 			ioapicrdtw(apic, aintr->intr->intin, hi, lo);
		//else
		//	print("lo not enabled 0x%uX %d\n",
		//		apic->flags, apic->type);
		break;
	}
	if (aintr) {
		v->isr = lapicisr;
		v->eoi = lapiceoi;
	}
	return vno;
}

int
mpintrenable(Vctl* v)
{
	int irq, tbdf, vno;

	/*
	 * If the bus is known, try it.
	 * BUSUNKNOWN is given both by [E]ISA devices and by
	 * interrupts local to the processor (local APIC, coprocessor
	 * breakpoint and page-fault).
	 */
	tbdf = v->tbdf;
	if(tbdf != BUSUNKNOWN && (vno = mpintrenablex(v, tbdf)) != -1)
		return vno;

	irq = v->irq;
	if(irq >= IrqLINT0 && irq <= MaxIrqLAPIC){
		if(irq != IrqSPURIOUS)
			v->isr = lapiceoi;
		return VectorPIC+irq;
	}
	if(irq < 0 || irq > MaxIrqPIC){
		print("mpintrenable: irq %d out of range\n", irq);
		return -1;
	}

	/*
	 * Either didn't find it or have to try the default buses
	 * (ISA and EISA). This hack is due to either over-zealousness 
	 * or laziness on the part of some manufacturers.
	 *
	 * The MP configuration table on some older systems
	 * (e.g. ASUS PCI/E-P54NP4) has an entry for the EISA bus
	 * but none for ISA. It also has the interrupt type and
	 * polarity set to 'default for this bus' which wouldn't
	 * be compatible with ISA.
	 */
	if(mpeisabus != -1){
		vno = mpintrenablex(v, MKBUS(BusEISA, 0, 0, 0));
		if(vno != -1)
			return vno;
	}
	if(mpisabus != -1){
		vno = mpintrenablex(v, MKBUS(BusISA, 0, 0, 0));
		if(vno != -1)
			return vno;
	}
	print("mpintrenable: out of choices eisa %d isa %d tbdf %#ux irq %d\n",
		mpeisabus, mpisabus, v->tbdf, v->irq);
	return -1;
}

static Lock mpshutdownlock;

void
mpshutdown(void)
{
	/*
	 * To be done...
	 */
	if(!canlock(&mpshutdownlock)){
		/*
		 * If this processor received the CTRL-ALT-DEL from
		 * the keyboard, acknowledge it. Send an INIT to self.
		 */
#ifdef FIXTHIS
		if(lapicisr(VectorKBD))
			lapiceoi(VectorKBD);
#endif /* FIX THIS */
		arch->introff();
		idle();
	}

	if(active.rebooting)
		return;
	print("apshutdown: active = %#8.8ux\n", active.machs);
	delay(1000);
	splhi();
	arch->resetothers();

	pcireset();
	i8042reset();

	/*
	 * Often the BIOS hangs during restart if a conventional 8042
	 * warm-boot sequence is tried. The following is Intel specific and
	 * seems to perform a cold-boot, but at least it comes back.
	 * And sometimes there is no keyboard...
	 *
	 * The reset register (0xcf9) is usually in one of the bridge
	 * chips. The actual location and sequence could be extracted from
	 * ACPI but why bother, this is the end of the line anyway.
	 */
	print("no kbd; trying bios warm boot...");
	*(ushort*)KADDR(0x472) = 0x1234;	/* BIOS warm-boot flag */
	outb(0xCF9, 0x02);
	outb(0xCF9, 0x06);

	print("can't reset\n");
	for(;;)
		idle();
}
