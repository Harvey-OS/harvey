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
		if(mp = mpscan(KADDR(p), 1024))
			return mp;
	}
	else{
		p = ((bda[0x14]<<8)|bda[0x13])*1024;
		if(mp = mpscan(KADDR(p-1024), 1024))
			return mp;
	}
	return mpscan(KADDR(0xF0000), 0x10000);
}

static int
identify(void)
{
	PCMP *pcmp;
	uchar *p, sum;
	ulong length;

	if(getconf("*nomp"))
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

	return 0;
}

Lock mpsynclock;

void
syncclock(void)
{
	uvlong x;

	if(m->machno == 0){
		wrmsr(0x10, 0);
		m->tscticks = 0;
		m->tscoff = 0;
	} else {
		x = MACHP(0)->tscticks;
		while(x == MACHP(0)->tscticks)
			;
		m->tscoff = MACHP(0)->tscticks;
		wrmsr(0x10, 0);
		m->tscticks = 0;
	}
}

uvlong
tscticks(uvlong *hz)
{
	uvlong t;

	if(hz != nil)
		*hz = m->cpuhz;

	rdtsc(&t);
	m->tscticks = t;

	return t+m->tscoff;
}

PCArch archmp = {
.id=		"_MP_",	
.ident=		identify,
.reset=		mpshutdown,
.intrinit=	mpinit,
.intrenable=	mpintrenable,
.fastclock=	tscticks,
.timerset=	lapictimerset,
};
