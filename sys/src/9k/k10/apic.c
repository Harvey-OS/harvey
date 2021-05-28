#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include "apic.h"
#include "io.h"

enum {						/* Local APIC registers */
	Id		= 0x0020,		/* Identification */
	Ver		= 0x0030,		/* Version */
	Tp		= 0x0080,		/* Task Priority */
	Ap		= 0x0090,		/* Arbitration Priority */
	Pp		= 0x00a0,		/* Processor Priority */
	Eoi		= 0x00b0,		/* EOI */
	Ld		= 0x00d0,		/* Logical Destination */
	Df		= 0x00e0,		/* Destination Format */
	Siv		= 0x00f0,		/* Spurious Interrupt Vector */
	Is		= 0x0100,		/* Interrupt Status (8) */
	Tm		= 0x0180,		/* Trigger Mode (8) */
	Ir		= 0x0200,		/* Interrupt Request (8) */
	Es		= 0x0280,		/* Error Status */
	Iclo		= 0x0300,		/* Interrupt Command */
	Ichi		= 0x0310,		/* Interrupt Command [63:32] */
	Lvt0		= 0x0320,		/* Local Vector Table 0 */
	Lvt5		= 0x0330,		/* Local Vector Table 5 */
	Lvt4		= 0x0340,		/* Local Vector Table 4 */
	Lvt1		= 0x0350,		/* Local Vector Table 1 */
	Lvt2		= 0x0360,		/* Local Vector Table 2 */
	Lvt3		= 0x0370,		/* Local Vector Table 3 */
	Tic		= 0x0380,		/* Timer Initial Count */
	Tcc		= 0x0390,		/* Timer Current Count */
	Tdc		= 0x03e0,		/* Timer Divide Configuration */

	Tlvt		= Lvt0,			/* Timer */
	Lint0		= Lvt1,			/* Local Interrupt 0 */
	Lint1		= Lvt2,			/* Local Interrupt 1 */
	Elvt		= Lvt3,			/* Error */
	Pmclvt		= Lvt4,			/* Performance Mon. Counter */
	Thslvt		= Lvt5,			/* Thermal Sensor */
};

enum {						/* Siv */
	Swen		= 0x00000100,		/* Software Enable */
	Fdis		= 0x00000200,		/* Focus Disable */
};

enum {						/* Iclo */
	Lassert		= 0x00004000,		/* Assert level */

	DSnone		= 0x00000000,		/* Use Destination Field */
	DSself		= 0x00040000,		/* Self is only destination */
	DSallinc	= 0x00080000,		/* All including self */
	DSallexc	= 0x000c0000,		/* All Excluding self */
};

enum {						/* Tlvt */
	Periodic	= 0x00020000,		/* Periodic Timer Mode */
};

enum {						/* Tdc */
	DivX2		= 0x00000000,		/* Divide by 2 */
	DivX4		= 0x00000001,		/* Divide by 4 */
	DivX8		= 0x00000002,		/* Divide by 8 */
	DivX16		= 0x00000003,		/* Divide by 16 */
	DivX32		= 0x00000008,		/* Divide by 32 */
	DivX64		= 0x00000009,		/* Divide by 64 */
	DivX128		= 0x0000000a,		/* Divide by 128 */
	DivX1		= 0x0000000b,		/* Divide by 1 */
};

static u8int* apicbase;

static u32int
apicrget(int r)
{
	return *((u32int*)(apicbase+r));
}

static void
apicrput(int r, u32int data)
{
	*((u32int*)(apicbase+r)) = data;
}

void
apictprput(int priority)
{
	apicrput(Tp, priority);
}

int
apiceoi(int vecno)
{
	apicrput(Eoi, 0);

	return vecno;
}

int
apicisr(int vecno)
{
	int isr;

	isr = apicrget(Is + (vecno/32)*16);

	return isr & (1<<(vecno%32));
}

void
apicinit(int apicno, uintmem pa, int isbp)
{
	Apic *apic;

	/*
	 * Mark the APIC useable if it has a good ID
	 * and the registers can be mapped.
	 * The APIC Extended Broadcast and ID bits in the HyperTransport
	 * Transaction Control register determine whether 4 or 8 bits
	 * are used for the APIC ID. There is also xAPIC and x2APIC
	 * to be dealt with sometime.
	 */
	DBG("apic%d: pa %#P isbp %d\n", apicno, pa, isbp);
	if(apicno >= Napic){
		print("apic%d: out of range\n", apicno);
		return;
	}
	if((apic = &xapic[apicno])->useable){
		print("apicinit%d: already initialised\n", apicno);
		return;
	}
	if(apicbase == nil){
		if((apicbase = vmap(pa, 1024)) == nil){
			print("apic%d: can't map apicbase\n", apicno);
			return;
		}
		DBG("apic%d: apicbase %#P -> %#p\n", apicno, pa, apicbase);
	}
	apic->useable = 1;

	/*
	 * Assign a machno to the processor associated with this
	 * APIC, it may not be an identity map.
	 * Machno 0 is always the bootstrap processor.
	 */
	if(isbp){
		apic->machno = 0;
		m->apicno = apicno;
	}
	else
		apic->machno = sys->nmach++;
}

void
apicdump(void)
{
	int i;
	Apic *apic;

	if(!DBGFLG)
		return;

	DBG("apicbase %#p sys->nmach %d\n", apicbase, sys->nmach);
	for(i = 0; i < Napic; i++){
		apic = &xapic[i];
		if(!apic->useable || apic->addr != 0)
			continue;
		DBG("apic%d: machno %d lint0 %#8.8ux lint1 %#8.8ux\n",
			i, apic->machno, apic->lvt[0], apic->lvt[1]);
		DBG(" thslvt %#8.8ux pmclvt %#8.8ux elvt %#8.8ux\n",
			apicrget(Thslvt), apicrget(Pmclvt), apicrget(Elvt));
		DBG(" tlvt %#8.8ux lint0 %#8.8ux lint1 %#8.8ux siv %#8.8ux\n",
			apicrget(Tlvt), apicrget(Lint0),
			apicrget(Lint1), apicrget(Siv));
	}
}

int
apiconline(void)
{
	Apic *apic;
	u64int tsc;
	u32int dfr, ver;
	int apicno, nlvt;

	if(apicbase == nil)
		return 0;
	if((apicno = ((apicrget(Id)>>24) & 0xff)) >= Napic)
		return 0;
	apic = &xapic[apicno];
	if(!apic->useable || apic->addr != nil)
		return 0;

	/*
	 * Things that can only be done when on the processor
	 * owning the APIC, apicinit above runs on the bootstrap
	 * processor.
	 */
	ver = apicrget(Ver);
	nlvt = ((ver>>16) & 0xff) + 1;
	if(nlvt > nelem(apic->lvt)){
		print("apicinit%d: nlvt %d > max (%d)\n",
			apicno, nlvt, nelem(apic->lvt));
		nlvt = nelem(apic->lvt);
	}
	apic->nlvt = nlvt;
	apic->ver = ver & 0xff;

	/*
	 * These don't really matter in Physical mode;
	 * set the defaults anyway.
	 */
	if(memcmp(&m->cpuinfo[0][1], "AuthenticAMD", 12) == 0)
		dfr = 0xf0000000;
	else
		dfr = 0xffffffff;
	apicrput(Df, dfr);
	apicrput(Ld, 0x00000000);

	/*
	 * Disable interrupts until ready by setting the Task Priority
	 * register to 0xff.
	 */
	apicrput(Tp, 0xff);

	/*
	 * Software-enable the APIC in the Spurious Interrupt Vector
	 * register and set the vector number. The vector number must have
	 * bits 3-0 0x0f unless the Extended Spurious Vector Enable bit
	 * is set in the HyperTransport Transaction Control register.
	 */
	apicrput(Siv, Swen|IdtSPURIOUS);

	/*
	 * Acknowledge any outstanding interrupts.
	 */
	apicrput(Eoi, 0);

	/*
	 * Use the TSC to determine the APIC timer frequency.
	 * It might be possible to snarf this from a chipset
	 * register instead.
	 */
	apicrput(Tdc, DivX1);
	apicrput(Tlvt, Im|IdtTIMER);
	tsc = rdtsc() + m->cpuhz/10;
	apicrput(Tic, 0xffffffff);

	while(rdtsc() < tsc)
		;

	apic->hz = (0xffffffff-apicrget(Tcc))*10;
	apic->max = apic->hz/HZ;
	apic->min = apic->hz/(100*HZ);
	apic->div = ((m->cpuhz/apic->max)+HZ/2)/HZ;

	if(m->machno == 0 || DBGFLG){
		print("apic%d: hz %lld max %lld min %lld div %lld\n", apicno,
			apic->hz, apic->max, apic->min, apic->div);
	}

	/*
	 * Mask interrupts on Performance Monitor Counter overflow and
	 * Thermal Sensor if implemented, and on Lintr0 (Legacy INTR),
	 * and Lintr1 (Legacy NMI).
	 * Clear any Error Status (write followed by read) and enable
	 * the Error interrupt.
	 */
	switch(apic->nlvt){
	case 6:
		apicrput(Thslvt, Im|IdtTHS);
		/*FALLTHROUGH*/
	case 5:
		apicrput(Pmclvt, Im|IdtPMC);
		/*FALLTHROUGH*/
	default:
		break;
	}
	apicrput(Lint1, apic->lvt[1]|Im|IdtLINT1);
	apicrput(Lint0, apic->lvt[0]|Im|IdtLINT0);

	apicrput(Es, 0);
	apicrget(Es);
	apicrput(Elvt, IdtERROR);

	/*
	 * Issue an INIT Level De-Assert to synchronise arbitration ID's.
	 * (Necessary in this implementation? - not if Pentium 4 or Xeon
	 * (APIC Version >= 0x14), or AMD).
	apicrput(Ichi, 0);
	apicrput(Iclo, DSallinc|Lassert|MTir);
	while(apicrget(Iclo) & Ds)
		;
	 */

	/*
	 * Reload the timer to de-synchronise the processors.
	 * When the caller is ready for the APIC to accept interrupts,
	 * it should call apictprput to lower the task priority.
	 *
	 * The timer is enabled later by the core-specific startup
	 * i.e. don't start the timer unless the core needs it,
	 * to reduce the likelihood of at least one (spurious) interrupt
	 * from the timer when priority is lowered.
	 */
	microdelay((TK2MS(1)*1000/sys->nmach) * m->machno);
	apicrput(Tic, apic->max);

	return 1;
}

void
apictimerenable(void)
{
	/*
	 * Perhaps apictimerenable/apictimerdisable should just
	 * clear/set Im in the existing settings of Tlvt, there may
	 * be a time when the timer is used in a different mode;
	 * if so will need to ensure the mode is set when the timer
	 * is initialised.
	 */
	apicrput(Tlvt, Periodic|IdtTIMER);
}

void
apictimerdisable(void)
{
	apicrput(Tlvt, Im|IdtTIMER);
}

void
apictimerset(uvlong next)
{
	Mpl pl;
	Apic *apic;
	vlong period;

	/*
	 * APIC Timer.
	 */
	apic = &xapic[m->apicno];

	pl = splhi();
	lock(&m->apictimerlock);

	period = apic->max;
	if(next != 0){
		period = next - fastticks(nil);	/* fastticks is just rdtsc() */
		period /= apic->div;

		if(period < apic->min)
			period = apic->min;
		else if(period > apic->max - apic->min)
			period = apic->max;
	}
	apicrput(Tic, period);

	unlock(&m->apictimerlock);
	splx(pl);
}

void
apictimerintr(Ureg* ureg, void*)
{
	timerintr(ureg, 0);
}

void
apicsipi(int apicno, uintptr pa)
{
	int i;
	u32int crhi, crlo;

	/*
	 * SIPI - Start-up IPI.
	 * To do: checks on apic validity.
	 */
	crhi = apicno<<24;
	apicrput(Ichi, crhi);
	apicrput(Iclo, DSnone|TMlevel|Lassert|MTir);
	microdelay(200);
	apicrput(Iclo, DSnone|TMlevel|MTir);
	millidelay(10);

	crlo = DSnone|TMedge|MTsipi|((u32int)pa/(4*KiB));
	for(i = 0; i < 2; i++){
		apicrput(Ichi, crhi);
		apicrput(Iclo, crlo);
		microdelay(200);
	}
}

void
apicipi(int apicno)
{
	apicrput(Ichi, apicno<<24);
	apicrput(Iclo, DSnone|TMedge|Lassert|MTf|IdtIPI);
	while(apicrget(Iclo) & Ds)
		;
}
