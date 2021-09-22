/*
 * mapping twixt kernel-virtual and physical addresses.
 * trivial mapping on polarfire.
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

void*
KADDR(uintptr pa)
{
	if (pa < (uintptr)KSEG0)
		return (void *)(pa + KSEG0);	/* but is it mapped? */
	panic("KADDR: pa %#p not in kernel from %#p", pa, getcallerpc(&pa));
	return (void *)pa;
}

uintmem
PADDR(void* va)
{
	if((uintptr)va >= KSEG0)
		return (uintptr)va - KSEG0;
	/* probably in user space, could be unmapped peripherals */
	panic("PADDR: va %#p from %#p", va, getcallerpc(&va));
	return (uintmem)va;
}

/*
 * KMap is used to map individual pages into virtual memory.
 * It is rare to have more than a few KMaps at a time (in the
 * absence of interrupts, only two at a time are ever used,
 * but interrupts can stack).  The mappings are local to a process,
 * so we can use the same range of virtual address space for
 * all processes without any coordination.
 */
/* return a working virtual address for page->pa */
KMap*
kmap(Page* page)
{
	void *va;
	uintptr pa;

	if (page == nil)
		panic("kmap: nil page");
	if (page->pa == 0)
		panic("kmap: nil page->pa");
	va = KADDR(page->pa);
//	print("kmap(%#p) @ %#p: %#p %#p\n",
//		page->pa, getcallerpc(&page), page->pa, va);
	/* apparently va need not be page->va */
	pa = mmuphysaddr((uintptr)va);
	if (pa == ~0ull)
		print("kmap: no mapped pa for va %#p (from pa %#p)\n",
			va, page->pa);
	else if (pa != page->pa)
		print("kmap: pa %#p != page->pa %#p; va %#p\n",
			pa, page->pa, va);
	else if (0)
		print("kmap: va %#p maps to pa %#p\n", va, pa);
	return va;
}

/*
 * Return the number of bytes that can be accessed via KADDR(pa)
 * in the first memory bank.
 * If pa is not a valid argument to KADDR, return 0.
 */
uintptr
cankaddr(uintptr pa)
{
	if(pa >= (uintptr)KZERO && pa < KTZERO + membanks[0].size)
		return PHYSMEM + membanks[0].size - pa;
	return 0;
}
