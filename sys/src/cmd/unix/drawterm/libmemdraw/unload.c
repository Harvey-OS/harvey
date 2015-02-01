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

int
unloadmemimage(Memimage *i, Rectangle r, uchar *data, int ndata)
{
	int y, l;
	uchar *q;

	if(!rectinrect(r, i->r))
		return -1;
	l = bytesperline(r, i->depth);
	if(ndata < l*Dy(r))
		return -1;
	ndata = l*Dy(r);
	q = byteaddr(i, r.min);
	for(y=r.min.y; y<r.max.y; y++){
		memmove(data, q, l);
		q += i->width*sizeof(ulong);
		data += l;
	}
	return ndata;
}
