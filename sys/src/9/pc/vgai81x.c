#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/error.h"

#define	Image	IMAGE
#include <draw.h>
#include <memdraw.h>
#include <cursor.h>
#include "screen.h"

typedef struct
{
	ushort	ctl;
	ushort	pad;
	ulong	base;
	ulong	pos;
} CursorI81x;

enum {
	Fbsize		= 8*MB,

	hwCur		= 0x70080,
	SRX		= 0x3c4,
	DPMSsync	= 0x5002,
};

static void
i81xblank(VGAscr *scr, int blank)
{
	char *srx, *srxd, *dpms;
	char sr01, mode;

	srx = (char *)scr->mmio+SRX;
	srxd = srx+1;
	dpms = (char *)scr->mmio+DPMSsync;

	*srx = 0x01;
	sr01 = *srxd & ~0x20;
	mode = *dpms & 0xf0;

	if(blank) {
		sr01 |= 0x20;
		mode |= 0x0a;
	}
	*srxd = sr01;
	*dpms = mode;
}

static Pcidev *
i81xpcimatch(void)
{
	Pcidev *p;

	p = nil;
	while((p = pcimatch(p, 0x8086, 0)) != nil){
		switch(p->did){
		default:
			continue;
		case 0x7121:
		case 0x7123:
		case 0x7125:
		case 0x1102:
		case 0x1112:
		case 0x1132:
		case 0x3577:	/* IBM R31 uses intel 830M chipset */
			return p;
		}
	}
	return nil;
}

static void
i81xenable(VGAscr* scr)
{
	Pcidev *p;
	int size;
	Mach *mach0;
	ulong *pgtbl, *rp, cursor, *pte, fbuf, fbend;
	
	if(scr->mmio)
		return;
	p = i81xpcimatch();
	if(p == nil)
		return;
	scr->mmio = vmap(p->mem[1].bar & ~0x0F, p->mem[1].size);
	if(scr->mmio == 0)
		return;
	addvgaseg("i81xmmio", p->mem[1].bar&~0x0F, p->mem[1].size);

	/* allocate page table */
	pgtbl = xspanalloc(64*1024, BY2PG, 0);
	scr->mmio[0x2020/4] = PADDR(pgtbl) | 1;

	size = p->mem[0].size;
	if(size > 0)
		size = Fbsize;
	vgalinearaddr(scr, p->mem[0].bar&~0xF, size);
	addvgaseg("i81xscreen", p->mem[0].bar&~0xF, size);

	/*
	 * allocate backing store for frame buffer
	 * and populate device page tables.
	 */
	fbuf = PADDR(xspanalloc(size, BY2PG, 0));
	fbend = PGROUND(fbuf+size);
	rp = scr->mmio+0x10000/4;
	while(fbuf < fbend) {
		*rp++ = fbuf | 1;
		fbuf += BY2PG;
	}

	/*
	 * allocate space for the cursor data in system memory.
	 * must be uncached.
	 */
	cursor = (ulong)xspanalloc(BY2PG, BY2PG, 0);
	mach0 = MACHP(0);
	pte = mmuwalk(mach0->pdb, cursor, 2, 0);
	if(pte == nil)
		panic("i81x cursor mmuwalk");
	*pte |= PTEUNCACHED;
	scr->storage = cursor;

	scr->blank = i81xblank;
	hwblank = 1;
}

static void
i81xcurdisable(VGAscr* scr)
{
	CursorI81x *hwcurs;

	if(scr->mmio == 0)
		return;
	hwcurs = (void*)((uchar*)scr->mmio+hwCur);
	hwcurs->ctl = (1<<4);
}

static void
i81xcurload(VGAscr* scr, Cursor* curs)
{
	int y;
	uchar *p;
	CursorI81x *hwcurs;

	if(scr->mmio == 0)
		return;
	hwcurs = (void*)((uchar*)scr->mmio+hwCur);

	/*
	 * Disable the cursor then load the new image in
	 * the top-left of the 32x32 array.
	 * Unused portions of the image have been initialised to be
	 * transparent.
	 */
	hwcurs->ctl = (1<<4);
	p = (uchar*)scr->storage;
	for(y = 0; y < 16; y += 2) {
		*p++ = ~(curs->clr[2*y]|curs->set[2*y]);
		*p++ = ~(curs->clr[2*y+1]|curs->set[2*y+1]);
		p += 2;
		*p++ = ~(curs->clr[2*y+2]|curs->set[2*y+2]);
		*p++ = ~(curs->clr[2*y+3]|curs->set[2*y+3]);
		p += 2;
		*p++ = curs->set[2*y];
		*p++ = curs->set[2*y+1];
		p += 2;
		*p++ = curs->set[2*y+2];
		*p++ = curs->set[2*y+3];
		p += 2;
	}

	/*
	 * Save the cursor hotpoint and enable the cursor.
	 * The 0,0 cursor point is top-left.
	 */
	scr->offset.x = curs->offset.x;
	scr->offset.y = curs->offset.y;
	hwcurs->ctl = (1<<4)|1;
}

static int
i81xcurmove(VGAscr* scr, Point p)
{
	int x, y;
	ulong pos;
	CursorI81x *hwcurs;

	if(scr->mmio == 0)
		return 1;
	hwcurs = (void*)((uchar*)scr->mmio+hwCur);

	x = p.x+scr->offset.x;
	y = p.y+scr->offset.y;
	pos = 0;
	if(x < 0) {
		pos |= (1<<15);
		x = -x;
	}
	if(y < 0) {
		pos |= (1<<31);
		y = -y;
	}
	pos |= ((y&0x7ff)<<16)|(x&0x7ff);
	hwcurs->pos = pos;

	return 0;
}

static void
i81xcurenable(VGAscr* scr)
{
	int i;
	uchar *p;
	CursorI81x *hwcurs;

	i81xenable(scr);
	if(scr->mmio == 0)
		return;
	hwcurs = (void*)((uchar*)scr->mmio+hwCur);

	/*
	 * Initialise the 32x32 cursor to be transparent in 2bpp mode.
	 */
	hwcurs->base = PADDR(scr->storage);
	p = (uchar*)scr->storage;
	for(i = 0; i < 32/2; i++) {
		memset(p, 0xff, 8);
		memset(p+8, 0, 8);
		p += 16;
	}
	/*
	 * Load, locate and enable the 32x32 cursor in 2bpp mode.
	 */
	i81xcurload(scr, &arrow);
	i81xcurmove(scr, ZP);
}

VGAdev vgai81xdev = {
	"i81x",

	i81xenable,
	nil,
	nil,
	nil,
};

VGAcur vgai81xcur = {
	"i81xhwgc",

	i81xcurenable,
	i81xcurdisable,
	i81xcurload,
	i81xcurmove,
};
