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
 * TVP3020 Viewpoint Video Interface Pallette.
 * Assumes hooked up to an S3 86C928.
 */
enum {
	Index		= 0x06,				/* Index register */
	Data		= 0x07,				/* Data register */
};

/*
 * Lower 2-bits of indirect DAC register
 * addressing.
 */
static ushort dacxreg[4] = {
	PaddrW, Pdata, Pixmask, PaddrR
};

static uchar
tvp3020io(uchar reg, uchar data)
{
	uchar crt55;

	crt55 = vgaxi(Crtx, 0x55) & 0xFC;
	vgaxo(Crtx, 0x55, crt55|((reg>>2) & 0x03));
	vgao(dacxreg[reg & 0x03], data);

	return crt55;
}

static void
tvp3020xo(uchar index, uchar data)
{
	uchar crt55;

	crt55 = tvp3020io(Index, index);
	vgao(dacxreg[Data & 0x03], data);
	vgaxo(Crtx, 0x55, crt55);
}

static void
tvp3020disable(VGAscr*)
{
	uchar r;

	/*
	 * Disable 
	 *	cursor;
	 *	cursor control enable for Bt485 DAC (!);
	 *	the hardware cursor external operation mode.
	 */
	tvp3020xo(0x06, 0x10);				/* Cursor Control Register */

	r = vgaxi(Crtx, 0x45) & ~0x20;
	vgaxo(Crtx, 0x45, r);

	r = vgaxi(Crtx, 0x55) & ~0x20;
	vgaxo(Crtx, 0x55, r);
}

static void
tvp3020enable(VGAscr*)
{
	uchar r;

	/*
	 * Make sure cursor is off by initialising the cursor
	 * control to defaults + X-Windows cursor mode.
	 */
	tvp3020xo(0x06, 0x10);				/* Cursor Control Register */

	/*
	 * Overscan colour,
	 * cursor colour 1 (white),
	 * cursor colour 2 (black).
	 */
	tvp3020xo(0x20, Pwhite); tvp3020xo(0x21, Pwhite); tvp3020xo(0x22, Pwhite);
	tvp3020xo(0x23, Pwhite); tvp3020xo(0x24, Pwhite); tvp3020xo(0x25, Pwhite);
	tvp3020xo(0x26, Pblack); tvp3020xo(0x27, Pblack); tvp3020xo(0x28, Pblack);

	/*
	 * Finally, enable
	 *	the hardware cursor external operation mode;
	 *	cursor control enable for Bt485 DAC (!).
	 */
	r = vgaxi(Crtx, 0x55)|0x20;
	vgaxo(Crtx, 0x55, r);

	r = vgaxi(Crtx, 0x45)|0x20;
	vgaxo(Crtx, 0x45, r);
}

static void
tvp3020load(VGAscr*, Cursor* curs)
{
	uchar p, p0, p1;
	int x, y;

	/*
	 * Make sure cursor is off by initialising the cursor
	 * control to defaults + X-Windows cursor mode.
	 */
	tvp3020xo(0x06, 0x10);				/* Cursor Control Register */

	/*
	 * Initialise the cursor RAM LS and MS address
	 * (LS must be first).
	 */
	tvp3020xo(0x08, 0x00);				/* Cursor RAM LS Address */
	tvp3020xo(0x09, 0x00);				/* Cursor RAM MS Address */

	/*
	 * Initialise the 64x64 cursor RAM array. There are 2 planes,
	 * p0 and p1. Data is written 4 pixels per byte, with p1 the
	 * MS bit of each pixel.
	 * The cursor is set in X-Windows mode which gives the following
	 * truth table:
	 *	p1 p0	colour
	 *	 0  0	underlying pixel colour
	 *	 0  1	underlying pixel colour
	 *	 1  0	cursor colour 1
	 *	 1  1	cursor colour 2
	 * Put the cursor into the top-left of the 64x64 array.
	 */
	for(y = 0; y < 64; y++){
		for(x = 0; x < 64/8; x++){
			if(x < 16/8 && y < 16){
				p0 = curs->clr[x+y*2];
				p1 = curs->set[x+y*2];

				p = 0x00;
				if(p1 & 0x10)
					p |= 0x03;
				else if(p0 & 0x10)
					p |= 0x02;
				if(p1 & 0x20)
					p |= 0x0C;
				else if(p0 & 0x20)
					p |= 0x08;
				if(p1 & 0x40)
					p |= 0x30;
				else if(p0 & 0x40)
					p |= 0x20;
				if(p1 & 0x80)
					p |= 0xC0;
				else if(p0 & 0x80)
					p |= 0x80;
				tvp3020xo(0x0A, p);	/* Cursor RAM Data */

				p = 0x00;
				if(p1 & 0x01)
					p |= 0x03;
				else if(p0 & 0x01)
					p |= 0x02;
				if(p1 & 0x02)
					p |= 0x0C;
				else if(p0 & 0x02)
					p |= 0x08;
				if(p1 & 0x04)
					p |= 0x30;
				else if(p0 & 0x04)
					p |= 0x20;
				if(p1 & 0x08)
					p |= 0xC0;
				else if(p0 & 0x08)
					p |= 0x80;
				tvp3020xo(0x0A, p);	/* Cursor RAM Data */
			}
			else{
				tvp3020xo(0x0A, 0x00);	/* Cursor RAM Data */
				tvp3020xo(0x0A, 0x00);
			}
		}
	}

	/*
	 * Initialise the cursor hotpoint
	 * and enable the cursor.
	 */
	tvp3020xo(0x04, -curs->offset.x);		/* Sprite Origin X */
	tvp3020xo(0x05, -curs->offset.y);		/* Sprite Origin Y */

	tvp3020xo(0x06, 0x40|0x10);			/* Cursor Control Register */
}

static int
tvp3020move(VGAscr*, Point p)
{
	tvp3020xo(0x00, p.x & 0xFF);			/* Cursor Position X LSB */
	tvp3020xo(0x01, (p.x>>8) & 0x0F);		/* Cursor Position X MSB */
	tvp3020xo(0x02, p.y & 0xFF);			/* Cursor Position Y LSB */
	tvp3020xo(0x03, (p.y>>8) & 0x0F);		/* Cursor Position Y MSB */

	return 0;
}

VGAcur vgatvp3020cur = {
	"tvp3020hwgc",

	tvp3020enable,
	tvp3020disable,
	tvp3020load,
	tvp3020move,
};
