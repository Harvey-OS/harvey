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

static int
ark2000pvpageset(VGAscr*, int page)
{
	uchar seq15;

	seq15 = vgaxi(Seqx, 0x15);
	vgaxo(Seqx, 0x15, page);
	vgaxo(Seqx, 0x16, page);

	return seq15;
}

static void
ark2000pvpage(VGAscr* scr, int page)
{
	lock(&scr->devlock);
	ark2000pvpageset(scr, page);
	unlock(&scr->devlock);
}

static void
ark2000pvdisable(VGAscr*)
{
	uchar seq20;

	seq20 = vgaxi(Seqx, 0x20) & ~0x08;
	vgaxo(Seqx, 0x20, seq20);
}

static void
ark2000pvenable(VGAscr* scr)
{
	uchar seq20;
	ulong storage;

	/*
	 * Disable the cursor then configure for X-Windows style,
	 * 32x32 and 4/8-bit colour depth.
	 * Set cursor colours for 4/8-bit.
	 */
	seq20 = vgaxi(Seqx, 0x20) & ~0x1F;
	vgaxo(Seqx, 0x20, seq20);
	seq20 |= 0x18;

	vgaxo(Seqx, 0x26, 0x00);
	vgaxo(Seqx, 0x27, 0x00);
	vgaxo(Seqx, 0x28, 0x00);
	vgaxo(Seqx, 0x29, 0xFF);
	vgaxo(Seqx, 0x2A, 0xFF);
	vgaxo(Seqx, 0x2B, 0xFF);

	/*
	 * Cursor storage is a 256 byte or 1Kb block located in the last
	 * 16Kb of video memory. Crt25 is the index of which block.
	 */
	storage = (vgaxi(Seqx, 0x10)>>6) & 0x03;
	storage = (1024*1024)<<storage;
	storage -= 256;
	scr->storage = storage;
	vgaxo(Seqx, 0x25, 0x3F);

	/*
	 * Enable the cursor.
	 */
	vgaxo(Seqx, 0x20, seq20);
}

static void
ark2000pvload(VGAscr* scr, Cursor* curs)
{
	uchar *p, seq10;
	int opage, x, y;

	/*
	 * Is linear addressing turned on? This will determine
	 * how we access the cursor storage.
	 */
	seq10 = vgaxi(Seqx, 0x10);
	opage = 0;
	p = KADDR(scr->aperture);
	if(!(seq10 & 0x10)){
		lock(&scr->devlock);
		opage = ark2000pvpageset(scr, scr->storage>>16);
		p += (scr->storage & 0xFFFF);
	}
	else
		p += scr->storage;

	/*
	 * The cursor is set in X11 mode which gives the following
	 * truth table:
	 *	and xor	colour
	 *	 0   0	underlying pixel colour
	 *	 0   1	underlying pixel colour
	 *	 1   0	background colour
	 *	 1   1	foreground colour
	 * Put the cursor into the top-left of the 32x32 array.
	 * The manual doesn't say what the data layout in memory is -
	 * this worked out by trial and error.
	 */
	for(y = 0; y < 32; y++){
		for(x = 0; x < 32/8; x++){
			if(x < 16/8 && y < 16){
				*p++ = curs->clr[2*y + x]|curs->set[2*y + x];
				*p++ = curs->set[2*y + x];
			}
			else {
				*p++ = 0x00;
				*p++ = 0x00;
			}
		}
	}

	if(!(seq10 & 0x10)){
		ark2000pvpageset(scr, opage);
		unlock(&scr->devlock);
	}

	/*
	 * Save the cursor hotpoint.
	 */
	scr->offset = curs->offset;
}

static int
ark2000pvmove(VGAscr* scr, Point p)
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
	vgaxo(Seqx, 0x2C, xo);
	vgaxo(Seqx, 0x2D, yo);
	vgaxo(Seqx, 0x21, (x>>8) & 0x0F);
	vgaxo(Seqx, 0x22, x & 0xFF);
	vgaxo(Seqx, 0x23, (y>>8) & 0x0F);
	vgaxo(Seqx, 0x24, y & 0xFF);

	return 0;
}

VGAdev vgaark2000pvdev = {
	"ark2000pv",

	0,
	0,
	ark2000pvpage,
	0,
};

VGAcur vgaark2000pvcur = {
	"ark2000pvhwgc",

	ark2000pvenable,
	ark2000pvdisable,
	ark2000pvload,
	ark2000pvmove,
};
