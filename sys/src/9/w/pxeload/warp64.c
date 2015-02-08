/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "u.h"
#include "lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

#include "mmu64.h"

#define Pml4	0x00108000

typedef unsigned long long u64intptr;

#define u64ptr2int(p)	((u64intptr)(p))
#define u64int2ptr(i)	((void*)(i))

static u64intptr
mach0ptalloc(int l)
{
	static u64intptr table = Pml4;
	static int ntable = 6;

	if(ntable <= 0)
		return ~0ULL;

	table += PGSIZE64;
	memset(KADDR(table), 0, PGSIZE64);
	ntable--;
	if(vflag)
		print("table[%d] used 0x%llux\n", l, table);

	return table;
}

PTE*
mmuwalk64(PTE* pml4, u64intptr va, int level, u64intptr (*alloc)(int))
{
	int l, idx;
	PTE *pte;
	u64intptr pa;

	idx = PTEX64(va, 4);
	pte = &pml4[idx];
	for(l = 4; l > 0; l--){
		if(vflag)
			print("mmuwalk64 0x%p 0x%llux %d: entry %d\n",
				pml4, va, l, idx);
		if(l == level)
			return pte;
		if(l == 2 && (*pte & PtePS))
			break;
		if(!(*pte & PteP)){
			if(alloc == nil)
				break;
			pa = alloc(l);
			if(pa == ~0ULL)
				break;
			*pte = pa|PteRW|PteP;
			if(vflag)
				print("mmuwalk64 0x%p 0x%llux %d: alloc 0x%llux\n",
					pml4, va, l, pa);
		}
		pte = u64int2ptr(KADDR(PPN64(*pte)));
		idx = PTEX64(va, l-1);
		pte += idx;
	}

	return nil;
}

void
mkmach0pt(u64intptr kzero64)
{
	//u32int r;
	u64intptr pa, va;
	PTE *pml4, *pte, *pte0;
	//uvlong uvlr;

	pml4 = u64int2ptr(KADDR(Pml4));
	if(vflag)
		print("pml4 = %p\n", pml4);
	memset(pml4, 0, PGSIZE64);

	va = kzero64;
	for(pa = 0; pa < 4*MiB; pa += 2*MiB){
		pte = mmuwalk64(pml4, va, 2, mach0ptalloc);
		*pte = (PPN64(pa))|PtePS|PteRW|PteP;
		if(vflag)
			print("va %#llux pte %#p *pte %#llux\n", va, pte, *pte);
		va += 2*MiB;
	}
	pte = mmuwalk64(pml4, kzero64, 4, 0);
	if(vflag)
		print("pte l4 %llux == 0x%llux\n", kzero64, *pte);

	pte0 = mmuwalk64(pml4, 0ULL, 2, mach0ptalloc);
	pte0 += PTEX64(0, 2);
	if(vflag)
		print("pte0 l2 @ 0x%p  0 == 0x%llux\n", pte0, *pte0);
	*pte0 = (PPN64(0))|PtePS|PteRW|PteP;
	if(vflag)
		print("pte0 l2 @ 0x%p  0 == 0x%llux\n", pte0, *pte0);

	/*
	 * Have to do this with paging turned off. Bugger.
	r = getcr4();
	r |= Pae;
	putcr4(r);

	r = getcr3();
	putcr3(Pml4);

	rdmsr(Efer, &uvlr);
	uvlr |= Lme;
	wrmsr(Efer, uvlr);
	 */
}

void
warp64(uvlong entry)
{
	u64intptr kzero64;
	extern void _warp64(ulong);

//	kzero64 = KZERO64;
//	if(entry != 0xFFFFFFFF80110000ULL){
//		print("bad entry address %#llux\n", entry);
//		return;
//	}
kzero64 = entry & ~0x000000000fffffffull;
	print("warp64(%#llux) %#llux %d\n", entry, kzero64, nmmap);
	if(vflag)
		print("mkmultiboot\n");
	mkmultiboot();
	if(vflag)
		print("mkmach0pt\n");
	mkmach0pt(kzero64);
	if(vflag)
		print("impulse\n");

	/*
	 * No output after impulse().
	 */
	if(vflag)
		print("_warp64\n");
	impulse();
	_warp64(Pml4);
}
