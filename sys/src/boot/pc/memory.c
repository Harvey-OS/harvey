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

#define MEMDEBUG	0

#define PDX(va)		((((ulong)(va))>>22) & 0x03FF)
#define PTX(va)		((((ulong)(va))>>12) & 0x03FF)

enum {
	MemUPA		= 0,		/* unbacked physical address */
	MemRAM		= 1,		/* physical memory */
	MemUMB		= 2,		/* upper memory block (<16MB) */
	NMemType	= 3,

	KB		= 1024,

	MemMinMB	= 4,		/* minimum physical memory (<=4MB) */
	MemMaxMB	= 768,		/* maximum physical memory to check */

	NMemBase	= 10,
};

typedef struct {
	int	size;
	ulong	addr;
} Map;

typedef struct {
	char*	name;
	Map*	map;
	Map*	mapend;

	Lock;
} RMap;

static Map mapupa[8];
static RMap rmapupa = {
	"unallocated unbacked physical memory",
	mapupa,
	&mapupa[7],
};

static Map xmapupa[8];
static RMap xrmapupa = {
	"unbacked physical memory",
	xmapupa,
	&xmapupa[7],
};

static Map mapram[8];
static RMap rmapram = {
	"physical memory",
	mapram,
	&mapram[7],
};

static Map mapumb[64];
static RMap rmapumb = {
	"upper memory block",
	mapumb,
	&mapumb[63],
};

static Map mapumbrw[8];
static RMap rmapumbrw = {
	"UMB device memory",
	mapumbrw,
	&mapumbrw[7],
};

void
memdebug(void)
{
	Map *mp;
	ulong maxpa, maxpa1, maxpa2;

	if(MEMDEBUG == 0)
		return;

	maxpa = (nvramread(0x18)<<8)|nvramread(0x17);
	maxpa1 = (nvramread(0x31)<<8)|nvramread(0x30);
	maxpa2 = (nvramread(0x16)<<8)|nvramread(0x15);
	print("maxpa = %uX -> %uX, maxpa1 = %uX maxpa2 = %uX\n",
		maxpa, MB+maxpa*KB, maxpa1, maxpa2);

	for(mp = rmapram.map; mp->size; mp++)
		print("%8.8uX %8.8uX %8.8uX\n", mp->addr, mp->size, mp->addr+mp->size);
	for(mp = rmapumb.map; mp->size; mp++)
		print("%8.8uX %8.8uX %8.8uX\n", mp->addr, mp->size, mp->addr+mp->size);
	for(mp = rmapumbrw.map; mp->size; mp++)
		print("%8.8uX %8.8uX %8.8uX\n", mp->addr, mp->size, mp->addr+mp->size);
	for(mp = rmapupa.map; mp->size; mp++)
		print("%8.8uX %8.8uX %8.8uX\n", mp->addr, mp->size, mp->addr+mp->size);
}

void
mapfree(RMap* rmap, ulong addr, ulong size)
{
	Map *mp;
	ulong t;

	if(size <= 0)
		return;

	lock(rmap);
	for(mp = rmap->map; mp->addr <= addr && mp->size; mp++)
		;

	if(mp > rmap->map && (mp-1)->addr+(mp-1)->size == addr){
		(mp-1)->size += size;
		if(addr+size == mp->addr){
			(mp-1)->size += mp->size;
			while(mp->size){
				mp++;
				(mp-1)->addr = mp->addr;
				(mp-1)->size = mp->size;
			}
		}
	}
	else{
		if(addr+size == mp->addr && mp->size){
			mp->addr -= size;
			mp->size += size;
		}
		else do{
			if(mp >= rmap->mapend){
				print("mapfree: %s: losing 0x%uX, %d\n",
					rmap->name, addr, size);
				break;
			}
			t = mp->addr;
			mp->addr = addr;
			addr = t;
			t = mp->size;
			mp->size = size;
			mp++;
		}while(size = t);
	}
	unlock(rmap);
}

ulong
mapalloc(RMap* rmap, ulong addr, int size, int align)
{
	Map *mp;
	ulong maddr, oaddr;

	lock(rmap);
	for(mp = rmap->map; mp->size; mp++){
		maddr = mp->addr;

		if(addr){
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
			if(maddr+mp->size < addr)
				continue;
			if(addr+size > maddr+mp->size)
				break;
			maddr = addr;
		}

		if(align > 0)
			maddr = ((maddr+align-1)/align)*align;
		if(mp->addr+mp->size-maddr < size)
			continue;

		oaddr = mp->addr;
		mp->addr = maddr+size;
		mp->size -= maddr-oaddr+size;
		if(mp->size == 0){
			do{
				mp++;
				(mp-1)->addr = mp->addr;
			}while((mp-1)->size = mp->size);
		}

		unlock(rmap);
		if(oaddr != maddr)
			mapfree(rmap, oaddr, maddr-oaddr);

		return maddr;
	}
	unlock(rmap);

	return 0;
}

static void
umbscan(void)
{
	uchar *p;

	/*
	 * Scan the Upper Memory Blocks (0xA0000->0xF0000) for pieces
	 * which aren't used; they can be used later for devices which
	 * want to allocate some virtual address space.
	 * Check for two things:
	 * 1) device BIOS ROM. This should start with a two-byte header
	 *    of 0x55 0xAA, followed by a byte giving the size of the ROM
	 *    in 512-byte chunks. These ROM's must start on a 2KB boundary.
	 * 2) device memory. This is read-write.
	 * There are some assumptions: there's VGA memory at 0xA0000 and
	 * the VGA BIOS ROM is at 0xC0000. Also, if there's no ROM signature
	 * at 0xE0000 then the whole 64KB up to 0xF0000 is theoretically up
	 * for grabs; check anyway.
	 */
	p = KADDR(0xD0000);	/*RSC: changed from 0xC0000 */
	while(p < (uchar*)KADDR(0xE0000)){
		p[0] = 0xCC;
		p[2*KB-1] = 0xCC;
		if(p[0] != 0xCC || p[2*KB-1] != 0xCC){
			p[0] = 0x55;
			p[1] = 0xAA;
			p[2] = 4;
			if(p[0] == 0x55 && p[1] == 0xAA){
				p += p[2]*512;
				continue;
			}
			if(p[0] == 0xFF && p[1] == 0xFF)
				mapfree(&rmapumb, PADDR(p), 2*KB);
		}
		else
			mapfree(&rmapumbrw, PADDR(p), 2*KB);
		p += 2*KB;
	}

	p = KADDR(0xE0000);
	if(p[0] != 0x55 || p[1] != 0xAA){
		p[0] = 0xCC;
		p[64*KB-1] = 0xCC;
		if(p[0] != 0xCC && p[64*KB-1] != 0xCC)
			mapfree(&rmapumb, PADDR(p), 64*KB);
	}
}


void
meminit(ulong)
{
	umbscan();
	if(MEMDEBUG)
		memdebug();
}

ulong
umbmalloc(ulong addr, int size, int align)
{
	ulong a;

	if(a = mapalloc(&rmapumb, addr, size, align))
		return (ulong)KADDR(a);

	return 0;
}

void
umbfree(ulong addr, int size)
{
	mapfree(&rmapumb, PADDR(addr), size);
}

ulong
umbrwmalloc(ulong addr, int size, int align)
{
	ulong a;
	uchar *p;

	if(a = mapalloc(&rmapumbrw, addr, size, align))
		return(ulong)KADDR(a);

	/*
	 * Perhaps the memory wasn't visible before
	 * the interface is initialised, so try again.
	 */
	if((a = umbmalloc(addr, size, align)) == 0)
		return 0;
	p = (uchar*)a;
	p[0] = 0xCC;
	p[size-1] = 0xCC;
	if(p[0] == 0xCC && p[size-1] == 0xCC)
		return a;
	umbfree(a, size);

	return 0;
}

void
umbrwfree(ulong addr, int size)
{
	mapfree(&rmapumbrw, PADDR(addr), size);
}
