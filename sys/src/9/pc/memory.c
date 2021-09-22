/*
 * Size memory and create the kernel page-tables on the fly while doing so.
 * Called from main(), this code should only be run by the bootstrap processor.
 *
 * MemMin is what the bootstrap code in l.s has already mapped;
 * in boot kernels, MemMax is the maximum to consider.
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "multiboot.h"
#include "memmap.h"

int v_flag;
Mbi *multibootheader = nil;		/* force into data segment, not bss */

/*
 * Memory allocation tracking.
 */
RMap rmapupa = {
	"unallocated unbacked physical addresses",
	mapupa,
	&mapupa[nelem(mapupa)-1],
};

RMap rmapram = {
	"physical memory",
	mapram,
	&mapram[nelem(mapram)-1],
};

RMap rmapumb = {
	"upper memory block",
	mapumb,
	&mapumb[nelem(mapumb)-1],
};

RMap rmapumbrw = {
	"UMB device memory",
	mapumbrw,
	&mapumbrw[nelem(mapumbrw)-1],
};

void
mapprint(RMap *rmap)
{
	Map *mp;

	print("%s\n", rmap->name);
	for(mp = rmap->map; mp->size; mp++)
		print("\t%8.8luX %8.8luX (%lud)\n", mp->addr, mp->addr+mp->size, mp->size);
}

void
memdebug(void)
{
	ulong maxpa, maxpa1, maxpa2;

	maxpa = (nvramread(0x18)<<8)|nvramread(0x17);
	maxpa1 = (nvramread(0x31)<<8)|nvramread(0x30);
	maxpa2 = (nvramread(0x16)<<8)|nvramread(0x15);
	print("maxpa = %luX -> %luX, maxpa1 = %luX maxpa2 = %luX\n",
		maxpa, MB+maxpa*KB, maxpa1, maxpa2);

	mapprint(&rmapram);
	mapprint(&rmapumb);
	mapprint(&rmapumbrw);
	mapprint(&rmapupa);
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
	} else if(addr+size == mp->addr && mp->size){
		mp->addr -= size;
		mp->size += size;
	} else
		do {
			if(mp >= rmap->mapend){
				if (MEMDEBUG)
					print("mapfree: %s: losing %ld at %#p\n",
						rmap->name, size, addr);
				break;
			}
			t = mp->addr;
			mp->addr = addr;
			addr = t;
			t = mp->size;
			mp->size = size;
			mp++;
		} while(size = t);
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
			/* maddr+mp->size < addr, but no overflow? */
			if(mp->size < addr - maddr)
				continue;
			/* addr+size > maddr+mp->size, but no overflow? */
			if(addr - maddr > mp->size - size)
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

/*
 * Allocate from the ram map directly to make page tables.
 * Called by mmuwalk during e820scan.
 */
void*
rampage(void)
{
	ulong m;

	m = mapalloc(&rmapram, 0, BY2PG, BY2PG);
	if(m == 0)
		return nil;
	return KADDR(m);
}

static void
umbexclude(void)
{
	int size;
	ulong addr;
	char *op, *p, *rptr;

	if((p = getconf("umbexclude")) == nil)
		return;

	while(p && *p != '\0' && *p != '\n'){
		op = p;
		addr = strtoul(p, &rptr, 0);
		if(rptr == nil || rptr == p || *rptr != '-'){
			print("umbexclude: invalid argument <%s>\n", op);
			break;
		}
		p = rptr+1;

		size = strtoul(p, &rptr, 0) - addr + 1;
		if(size <= 0){
			print("umbexclude: bad range <%s>\n", op);
			break;
		}
		if(rptr != nil && *rptr == ',')
			*rptr++ = '\0';
		p = rptr;

		mapalloc(&rmapumb, addr, size, 0);
	}
}

void
umbscan(void)
{
	uchar o[2], *p;

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
	p = (uchar *)(KZERO + 0xD0000);
	while(p < (uchar*)(KZERO + 0xE0000)){
		/*
		 * Check for the ROM signature, skip if valid.
		 */
		if(p[0] == 0x55 && p[1] == 0xAA){
			p += p[2]*512;
			continue;
		}

		/*
		 * Is it writeable? If yes, then stick it in
		 * the UMB device memory map. A floating bus will
		 * return 0xff, so add that to the map of the
		 * UMB space available for allocation.
		 * If it is neither of those, ignore it.
		 */
		o[0] = p[0];
		p[0] = 0xCC;
		o[1] = p[2*KB-1];
		p[2*KB-1] = 0xCC;
		if(p[0] == 0xCC && p[2*KB-1] == 0xCC){
			p[0] = o[0];
			p[2*KB-1] = o[1];
			mapfree(&rmapumbrw, PADDR(p), 2*KB);
		}
		else if(p[0] == 0xFF && p[1] == 0xFF)
			mapfree(&rmapumb, PADDR(p), 2*KB);
		p += 2*KB;
	}

	p = (uchar *)(KZERO + 0xE0000);
	if(p[0] != 0x55 || p[1] != 0xAA){
		p[0] = 0xCC;
		p[64*KB-1] = 0xCC;
		if(p[0] != 0xCC && p[64*KB-1] != 0xCC)
			mapfree(&rmapumb, PADDR(p), 64*KB);
	}

	umbexclude();
}

static void*
sigscan(uchar* addr, int len, char* signature)
{
	int sl;
	uchar *e, *p;

	if (len < 0)
		print("sigscan %#p len %d %s\n", addr, len, signature);
	e = addr+len;
	sl = strlen(signature);
	for(p = addr; p+sl < e; p += 16)
		if(memcmp(p, signature, sl) == 0)
			return p;
	return nil;
}

/* returns number of bytes of free base (conventional) memory */
uintptr
freebasemem(void)
{
	return L16GET((uchar *)FBMADDR) * KB;
}

void*
sigsearch(char* signature)
{
	uintptr p;
	void *r;

	/*
	 * Search for the data structure:
	 * 1) within the Extended BIOS Data Area (EBDA), or
	 * 2) within the last KiB of system base memory if the EBDA segment
	 *    is undefined, or
	 * 3) within the BIOS ROM address space between 0xf0000 and 0xfffff
	 *    (but will actually check 0xe0000 to 0xfffff).
	 */
	if((p = (uintptr)BIOSSEG(L16GET((uchar *)EBDAADDR))) != 0)
		if((r = sigscan((void *)p, LOWMEMEND - p, signature)) != nil)
			return r;

	p = freebasemem();
	if(p == 0)
		/* hack for virtualbox: look in last KiB below 0xa0000 */
		p = LOWMEMEND;
	else
		p += KZERO;
	if((r = sigscan((uchar *)p - KB, KB, signature)) != nil)
		return r;

	return sigscan((uchar *)BIOSROMS, KZERO+MB - BIOSROMS, signature);
}

/* validate & copy multiboot info mem map to mmap[] */
int
rdmbootmmap(void)
{
	int bytes;
	Mbi *mbi;
	MMap *lmmap;

	if (multibootheader == nil)
		return -1;
	/* passed from a multiboot loader */
	mbi = KADDR((uintptr)multibootheader);
	print("found one at %#p...", mbi);
	if (!(mbi->flags & Fmmap)) {
		print("no mmap...");
		return -1;
	}
	if (mbi->mmaplength < 2*sizeof mmap[0]) {
		print("%d bytes is too few...", mbi->mmaplength);
		return -1;
	}
	if (mbi->mmapaddr == 0) {
		print("nil mmap addr...");
		return -1;
	}
	lmmap = KADDR(mbi->mmapaddr);
	if (lmmap->size == 0) {
		print("zero size first entry...");
		return -1;		/* bogus initial entry in map */
	}
	if (lmmap->len == 0) {
		print("zero length first entry...");
		return -1;		/* bogus initial entry in map */
	}
	memset(mmap, 0, sizeof mmap);
	bytes = mbi->mmaplength < sizeof mmap? mbi->mmaplength: sizeof mmap;
	memmove(mmap, lmmap, bytes);
	nmmap = bytes / sizeof mmap[0];
	return 0;
}

#include "e820.c"

/*
 * Allocate memory from the upper memory blocks.
 */
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
	uchar o[2], *p;

	if(a = mapalloc(&rmapumbrw, addr, size, align))
		return(ulong)KADDR(a);

	/*
	 * Perhaps the memory wasn't visible before
	 * the interface is initialised, so try again.
	 */
	if((a = umbmalloc(addr, size, align)) == 0)
		return 0;
	p = (uchar*)a;
	o[0] = p[0];
	p[0] = 0xCC;
	o[1] = p[size-1];
	p[size-1] = 0xCC;
	if(p[0] == 0xCC && p[size-1] == 0xCC){
		p[0] = o[0];
		p[size-1] = o[1];
		return a;
	}
	umbfree(a, size);

	return 0;
}

void
umbrwfree(ulong addr, int size)
{
	mapfree(&rmapumbrw, PADDR(addr), size);
}

/*
 * Give out otherwise-unused physical address space
 * for use in configuring devices.  Note that unlike upamalloc
 * before it, upaalloc does not map the physical address
 * into virtual memory.  Call vmap to do that.
 */
ulong
upaalloc(int size, int align)
{
	ulong a;

	a = mapalloc(&rmapupa, 0, size, align);
	if(a == 0){
		print("out of physical address space allocating %d\n", size);
		mapprint(&rmapupa);
	}
	return a;
}

void
upafree(ulong pa, int size)
{
	mapfree(&rmapupa, pa, size);
}

void
upareserve(ulong pa, int size)
{
	ulong a;

	a = mapalloc(&rmapupa, pa, size, 0);
	if(a != pa){
		/*
		 * This can happen when we're using the E820
		 * map, which might have already reserved some
		 * of the regions claimed by the pci devices.
		 */
//		print("upareserve: cannot reserve pa=%#.8lux size=%d\n", pa, size);
		if(a != 0)
			mapfree(&rmapupa, a, size);
	}
}

void
memorysummary(void)
{
	memdebug();
}
