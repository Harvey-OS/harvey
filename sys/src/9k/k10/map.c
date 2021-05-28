#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#define _KADDR(pa)	UINT2PTR(kseg0+((uintptr)(pa)))
#define _PADDR(va)	PTR2UINT(((uintptr)(va)) - kseg0)

void*
KADDR(uintptr pa)
{
	u8int* va;

	va = UINT2PTR(pa);
	if(pa < TMFM)
		return KSEG0+va;
	return KSEG2+va;
}

uintmem
PADDR(void* va)
{
	uintmem pa;

	pa = PTR2UINT(va);
	if(pa >= KSEG0 && pa < KSEG0+TMFM)
		return pa-KSEG0;
	if(pa > KSEG2)
		return pa-KSEG2;

	panic("PADDR: va %#p pa #%p @ %#p\n", va, _PADDR(va), getcallerpc(&va));
	return 0;
}

KMap*
kmap(Page* page)
{
//	print("kmap(%#llux) @ %#p: %#p %#p\n",
//		page->pa, getcallerpc(&page),
//		page->pa, KADDR(page->pa));

	return KADDR(page->pa);
}
