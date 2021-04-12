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

#define _KADDR(pa) UINT2PTR(KZERO + ((usize)(pa)))
#define _PADDR(va) PTR2UINT(((usize)(va)) - KZERO)

void *
KADDR(usize pa)
{
	if(pa < KZERO)
		return _KADDR(pa);

	return UINT2PTR(pa);
}

u64
PADDR(void *va)
{
	u64 pa;

	pa = PTR2UINT(va);
	if(pa >= KZERO)
		return pa - KZERO;

	panic("PADDR: va %#p pa #%p @ %#p\n", va, _PADDR(va), getcallerpc());
	return 0;
}

KMap *
kmap(Page *page)
{
	DBG("kmap(%#llx) @ %#p: %#p %#p\n",
	    page->pa, getcallerpc(),
	    page->pa, KADDR(page->pa));

	return KADDR(page->pa);
}
