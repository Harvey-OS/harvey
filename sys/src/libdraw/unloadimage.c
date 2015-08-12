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
unloadimage(Image* i, Rectangle r, uint8_t* data, int ndata)
{
	int bpl, n, ntot, dy;
	uint8_t* a;
	Display* d;

	if(!rectinrect(r, i->r)) {
		werrstr("unloadimage: bad rectangle");
		return -1;
	}
	bpl = bytesperline(r, i->depth);
	if(ndata < bpl * Dy(r)) {
		werrstr("unloadimage: buffer too small");
		return -1;
	}

	d = i->display;
	flushimage(d, 0); /* make sure subsequent flush is for us only */
	ntot = 0;
	while(r.min.y < r.max.y) {
		a = bufimage(d, 1 + 4 + 4 * 4);
		if(a == 0) {
			werrstr("unloadimage: %r");
			return -1;
		}
		dy = 8000 / bpl;
		if(dy <= 0) {
			werrstr("unloadimage: image too wide");
			return -1;
		}
		if(dy > Dy(r))
			dy = Dy(r);
		a[0] = 'r';
		BPLONG(a + 1, i->id);
		BPLONG(a + 5, r.min.x);
		BPLONG(a + 9, r.min.y);
		BPLONG(a + 13, r.max.x);
		BPLONG(a + 17, r.min.y + dy);
		if(flushimage(d, 0) < 0)
			return -1;
		n = read(d->fd, data + ntot, ndata - ntot);
		if(n < 0)
			return n;
		ntot += n;
		r.min.y += dy;
	}
	return ntot;
}
