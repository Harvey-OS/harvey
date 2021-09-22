/*
 * more memory mapping for a boot pc kernel
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "memmap.h"

static void
lowraminit(void)
{
	/*
	 * low memory is in use by bootstrap kernels and ROMs.
	 * MemReserved is untouchable, so use MemRAM.
	 * address zero is special to mapalloc, and thus to map, so avoid it.
	 * we can thus load the new kernel directly at 1MB and up.
	 */
//	map(BY2PG, MB - BY2PG, MemRAM)	/* executing this map call is fatal */
	mapalloc(&rmapram, BY2PG, Mallocbase - BY2PG, 0);

	/*
	 * declare all RAM above Mallocbase to be free.
	 */
	map(Mallocbase, MemMax - Mallocbase, MemRAM);

	/* declare rest of physical address space above RAM to be available */
	map(MemMax, KZERO-MemMax, MemUPA);

	/* force the new mappings to take effect */
	mmuflushtlb(PADDR(m->pdb));
}

/*
 * add region at physical base of len bytes to map for `type', and
 * set up page tables to map virtual KZERO|base to physical base.
 */
void
map(ulong base, ulong len, int type)
{
	ulong n, flags, maxkpa;

//	iprint("map %.8lux %.8lux %d (", base, base+len, type);
	/*
	 * Split any call crossing MemMin to make below simpler.
	 */
	if(base < MemMin && len > MemMin-base){
		n = MemMin - base;
		map(base, n, type);
		map(MemMin, len-n, type);
		return;
	}

	switch(type){
	case MemRAM:
		mapfree(&rmapram, base, len);
		flags = PTEWRITE|PTEVALID;
		break;
	case MemUMB:
		mapfree(&rmapumb, base, len);
		flags = PTEWRITE|PTEUNCACHED|PTEVALID;
		break;
	case MemUPA:
		mapfree(&rmapupa, base, len);
		flags = 0;
		break;
	default:
	case MemReserved:
		flags = 0;
		break;
	}

	/*
	 * Only map from KZERO to 2^32.
	 */
	if(flags){
		maxkpa = -KZERO;
		if(base >= maxkpa)
			return;
		if(len > maxkpa-base)
			len = maxkpa - base;
		pdbmap(m->pdb, base|flags, base+KZERO, len);
	}
}

void
meminit(void)
{
	int i, kzsub;
	Map *mp;
	Confmem *cm;
	ulong lost, physpte, pa, *pte;
	uintptr e820;

	if ((e820 = finde820map()) != 0)
		readlsconf(e820);	/* pick up memory map from bios.s */

	/* no need to size memory, we don't need much. */
	pte = m->pdb + BY2PG/BY2WD;		/* see l*.s */

	/* populate pdb with double-mapping of low memory */
	kzsub = ((uintptr)KZERO >> (2*PGSHIFT - 4)) / sizeof(ulong);
	physpte = (uintptr)PADDR(pte);
	for (i = 0; i < LOWPTEPAGES; i++)
		m->pdb[kzsub + i] = m->pdb[i] =
			PTEVALID | PTEKERNEL | PTEWRITE | (physpte + i * BY2PG);

	/*
	 * Set special attributes for memory between 640KB and 1MB:
	 *   VGA memory is writethrough;
	 *   BIOS ROM's/UMB's are uncached;
	 * then scan for useful memory.
	 */
	for(pa = 0xA0000; pa < 0xC0000; pa += BY2PG){
		pte = mmuwalk(m->pdb, (ulong)KADDR(pa), 2, 0);
		*pte |= PTEWT;
	}
	for(pa = 0xC0000; pa < 0x100000; pa += BY2PG){
		pte = mmuwalk(m->pdb, (ulong)KADDR(pa), 2, 0);
		*pte |= PTEUNCACHED;
	}
	mmuflushtlb(PADDR(m->pdb));

	umbscan();
	lowraminit();

	/*
	 * Set the conf entries describing banks of allocatable memory.
	 */
	for(i=0; i<nelem(mapram) && i<nelem(conf.mem); i++){
		mp = &rmapram.map[i];
		if (mp->size == 0)
			break;
		cm = &conf.mem[i];
		cm->base = mp->addr;
		cm->npage = mp->size/BY2PG;
		if (i == 0 && cm->npage == 0)
			panic("meminit: no memory in conf.mem");
	}
	lost = 0;
	for(; i<nelem(mapram); i++)
		lost += rmapram.map[i].size;
	if(lost)
		print("meminit - lost %lud bytes\n", lost);

	if(MEMDEBUG)
		memdebug();
}
