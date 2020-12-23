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

#undef DBG
#define DBG   \
	if(1) \
	print
#define SIPIHANDLER (KZERO + 0x3000)

void
sipi(void)
{
	extern char b1978, e1978;
	Apic *apic;
	volatile Mach *mach;
	int apicno, i;
	volatile uint64_t *sipiptr;
	uintmem sipipa;
	uint8_t *p;
	extern void squidboy(int);
	usize apsize;

	/*
	 * Move the startup code into place,
	 * must be aligned properly.
	 */
	sipipa = mmuphysaddr(UINT2PTR(machp()->MMU.pml4->va), SIPIHANDLER);
	if((sipipa == 0 || sipipa & (PGSZ - 1)) || sipipa > (1 * MiB - 2 * PGSZ))
		panic("sipi: SIPI page improperly aligned or too far away, pa %#p", sipipa);
	sipiptr = KADDR(sipipa);
	DBG("sipiptr %#p sipipa %#llx\n", sipiptr, sipipa);
	memmove((void *)sipiptr, &b1978, &e1978 - &b1978);

	/*
	 * Notes: SMP startup algorithm.
	 * The data needed per-processor is the sum of the stack,
	 * vsvm pages and the Mach page. The layout is similar
	 * to that described in dat.h for the bootstrap processor,
	 * but with no unused, and we use the page tables from
	 * early boot.
	 */
	for(apicno = 0; apicno < Napic; apicno++){
		apic = &xlapic[apicno];
		if(!apic->useable || apic->Ioapic.addr || apic->Lapic.machno == 0)
			continue;

		/*
		 * NOTE: for now, share the page tables with the
		 * bootstrap processor, until the lsipi code is worked out,
		 * so only the Mach and stack portions are used below.
		 */
		apsize = MACHSTKSZ + PTSZ + PGSZ;
		p = mallocalign(apsize + MACHSZ, PGSZ, 0, 0);
		if(p == nil)
			panic("sipi: cannot allocate for apicno %d", apicno);

		sipiptr[511] = PTR2UINT(p);
		DBG("sipi mem for apicid %d is %#p sipiptr[511] %#llx\n", apicno, p, sipiptr[511]);

		/*
		 * Committed. If the AP startup fails, can't safely
		 * release the resources, who knows what mischief
		 * the AP is up to. Perhaps should try to put it
		 * back into the INIT state?
		 */
		mach = (volatile Mach *)(p + apsize);
		mach->self = PTR2UINT(mach);
		mach->machno = apic->Lapic.machno; /* NOT one-to-one... */
		mach->splpc = PTR2UINT(squidboy);
		mach->apicno = apicno;
		mach->stack = PTR2UINT(p);
		mach->vsvm = p + MACHSTKSZ + PTSZ;

		DBG("APICSIPI: %d, %p\n", apicno, (void *)sipipa);
		apicsipi(apicno, sipipa);

		for(i = 0; i < 5000; i++){
			if(mach->splpc == 0)
				break;
			millidelay(1);
		}

		DBG("mach %#p (%#p) apicid %d machno %2d %dMHz\n",
		    mach, sys->machptr[mach->machno],
		    apicno, mach->machno, mach->cpumhz);
	}
}
