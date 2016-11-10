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

#define _KADDR(pa)	UINT2PTR(kseg0+((uintptr)(pa)))
#define _PADDR(va)	PTR2UINT(((uintptr)(va)) - kseg0)

/* physical is from 2 -> 4 GiB */
#define TMFM		(4ULL*GiB)

/* the wacko hole in RISCV address space makes KADDR a bit more complex. */
int km, ku, k2;
void*
KADDR(uintptr_t pa)
{
	if((pa > 2 * GiB) && (pa < TMFM)) {
		km++;
		return (void *)(KSEG0|pa);
	}

	assert(pa < (uintptr_t)kseg2);
	k2++;
	return (void *)((uintptr_t)kseg2|pa);
}

uintmem
PADDR(void* va)
{
	uintmem pa;

print("PADDR(%pa)\n", va);
	pa = PTR2UINT(va);
print("pa %p \n", pa);
	if(pa >= KSEG0) {
		print("KSEG0 range\n");
		return (uintmem)(uint32_t)pa; //-KSEG0;
	}
	if(pa > (uintptr_t)kseg2) {
		print("kseg2 range\n");
		return pa-(uintptr_t)kseg2;
	}

	panic("PADDR: va %#p pa #%p @ %#p\n", va, _PADDR(va), getcallerpc());
	return 0;
}

KMap*
kmap(Page* page)
{
	DBG("kmap(%#llx) @ %#p: %#p %#p\n",
		page->pa, getcallerpc(),
		page->pa, KADDR(page->pa));

	return KADDR(page->pa);
}
