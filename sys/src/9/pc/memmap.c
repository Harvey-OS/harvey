/*
 * more memory mapping for a normal pc kernel
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "ureg.h"
#include "multiboot.h"
#include "memmap.h"

enum {
	GB4 = 1LL << 32,
};

static void
lowraminit(void)
{
	ulong n, pa, x;

	/*
	 * Initialise the memory bank information for conventional memory
	 * (i.e. less than 640KB). The base is the first location after the
	 * bootstrap processor MMU information and the limit is obtained from
	 * the BIOS data area.
	 */
	x = CPU0END-KZERO;
	n = freebasemem() - x;
	mapfree(&rmapram, x, n);
	memset(KADDR(x), 0, n);			/* keep us honest */

	x = PADDR(PGROUND((ulong)end));
	pa = MemMin;
	if(x > pa)
		panic("kernel too big");
	mapfree(&rmapram, x, pa-x);
	memset(KADDR(x), 0, pa-x);		/* keep us honest */
}


/*
 * BIOS Int 0x15 E820 memory map.
 */
enum
{
	Ememory = 1,
	Ereserved = 2,
	Carry = 1,
};

static char *etypes[] =
{
	"type=0",
	"memory",
	"reserved",
	"acpi reclaim",
	"acpi nvs",
};

static int
mmapcmp(const void *va, const void *vb)
{
	MMap *a, *b;

	a = (MMap*)va;
	b = (MMap*)vb;
	if(a->base < b->base)
		return -1;
	if(a->base > b->base)
		return 1;
	if(a->len < b->len)
		return -1;
	if(a->len > b->len)
		return 1;
	return a->type - b->type;
}

void
map(ulong base, ulong len, int type)
{
	ulong e, n;
	ulong *table, flags, maxkpa;

	/*
	 * Split any call crossing MemMin to make below simpler.
	 */
	if(base < MemMin && len > MemMin-base){
		n = MemMin - base;
		map(base, n, type);
		map(MemMin, len-n, type);
	}

	/*
	 * Let lowraminit and umbscan hash out the low MemMin.
	 */
	if(base < MemMin)
		return;

	/*
	 * Any non-memory below 16*MB is used as upper mem blocks.
	 */
	if(type == MemUPA && base < 16*MB && base+len > 16*MB){
		map(base, 16*MB-base, MemUMB);
		map(16*MB, len-(16*MB-base), MemUPA);
		return;
	}

	/*
	 * Memory below CPU0END is reserved for the kernel
	 * and already mapped.
	 */
	if(base < CPU0END-KZERO){
		n = CPU0END-KZERO - base;
		if(len <= n)
			return;
		map(CPU0END-KZERO, len-n, type);
		return;
	}

	/*
	 * Memory between KTZERO and end is the kernel itself
	 * and is already mapped.
	 */
	if(base < KTZERO-KZERO && base+len > KTZERO-KZERO){
		map(base, KTZERO-KZERO-base, type);
		return;
	}
	if(KTZERO-KZERO < base && base < PADDR(PGROUND((ulong)end))){
		n = PADDR(PGROUND((ulong)end));
		if(len <= n)
			return;
		map(PADDR(PGROUND((ulong)end)), len-n, type);
		return;
	}

	/*
	 * Now we have a simple case.
	 */
	// print("map %.8lux %.8lux %d\n", base, base+len, type);
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
	 * bottom MemMin is already mapped - just twiddle flags.
	 * (not currently used - see above)
	 */
	if(base < MemMin){
		table = KADDR(PPN(m->pdb[PDX(base)]));
		e = base+len;
		base = PPN(base);
		for(; base<e; base+=BY2PG)
			table[PTX(base)] |= flags;
		return;
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

/* create an mmap for only 1 MB to bytes */
void
fakemmap(uvlong bytes)
{
	MMap *e;

	memset(mmap, 0, sizeof mmap);
	e = mmap;
	e->size = sizeof(MMap) - sizeof e->size;
	e->base = MB;
	e->len = bytes - e->base;
	e->type = MemRAM;
	nmmap = 1;
	print("assuming %lldMB - %lldMB available as memory\n",
		e->base/MB, bytes/MB);
}

/* print memory map in 2 columns to avoid scrolling everything off the screen */
void
printmmap(void)
{
	int i;
	MMap *e;

	if(getconf("*noe820print") == nil)
		for(i=0; i<nmmap; i++){
			e = &mmap[i];
			if ((i & 1) == 0)
				print("E820: ");
			print("%.8llux %.8llux ", e->base, e->base + e->len);
			if(e->type < nelem(etypes))
				print("%-12s", etypes[e->type]);
			else
				print("type=%-7ud", e->type);
			print("%c", (i & 1 || i == nmmap-1)? '\n': ' ');
		}
}

/*
 * if loaded by pcboot, this info. is in the tables at BIOSTABLES or MBOOTTAB.
 * if loaded by expand, conf.c puts it in mmap[0] through mmap[nmmap-1].
 */
int
e820scan(void)
{
	int rv;
	ulong base, len;
	uvlong last;
	uintptr e820;
	MMap *e;

	rv = 0;
	e820 = finde820map();
	if (e820 != 0)
		readlsconf(e820); /* translate e820 map from bios.s to mmap[] */
	else {
		print("looking for multiboot map...");
		rv = rdmbootmmap();
		if (rv < 0)
			print("but it's buggered.");
		print("\n");
	}
	if (rv < 0 || nmmap == 0) {		/* must be a pre-2002 PC */
		print("e820scan: old PC has no e820 map & no multiboot map\n");
		fakemmap(64*MB);
	}

	qsort(mmap, nmmap, sizeof mmap[0], mmapcmp);
	printmmap();

	/*
	 * pull out the info but only about the low 32 bits...
	 */
	last = 0;
	for(e = mmap; e < &mmap[nmmap] && e->base < GB4; e++){
		base = e->base;
		if(base+e->len > GB4)
			len = -base;
		else
			len = e->len;
		/*
		 * If the map skips addresses, mark them available.
		 */
		if(last < base)
			map(last, base-last, MemUPA);
		last = base+len;

		if(e->type == Ememory)
			map(base, len, MemRAM);
		else
			map(base, len, MemReserved);
	}
	if(last < GB4)
		map(last, (uint)-last, MemUPA);
	return 0;
}

void
meminit(void)
{
	int i, ci;
	Map *mp;
	Confmem *cm;
	ulong lost, pa, *pte;

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
	if(e820scan() < 0)		/* this never fails in post-2001 PCs */
		panic("e820scan failed on 20th century PC");
		/* ramscan(0); */

	/*
	 * Set the conf entries describing banks of allocatable memory.
	 */
	ci = 0;
	memset(conf.mem, 0, sizeof conf.mem);
	for(i=0; i<nelem(mapram) && ci<nelem(conf.mem); i++){
		mp = &rmapram.map[i];
		if (mp->size == 0)
			break;
		cm = &conf.mem[ci++];
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
