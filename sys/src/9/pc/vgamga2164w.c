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

/*
 * Matrox Millennium and Matrox Millennium II.
 * Matrox MGA-2064W, MGA-2164W 3D graphics accelerators.
 * Texas Instruments Tvp3026 RAMDAC.
 */

enum {
	/* pci chip manufacturer */
	MATROX		= 0x102B,

	/* pci chip device ids */
	MGA2064		= 0x0519,
	MGA2164		= 0x051B,
	MGA2164AGP	= 0x051F
};

static Pcidev*
mgapcimatch(void)
{
	Pcidev *p;

	p = pcimatch(nil, MATROX, MGA2164AGP);
	if(p == nil) {
		p = pcimatch(nil, MATROX, MGA2164);
		if(p == nil)
			p = pcimatch(nil, MATROX, MGA2064);
	}
	return p;
}

static ulong
mga2164wlinear(VGAscr* scr, int* size, int* align)
{
	ulong aperture, oaperture;
	int oapsize, wasupamem;
	Pcidev *p;

	oaperture = scr->aperture;
	oapsize = scr->apsize;
	wasupamem = scr->isupamem;

	if(p = mgapcimatch()){
		aperture = p->mem[p->did==MGA2064? 1 : 0].bar & ~0x0F;
		*size = (p->did==MGA2064? 8 :16)*1024*1024;
	}
	else
		aperture = 0;

	if(wasupamem) {
		if(oaperture == aperture)
			return oaperture;
		upafree(oaperture, oapsize);
	}
	scr->isupamem = 0;

	aperture = upamalloc(aperture, *size, *align);
	if(aperture == 0){
		if(wasupamem && upamalloc(oaperture, oapsize, 0)) {
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
mga2164wenable(VGAscr* scr)
{
	Pcidev *p;
	int size, align, immio;
	ulong aperture;

	/*
	 * Only once, can't be disabled for now.
	 * scr->io holds the virtual address of
	 * the MMIO registers.
	 */
	if(scr->io)
		return;

	p = mgapcimatch();
	if(p == nil)
		return;

	immio = p->did==MGA2064? 0 : 1;
	scr->io = upamalloc(p->mem[immio].bar & ~0x0F, p->mem[immio].size, 0);
	if(scr->io == 0)
		return;
	addvgaseg("mga2164wmmio", scr->io, p->mem[immio].size);

	scr->io = (ulong)KADDR(scr->io);

	/* need to map frame buffer here too, so vga can find memory size */
	size = (p->did==MGA2064? 8 :16)*1024*1024;
	align = 0;
	aperture = mga2164wlinear(scr, &size, &align);
	if(aperture) {
		scr->aperture = aperture;
		scr->apsize = size;
		addvgaseg("mga2164wscreen", aperture, size);
	}
}

enum {
	Index		= 0x00,		/* Index */
	Data		= 0x0A,		/* Data */

	CaddrW		= 0x04,		/* Colour Write Address */
	Cdata		= 0x05,		/* Colour Data */

	Cctl		= 0x09,		/* Direct Cursor Control */
	Cram		= 0x0B,		/* Cursor Ram Data */
	Cxlsb		= 0x0C,		/* Cursor X LSB */
	Cxmsb		= 0x0D,		/* Cursor X MSB */
	Cylsb		= 0x0E,		/* Cursor Y LSB */
	Cymsb		= 0x0F,		/* Cursor Y MSB */

	Icctl		= 0x06,		/* Indirect Cursor Control */
};

static void
tvp3026disable(VGAscr* scr)
{
	uchar *tvp3026;

	if(scr->io == 0)
		return;
	tvp3026 = KADDR(scr->io+0x3C00);

	/*
	 * Make sure cursor is off
	 * and direct control enabled.
	 */
	*(tvp3026+Index) = Icctl;
	*(tvp3026+Data) = 0x90;
	*(tvp3026+Cctl) = 0x00;
}

static void
tvp3026load(VGAscr* scr, Cursor* curs)
{
	int x, y;
	uchar *tvp3026;

	if(scr->io == 0)
		return;
	tvp3026 = KADDR(scr->io+0x3C00);

	/*
	 * Make sure cursor is off by initialising the cursor
	 * control to defaults.
	 * Write to the indirect control register to make sure
	 * direct register is enabled and upper 2 bits of cursor
	 * RAM address are 0.
	 * Put 0 in index register for lower 8 bits of cursor RAM address.
	 */
	tvp3026disable(scr);
	*(tvp3026+Index) = 0;

	/*
	 * Initialise the 64x64 cursor RAM array. There are 2 planes,
	 * p0 and p1. Data is written 8 pixels per byte, with p0 in the
	 * first 512 bytes of the array and p1 in the second.
	 * The cursor is set in 3-colour mode which gives the following
	 * truth table:
	 *	p1 p0	colour
	 *	 0  0	transparent
	 *	 0  1	cursor colour 0
	 *	 1  0	cursor colour 1
	 *	 1  1	cursor colour 2
	 * Put the cursor into the top-left of the 64x64 array.
	 * The 0,0 cursor point is bottom-right, so positioning will
	 * have to take that into account.
	 */
	for(y = 0; y < 64; y++){
		for(x = 0; x < 64/8; x++){
			if(x < 16/8 && y < 16)
				*(tvp3026+Cram) = curs->clr[x+y*2];
			else
				*(tvp3026+Cram) = 0x00;
		}
	}
	for(y = 0; y < 64; y++){
		for(x = 0; x < 64/8; x++){
			if(x < 16/8 && y < 16)
				*(tvp3026+Cram) = curs->set[x+y*2];
			else
				*(tvp3026+Cram) = 0x00;
		}
	}

	/*
	 * Initialise the cursor hotpoint
	 * and enable the cursor in 3-colour mode.
	 */
	scr->offset.x = 64+curs->offset.x;
	scr->offset.y = 64+curs->offset.y;
	*(tvp3026+Cctl) = 0x01;
}

static int
tvp3026move(VGAscr* scr, Point p)
{
	int x, y;
	uchar *tvp3026;

	if(scr->io == 0)
		return 1;
	tvp3026 = KADDR(scr->io+0x3C00);

	x = p.x+scr->offset.x;
	y = p.y+scr->offset.y;

	*(tvp3026+Cxlsb) = x & 0xFF;
	*(tvp3026+Cxmsb) = (x>>8) & 0x0F;
	*(tvp3026+Cylsb) = y & 0xFF;
	*(tvp3026+Cymsb) = (y>>8) & 0x0F;

	return 0;
}

static void
tvp3026enable(VGAscr* scr)
{
	int i;
	uchar *tvp3026;

	if(scr->io == 0)
		return;
	tvp3026 = KADDR(scr->io+0x3C00);

	tvp3026disable(scr);

	/*
	 * Overscan colour,
	 * cursor colour 1 (white),
	 * cursor colour 2, 3 (black).
	 */
	*(tvp3026+CaddrW) = 0x00;
	for(i = 0; i < 6; i++)
		*(tvp3026+Cdata) = Pwhite; 
	for(i = 0; i < 6; i++)
		*(tvp3026+Cdata) = Pblack; 

	/*
	 * Load, locate and enable the
	 * 64x64 cursor in 3-colour mode.
	 */
	tvp3026load(scr, &arrow);
	tvp3026move(scr, ZP);
	*(tvp3026+Cctl) = 0x01;
}

VGAdev vgamga2164wdev = {
	"mga2164w",

	mga2164wenable,			/* enable */
	0,				/* disable */
	0,				/* page */
	mga2164wlinear,			/* linear */
};

VGAcur vgamga2164wcur = {
	"mga2164whwgc",

	tvp3026enable,
	tvp3026disable,
	tvp3026load,
	tvp3026move,
};
