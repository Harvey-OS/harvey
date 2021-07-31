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
setet4000page(int page)
{
	uchar p;

	p = page & 0x0F;
	p |= p<<4;
	outb(0x3CD, p);

	p = (page & 0x30);
	p |= p>>4;
	outb(0x3CB, p);
}

static void
et4000page(VGAscr *scr, int page)
{
	lock(&scr->devlock);
	setet4000page(page);
	unlock(&scr->devlock);
}

static void
et4000disable(VGAscr*)
{
	uchar imaF7;

	outb(0x217A, 0xF7);
	imaF7 = inb(0x217B) & ~0x80;
	outb(0x217B, imaF7);
}

static void
et4000enable(VGAscr *scr)
{
	uchar imaF7;

	et4000disable(scr);

	/*
	 * Configure CRTCB for Sprite, 64x64,
	 * CRTC pixel overlay.
	 */
	outb(0x217A, 0xEF);
	outb(0x217B, 0x02);

	/*
	 * Cursor goes in the top left corner
	 * of the Sprite area, so the horizontal and
	 * vertical presets are 0.
	 */
	outb(0x217A, 0xE2);
	outb(0x217B, 0x00);
	outb(0x217A, 0xE3);
	outb(0x217B, 0x00);

	outb(0x217A, 0xE6);
	outb(0x217B, 0x00);
	outb(0x217A, 0xE7);
	outb(0x217B, 0x00);

	/*
	 * Find a place for the cursor data in display memory.
	 * Must be on a "doubleword" boundary, but put it on a
	 * 1024-byte boundary so that there's no danger of it
	 * crossing a page.
	 */
	scr->storage = (scr->gscreen->width*BY2WD*scr->gscreen->r.max.y+1023)/1024;
	scr->storage *= 1024/4;
	outb(0x217A, 0xE8);
	outb(0x217B, scr->storage & 0xFF);
	outb(0x217A, 0xE9);
	outb(0x217B, (scr->storage>>8) & 0xFF);
	outb(0x217A, 0xEA);
	outb(0x217B, (scr->storage>>16) & 0x0F);
	scr->storage *= 4;

	/*
	 * Row offset in "quadwords". Must be 2 for Sprite.
	 * Bag the pixel-panning.
	 * Colour depth, must be 2 for Sprite.
	 */
	outb(0x217A, 0xEB);
	outb(0x217B, 0x02);
	outb(0x217A, 0xEC);
	outb(0x217B, 0x00);

	outb(0x217A, 0xED);
	outb(0x217B, 0x00);

	outb(0x217A, 0xEE);
//	if(vgascreen.ldepth == 3)
		outb(0x217B, 0x01);
//	else
//		outb(0x217B, 0x00);

	/*
	 * Enable the CRTCB/Sprite.
	 */
	outb(0x217A, 0xF7);
	imaF7 = inb(0x217B);
	outb(0x217B, 0x80|imaF7);
}

static void
et4000load(VGAscr *scr, Cursor *c)
{
	uchar p0, p1, *mem;
	int i, x, y;
	ushort p;
	uchar clr[2*16], set[2*16];

	/*
	 * Lock the display memory so we can update the
	 * cursor bitmap if necessary.
	 */
	lock(&scr->devlock);

	/*
	 * Disable the cursor.
	 * Set the display page (do we need to restore
	 * the current contents when done?) and the
	 * pointer to the two planes. What if this crosses
	 * into a new page?
	 */
	et4000disable(scr);

	setet4000page(scr->storage>>16);
	mem = (uchar*)KADDR(scr->aperture) + (scr->storage & 0xFFFF);

	/*
	 * Initialise the 64x64 cursor RAM array. There are 2 planes,
	 * p0 and p1. Data is written 4 pixels per byte, with p1 the
	 * MS bit of each pixel.
	 * The cursor mode gives the following truth table:
	 *	p1 p0	colour
	 *	 0  0	Sprite Colour 0 (defined as 0x00)
	 *	 0  1	Sprite Colour 1 (defined as 0xFF)
	 *	 1  0	Transparent (allow CRTC pixel pass through)
	 *	 1  1	Invert (allow CRTC pixel invert through)
	 * Put the cursor into the top-left of the 64x64 array.
	 *
	 * This is almost certainly wrong, since it has not
	 * been updated for the 3rd edition color values.
	 */
	memmove(clr, c->clr, sizeof(clr));
//	pixreverse(clr, sizeof(clr), 0);
	memmove(set, c->set, sizeof(set));
//	pixreverse(set, sizeof(set), 0);
	for(y = 0; y < 64; y++){
		for(x = 0; x < 64/8; x++){
			if(x < 16/8 && y < 16){
				p0 = clr[x+y*2];
				p1 = set[x+y*2];

				p = 0x0000;
				for(i = 0; i < 8; i++){
					if(p1 & (1<<(7-i))){
						/* nothing to do */
					}
					else if(p0 & (1<<(7-i)))
						p |= 0x01<<(2*i);
					else
						p |= 0x02<<(2*i);
				}
				*mem++ = p & 0xFF;
				*mem++ = (p>>8) & 0xFF;
			}
			else {
				*mem++ = 0xAA;
				*mem++ = 0xAA;
			}
		}
	}

	/*
	 * enable the cursor.
	 */
	outb(0x217A, 0xF7);
	p = inb(0x217B)|0x80;
	outb(0x217B, p);

	unlock(&scr->devlock);
}

static int
et4000move(VGAscr *scr, Point p)
{
	int x, xo, y, yo;

	if(canlock(&scr->devlock) == 0)
		return 1;

	/*
	 * Mustn't position the cursor offscreen even partially,
	 * or it disappears. Therefore, if x or y is -ve, adjust the
	 * cursor presets instead.
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
	 * The cursor image is jerky if we don't do this.
	 * The cursor information is probably fetched from
	 * display memory during the horizontal blank active
	 * time and it doesn't like it if the coordinates
	 * are changed underneath.
	 */
	while((vgai(Status1) & 0x08) == 0)
		;

	outb(0x217A, 0xE2);
	outb(0x217B, xo);

	outb(0x217A, 0xE6);
	outb(0x217B, yo);

	outb(0x217A, 0xE1);
	outb(0x217B, (x>>8) & 0xFF);
	outb(0x217A, 0xE0);
	outb(0x217B, x & 0xFF);
	outb(0x217A, 0xE5);
	outb(0x217B, (y>>8) & 0xFF);
	outb(0x217A, 0xE4);
	outb(0x217B, y & 0xFF);

	unlock(&scr->devlock);
	return 0;
}

VGAcur vgaet4000cur = {
	"et4000hwgc",

	et4000enable,
	et4000disable,
	et4000load,
	et4000move,
};

VGAdev vgaet4000dev = {
	"et4000",

	0,
	0,
	et4000page,
	0
};
