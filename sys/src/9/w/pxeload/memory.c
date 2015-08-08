/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * Size memory and create the kernel page-tables on the fly while doing so.
 * Called from main(), this code should only be run by the bootstrap processor.
 */
#include "u.h"
#include "lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

#define MEMDEBUG 0

#define PDX(va) ((((ulong)(va)) >> 22) & 0x03FF)
#define PTX(va) ((((ulong)(va)) >> 12) & 0x03FF)

enum { MemUPA = 0, /* unbacked physical address */
       MemRAM = 1, /* physical memory */
       MemUMB = 2, /* upper memory block (<16MiB) */
       NMemType = 3,

       MemMinMiB = 4,   /* minimum physical memory (<=4MiB) */
       MemMaxMiB = 768, /* maximum physical memory to check */

       NMemBase = 10,
};

typedef struct Map Map;
struct Map {
	uintptr addr;
	uint32_t size;
};

typedef struct {
	char* name;
	Map* map;
	Map* mapend;

	Lock;
} RMap;

static Map mapupa[8];
static RMap rmapupa = {
    "unallocated unbacked physical memory", mapupa, &mapupa[7],
};

static Map xmapupa[8];
static RMap xrmapupa = {
    "unbacked physical memory", xmapupa, &xmapupa[7],
};

static Map mapram[8];
static RMap rmapram = {
    "physical memory", mapram, &mapram[7],
};

static Map mapumb[64];
static RMap rmapumb = {
    "upper memory block", mapumb, &mapumb[63],
};

static Map mapumbrw[8];
static RMap rmapumbrw = {
    "UMB device memory", mapumbrw, &mapumbrw[7],
};

void
mapprint(RMap* rmap)
{
	Map* mp;

	print("%s\n", rmap->name);
	for(mp = rmap->map; mp->size; mp++) {
		print("\t%#16.16p %#8.8lux %#16.16p\n", mp->addr, mp->size,
		      mp->addr + mp->size);
	}
}

void
memdebug(void)
{
	uintptr maxpa, maxpa1, maxpa2;

	if(!MEMDEBUG)
		return;

	maxpa = (nvramread(0x18) << 8) | nvramread(0x17);
	maxpa1 = (nvramread(0x31) << 8) | nvramread(0x30);
	maxpa2 = (nvramread(0x16) << 8) | nvramread(0x15);
	print("maxpa = %#p -> %#p, maxpa1 = %#p maxpa2 = %#p\n", maxpa,
	      MiB + maxpa * KiB, maxpa1, maxpa2);

	mapprint(&rmapram);
	mapprint(&rmapumb);
	mapprint(&rmapumbrw);
	mapprint(&rmapupa);
}

void
mapfree(RMap* rmap, uint32_t addr, uint32_t size)
{
	Map* mp;
	uint32_t t;

	if(size == 0)
		return;

	lock(rmap);
	for(mp = rmap->map; mp->addr <= addr && mp->size; mp++)
		;

	if(mp > rmap->map && (mp - 1)->addr + (mp - 1)->size == addr) {
		(mp - 1)->size += size;
		if(addr + size == mp->addr) {
			(mp - 1)->size += mp->size;
			while(mp->size) {
				mp++;
				(mp - 1)->addr = mp->addr;
				(mp - 1)->size = mp->size;
			}
		}
	} else {
		if(addr + size == mp->addr && mp->size) {
			mp->addr -= size;
			mp->size += size;
		} else
			do {
				if(mp >= rmap->mapend) {
					print("mapfree: %s: losing 0x%luX, "
					      "%lud\n",
					      rmap->name, addr, size);
					break;
				}
				t = mp->addr;
				mp->addr = addr;
				addr = t;
				t = mp->size;
				mp->size = size;
				mp++;
			} while(size = t);
	}
	unlock(rmap);
}

uint32_t
mapalloc(RMap* rmap, uint32_t addr, int size, int align)
{
	Map* mp;
	uint32_t maddr, oaddr;

	lock(rmap);
	for(mp = rmap->map; mp->size; mp++) {
		maddr = mp->addr;

		if(addr) {
			/*
			 * A specific address range has been given:
			 *   if the current map entry is greater then
			 *   the address is not in the map;
			 *   if the current map entry does not overlap
			 *   the beginning of the requested range then
			 *   continue on to the next map entry;
			 *   if the current map entry does not entirely
			 *   contain the requested range then the range
			 *   is not in the map.
			 */
			if(maddr > addr)
				break;
			if(mp->size < addr - maddr) /* maddr+mp->size < addr,
			                               but no overflow */
				continue;
			if(addr - maddr > mp->size - size) /* addr+size >
			                                      maddr+mp->size,
			                                      but no overflow */
				break;
			maddr = addr;
		}

		if(align > 0)
			maddr = ((maddr + align - 1) / align) * align;
		if(mp->addr + mp->size - maddr < size)
			continue;

		oaddr = mp->addr;
		mp->addr = maddr + size;
		mp->size -= maddr - oaddr + size;
		if(mp->size == 0) {
			do {
				mp++;
				(mp - 1)->addr = mp->addr;
			} while((mp - 1)->size = mp->size);
		}

		unlock(rmap);
		if(oaddr != maddr)
			mapfree(rmap, oaddr, maddr - oaddr);

		return maddr;
	}
	unlock(rmap);

	return 0;
}

static void
umbscan(void)
{
	uint8_t* p;

	/*
	 * Scan the Upper Memory Blocks (0xA0000->0xF0000) for pieces
	 * which aren't used; they can be used later for devices which
	 * want to allocate some virtual address space.
	 * Check for two things:
	 * 1) device BIOS ROM. This should start with a two-byte header
	 *    of 0x55 0xAA, followed by a byte giving the size of the ROM
	 *    in 512-byte chunks. These ROM's must start on a 2KiB boundary.
	 * 2) device memory. This is read-write.
	 * There are some assumptions: there's VGA memory at 0xA0000 and
	 * the VGA BIOS ROM is at 0xC0000. Also, if there's no ROM signature
	 * at 0xE0000 then the whole 64KiB up to 0xF0000 is theoretically up
	 * for grabs; check anyway.
	 */
	p = KADDR(0xD0000); /*RSC: changed from 0xC0000 */
	while(p < (uint8_t*)KADDR(0xE0000)) {
		if(p[0] == 0x55 && p[1] == 0xAA) {
			/* Skip p[2] chunks of 512 bytes.  Test for 0x55 AA
			   before
			     poking obtrusively, or else the Thinkpad X20 dies
			   when
			     setting up the cardbus (PB) */
			p += p[2] * 512;
			continue;
		}

		p[0] = 0xCC;
		p[2 * KiB - 1] = 0xCC;
		if(p[0] != 0xCC || p[2 * KiB - 1] != 0xCC) {
			p[0] = 0x55;
			p[1] = 0xAA;
			p[2] = 4;
			if(p[0] == 0x55 && p[1] == 0xAA) {
				p += p[2] * 512;
				continue;
			}
			if(p[0] == 0xFF && p[1] == 0xFF)
				mapfree(&rmapumb, PADDR(p), 2 * KiB);
		} else
			mapfree(&rmapumbrw, PADDR(p), 2 * KiB);
		p += 2 * KiB;
	}

	p = KADDR(0xE0000);
	if(p[0] != 0x55 || p[1] != 0xAA) {
		p[0] = 0xCC;
		p[64 * KiB - 1] = 0xCC;
		if(p[0] != 0xCC && p[64 * KiB - 1] != 0xCC)
			mapfree(&rmapumb, PADDR(p), 64 * KiB);
	}
}

void meminit(uint32_t)
{
	MMap* map;
	uint32_t modend;
	uint64_t addr, last, len;

	umbscan();

	modend = PADDR(memstart);
	last = 0;
	for(map = mmap; map < &mmap[nmmap]; map++) {
		addr = (((uint64_t)map->base[1]) << 32) | map->base[0];
		len = (((uint64_t)map->length[1]) << 32) | map->length[0];

		switch(map->type) {
		default:
		case 2: /* reserved */
		case 3: /* ACPI Reclaim Memory */
		case 4: /* ACPI NVS Memory */
			break;
		case 1: /* Memory */
			if(addr < 1 * MiB || addr + len < modend)
				break;
			if(addr < modend) {
				len -= modend - addr;
				addr = modend;
			}
			mapraminit(addr, len);
			break;
		}

		if(addr != last && addr > modend) {
			/*
			 * See the comments in main about space < 1MiB.
			 */
			if(addr >= 1 * MiB || addr < 0x000A0000)
				mapupainit(last, addr - last);
		}
		last = addr + len;
	}

	/*
	changeconf("*noe820scan=1\n");
	 */

	if(MEMDEBUG)
		memdebug();
}

uint32_t
umbmalloc(uint32_t addr, int size, int align)
{
	uint32_t a;

	if(a = mapalloc(&rmapumb, addr, size, align))
		return (uint32_t)KADDR(a);

	return 0;
}

void
umbfree(uint32_t addr, int size)
{
	mapfree(&rmapumb, PADDR(addr), size);
}

uint32_t
umbrwmalloc(uint32_t addr, int size, int align)
{
	uint32_t a;
	uint8_t* p;

	if(a = mapalloc(&rmapumbrw, addr, size, align))
		return (uint32_t)KADDR(a);

	/*
	 * Perhaps the memory wasn't visible before
	 * the interface is initialised, so try again.
	 */
	if((a = umbmalloc(addr, size, align)) == 0)
		return 0;
	p = (uint8_t*)a;
	p[0] = 0xCC;
	p[size - 1] = 0xCC;
	if(p[0] == 0xCC && p[size - 1] == 0xCC)
		return a;
	umbfree(a, size);

	return 0;
}

void
umbrwfree(uint32_t addr, int size)
{
	mapfree(&rmapumbrw, PADDR(addr), size);
}

uint32_t*
mmuwalk(uint32_t* pdb, uint32_t va, int level, int create)
{
	uint32_t pa, *table;

	/*
	 * Walk the page-table pointed to by pdb and return a pointer
	 * to the entry for virtual address va at the requested level.
	 * If the entry is invalid and create isn't requested then bail
	 * out early. Otherwise, for the 2nd level walk, allocate a new
	 * page-table page and register it in the 1st level.
	 */
	table = &pdb[PDX(va)];
	if(!(*table & PTEVALID) && create == 0)
		return 0;

	switch(level) {

	default:
		return 0;

	case 1:
		return table;

	case 2:
		if(*table & PTESIZE)
			panic("mmuwalk2: va 0x%ux entry 0x%ux\n", va, *table);
		if(!(*table & PTEVALID)) {
			pa = PADDR(ialloc(BY2PG, BY2PG));
			*table = pa | PTEWRITE | PTEVALID;
		}
		table = KADDR(PPN(*table));

		return &table[PTX(va)];
	}
}

static Lock mmukmaplock;

uint32_t
mmukmap(uint32_t pa, uint32_t va, int size)
{
	uint32_t pae, *table, *pdb, pgsz, *pte, x;
	int pse, sync;
	extern int cpuidax, cpuiddx;

	pdb = KADDR(getcr3());
	if((cpuiddx & 0x08) && (getcr4() & 0x10))
		pse = 1;
	else
		pse = 0;
	sync = 0;

	pa = PPN(pa);
	if(va == 0)
		va = (uint32_t)KADDR(pa);
	else
		va = PPN(va);

	pae = pa + size;
	lock(&mmukmaplock);
	while(pa < pae) {
		table = &pdb[PDX(va)];
		/*
		 * Possibly already mapped.
		 */
		if(*table & PTEVALID) {
			if(*table & PTESIZE) {
				/*
				 * Big page. Does it fit within?
				 * If it does, adjust pgsz so the correct end
				 * can be
				 * returned and get out.
				 * If not, adjust pgsz up to the next 4MiB
				 * boundary
				 * and continue.
				 */
				x = PPN(*table);
				if(x != pa)
					panic(
					    "mmukmap1: pa 0x%ux  entry 0x%ux\n",
					    pa, *table);
				x += 4 * MiB;
				if(pae <= x) {
					pa = pae;
					break;
				}
				pgsz = x - pa;
				pa += pgsz;
				va += pgsz;

				continue;
			} else {
				/*
				 * Little page. Walk to the entry.
				 * If the entry is valid, set pgsz and continue.
				 * If not, make it so, set pgsz, sync and
				 * continue.
				 */
				pte = mmuwalk(pdb, va, 2, 0);
				if(pte && *pte & PTEVALID) {
					x = PPN(*pte);
					if(x != pa)
						panic("mmukmap2: pa 0x%ux "
						      "entry 0x%ux\n",
						      pa, *pte);
					pgsz = BY2PG;
					pa += pgsz;
					va += pgsz;
					sync++;

					continue;
				}
			}
		}

		/*
		 * Not mapped. Check if it can be mapped using a big page -
		 * starts on a 4MiB boundary, size >= 4MiB and processor can do
		 * it.
		 * If not a big page, walk the walk, talk the talk.
		 * Sync is set.
		 */
		if(pse && (pa % (4 * MiB)) == 0 && (pae >= pa + 4 * MiB)) {
			*table =
			    pa | PTESIZE | PTEWRITE | PTEUNCACHED | PTEVALID;
			pgsz = 4 * MiB;
		} else {
			pte = mmuwalk(pdb, va, 2, 1);
			*pte = pa | PTEWRITE | PTEUNCACHED | PTEVALID;
			pgsz = BY2PG;
		}
		pa += pgsz;
		va += pgsz;
		sync++;
	}
	unlock(&mmukmaplock);

	/*
	 * If something was added
	 * then need to sync up.
	 */
	if(sync)
		putcr3(PADDR(pdb));

	return pa;
}

uint32_t
upamalloc(uint32_t addr, int size, int align)
{
	uint32_t ae, a;

	USED(align);

	if((a = mapalloc(&rmapupa, addr, size, align)) == 0) {
		memdebug();
		return 0;
	}

	/*
	 * This is a travesty, but they all are.
	 */
	ae = mmukmap(a, 0, size);

	/*
	 * Should check here that it was all delivered
	 * and put it back and barf if not.
	 */
	USED(ae);

	/*
	 * Be very careful this returns a PHYSICAL address.
	 */
	return a;
}

void
upafree(uint32_t pa, int size)
{
	USED(pa, size);
}

void
mapraminit(uint64_t addr, uint32_t size)
{
	uint32_t s;
	uint64_t a;

	/*
	 * Careful - physical address.
	 * Boot time only to populate map.
	 */
	a = PGROUND(addr);
	s = a - addr;
	if(s >= size)
		return;
	mapfree(&rmapram, a, size - s);
}

void
mapupainit(uint64_t addr, uint32_t size)
{
	uint32_t s;
	uint64_t a;

	/*
	 * Careful - physical address.
	 * Boot time only to populate map.
	 */
	a = PGROUND(addr);
	s = a - addr;
	if(s >= size)
		return;
	mapfree(&rmapupa, a, size - s);
}
