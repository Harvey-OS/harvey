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

typedef struct {
	int	vidProcCfg;
	int	hwCurPatAddr;
	int	hwCurLoc;
	int	hwCurC0;
	int	hwCurC1;
} Cursor3dfx;

enum {
	dramInit0	= 0x18,
	dramInit1	= 0x1C,

	hwCur		= 0x5C,
};

static ulong
tdfxlinear(VGAscr* scr, int* size, int* align)
{
	Pcidev *p;
	int oapsize, wasupamem;
	ulong aperture, oaperture;

	oaperture = scr->aperture;
	oapsize = scr->apsize;
	wasupamem = scr->isupamem;

	aperture = 0;
	if(p = pcimatch(nil, 0x121A, 0)){
		switch(p->did){
		case 0x0003:		/* Banshee */
		case 0x0005:		/* Avenger (a.k.a. Voodoo3) */
		case 0x0009:		/* Voodoo5 */
			aperture = p->mem[1].bar & ~0x0F;
			*size = p->mem[1].size;
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
tdfxenable(VGAscr* scr)
{
	Pcidev *p;
	ulong aperture;
	int align, i, *mmio, size;

	/*
	 * Only once, can't be disabled for now.
	 * scr->io holds the physical address of
	 * the MMIO registers.
	 */
	if(scr->io)
		return;
	if(p = pcimatch(nil, 0x121A, 0)){
		switch(p->did){
		case 0x0003:		/* Banshee */
		case 0x0005:		/* Avenger (a.k.a. Voodoo3) */
			break;
		default:
			return;
		}
	}
	else
		return;
	scr->io = upamalloc(p->mem[0].bar & ~0x0F, p->mem[0].size, 0);
	if(scr->io == 0)
		return;

	addvgaseg("3dfxmmio", (ulong)scr->io, p->mem[0].size);

	size = p->mem[1].size;
	align = 0;
	aperture = tdfxlinear(scr, &size, &align);
	if(aperture){
		scr->aperture = aperture;
		scr->apsize = size;
		addvgaseg("3dfxscreen", aperture, size);
	}

	/*
	 * Find a place for the cursor data in display memory.
	 * If SDRAM then there's 16MB memory else it's SGRAM
	 * and can count it based on the power-on straps -
	 * chip size can be 8Mb or 16Mb, and there can be 4 or
	 * 8 of them.
	 * Use the last 1KB of the framebuffer.
	 */
	mmio = KADDR(scr->io + dramInit0);
	if(*(mmio+1) & 0x40000000)
		i = 16*1024*1024;
	else{
		if(*mmio & 0x08000000)
			i = 16*1024*1024/8;
		else
			i = 8*1024*1024/8;
		if(*mmio & 0x04000000)
			i *= 8;
		else
			i *= 4;
	}
	scr->storage = i - 1024;
}

static void
tdfxcurdisable(VGAscr* scr)
{
	Cursor3dfx *cursor3dfx;

	if(scr->io == 0)
		return;
	cursor3dfx = KADDR(scr->io+hwCur);
	cursor3dfx->vidProcCfg &= ~0x08000000;
}

static void
tdfxcurload(VGAscr* scr, Cursor* curs)
{
	int y;
	uchar *p;
	Cursor3dfx *cursor3dfx;

	if(scr->io == 0)
		return;
	cursor3dfx = KADDR(scr->io+hwCur);

	/*
	 * Disable the cursor then load the new image in
	 * the top-left of the 64x64 array.
	 * The cursor data is stored in memory as 128-bit
	 * words consisting of plane 0 in the least significant 64-bits
	 * and plane 1 in the most significant.
	 * The X11 cursor truth table is:
	 *	p0 p1	colour
	 *	 0  0	transparent
	 *	 0  1	transparent
	 *	 1  0	hwCurC0
	 *	 1  1	hwCurC1
	 * Unused portions of the image have been initialised to be
	 * transparent.
	 */
	cursor3dfx->vidProcCfg &= ~0x08000000;
	p = KADDR(scr->aperture + scr->storage);
	for(y = 0; y < 16; y++){
		*p++ = curs->clr[2*y]|curs->set[2*y];
		*p++ = curs->clr[2*y+1]|curs->set[2*y+1];
		p += 6;
		*p++ = curs->set[2*y];
		*p++ = curs->set[2*y+1];
		p += 6;
	}

	/*
	 * Save the cursor hotpoint and enable the cursor.
	 * The 0,0 cursor point is bottom-right.
	 */
	scr->offset.x = 63+curs->offset.x;
	scr->offset.y = 63+curs->offset.y;
	cursor3dfx->vidProcCfg |= 0x08000000;
}

static int
tdfxcurmove(VGAscr* scr, Point p)
{
	Cursor3dfx *cursor3dfx;

	if(scr->io == 0)
		return 1;
	cursor3dfx = KADDR(scr->io+hwCur);

	cursor3dfx->hwCurLoc = ((p.y+scr->offset.y)<<16)|(p.x+scr->offset.x);

	return 0;
}

static void
tdfxcurenable(VGAscr* scr)
{
	Cursor3dfx *cursor3dfx;

	tdfxenable(scr);
	if(scr->io == 0)
		return;
	cursor3dfx = KADDR(scr->io+hwCur);

	/*
	 * Cursor colours.
	 */
	cursor3dfx->hwCurC0 = 0xFFFFFFFF;
	cursor3dfx->hwCurC1 = 0x00000000;

	/*
	 * Initialise the 64x64 cursor to be transparent (X11 mode).
	 */
	cursor3dfx->hwCurPatAddr = scr->storage;
	memset(KADDR(scr->aperture + scr->storage), 0, 64*16);

	/*
	 * Load, locate and enable the 64x64 cursor in X11 mode.
	 */
	tdfxcurload(scr, &arrow);
	tdfxcurmove(scr, ZP);
	cursor3dfx->vidProcCfg |= 0x08000002;
}

VGAdev vga3dfxdev = {
	"3dfx",

	tdfxenable,
	nil,
	nil,
	tdfxlinear,
};

VGAcur vga3dfxcur = {
	"3dfxhwgc",

	tdfxcurenable,
	tdfxcurdisable,
	tdfxcurload,
	tdfxcurmove,
};
