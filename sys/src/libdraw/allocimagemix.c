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

Image*
allocimagemix(Display *d, ulong color1, ulong color3)
{
	Image *t, *b;
	static Image *qmask;

	if(qmask == nil)
		qmask = allocimage(d, Rect(0,0,1,1), GREY8, 1, 0x3F3F3FFF);
		
	if(d->screenimage->depth <= 8){	/* create a 2×2 texture */
		t = allocimage(d, Rect(0,0,1,1), d->screenimage->chan, 0, color1);
		if(t == nil)
			return nil;

		b = allocimage(d, Rect(0,0,2,2), d->screenimage->chan, 1, color3);
		if(b == nil){
			freeimage(t);
			return nil;
		}

		draw(b, Rect(0,0,1,1), t, nil, ZP);
		freeimage(t);
		return b;
	}else{	/* use a solid color, blended using alpha */
		t = allocimage(d, Rect(0,0,1,1), d->screenimage->chan, 1, color1);
		if(t == nil)
			return nil;

		b = allocimage(d, Rect(0,0,1,1), d->screenimage->chan, 1, color3);
		if(b == nil){
			freeimage(t);
			return nil;
		}

		draw(b, b->r, t, qmask, ZP);
		freeimage(t);
		return b;
	}
}
