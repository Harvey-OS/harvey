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
#include <memlayer.h>

int
memunload(Memimage *src, Rectangle r, uchar *data, int n)
{
	Memimage *tmp;
	Memlayer *dl;
	Rectangle lr;
	int dx;

    Top:
	dl = src->layer;
	if(dl == nil)
		return unloadmemimage(src, r, data, n);

	/*
 	 * Convert to screen coordinates.
	 */
	lr = r;
	r.min.x += dl->delta.x;
	r.min.y += dl->delta.y;
	r.max.x += dl->delta.x;
	r.max.y += dl->delta.y;
	dx = dl->delta.x&(7/src->depth);
	if(dl->clear && dx==0){
		src = dl->screen->image;
		goto Top;
	}

	/*
	 * src is an obscured layer or data is unaligned
	 */
	if(dl->save && dx==0){
		if(dl->refreshfn != nil)
			return -1;	/* can't unload window if it's not Refbackup */
		if(n > 0)
			memlhide(src, r);
		n = unloadmemimage(dl->save, lr, data, n);
		return n;
	}
	tmp = allocmemimage(lr, src->chan);
	if(tmp == nil)
		return -1;
	memdraw(tmp, lr, src, lr.min, nil, lr.min, S);
	n = unloadmemimage(tmp, lr, data, n);
	freememimage(tmp);
	return n;
}
