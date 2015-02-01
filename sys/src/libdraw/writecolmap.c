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

/*
 * This code (and the devdraw interface) will have to change
 * if we ever get bitmaps with ldepth > 3, because the
 * colormap will have to be written in chunks
 */

void
writecolmap(Display *d, RGB *m)
{
	int i, n, fd;
	char buf[64], *t;
	ulong r, g, b;

	sprint(buf, "/dev/draw/%d/colormap", d->dirno);
	fd = open(buf, OWRITE);
	if(fd < 0)
		drawerror(d, "writecolmap: open colormap failed");
	t = malloc(8192);
	if (t == nil)
		drawerror(d, "writecolmap: no memory");
	n = 0;
	for(i = 0; i < 256; i++) {
		r = m[i].red>>24;
		g = m[i].green>>24;
		b = m[i].blue>>24;
		n += sprint(t+n, "%d %lud %lud %lud\n", 255-i, r, g, b);
	}
	i = write(fd, t, n);
	free(t);
	close(fd);
	if(i != n)
		drawerror(d, "writecolmap: bad write");
}
