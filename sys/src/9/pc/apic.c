/*
 * Advanced Programmable Interrupt Controllers (Local and I/O).
 * the first APIC was the 82489dx and the first I/O APIC was the 82093aa.
 *
 * lapic ids tend to be cpu number (possibly plus 1) or
 * double it (due to hyperthreading?).
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "mp.h"

ulong* lapicbase;  /* assume all cpus' lapics are at this virtual address */

struct
{
	uvlong	hz;
	ulong	max;
	ulong	min;
	ulong	div;
} lapictimer;

static ulong
lapicr(uint r)
{
	if(lapicbase == 0)
		panic("lapicr: no lapic");
	return lapicbase[r/sizeof *lapicbase];
}

static void
lapicw(uint r, ulong data)
{
	if(lapicbase == 0)
		panic("lapicw: no lapic");
	lapicbase[r/sizeof *lapicbase] = data;
	coherence();
	data = lapicbase[LapicID/sizeof *lapicbase];	/* force the write? */
	USED(data);
}

void
lapiconline(void)
{
	/*
	 * Reload the timer to de-synchronise the processors,
	 * then lower the task priority to allow interrupts to be
	 * accepted by the APIC.
	 */
	microdelay((TK2MS(1)*1000/conf.nmach) * m->machno);
	lapicw(LapicTICR, lapictimer.max);
	lapicw(LapicTIMER, LapicCLKIN|LapicPERIODIC|(VectorPIC+IrqTIMER));

	lapicw(LapicTPR, 0);		/* arch->intron(); */
}

/*
 *  use the i8253 clock to figure out our lapic timer rate.
 */
static void
lapictimerinit(void)
{
	uvlong x, v, hz;

	v = m->cpuhz/1000;
	lapicw(LapicTDCR, LapicX1);
	lapicw(LapicTIMER, ApicIMASK|LapicCLKIN|LapicONESHOT|(VectorPIC+IrqTIMER));

	if(lapictimer.hz == 0){
		x = fastticks(&hz);
		x += hz/10;
		lapicw(LapicTICR, 0xffffffff);
		do{
			v = fastticks(nil);
		}while(v < x);

		lapictimer.hz = (0xffffffffUL-lapicr(LapicTCCR))*10;
		lapictimer.max = lapictimer.hz/HZ;
		lapictimer.min = lapictimer.hz/(100*HZ);

		if(lapictimer.hz > hz-(hz/10)){
			if(lapictimer.hz > hz+(hz/10))
				panic("lapic clock %lld > cpu clock > %lld",
					lapictimer.hz, hz);
			lapictimer.hz = hz;
		}
		assert(lapictimer.hz != 0);
		lapictimer.div = hz/lapictimer.hz;
	}
}

void
lapicinit(Apic* apic)
{
	ulong dfr, ldr, lvt;

	if(lapicbase == nil)
		lapicbase = apic->addr;
	if(lapicbase == nil) {		/* should never happen */
		print("lapicinit: no lapic\n");
		return;
	}

	/*
	 * These don't really matter in Physical mode;
	 * set the defaults anyway.
	 */
	if(conf.x86type == Amd)
		dfr = 0xf0000000;
	else
		dfr = 0xffffffff;
	ldr = 0x00000000;

	lapicw(LapicDFR, dfr);
	lapicw(LapicLDR, ldr);
	lapicw(LapicTPR, 0xff);			/* arch->introff(); */
	lapicw(LapicSVR, LapicENABLE|(VectorPIC+IrqSPURIOUS));

	lapictimerinit();

	/*
	 * Some Pentium revisions have a bug whereby spurious
	 * interrupts are generated in the through-local mode.
	 */
	switch(conf.cpuidax & 0xFFF){
	case 0x526:				/* stepping cB1 */
	case 0x52B:				/* stepping E0 */
	case 0x52C:				/* stepping cC0 */
		/* ignore interrupt immediately after CLI and before STI */
		wrmsr(Msrtr12, 1<<14);
		break;
	}

	/*
	 * Set the local interrupts. It's likely these should just be
	 * masked off for SMP mode as some Pentium Pros have problems if
	 * LINT[01] are set to ExtINT.
	 * Acknowledge any outstanding interrupts.
	lapicw(LapicLINT0, apic->lintr[0]);
	lapicw(LapicLINT1, apic->lintr[1]);
	 */
	lapiceoi(0);

	lvt = (lapicr(LapicVER)>>16) & 0xFF;
	if(lvt >= 4)
		lapicw(LapicPCINT, ApicIMASK|(VectorPIC+IrqPCINT));
	lapicw(LapicERROR, VectorPIC+IrqERROR);
	lapicw(LapicESR, 0);
	lapicr(LapicESR);

	/*
	 * Issue an INIT Level De-Assert to synchronise arbitration ID's.
	 */
	lapicw(LapicICRHI, 0);
	lapicw(LapicICRLO, LapicALLINC|ApicLEVEL|LapicDEASSERT|ApicINIT);
	while(lapicr(LapicICRLO) & ApicDELIVS)
		;

	/*
	 * Do not allow acceptance of interrupts until all initialisation
	 * for this processor is done. For the bootstrap processor this can be
	 * early during initialisation. For the application processors this should
	 * be after the bootstrap processor has lowered priority and is accepting
	 * interrupts.
	lapicw(LapicTPR, 0);
	 */
}

void
lapicstartap(Apic* apic, int v)
{
	int i;
	ulong crhi;

	/* make apic's processor do a warm reset */
	crhi = apic->apicno<<24;
	lapicw(LapicICRHI, crhi);
	lapicw(LapicICRLO, LapicFIELD|ApicLEVEL|LapicASSERT|ApicINIT);
	microdelay(200);
	lapicw(LapicICRLO, LapicFIELD|ApicLEVEL|LapicDEASSERT|ApicINIT);
	delay(10);

	/* assumes apic is not an 82489dx */
	for(i = 0; i < 2; i++){
		lapicw(LapicICRHI, crhi);
		/* make apic's processor start at v in real mode */
		lapicw(LapicICRLO, LapicFIELD|ApicEDGE|ApicSTARTUP|(v/BY2PG));
		microdelay(200);
	}
}

int
lapicerror(Ureg*, void*)
{
	ulong esr;

	lapicw(LapicESR, 0);			/* required by intel */
	esr = lapicr(LapicESR);
	switch(conf.cpuidax & 0xFFF){
	case 0x526:				/* stepping cB1 */
	case 0x52B:				/* stepping E0 */
	case 0x52C:				/* stepping cC0 */
		return Intrtrap;
	}
	if (esr == 0) {
		// print("cpu%d: lapic error trap but no error!\n", m->machno);
	} else if (!active.exiting)
		print("cpu%d: lapic error trap: esr %#lux\n", m->machno, esr);
	return Intrtrap;
}

int
lapicspurious(Ureg*, void*)
{
	static int traps;

	if (++traps > 1)
		print("cpu%d: lapic spurious intr\n", m->machno);
	return Intrtrap;
}

/*
 * get in-service interrupt bit for vector v.
 *
 * the function's type signature is fixed by Vctl.
 */
int
lapicisr(int av)
{
	ulong isr, v;

	v = av;
	isr = lapicr(LapicISR + v/32);
	return isr & (1<<(v%32));
}

/*
 * dismiss interrupt for vector v (actually the current one).
 */
int
lapiceoi(int v)
{
	lapicw(LapicEOI, 0);
	return v;
}

void
lapicicrw(ulong hi, ulong lo)
{
	lapicw(LapicICRHI, hi);
	lapicw(LapicICRLO, lo);
}

void
ioapicrdtr(Apic* apic, int sel, int* hi, int* lo)
{
	ulong *iowin;

	iowin = apic->addr+(0x10/sizeof(ulong));
	sel = IoapicRDT + 2*sel;

	lock(apic);
	*apic->addr = sel+1;
	if(hi)
		*hi = *iowin;
	*apic->addr = sel;
	if(lo)
		*lo = *iowin;
	unlock(apic);
}

void
ioapicrdtw(Apic* apic, int sel, int hi, int lo)
{
	ulong *iowin;

	iowin = apic->addr+(0x10/sizeof(ulong));
	sel = IoapicRDT + 2*sel;

	lock(apic);
	*apic->addr = sel+1;
	*iowin = hi;
	*apic->addr = sel;
	*iowin = lo;
	unlock(apic);
}

void
ioapicinit(Apic* apic, int apicno)
{
	int hi, lo, v;
	ulong *iowin;

	/*
	 * Initialise the I/O APIC.
	 * The MultiProcessor Specification says it is the responsibility
	 * of the O/S to set the APIC id.
	 * Make sure interrupts are all masked off for now.
	 */
	iowin = apic->addr+(0x10/sizeof(ulong));
	lock(apic);
	*apic->addr = IoapicVER;
	apic->mre = (*iowin>>16) & 0xFF;

	*apic->addr = IoapicID;
	*iowin = apicno<<24;
	unlock(apic);

	hi = 0;
	lo = ApicIMASK;
	for(v = 0; v <= apic->mre; v++)
		ioapicrdtw(apic, v, hi, lo);
}

unsigned
lapicid(uintptr lapicaddr)
{
	return (*(ulong *)(lapicaddr + LapicID)) >> 24;
}

void
lapictimerset(uvlong next)
{
	vlong period;
	int x;

	x = splhi();
	lock(&m->apictimerlock);

	period = lapictimer.max;
	if(next != 0){
		if (lapictimer.div == 0)
			panic("lapictimerset: zero lapictimer.div");
		period = (next - fastticks(nil)) / lapictimer.div;
		if(period < lapictimer.min)
			period = lapictimer.min;
		else if(period > lapictimer.max - lapictimer.min)
			period = lapictimer.max;
	}
	lapicw(LapicTICR, period);

	unlock(&m->apictimerlock);
	splx(x);
}

int
lapicclock(Ureg *u, void*)
{
	/*
	 * since the MTRR updates need to be synchronized across processors,
	 * we want to do this within the clock tick.
	 */
	mtrrclock();
	timerintr(u, 0);
	return Intrforme;
}

void
lapicintron(void)
{
	lapicw(LapicTPR, 0);
}

void
lapicintroff(void)
{
	lapicw(LapicTPR, 0xFF);
}

void
lapicnmienable(void)
{
	/*
	 * On the one hand the manual says the vector information
	 * is ignored if the delivery mode is NMI, and on the other
	 * a "Receive Illegal Vector" should be generated for a
	 * vector in the range 0 through 15.
	 * Some implementations generate the error interrupt if the
	 * NMI vector is invalid, so always give a valid value.
	 */
	if(lapicbase != nil)		/* always true */
		lapicw(LapicPCINT, ApicNMI|(VectorPIC+IrqPCINT));
	else
		print("lapicnmienable: no lapic\n");
}

void
lapicnmidisable(void)
{
	if(lapicbase != nil)		/* always true */
		lapicw(LapicPCINT, ApicIMASK|(VectorPIC+IrqPCINT));
	else
		print("lapicnmidisable: no lapic\n");
}

void
mpresetothers(void)
{
	/*
	 * INIT all excluding self.
	 */
	lapicicrw(0, LapicALLEXC|ApicINIT);
}

void
nmitoself(void)
{
	if(lapicbase != nil)		/* always true */
		lapicicrw(0, LapicSELF|ApicNMI);
}
