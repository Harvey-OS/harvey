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

int
loadimage(Image *i, Rectangle r, uint8_t *data, int ndata)
{
	int32_t dy;
	int n, bpl;
	uint8_t *a;
	int chunk;

	chunk = i->display->bufsize - 64;

	if(!rectinrect(r, i->r)) {
		werrstr("loadimage: bad rectangle");
		return -1;
	}
	bpl = bytesperline(r, i->depth);
	n = bpl * Dy(r);
	if(n > ndata) {
		werrstr("loadimage: insufficient data");
		return -1;
	}
	ndata = 0;
	while(r.max.y > r.min.y) {
		dy = r.max.y - r.min.y;
		if(dy * bpl > chunk)
			dy = chunk / bpl;
		if(dy <= 0) {
			werrstr("loadimage: image too wide for buffer");
			return -1;
		}
		n = dy * bpl;
		a = bufimage(i->display, 21 + n);
		if(a == nil) {
			werrstr("bufimage failed");
			return -1;
		}
		a[0] = 'y';
		BPLONG(a + 1, i->id);
		BPLONG(a + 5, r.min.x);
		BPLONG(a + 9, r.min.y);
		BPLONG(a + 13, r.max.x);
		BPLONG(a + 17, r.min.y + dy);
		memmove(a + 21, data, n);
		ndata += n;
		data += n;
		r.min.y += dy;
	}
	if(flushimage(i->display, 0) < 0)
		return -1;
	return ndata;
}
