#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include "apic.h"
#include "sipi.h"

#define SIPIHANDLER	(KZERO+0x3000)

/*
 * Parameters are passed to the bootstrap code via a vector
 * in low memory indexed by the APIC number of the processor.
 * The layout, size, and location have to be kept in sync
 * with the handler code in l64sipi.s.
 */
typedef struct Sipi Sipi;
struct Sipi {
	u32int	pml4;
	u32int	_4_;
	uintptr	stack;
	Mach*	mach;
	uintptr	pc;
};

void
sipi(void)
{
	Apic *apic;
	Mach *mach;
	Sipi *sipi;
	int apicno, i;
	u8int *sipiptr;
	uintmem sipipa;
	u8int *alloc, *p;
	extern void squidboy(int);

	/*
	 * Move the startup code into place,
	 * must be aligned properly.
	 */
	sipipa = mmuphysaddr(SIPIHANDLER);
	if((sipipa & (4*KiB - 1)) || sipipa > (1*MiB - 2*4*KiB))
		return;
	sipiptr = UINT2PTR(SIPIHANDLER);
	memmove(sipiptr, sipihandler, sizeof(sipihandler));
	memset(sipiptr+4*KiB, 0, sizeof(Sipi)*Napic);

	/*
	 * Notes:
	 * The Universal Startup Algorithm described in the MP Spec. 1.4.
	 * The data needed per-processor is the sum of the stack, page
	 * table pages, vsvm page and the Mach page. The layout is similar
	 * to that described in data.h for the bootstrap processor, but
	 * with any unused space elided.
	 */
	for(apicno = 0; apicno < Napic; apicno++){
		apic = &xapic[apicno];
		if(!apic->useable || apic->addr || apic->machno == 0)
			continue;
		sipi = &((Sipi*)(sipiptr+4*KiB))[apicno];

		/*
		 * NOTE: for now, share the page tables with the
		 * bootstrap processor, until this code is worked out,
		 * so only the Mach and stack portions are used below.
		 */
		alloc = mallocalign(MACHSTKSZ+4*PTSZ+4*KiB+MACHSZ, 4096, 0, 0);
		if(alloc == nil)
			continue;
		memset(alloc, 0, MACHSTKSZ+4*PTSZ+4*KiB+MACHSZ);
		p = alloc+MACHSTKSZ;

		sipi->pml4 = cr3get();
		sipi->stack = PTR2UINT(p);

		p += 4*PTSZ+4*KiB;

		/*
		 * Committed. If the AP startup fails, can't safely
		 * release the resources, who knows what mischief
		 * the AP is up to. Perhaps should try to put it
		 * back into the INIT state?
		 */
		mach = (Mach*)p;
		sipi->mach = mach;
		mach->machno = apic->machno;		/* NOT one-to-one... */
		mach->splpc = PTR2UINT(squidboy);
		sipi->pc = mach->splpc;
		mach->apicno = apicno;
		mach->stack = PTR2UINT(alloc);
		mach->vsvm = alloc+MACHSTKSZ+4*PTSZ;
		mach->pml4 = m->pml4;

		p = KADDR(0x467);
		*p++ = sipipa;
		*p++ = sipipa>>8;
		*p++ = 0;
		*p = 0;

		nvramwrite(0x0f, 0x0a);
		apicsipi(apicno, sipipa);

		for(i = 0; i < 1000; i++){
			if(mach->splpc == 0)
				break;
			millidelay(5);
		}
		nvramwrite(0x0f, 0x00);

		DBG("apicno%d: machno %d mach %#p (%#p) %dMHz\n",
			apicno, mach->machno,
			mach, sys->machptr[mach->machno],
			mach->cpumhz);
	}
}
