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
	Xrx		= 0x3D6,	/* Configuration Extensions Index */
};

static uchar
hiqvideoxi(long port, uchar index)
{
	uchar data;

	outb(port, index);
	data = inb(port+1);

	return data;
}

static void
hiqvideoxo(long port, uchar index, uchar data)
{
	outb(port, index);
	outb(port+1, data);
}

static ulong
hiqvideolinear(VGAscr* scr, int* size, int* align)
{
	ulong aperture, oaperture;
	int oapsize, wasupamem;
	Pcidev *p;

	oaperture = scr->aperture;
	oapsize = scr->apsize;
	wasupamem = scr->isupamem;

	aperture = 0;
	if(p = pcimatch(nil, 0x102C, 0)){
		switch(p->did){
		case 0x00C0:		/* 69000 HiQVideo */
		case 0x00E0:		/* 65550 HiQV32 */
		case 0x00E4:		/* 65554 HiQV32 */
		case 0x00E5:		/* 65555 HiQV32 */
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
hiqvideoenable(VGAscr* scr)
{
	Pcidev *p;
	int align, size, vmsize;
	ulong aperture;

	/*
	 * Only once, can't be disabled for now.
	 */
	if(scr->io)
		return;
	if(p = pcimatch(nil, 0x102C, 0)){
		switch(p->did){
		case 0x00C0:		/* 69000 HiQVideo */
			vmsize = 2*1024*1024;
			break;
		case 0x00E0:		/* 65550 HiQV32 */
		case 0x00E4:		/* 65554 HiQV32 */
		case 0x00E5:		/* 65555 HiQV32 */
			switch((hiqvideoxi(Xrx, 0x43)>>1) & 0x03){
			default:
			case 0:
				vmsize = 1*1024*1024;
				break;
			case 1:
				vmsize = 2*1024*1024;
				break;
			}
			break;
		default:
			return;
		}
	}
	else
		return;

	size = p->mem[0].size;
	align = 0;
	aperture = hiqvideolinear(scr, &size, &align);
	if(aperture) {
		scr->aperture = aperture;
		scr->apsize = size;
		addvgaseg("hiqvideoscreen", aperture, size);
	}

	/*
	 * Find a place for the cursor data in display memory.
	 * Must be on a 4096-byte boundary.
	 * scr->io holds the physical address of the cursor
	 * storage area in the framebuffer region.
	 */
	scr->storage = vmsize-4096;
	scr->io = scr->aperture+scr->storage;
}

static void
hiqvideocurdisable(VGAscr*)
{
	hiqvideoxo(Xrx, 0xA0, 0x10);
}

static void
hiqvideocurload(VGAscr* scr, Cursor* curs)
{
	uchar *p;
	int x, y;

	/*
	 * Disable the cursor.
	 */
	hiqvideocurdisable(scr);

	if(scr->io == 0)
		return;
	p = KADDR(scr->io);

	for(y = 0; y < 16; y += 2){
		*p++ = ~(curs->clr[2*y]|curs->set[2*y]);
		*p++ = ~(curs->clr[2*y+1]|curs->set[2*y+1]);
		*p++ = 0xFF;
		*p++ = 0xFF;
		*p++ = ~(curs->clr[2*y+2]|curs->set[2*y+2]);
		*p++ = ~(curs->clr[2*y+3]|curs->set[2*y+3]);
		*p++ = 0xFF;
		*p++ = 0xFF;
		*p++ = curs->set[2*y];
		*p++ = curs->set[2*y+1];
		*p++ = 0x00;
		*p++ = 0x00;
		*p++ = curs->set[2*y+2];
		*p++ = curs->set[2*y+3];
		*p++ = 0x00;
		*p++ = 0x00;
	}
	while(y < 32){
		for(x = 0; x < 64; x += 8)
			*p++ = 0xFF;
		for(x = 0; x < 64; x += 8)
			*p++ = 0x00;
		y += 2;
	}

	/*
	 * Save the cursor hotpoint and enable the cursor.
	 */
	scr->offset = curs->offset;
	hiqvideoxo(Xrx, 0xA0, 0x11);
}

static int
hiqvideocurmove(VGAscr* scr, Point p)
{
	int x, y;

	if(scr->io == 0)
		return 1;

	if((x = p.x+scr->offset.x) < 0)
		x = 0x8000|(-x & 0x07FF);
	if((y = p.y+scr->offset.y) < 0)
		y = 0x8000|(-y & 0x07FF);

	hiqvideoxo(Xrx, 0xA4, x & 0xFF);
	hiqvideoxo(Xrx, 0xA5, (x>>8) & 0xFF);
	hiqvideoxo(Xrx, 0xA6, y & 0xFF);
	hiqvideoxo(Xrx, 0xA7, (y>>8) & 0xFF);

	return 0;
}

static void
hiqvideocurenable(VGAscr* scr)
{
	uchar xr80;

	hiqvideoenable(scr);
	if(scr->io == 0)
		return;

	/*
	 * Disable the cursor.
	 */
	hiqvideocurdisable(scr);

	/*
	 * Cursor colours.
	 * Can't call setcolor here as cursor is already locked.
	 * When done make sure the cursor enable in Xr80 is set.
	 */
	xr80 = hiqvideoxi(Xrx, 0x80);
	hiqvideoxo(Xrx, 0x80, xr80|0x01);
	vgao(PaddrW, 0x04);
	vgao(Pdata, Pwhite);
	vgao(Pdata, Pwhite);
	vgao(Pdata, Pwhite);
	vgao(Pdata, Pblack);
	vgao(Pdata, Pblack);
	vgao(Pdata, Pblack);
	hiqvideoxo(Xrx, 0x80, xr80|0x10);

	hiqvideoxo(Xrx, 0xA2, (scr->storage>>12)<<4);
	hiqvideoxo(Xrx, 0xA3, (scr->storage>>16) & 0x3F);

	/*
	 * Load, locate and enable the 32x32 cursor.
	 * Cursor enable in Xr80 better be set already.
	 */
	hiqvideocurload(scr, &arrow);
	hiqvideocurmove(scr, ZP);
	hiqvideoxo(Xrx, 0xA0, 0x11);
}

VGAdev vgahiqvideodev = {
	"hiqvideo",

	hiqvideoenable,			/* enable */
	nil,				/* disable */
	nil,				/* page */
	hiqvideolinear,			/* linear */
};

VGAcur vgahiqvideocur = {
	"hiqvideohwgc",

	hiqvideocurenable,		/* enable */
	hiqvideocurdisable,		/* disable */
	hiqvideocurload,		/* load */
	hiqvideocurmove,		/* move */
};
