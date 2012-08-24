#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

#include "mp.h"

_MP_ *_mp_;

static void
mpresetothers(void)
{
	/*
	 * INIT all excluding self.
	 */
	lapicicrw(0, 0x000C0000|ApicINIT);
}

static int identify(void);

PCArch archmp = {
.id=		"_MP_",	
.ident=		identify,
.reset=		mpshutdown,
.intrinit=	mpinit,
.intrenable=	mpintrenable,
.intron=	lapicintron,
.introff=	lapicintroff,
.fastclock=	i8253read,
.timerset=	lapictimerset,
.resetothers=	mpresetothers,
};

static int
identify(void)
{
	char *cp;
	PCMP *pcmp;
	uchar *p, sum;
	ulong length;

	if((cp = getconf("*nomp")) != nil && strtol(cp, 0, 0) != 0)
		return 1;

	/*
	 * Search for an MP configuration table. For now,
	 * don't accept the default configurations (physaddr == 0).
	 * Check for correct signature, calculate the checksum and,
	 * if correct, check the version.
	 * To do: check extended table checksum.
	 */
	if((_mp_ = sigsearch("_MP_")) == 0 || _mp_->physaddr == 0) {
		/*
		 * we can easily get processor info from acpi, but
		 * interrupt routing, etc. would require interpreting aml.
		 */
		print("archmp: no mp table found, assuming uniprocessor\n");
		return 1;
	}

	if (0)
		iprint("mp physaddr %#lux\n", _mp_->physaddr);
	pcmp = KADDR(_mp_->physaddr);
	if(memcmp(pcmp, "PCMP", 4) != 0) {
		print("archmp: mp table has bad magic");
		return 1;
	}

	length = pcmp->length;
	sum = 0;
	for(p = (uchar*)pcmp; length; length--)
		sum += *p++;

	if(sum || (pcmp->version != 1 && pcmp->version != 4))
		return 1;

	if(cpuserver && m->havetsc)
		archmp.fastclock = tscticks;
	return 0;
}

Lock mpsynclock;

void
syncclock(void)
{
	uvlong x;

	if(arch->fastclock != tscticks)
		return;

	if(m->machno == 0){
		wrmsr(0x10, 0);
		m->tscticks = 0;
	} else {
		x = MACHP(0)->tscticks;
		while(x == MACHP(0)->tscticks)
			;
		wrmsr(0x10, MACHP(0)->tscticks);
		cycles(&m->tscticks);
	}
}

uvlong
tscticks(uvlong *hz)
{
	if(hz != nil)
		*hz = m->cpuhz;

	cycles(&m->tscticks);	/* Uses the rdtsc instruction */
	return m->tscticks;
}
