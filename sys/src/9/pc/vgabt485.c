#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"

#define	Image	IMAGE
#include <draw.h>
#include <memdraw.h>
#include <cursor.h>
#include "screen.h"

/*
 * Hardware graphics cursor support for
 * Brooktree Bt485 Monolithic True-Color RAMDAC.
 * Assumes hooked up to an S3 86C928.
 *
 * BUGS:
 *	64x64x2 cursor always used;
 *	no support for interlaced mode.
 */
enum {
	AddrW		= 0x00,		/* Address register; palette/cursor RAM write */
	Palette		= 0x01,		/* 6/8-bit color palette data */
	Pmask		= 0x02,		/* Pixel mask register */
	AddrR		= 0x03,		/* Address register; palette/cursor RAM read */
	ColorW		= 0x04,		/* Address register; cursor/overscan color write */
	Color		= 0x05,		/* Cursor/overscan color data */
	Cmd0		= 0x06,		/* Command register 0 */
	ColorR		= 0x07,		/* Address register; cursor/overscan color read */
	Cmd1		= 0x08,		/* Command register 1 */
	Cmd2		= 0x09,		/* Command register 2 */
	Status		= 0x0A,		/* Status */
	Cmd3		= 0x1A,		/* Command register 3 */
	Cram		= 0x0B,		/* Cursor RAM array data */
	Cxlr		= 0x0C,		/* Cursor x-low register */
	Cxhr		= 0x0D,		/* Cursor x-high register */
	Cylr		= 0x0E,		/* Cursor y-low register */
	Cyhr		= 0x0F,		/* Cursor y-high register */

	Nreg		= 0x10,
};

/*
 * Lower 2-bits of indirect DAC register
 * addressing.
 */
static ushort dacxreg[4] = {
	PaddrW, Pdata, Pixmask, PaddrR
};

static uchar
bt485io(uchar reg)
{
	uchar crt55, cr0;

	crt55 = vgaxi(Crtx, 0x55) & 0xFC;
	if((reg & 0x0F) == Status){
		/*
		 * 1,2: Set indirect addressing for Status or
		 *      Cmd3 - set bit7 of Cr0.
		 */
		vgaxo(Crtx, 0x55, crt55|((Cmd0>>2) & 0x03));
		cr0 = vgai(dacxreg[Cmd0 & 0x03])|0x80;
		vgao(dacxreg[Cmd0 & 0x03], cr0);

		/*
		 * 3,4: Set the index into the Write register,
		 *      index == 0x00 for Status, 0x01 for Cmd3.
		 */
		vgaxo(Crtx, 0x55, crt55|((AddrW>>2) & 0x03));
		vgao(dacxreg[AddrW & 0x03], (reg == Status) ? 0x00: 0x01);

		/*
		 * 5,6: Get the contents of the appropriate
		 *      register at 0x0A.
		 */
	}

	return crt55;
}

static uchar
bt485i(uchar reg)
{
	uchar crt55, r;

	crt55 = bt485io(reg);
	vgaxo(Crtx, 0x55, crt55|((reg>>2) & 0x03));
	r = vgai(dacxreg[reg & 0x03]);
	vgaxo(Crtx, 0x55, crt55);

	return r;
}

static void
bt485o(uchar reg, uchar data)
{
	uchar crt55;

	crt55 = bt485io(reg);
	vgaxo(Crtx, 0x55, crt55|((reg>>2) & 0x03));
	vgao(dacxreg[reg & 0x03], data);
	vgaxo(Crtx, 0x55, crt55);
}

static void
bt485disable(VGAscr*)
{
	uchar r;

	/*
	 * Disable 
	 *	cursor mode 3;
	 *	cursor control enable for Bt485 DAC;
	 *	the hardware cursor external operation mode.
	 */
	r = bt485i(Cmd2) & ~0x03;
	bt485o(Cmd2, r);

	r = vgaxi(Crtx, 0x45) & ~0x20;
	vgaxo(Crtx, 0x45, r);

	r = vgaxi(Crtx, 0x55) & ~0x20;
	vgaxo(Crtx, 0x55, r);
}

static void
bt485enable(VGAscr*)
{
	uchar r;

	/*
	 * Turn cursor off.
	 */
	r = bt485i(Cmd2) & 0xFC;
	bt485o(Cmd2, r);

	/*
	 * Overscan colour,
	 * cursor colour 1 (white),
	 * cursor colour 2, 3 (black).
	 */
	bt485o(ColorW, 0x00);
	bt485o(Color, Pwhite); bt485o(Color, Pwhite); bt485o(Color, Pwhite);

	bt485o(Color, Pwhite); bt485o(Color, Pwhite); bt485o(Color, Pwhite);

	bt485o(Color, Pblack); bt485o(Color, Pblack); bt485o(Color, Pblack);
	bt485o(Color, Pblack); bt485o(Color, Pblack); bt485o(Color, Pblack);

	/*
	 * Finally, enable
	 *	the hardware cursor external operation mode;
	 *	cursor control enable for Bt485 DAC.
	 * The #9GXE cards seem to need the 86C928 Bt485 support
	 * enabled in order to work at all in enhanced mode.
	 */

	r = vgaxi(Crtx, 0x55)|0x20;
	vgaxo(Crtx, 0x55, r);

	r = vgaxi(Crtx, 0x45)|0x20;
	vgaxo(Crtx, 0x45, r);
}

static void
bt485load(VGAscr* scr, Cursor* curs)
{
	uchar r;
	int x, y;

	/*
	 * Turn cursor off;
	 * put cursor into 64x64x2 mode and clear MSBs of address;
	 * clear LSBs of address;
	 */
	r = bt485i(Cmd2) & 0xFC;
	bt485o(Cmd2, r);

	r = (bt485i(Cmd3) & 0xFC)|0x04;
	bt485o(Cmd3, r);

	bt485o(AddrW, 0x00);

	/*
	 * Now load the cursor RAM array, both planes.
	 * The cursor is 16x16, the array 64x64; put
	 * the cursor in the top left. The 0,0 cursor
	 * point is bottom-right, so positioning will
	 * have to take that into account.
	 */
	for(y = 0; y < 64; y++){
		for(x = 0; x < 64/8; x++){
			if(x < 16/8 && y < 16)
				bt485o(Cram, curs->clr[x+y*2]);
			else
				bt485o(Cram, 0x00);
		}
	}
	for(y = 0; y < 64; y++){
		for(x = 0; x < 64/8; x++){
			if(x < 16/8 && y < 16)
				bt485o(Cram, curs->set[x+y*2]);
			else
				bt485o(Cram, 0x00);
		}
	}

	/*
	 * Initialise the cursor hot-point
	 * and enable the cursor.
	 */
	scr->offset.x = 64+curs->offset.x;
	scr->offset.y = 64+curs->offset.y;

	r = (bt485i(Cmd2) & 0xFC)|0x01;
	bt485o(Cmd2, r);
}

static int
bt485move(VGAscr* scr, Point p)
{
	int x, y;

	x = p.x+scr->offset.x;
	y = p.y+scr->offset.y;

	bt485o(Cxlr, x & 0xFF);
	bt485o(Cxhr, (x>>8) & 0x0F);
	bt485o(Cylr, y & 0xFF);
	bt485o(Cyhr, (y>>8) & 0x0F);

	return 0;
}

VGAcur vgabt485cur = {
	"bt485hwgc",

	bt485enable,
	bt485disable,
	bt485load,
	bt485move,
};
