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

static void
ct65545page(VGAscr*, int page)
{
	outb(0x3D6, 0x10);
	outb(0x3D7, page<<6);
}

static void
ct65545disable(VGAscr*)
{
	outl(0xA3D0, 0);
}

static void
ct65545enable(VGAscr* scr)
{
	ulong storage;

	/*
	 * Find a place for the cursor data in display memory.
	 * Must be on a 1024-byte boundary.
	 */
	storage = ROUND(scr->gscreen->width*BY2WD*scr->gscreen->r.max.y, 1024);
	outl(0xB3D0, storage);
	scr->storage = storage;

	/*
	 * Set the colours.
	 * Enable the cursor.
	 */
	outl(0xA7D0, 0xFFFF0000);
	outl(0xA3D0, 2);
}

static void
ct65545initcursor(VGAscr* scr, int xo, int yo, int index)
{
	uchar *mem;
	uint and, clr, set, xor;
	int i, x, y;

	mem = KADDR(scr->aperture);
	mem += scr->storage + index*1024;

	for(y = yo; y < 16; y++){
		clr = (scr->clr[2*y]<<8)|scr->clr[2*y+1];
		set = (scr->set[2*y]<<8)|scr->set[2*y+1];
		if(xo){
			clr <<= xo;
			set <<= xo;
		}

		and = 0;
		xor = 0;
		for(i = 0; i < 16; i++){
			if(set & (1<<i)){
				/* nothing to do */
			}
			else if(clr & (1<<i))
				xor |= 1<<i;
			else
				and |= 1<<i;
		}
		*mem++ = and>>8;
		*mem++ = xor>>8;
		*mem++ = and;
		*mem++ = xor;

		for(x = 16; x < 64; x += 8){
			*mem++ = 0xFF;
			*mem++ = 0x00;
		}
	}
	while(y < 64+yo){
		for(x = 0; x < 64; x += 8){
			*mem++ = 0xFF;
			*mem++ = 0x00;
		}
		y++;
	}
}

static void
ct65545load(VGAscr* scr, Cursor* curs)
{
	memmove(&scr->Cursor, curs, sizeof(Cursor));
	ct65545initcursor(scr, 0, 0, 0);
}

static int
ct65545move(VGAscr* scr, Point p)
{
	int index, x, xo, y, yo;

	index = 0;
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

	if(xo || yo){
		ct65545initcursor(scr, xo, yo, 1);
		index = 1;
	}
	outl(0xB3D0, scr->storage + index*1024);

	outl(0xAFD0, (y<<16)|x);

	return 0;
}

VGAdev vgact65545dev = {
	"ct65540",				/* BUG: really 65545 */

	0,
	0,
	ct65545page,
	0,
};

VGAcur vgact65545cur = {
	"ct65545hwgc",

	ct65545enable,
	ct65545disable,
	ct65545load,
	ct65545move,
};
