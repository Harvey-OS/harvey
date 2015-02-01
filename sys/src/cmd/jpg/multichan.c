/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <draw.h>
#include <memdraw.h>
#include <bio.h>
#include "imagefile.h"

/* Separate colors, if not a grey scale or bitmap, into one byte per color per pixel, no alpha or X */
/* Result is GREY[1248] or RGB24 */

static
int
notrans(ulong chan)
{
	switch(chan){
	case GREY1:
	case GREY2:
	case GREY4:
	case GREY8:
	case RGB24:
		return 1;
	}
	return 0;
}

Image*
multichan(Image *i)
{
	Image *ni;

	if(notrans(i->chan))
		return i;

	ni = allocimage(display, i->r, RGB24, 0, DNofill);
	if(ni == nil)
		return ni;
	draw(ni, ni->r, i, nil, i->r.min);
	return ni;
}

Memimage*
memmultichan(Memimage *i)
{
	Memimage *ni;

	if(notrans(i->chan))
		return i;

	ni = allocmemimage(i->r, RGB24);
	if(ni == nil)
		return ni;
	memimagedraw(ni, ni->r, i, i->r.min, nil, i->r.min, S);
	return ni;
}
