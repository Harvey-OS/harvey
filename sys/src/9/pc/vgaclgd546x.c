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
	ushort	x;
	ushort	y;
	ushort	preset;
	ushort	enable;
	ushort	addr;
};

enum {
	PaletteState	= 0xB0,
	CursorMMIO	= 0xE0,
};

static ulong
clgd546xlinear(VGAscr* scr, int* size, int* align)
{
	ulong aperture, oaperture;
	int oapsize, wasupamem;
	Pcidev *p;

	oaperture = scr->aperture;
	oapsize = scr->apsize;
	wasupamem = scr->isupamem;

	aperture = 0;
	if(p = pcimatch(nil, 0x1013, 0)){
		switch(p->did){
		case 0xD0:
		case 0xD4:
		case 0xD6:
			aperture = p->mem[0].bar & ~0x0F;
			*size = p->mem[0].size;
			break;
		default:
			break;
		}
	}

	if(wasupamem){
		if(oaperture == aperture)
			return oaperture;
		upafree(oaperture, oapsize);
	}
	scr->isupamem = 0;

	aperture = upamalloc(aperture, *size, *align);
	if(aperture == 0){
		if(wasupamem && upamalloc(oaperture, oapsize, 0)){
			aperture = oaperture;
			scr->isupamem = 1;
		}
		else
			scr->isupamem = 0;
	}
	else
		scr->isupamem = 1;

	return aperture;
}
static void
clgd546xenable(VGAscr* scr)
{
	Pcidev *p;
	int size, align;
	ulong aperture;

	/*
	 * Only once, can't be disabled for now.
	 * scr->io holds the virtual address of
	 * the MMIO registers.
	 */
	if(scr->io)
		return;
	if(p = pcimatch(nil, 0x1013, 0)){
		switch(p->did){
		case 0xD0:
		case 0xD4:
		case 0xD6:
			break;
		default:
			return;
		}
	}
	else
		return;
	scr->io = upamalloc(p->mem[1].bar & ~0x0F, p->mem[1].size, 0);
	if(scr->io == 0)
		return;
	addvgaseg("clgd546xmmio", scr->io, p->mem[1].size);

	scr->io = (ulong)KADDR(scr->io);

	size = p->mem[0].size;
	align = 0;
	aperture = clgd546xlinear(scr, &size, &align);
	if(aperture) {
		scr->aperture = aperture;
		scr->apsize = size;
		addvgaseg("clgd546xscreen", aperture, size);
	}
}

static void
clgd546xcurdisable(VGAscr* scr)
{
	Cursor546x *cursor546x;

	if(scr->io == 0)
		return;
	cursor546x = (Cursor546x*)(scr->io+CursorMMIO);
	cursor546x->enable = 0;
}

static void
clgd546xcurload(VGAscr* scr, Cursor* curs)
{
	int c, i, m, y;
	uchar *p;
	Cursor546x *cursor546x;

	if(scr->io == 0)
		return;
	cursor546x = (Cursor546x*)(scr->io+CursorMMIO);

	/*
	 * Disable the cursor then change only the bits
	 * that need it.
	 */
	cursor546x->enable = 0;
	p = (uchar*)(scr->aperture + scr->storage);
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

	if(scr->io == 0)
		return 1;
	cursor546x = (Cursor546x*)(scr->io+CursorMMIO);

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
	uchar *p;
	Cursor546x *cursor546x;

	clgd546xenable(scr);
	if(scr->io == 0)
		return;
	cursor546x = (Cursor546x*)(scr->io+CursorMMIO);

	/*
	 * Cursor colours.
	 * Can't call setcolor here as cursor is already locked.
	 */
	p = (uchar*)(scr->io+PaletteState);
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
	memset((uchar*)(scr->aperture + scr->storage), 0, 2*64*16);

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
