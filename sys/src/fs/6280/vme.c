#include "u.h"
#include "lib.h"
#include "dat.h"
#include "fns.h"
#include "mem.h"
#include "io.h"

/*
 * To Do:
 *	find out why running uncached doesn't work
 */

/*
 * the 6280 has I/O control space starting at 0xBE000000.
 * within that space, each slot (each System Bus Chip - SBC)
 * has a 64Kb (0x10000) control space. this SBC control space
 * is sparsely populated with registers for both the SBC
 * itself and any VME buses connected (max of 2 per SBC).
 * the CPU board appears to occupy the first two SBC slot
 * addresses, so the first I/O or memory board is in slot 2,
 * having a base address of 0xBE020000.
 *
 * the SBC (I/O Adapter) space appears in the control space as
 * follows:
 *   0x7000		32 words of bitmap
 *   0x7800		DUART interrupt vector
 *   0xC000		TOD base (?)
 *   0xE000->0xEE04	miscellaneous I/O registers
 *   0xF000->0xF030	SBC registers
 *   0xF800->0xF9FF	ID prom
 *
 * the SBC VME space appears in the control space as follows:
 *   0x0000->0x0FFF	VME0 map (system bus to VME address)
 *   0x1000->0x1FFF	VME1 map (system bus to VME address)
 *   0x2000->0x2FFF	VME0 map (VME address to system bus)
 *   0x3000->0x3FFF	VME1 map (VME address to system bus)
 *   0x4000->0x4FFF	VME0 interrupt info
 *   0x5000->0x5FFF	VME1 interrupt info
 *   0x8000->0x8FFF	VME0 miscellaneous registers
 *   0x9000->0x9FFF	VME1 miscellaneous registers
 */

typedef
struct	Map
{
	Lock;
	struct	map
	{
		ulong	addr;
		ulong	size;
	} *map;
} Map;

unsigned	mapalloc(Map*, ulong);
void		mapfree(Map*, ulong, ulong);

struct	vmebus
{
	ulong*	ioa;				/* IOA registers */
	ulong*	ctl;				/* VME registers */
	uchar*	a16;				/* VME address space */
	uchar	exists;
	Map	sysmap;
	Vmedevice *vec[256];
} vmebus[] =
{
	{ (ulong*)0xBE020000, (ulong*)0xBE020000, (ulong*)0xBC000000, },
	{ (ulong*)0xBE020000, (ulong*)0xBE021000, (ulong*)0xBD000000, },
	{ 0, 0, },
};
int	maxvmebus;

/*
 * VME registers
 * indexed off vmebus.ctl
 */
#define VMEMAP		(0x0000/sizeof(ulong))
#define NVMEMAP		1024
#define SYSMAP		(0x2000/sizeof(ulong))
#define NSYSMAP		1024

#define VMELOP		(0x4000/sizeof(ulong))
#define NVMELOP		16

#define VMESTATUS	(0x8100/sizeof(ulong))

#define VMECACHEADDR	(0x8200/sizeof(ulong))
#define VMECACHETAG	(0x8400/sizeof(ulong))
#define VMECACHEINFO	(0x8500/sizeof(ulong))
#define VMECACHEDIRTY	(0x8600/sizeof(ulong))
#define VMECACHEDATA	(0x8700/sizeof(ulong))
#define VMECACHESZ	(512*1024)
#define NVMECACHE	32

#define VMEISR		(0x8800/sizeof(ulong))
#define VMEADDRCMP	(0x8A00/sizeof(ulong))
#define VMEFLUSH	(0x8D00/sizeof(ulong))
#define VMEBADDR	(0x8E00/sizeof(ulong))
#define VMEERROR	(0x8F00/sizeof(ulong))

/*
 * IOA registers
 * indexed off vmebus.ioa
 */
#define IOASTATUS1	(0xE400/sizeof(ulong))
#define IOASTATUS0	(0xE600/sizeof(ulong))
#define IOAERROR	(0xEA00/sizeof(ulong))
#define IOABADDR0	(0xEC00/sizeof(ulong))
#define IOABADDR1	(0xEE00/sizeof(ulong))

/*
 */
void
setioavector(ulong *vector, ulong irq)
{
	ulong pa = CPUIVECTSET;

	*vector = pa & (BY2PG-1);
	*(vector+1) = (pa>>14)<<10;
	*(vector+2) = irq;
}

static void
vmedevinit(void)
{
	Vmedevice *vp;
	struct vmebus *v;

	for (vp = vmedevtab; vp->init; vp++) {
		if (vp->bus > maxvmebus)
			continue;
		if ((v = &vmebus[vp->bus])->exists == 0)
			continue;
		if (vp->vector > 0xFF || v->vec[vp->vector])
			continue;
		vp->address = v->a16 + (unsigned)vp->address;
		if ((*vp->init)(vp)) {
			print("%s at #%lux: cannot init\n", vp->name, vp->address);
			continue;
		}
		v->vec[vp->vector] = vp;
	}
}

void
vmeinit(void)
{
	struct vmebus *v;
	ulong addr, word, *map;

	print("vmeinit\n");
	for(v = vmebus; v->ioa; v++) {
		/*
		 * the IOAERROR register has a bit for each possible VME
		 * indicating whether it exists or not.
		 */
		if((v->ioa[IOAERROR] & ((v->ioa == v->ctl) ? 0x2000: 0x1000)) == 0)
			continue;
		v->exists = 1;
		print("\tvme%d\n", v - vmebus);
		/*
		 * most of what happens below is magic, taken from the
		 * RISC/os code, which is the only documentation.
		 * if i understood this, there wouldn't be any comments.
		 */
		/*
		 * clear the VME cache:
		 *   initialise the per-section control to
		 *     cacheable or non-cacheable,
		 *     allow partial transfers;
		 *   for each section
		 *     reset the tags to 0 (not-valid and not-dirty);
		 *     reset the per-line dirty bits.
		 * each VME bus has 16Mb address space, split into
		 * 32 512Kb sections.
		 */
		for(addr = 0; addr < NVMECACHE*VMECACHESZ; addr += VMECACHESZ) {
			v->ctl[VMECACHEADDR] = addr;
			v->ctl[VMECACHEINFO] = 0x00;	/* 0x08 = non-cacheable */
			/*
			 * there are 2 128-byte cache-lines per section.
			 * we don't really need to do this if we set the
			 * section control to non-cacheable.
			 */
			for(word = 0; word < 2*128; word += sizeof(word)) {
				v->ctl[VMECACHEADDR] = addr + word;
				if((word & 127) == 0)
					v->ctl[VMECACHETAG] = 0;
				v->ctl[VMECACHEDIRTY] = 0;
				v->ctl[VMECACHEDATA] = 0;
			}
		}
		/*
		 * we do nothing with the 'long-op' registers,
		 * or the VME address compare register (that we understand
		 * or that makes any difference).
		 * each long-op register occupies 16 bytes. the low 4-bits
		 * of the register address are used to select an operation
		 * on the register (write, and, or).
		 */
		for(addr = 0; addr < NVMELOP*16; addr += 16)
			v->ctl[VMELOP+addr] = 0;
		v->ctl[VMEADDRCMP] = 0xFFFF;
		/*
		 * initialise the VME->system bus map,
		 * trivial 1-to-1 map for 16Mb total.
		 * make the entries invalid (|0x8000000)
		 * to force use of map allocation routines.
		 */
		map = &v->ctl[SYSMAP];
		for(addr = 0; addr < NSYSMAP; addr++)
			*map++ = 0x8000000|(addr<<10);
		/*
		 * initialise the software VME->system bus map.
		 * maps can't have a base address of 0,
		 * vme2sysmap()/vme2sysfree() will compensate.
		 */
		v->sysmap.map = ialloc(NSYSMAP*sizeof(*v->sysmap.map), 0);
		mapfree(&v->sysmap, 1, 512);				/* 1st 8Mb */
		/*
		 * initialise the system->VME bus address map.
		 * set up trivial mapping, the first 4 pages for
		 * A16 space, the 5th page is used for the
		 * VME IACK address, and the rest for A24 space.
		 *
		 * later, the A24 space should probably be dynamically
		 * mapped.
		 *
		 * there are 1024 map registers of 16Kb to map the
		 * 16Mb space.
		 */
		map = &v->ctl[VMEMAP];
		for(addr = 0; addr < 1024; addr++) {
			if(addr < 4)
				*map++ = (addr<<14)|(0x2D<<4);	/* A16 */
			else
			if(addr == 4)
				*map++ = 0x0800;		/* IACK */
			else
				*map++ = (addr<<14)|(0x3D<<4);	/* A24 */
		}
	}
	maxvmebus = (v - vmebus) - 1;
	intrclr(VMEIE);
	SBCREG->intmask |= VMEIE;
	vmedevinit();
}

static ulong
vmeerror(int bus)
{
	struct vmebus *v = &vmebus[bus];
	ulong status;

	status = v->ctl[VMESTATUS] & 0xFF;
	/*
	 * should we ignore timeouts?
	 * this code does. the problem is that 'probe()'
	 * may cause a timeout, which we handle and recover from,
	 * but then we take the asynchronous timeout error later.
	 */
	if(status & 0x80)
		status &= ~0xC0;
	/*
	 * clear errors
	 */
	v->ctl[VMESTATUS] = 0;
	if(status)
		print("vme%derror: status 0x%lux, baddr 0x%lux, error 0x%lux\n",
			bus, status, v->ctl[VMEBADDR], v->ctl[VMEERROR]);
	return status;
}

void
ioaintr(void)
{
	struct vmebus *v;
	ulong status, badaddr, error;

	for(v = vmebus; v->ioa; v++) {
		if(v->exists == 0)
			continue;
		status = v->ioa[(v->ioa == v->ctl) ? IOASTATUS0: IOASTATUS1];
		badaddr = v->ioa[(v->ioa == v->ctl) ? IOABADDR0: IOABADDR1];
		error = v->ioa[IOAERROR];
		if(vmeerror(v - vmebus) == 0) {
			v->ioa[(v->ioa == v->ctl) ? IOASTATUS0: IOASTATUS1] = 0;
			continue;
		}
		print("ioaintr: status 0x%lux, baddr 0x%lux, error 0x%lux\n",
			status, badaddr, error);
		panic("ioaintr\n");
	}
}

void
vmeintr(int bus)
{
	struct vmebus *v;
	ulong mask, bit, irq, vec;

	if(bus > maxvmebus || (v = &vmebus[bus])->exists == 0) {
		panic("vmeintr(%d)\n", bus);
		return;
	}
	while(mask = (v->ctl[VMEISR] & 0xFF)) {
		if(mask & 0x01) {
			vmeerror(bus);
			continue;
		}
		for(bit = 2, irq = 1; irq < 8 && mask; irq++, bit <<= 1) {
			if((bit & mask) == 0)
				continue;
			mask &= ~bit;
			/*
			 * do the interrupt acknowledge and retrieve the
			 * vector. the IACK sequence is done by doing a read
			 * through the IACK map page (page 5); the irq
			 * being acknowledged is in bits <4..2>.
			 * this access, like any other vme space
			 * access, can generate a bus-error, in which case
			 * it should be retried (interrupts must also be
			 * disabled).
			 */
			vec = *((ulong *)(v->a16 + 0x10000 + (irq<<2)));
			vec &= 0xFF;
			if(v->vec[vec])
				(*v->vec[vec]->intr)(v->vec[vec]);
			else
				print("stray vme intr 0x%lux\n", vec);
		}
	}
}

/*
 * Allocate 'size' units from the given map.
 * Return the base of the allocated space.
 * In a map, the addresses are increasing and the
 * list is terminated by a 0 size.
 * Algorithm is first-fit.
 */
unsigned
mapalloc(Map *mp, ulong size)
{
	ulong addr;
	struct map *bp;

	lock(mp);
	for(addr = 0, bp = mp->map; bp->size; bp++) {
		if(bp->size >= size) {
			addr = bp->addr;
			bp->addr += size;
			if((bp->size -= size) == 0) {
				do {
					bp++;
					(bp-1)->addr = bp->addr;
				} while((bp-1)->size = bp->size);
			}
			break;
		}
	}
	unlock(mp);
	return addr;
}

/*
 * Free the previously allocated space addr
 * of size units into the specified map.
 * Sort addr into map and combine on one or
 * both ends if possible.
 */
void
mapfree(Map *mp, ulong addr, ulong size)
{
	struct map *bp = mp->map;
	ulong tmp;

	lock(mp);
	while(bp->addr <= addr && bp->size)
		bp++;
	if(bp > mp->map && (bp-1)->addr+(bp-1)->size == addr) {
		(bp-1)->size += size;
		if(addr+size == bp->addr) {
			(bp-1)->size += bp->size;
			while(bp->size) {
				bp++;
				(bp-1)->addr = bp->addr;
				(bp-1)->size = bp->size;
			}
		}
	}
	else {
		if(addr+size == bp->addr && bp->size) {
			bp->addr -= size;
			bp->size += size;
		}
		else if(size) {
			do {
				tmp = bp->addr;
				bp->addr = addr;
				addr = tmp;
				tmp = bp->size;
				bp->size = size;
				bp++;
			} while(size = tmp);
		}
	}
	unlock(mp);
}

/*
 * return a physical address useable by a vme device
 * to access system memory
 */
ulong
vme2sysmap(int bus, ulong va, ulong len)
{
	struct vmebus *v = &vmebus[bus];
	ulong vmeaddr, entries, pa, *map, begva, endva;

	/*
	 * sanity check for the fileserver:
	 *  all va's should be in the kernel;
	 * should check va isn't outside physical memory.
	 */
	if((va & 0xC0000000) != KZERO)
		panic("vme2sysmap(%d, #%lux, #%lux)\n", bus, va, len);
	/*
	 * calculate number of map entries
	 * needed.
	 */
	begva = va & ~(BY2PG-1);
	endva = (va+len+BY2PG-1) & ~(BY2PG-1);
	entries = (endva - begva) / BY2PG;
	/*
	 * allocate the map entries.
	 * maps can't be zero based, so subtract one.
	 */
	if((vmeaddr = mapalloc(&v->sysmap, entries)) == 0)
		panic("vme2sysmap(%d, #%lux, #%lux)\n", bus, va, len);
	vmeaddr--;
	/*
	 * init the map registers with the physical page number
	 * of the va range we want to map,
	 * and return the mapped address to be used by vme devices.
	 */
	pa = (va & ~KZERO)/BY2PG;
	map = &v->ctl[SYSMAP+vmeaddr];
	while(entries--)
		*map++ = pa++ << 10;
	return (vmeaddr*BY2PG) + (va & (BY2PG-1));
}

void
vmeflush(int bus, ulong vmeaddr, ulong len)
{
	struct vmebus *v = &vmebus[bus];
	ulong section, s, addrend;

	addrend = vmeaddr+len+VMECACHESZ-1;
	vmeaddr &= ~(VMECACHESZ-1);
	addrend &= ~(VMECACHESZ-1);
	s = splhi();
	while(vmeaddr < addrend) {
		v->ctl[VMEFLUSH] = vmeaddr;
		/*
		 * should be more careful about
		 * matching the correct section
		 */
		while((SBCREG->intvecset & VMELOPIE) == 0)
			;
		intrclr(VMELOPIE);
		while((section = v->ctl[VMELOP]) == 0)
			;
		v->ctl[VMELOP+1] = ~section;
		vmeaddr += VMECACHESZ;
	}
	splx(s);
}

/*
 * unmap and deallocate above mapping.
 */
void
vme2sysfree(int bus, ulong vmeaddr, ulong len)
{
	struct vmebus *v = &vmebus[bus];
	ulong addr, entries, *map, begva, endva;

	addr = vmeaddr/BY2PG;
	begva = vmeaddr & ~(BY2PG-1);
	endva = (vmeaddr+len+BY2PG-1) & ~(BY2PG-1);
	entries = (endva - begva) / BY2PG;
	vmeflush(bus, vmeaddr, len);
	map = &v->ctl[SYSMAP+addr];
	while(map < &v->ctl[SYSMAP+addr+entries])
		*map++ |= 0x8000000;
	mapfree(&v->sysmap, addr+1, entries);
}

/*
 * invalidate the primary and secondary
 * data caches a page at a time
 */
void
invalcache(void *addr, unsigned len)
{
	unsigned a;
	long l;

	a = (unsigned)addr & (16*1024-2);
	l = len+7;
	if(l >= 16*1024) {
		a = 0;
		l = 16*1024;
	}
	while(l > 0) {
		invaldline((void*)a);
		a += 16*8;
		l -= 16*8;
	}

	a = (unsigned)addr;
	while(len > 0) {
		l = BY2PG - (a & (BY2PG-1));
		if(l > len)
			l = len;
		inval2cache((void*)a, l);
		len -= l;
		a += l;
	}
}

/*
 * write-back the secondary cache a page
 * at a time
 */
void
wbackcache(void *addr, unsigned len)
{
	unsigned a, l;

	a = (unsigned)addr;
	while(len > 0) {
		l = BY2PG - (a & (BY2PG-1));
		if(l > len)
			l = len;
		wback2cache((void*)a, l);
		len -= l;
		a += l;
	}
}
