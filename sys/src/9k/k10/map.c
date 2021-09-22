/*
 * mapping twixt kernel-virtual and physical addresses.
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

void*
KADDR(uintptr pa)
{
	return (uchar *)pa + (pa < kernmem? KSEG0: KSEG2);
}

uintmem
PADDR(void* va)
{
	uintptr pa;

	pa = (uintptr)va;
	if(pa >= KSEG0 && pa < KSEG0+kernmem)
		return pa-KSEG0;
	if(pa >= KSEG2)
		return pa-KSEG2;

	panic("PADDR: va %#p pa #%p @ %#p", va, (uintptr)va - KSEG0,
		getcallerpc(&va));
	notreached();
}

/* return a working virtual address for page->pa */
KMap*
kmap(Page* page)
{
	void *va;

	va = KADDR(page->pa);
//	print("kmap(%#llux) @ %#p: %#p %#p\n",
//		page->pa, getcallerpc(&page), page->pa, va);
	if (mmuphysaddr((uintptr)va) == ~0ull)
		print("kmap: no mapped pa for va %#p\n", va);
	return va;
}
