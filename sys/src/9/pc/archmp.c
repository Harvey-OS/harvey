#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

#include "mp.h"

_MP_ *_mp_;

static _MP_*
mpscan(uchar *addr, int len)
{
	uchar *e, *p, sum;
	int i;

	e = addr+len;
	for(p = addr; p < e; p += sizeof(_MP_)){
		if(memcmp(p, "_MP_", 4))
			continue;
		sum = 0;
		for(i = 0; i < sizeof(_MP_); i++)
			sum += p[i];
		if(sum == 0)
			return (_MP_*)p;
	}
	return 0;
}

static _MP_*
mpsearch(void)
{
	uchar *bda;
	ulong p;
	_MP_ *mp;

	/*
	 * Search for the MP Floating Pointer Structure:
	 * 1) in the first KB of the EBDA;
	 * 2) in the last KB of system base memory;
	 * 3) in the BIOS ROM between 0xE0000 and 0xFFFFF.
	 */
	bda = KADDR(0x400);
	if((p = (bda[0x0F]<<8)|bda[0x0E])){
		if(mp = mpscan(KADDR(p<<4), 1024))
			return mp;
	}
	else{
		p = ((bda[0x14]<<8)|bda[0x13])*1024;
		if(mp = mpscan(KADDR(p-1024), 1024))
			return mp;
	}
	return mpscan(KADDR(0xF0000), 0x10000);
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
	if((_mp_ = mpsearch()) == 0 || _mp_->physaddr == 0)
		return 1;

	pcmp = KADDR(_mp_->physaddr);
	if(memcmp(pcmp, "PCMP", 4))
		return 1;

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
