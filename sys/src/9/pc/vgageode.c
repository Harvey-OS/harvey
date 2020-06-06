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
	DC_UNLOCK = 0,
	DC_UNLOCKVALUE = 0x4758,
	DC_GENERAL_CFG = 1,
	CURE = 2,
	DC_CURS_ST_OFFSET = 6,
	DC_CURSOR_X = 24,
	DC_CURSOR_Y = 25,
	DC_PAL_ADDRESS = 28,
	DC_PAL_DATA
};

static void
geodeenable(VGAscr* scr)
{
	Pcidev *p;
	
	if(scr->mmio)
		return;
	p = scr->pci;
	if(p == nil)
		return;
	if((p->mem[1].bar | p->mem[2].bar | p->mem[3].bar) & 1)
		return;
	scr->mmio = vmap(p->mem[2].bar&~0x0F, p->mem[2].size);
	if(scr->mmio == nil)
		return;
	addvgaseg("geodegp", p->mem[1].bar&~0x0F, p->mem[1].size);
	addvgaseg("geodemmio", p->mem[2].bar&~0x0F, p->mem[2].size);
	addvgaseg("geodevid", p->mem[3].bar&~0x0F, p->mem[3].size);
	vgalinearpci(scr);
	if(scr->apsize)
		addvgaseg("geodescreen", scr->paddr, scr->apsize);
	scr->storage = 0x800000;
}

static void
geodelinear(VGAscr*, int, int)
{
}

static void
geodecurload(VGAscr* scr, Cursor* curs)
{
	uvlong *p, and1, xor1, and2, xor2;
	int i;
	uchar *c, *s;

	if(!scr->mmio) return;
	p = (uvlong*)((uchar*)scr->vaddr + scr->storage);
	c = curs->clr;
	s = curs->set;
	for(i=0;i<16;i++) {
		and1 = 0xFF ^ (*s ^ *c++);
		xor1 = *s++;
		and1 &= ~xor1;
		and2 = 0xFF ^ (*s ^ *c++);
		xor2 = *s++;
		and2 &= ~xor2;
		*p++ = (and1 << 56) | (and2 << 48) | 0xFFFFFFFFFFFFLL;
		*p++ = (xor1 << 56) | (xor2 << 48);
	}
	for(;i<128;i++) {
		*p++ = -1;
		*p++ = 0;
	}
	scr->offset = curs->offset;
}

static int
geodecurmove(VGAscr* scr, Point p) {
	if(!scr->mmio) return 1;
	((ulong*)scr->mmio)[DC_UNLOCK] = DC_UNLOCKVALUE;
	((ulong*)scr->mmio)[DC_CURSOR_X] = p.x + scr->offset.x;
	((ulong*)scr->mmio)[DC_CURSOR_Y] = p.y + scr->offset.y;
	return 0;
}

static void
geodecurenable(VGAscr* scr)
{
	geodeenable(scr);
	if(!scr->mmio) return;
	geodecurload(scr, &cursor);
	geodecurmove(scr, ZP);
	((ulong*)scr->mmio)[DC_UNLOCK] = DC_UNLOCKVALUE;
	((ulong*)scr->mmio)[DC_CURS_ST_OFFSET] = scr->storage;
	((ulong*)scr->mmio)[DC_GENERAL_CFG] |= CURE;
	/* set cursor colours */
	((ulong*)scr->mmio)[DC_PAL_ADDRESS] = 0x100;
	((ulong*)scr->mmio)[DC_PAL_DATA] = -1;
	((ulong*)scr->mmio)[DC_PAL_DATA] = 0;
}

static void
geodecurdisable(VGAscr* scr)
{
	if(!scr->mmio) return;
	((ulong*)scr->mmio)[DC_UNLOCK] = DC_UNLOCKVALUE;
	((ulong*)scr->mmio)[DC_GENERAL_CFG] &= ~CURE;
}

VGAdev vgageodedev = {
	"geode",
	geodeenable,
	.linear = geodelinear,
};


VGAcur vgageodecur = {
	"geodehwgc",
	geodecurenable,
	geodecurdisable,
	geodecurload,
	geodecurmove,
};
