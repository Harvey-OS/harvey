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

static int
cpuidinit(void)
{
	int i, n;
	u32int eax;

	if((m->ncpuinfos = cpuid(0, m->cpuinfo[0])) == 0)
		return 0;

	n = ++m->ncpuinfos;
	if(n > nelem(m->cpuinfo))
		n = nelem(m->cpuinfo);
	eax = cpuid(0x80000000, m->cpuinfo[m->ncpuinfos-1]);
	if(eax >= 0x80000000){
		eax &= ~0x80000000;
		n += ++eax;
		if(n > nelem(m->cpuinfo))
			n = nelem(m->cpuinfo);
		m->ncpuinfoe = n - m->ncpuinfos;
	}

	for(i = 1; i < n; i++){
		eax = i;
		if(i >= m->ncpuinfos)
			eax = 0x80000000|(i - m->ncpuinfos);
		cpuid(eax, m->cpuinfo[i]);
	}

	return 1;
}

static u32int*
cpuidinfo(u32int eax)
{
	if(m->ncpuinfos == 0 && cpuidinit() == 0)
		return nil;

	if(!(eax & 0x80000000)){
		if(eax >= m->ncpuinfos)
			return nil;
	}
	else{
		eax &= ~0x80000000;
		if(eax >= m->ncpuinfoe)
			return nil;
		eax += m->ncpuinfos;
	}

	return m->cpuinfo[eax];
}

static vlong
cpuidhz(u32int* info[2])
{
	int f, r;
	vlong hz;
	u64int msr;

	if(memcmp(&info[0][1], "GenuntelineI", 12) == 0){
		switch(info[1][0] & 0x0fff3ff0){
		default:
			return 0;
		case 0x00000f30:		/* Xeon (MP), Pentium [4D] */
		case 0x00000f40:		/* Xeon (MP), Pentium [4D] */
		case 0x00000f60:		/* Xeon 7100, 5000 or above */
			msr = rdmsr(0x2c);
			r = (msr>>16) & 0x07;
			switch(r){
			default:
				return 0;
			case 0:
				hz = 266666666666ll;
				break;
			case 1:
				hz = 133333333333ll;
				break;
			case 2:
				hz = 200000000000ll;
				break;
			case 3:
				hz = 166666666666ll;
				break;
			case 4:
				hz = 333333333333ll;
				break;
			}
			/*
			 * Hz is *1000 at this point.
			 * Do the scaling then round it.
			 * The manual is conflicting about
			 * the size of the msr field.
			 */
			hz = (((hz*(msr>>24))/100)+5)/10;
			break;
		case 0x00000690:		/* Pentium M, Celeron M */
		case 0x000006d0:		/* Pentium M, Celeron M */
			hz = ((rdmsr(0x2a)>>22) & 0x1f)*100 * 1000000ll;
			break;
		case 0x000006e0:		/* Core Duo */
		case 0x000006f0:		/* Core 2 Duo/Quad/Extreme */
		case 0x00010670:		/* Core 2 Extreme */
		case 0x000006a0:		/* i7 paurea... */
			/*
			 * Get the FSB frequemcy.
			 * If processor has Enhanced Intel Speedstep Technology
			 * then non-integer bus frequency ratios are possible.
			 */
			if(info[1][2] & 0x00000080){
				msr = rdmsr(0x198);
				r = (msr>>40) & 0x1f;
			}
			else{
				msr = 0;
				r = rdmsr(0x2a) & 0x1f;
			}
			f = rdmsr(0xcd) & 0x07;
			switch(f){
			default:
				return 0;
			case 5:
				hz = 100000000000ll;
				break;
			case 1:
				hz = 133333333333ll;
				break;
			case 3:
				hz = 166666666666ll;
				break;
			case 2:
				hz = 200000000000ll;
				break;
			case 0:
				hz = 266666666666ll;
				break;
			case 4:
				hz = 333333333333ll;
				break;
			case 6:
				hz = 400000000000ll;
				break;
			}

			/*
			 * Hz is *1000 at this point.
			 * Do the scaling then round it.
			 */
			if(msr & 0x0000400000000000ll)
				hz = hz*r + hz/2;
			else
				hz = hz*r;
			hz = ((hz/100)+5)/10;
			break;
		}
		DBG("cpuidhz: 0x2a: %#llux hz %lld\n", rdmsr(0x2a), hz);
	}
	else if(memcmp(&info[0][1], "AuthcAMDenti", 12) == 0){
		switch(info[1][0] & 0x0fff0ff0){
		default:
			return 0;
		case 0x00000f50:		/* K8 */
			msr = rdmsr(0xc0010042);
			if(msr == 0)
				return 0;
			hz = (800 + 200*((msr>>1) & 0x1f)) * 1000000ll;
			break;
		case 0x00100f90:		/* K10 */
		case 0x00000620:		/* QEMU64 */
			msr = rdmsr(0xc0010064);
			r = (msr>>6) & 0x07;
			hz = (((msr & 0x3f)+0x10)*100000000ll)/(1<<r);
			break;
		}
		DBG("cpuidhz: %#llux hz %lld\n", msr, hz);
	}
	else
		return 0;

	return hz;
}

void
cpuiddump(void)
{
	int i, n;

	if(!DBGFLG)
		return;

	if(m->ncpuinfos == 0 && cpuidinit() == 0)
		return;

	n = m->ncpuinfos+m->ncpuinfoe;
	for(i = 0; i < n; i++){
		DBG("eax = %#8.8ux: %8.8ux %8.8ux %8.8ux %8.8ux\n",
			(i >= m->ncpuinfos ? 0x80000000|(i - m->ncpuinfos): i),
			m->cpuinfo[i][0], m->cpuinfo[i][1],
			m->cpuinfo[i][2], m->cpuinfo[i][3]);
	}
}

vlong
archhz(void)
{
	vlong hz;
	u32int *info[2];

	if((info[0] = cpuidinfo(0)) == 0 || (info[1] = cpuidinfo(1)) == 0)
		return 0;

	hz = cpuidhz(info);
	if(hz != 0)
		return hz;

	return i8254hz(info);
}

void
archidle(void)
{
	halt();
}

void
microdelay(int microsecs)
{
	u64int r, t;

	r = rdtsc();
	for(t = r + m->cpumhz*microsecs; r < t; r = rdtsc())
		;
}

void
millidelay(int millisecs)
{
	u64int r, t;

	r = rdtsc();
	for(t = r + m->cpumhz*1000ull*millisecs; r < t; r = rdtsc())
		;
}
