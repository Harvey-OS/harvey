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
#include <bio.h>

static ulong
getval(char **p)
{
	ulong v;
	char *q;

	v = strtoul(*p, &q, 0);
	v |= v<<8;
	v |= v<<16;
	*p = q;
	return v;
}

void
readcolmap(Display *d, RGB *colmap)
{
	int i;
	char *p, *q;
	Biobuf *b;
	char buf[128];

	USED(screen);

	sprint(buf, "/dev/draw/%d/colormap", d->dirno);
	b = Bopen(buf, OREAD);
	if(b == 0)
		drawerror(d, "rdcolmap: can't open colormap device");

	for(;;) {
		p = Brdline(b, '\n');
		if(p == 0)
			break;
		i = strtoul(p, &q, 0);
		if(i < 0 || i > 255) {
			fprint(2, "rdcolmap: bad index\n");
			exits("bad");
		}
		p = q;
		colmap[255-i].red = getval(&p);
		colmap[255-i].green = getval(&p);
		colmap[255-i].blue = getval(&p);
	}
	Bterm(b);
}
