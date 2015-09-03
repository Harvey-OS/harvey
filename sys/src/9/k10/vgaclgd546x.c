/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

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

typedef struct Cursor546x Cursor546x;
struct Cursor546x {
	uint16_t	x;
	uint16_t	y;
	uint16_t	preset;
	uint16_t	enable;
	uint16_t	addr;
};

enum {
	PaletteState	= 0xB0,
	CursorMMIO	= 0xE0,
};

static void
clgd546xlinear(VGAscr* scr, int i, int n)
{
	vgalinearpci(scr);
}

static void
clgd546xenable(VGAscr* scr)
{
	Pcidev *p;

	if(scr->mmio)
		return;
	if((p = pcimatch(nil, 0x1013, 0)) == nil)
		return;
	switch(p->did){
	case 0xD0:
	case 0xD4:
	case 0xD6:
		break;
	default:
		return;
	}

	scr->pci = p;
	scr->mmio = vmap(p->mem[1].bar&~0x0F, p->mem[1].size);
	if(scr->mmio == 0)
		return;
	addvgaseg("clgd546xmmio", p->mem[1].bar&~0x0F, p->mem[1].size);
}

static void
clgd546xcurdisable(VGAscr* scr)
{
	Cursor546x *cursor546x;

	if(scr->mmio == 0)
		return;
	cursor546x = (Cursor546x*)((unsigned char*)scr->mmio+CursorMMIO);
	cursor546x->enable = 0;
}

static void
clgd546xcurload(VGAscr* scr, Cursor* curs)
{
	int c, i, m, y;
	unsigned char *p;
	Cursor546x *cursor546x;

	if(scr->mmio == 0)
		return;
	cursor546x = (Cursor546x*)((unsigned char*)scr->mmio+CursorMMIO);

	/*
	 * Disable the cursor then change only the bits
	 * that need it.
	 */
	cursor546x->enable = 0;
	p = (unsigned char*)scr->vaddr + scr->storage;
	for(y = 0; y < 16; y++){
		c = curs->set[2*y];
		m = 0;
		for(i = 0; i < 8; i++){
			if(c & (1<<(7-i)))
				m |= 1<<i;
		}
		*p++ = m;
		c = curs->set[2*y + 1];
		m = 0;
		for(i = 0; i < 8; i++){
			if(c & (1<<(7-i)))
				m |= 1<<i;
		}
		*p++ = m;
		p += 6;
		c = curs->set[2*y]|curs->clr[2*y];
		m = 0;
		for(i = 0; i < 8; i++){
			if(c & (1<<(7-i)))
				m |= 1<<i;
		}
		*p++ = m;
		c = curs->set[2*y + 1]|curs->clr[2*y + 1];
		m = 0;
		for(i = 0; i < 8; i++){
			if(c & (1<<(7-i)))
				m |= 1<<i;
		}
		*p++ = m;
		p += 6;
	}

	/*
	 * Save the cursor hotpoint and enable the cursor.
	 */
	scr->offset = curs->offset;
	cursor546x->enable = 1;
}

static int
clgd546xcurmove(VGAscr* scr, Point p)
{
	int x, xo, y, yo;
	Cursor546x *cursor546x;

	if(scr->mmio == 0)
		return 1;
	cursor546x = (Cursor546x*)((unsigned char*)scr->mmio+CursorMMIO);

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

	cursor546x->preset = (xo<<8)|yo;
	cursor546x->x = x;
	cursor546x->y = y;

	return 0;
}

static void
clgd546xcurenable(VGAscr* scr)
{
	unsigned char *p;
	Cursor546x *cursor546x;

	clgd546xenable(scr);
	if(scr->mmio == 0)
		return;
	cursor546x = (Cursor546x*)((unsigned char*)scr->mmio+CursorMMIO);

	/*
	 * Cursor colours.
	 * Can't call setcolor here as cursor is already locked.
	 */
	p = (unsigned char*)scr->mmio+PaletteState;
	*p |= 0x08;
	vgao(PaddrW, 0x00);
	vgao(Pdata, Pwhite);
	vgao(Pdata, Pwhite);
	vgao(Pdata, Pwhite);
	vgao(PaddrW, 0x0F);
	vgao(Pdata, Pblack);
	vgao(Pdata, Pblack);
	vgao(Pdata, Pblack);
	*p &= ~0x08;

	/*
	 * Find a place for the cursor data in display memory.
	 * 2 cursor images might be needed, 1KB each so use the last
	 * 2KB of the framebuffer and initialise them to be
	 * transparent.
	 */
	scr->storage = ((vgaxi(Seqx, 0x14) & 0x07)+1)*1024*1022;
	cursor546x->addr = (scr->storage>>10)<<2;
	memset((unsigned char*)scr->vaddr + scr->storage, 0, 2*64*16);

	/*
	 * Load, locate and enable the 64x64 cursor.
	 */
	clgd546xcurload(scr, &arrow);
	clgd546xcurmove(scr, ZP);
	cursor546x->enable = 1;
}

VGAdev vgaclgd546xdev = {
	"clgd546x",

	clgd546xenable,
	nil,
	nil,
	clgd546xlinear,
};

VGAcur vgaclgd546xcur = {
	"clgd546xhwgc",

	clgd546xcurenable,
	clgd546xcurdisable,
	clgd546xcurload,
	clgd546xcurmove,
};
