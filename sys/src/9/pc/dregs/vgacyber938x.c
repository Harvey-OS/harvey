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

enum {
	CursorON	= 0xC8,
	CursorOFF	= 0x00,
};

static int
cyber938xpageset(VGAscr*, int page)
{
	int opage;

	opage = inb(0x3D8);

	outb(0x3D8, page);
	outb(0x3D9, page);

	return opage;
}

static void
cyber938xpage(VGAscr* scr, int page)
{
	lock(&scr->devlock);
	cyber938xpageset(scr, page);
	unlock(&scr->devlock);
}

static void
cyber938xlinear(VGAscr* scr, int, int)
{
	Pcidev *p;

	if(scr->vaddr)
		return;
	
	vgalinearpciid(scr, 0x1023, 0);
	p = scr->pci;

	/*
	 * Heuristic to detect the MMIO space.  We're flying blind
	 * here, with only the XFree86 source to guide us.
	 */
	if(p->mem[1].size == 0x20000)
		scr->mmio = vmap(p->mem[1].bar & ~0x0F, p->mem[1].size);

	if(scr->apsize)
		addvgaseg("cyber938xscreen", scr->paddr, scr->apsize);
	if(scr->mmio)
		addvgaseg("cyber938xmmio", p->mem[1].bar&~0x0F, 0x20000);
}

static void
cyber938xcurdisable(VGAscr*)
{
	vgaxo(Crtx, 0x50, CursorOFF);
}

static void
cyber938xcurload(VGAscr* scr, Cursor* curs)
{
	uchar *p;
	int islinear, opage, y;

	cyber938xcurdisable(scr);

	opage = 0;
	p = scr->vaddr;
	islinear = vgaxi(Crtx, 0x21) & 0x20;
	if(!islinear){
		lock(&scr->devlock);
		opage = cyber938xpageset(scr, scr->storage>>16);
		p += (scr->storage & 0xFFFF);
	}
	else
		p += scr->storage;

	for(y = 0; y < 16; y++){
		*p++ = curs->set[2*y]|curs->clr[2*y];
		*p++ = curs->set[2*y + 1]|curs->clr[2*y + 1];
		*p++ = 0x00;
		*p++ = 0x00;
		*p++ = curs->set[2*y];
		*p++ = curs->set[2*y + 1];
		*p++ = 0x00;
		*p++ = 0x00;
	}
	memset(p, 0, (32-y)*8);

	if(!islinear){
		cyber938xpageset(scr, opage);
		unlock(&scr->devlock);
	}

	/*
	 * Save the cursor hotpoint and enable the cursor.
	 */
	scr->offset = curs->offset;
	vgaxo(Crtx, 0x50, CursorON);
}

static int
cyber938xcurmove(VGAscr* scr, Point p)
{
	int x, xo, y, yo;

	/*
	 * Mustn't position the cursor offscreen even partially,
	 * or it might disappear. Therefore, if x or y is -ve, adjust the
	 * cursor origins instead.
	 */
	if((x = p.x+scr->offset.x) < 0){
		xo = -x;
		x = 0;
	}
	else
		xo = 0;
	if((y = p.y+scr->offset.y) < 0){
		yo = -y;
		y = 0;
	}
	else
		yo = 0;

	/*
	 * Load the new values.
	 */
	vgaxo(Crtx, 0x46, xo);
	vgaxo(Crtx, 0x47, yo);
	vgaxo(Crtx, 0x40, x & 0xFF);
	vgaxo(Crtx, 0x41, (x>>8) & 0xFF);
	vgaxo(Crtx, 0x42, y & 0xFF);
	vgaxo(Crtx, 0x43, (y>>8) & 0xFF);

	return 0;
}

static void
cyber938xcurenable(VGAscr* scr)
{
	int i;
	ulong storage;

	cyber938xcurdisable(scr);

	/*
	 * Cursor colours.
	 */
	for(i = 0x48; i < 0x4C; i++)
		vgaxo(Crtx, i, 0x00);
	for(i = 0x4C; i < 0x50; i++)
		vgaxo(Crtx, i, 0xFF);

	/*
	 * Find a place for the cursor data in display memory.
	 */
	storage = ((scr->gscreen->width*BY2WD*scr->gscreen->r.max.y+1023)/1024);
	vgaxo(Crtx, 0x44, storage & 0xFF);
	vgaxo(Crtx, 0x45, (storage>>8) & 0xFF);
	storage *= 1024;
	scr->storage = storage;

	/*
	 * Load, locate and enable the 32x32 cursor.
	 * (64x64 is bit 0, X11 format is bit 6 and cursor
	 * enable is bit 7). Bit 3 needs to be set on 9382
	 * chips otherwise even the white bits are black.
	 */
	cyber938xcurload(scr, &arrow);
	cyber938xcurmove(scr, ZP);
	vgaxo(Crtx, 0x50, CursorON);
}

VGAdev vgacyber938xdev = {
	"cyber938x",

	nil,				/* enable */
	nil,				/* disable */
	cyber938xpage,			/* page */
	cyber938xlinear,		/* linear */
	nil,				/* drawinit */
};

VGAcur vgacyber938xcur = {
	"cyber938xhwgc",

	cyber938xcurenable,		/* enable */
	cyber938xcurdisable,		/* disable */
	cyber938xcurload,		/* load */
	cyber938xcurmove,		/* move */
};
