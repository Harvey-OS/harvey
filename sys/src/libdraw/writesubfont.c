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

static
void
packinfo(Fontchar *fc, uchar *p, int n)
{
	int j;

	for(j=0;  j<=n;  j++){
		p[0] = fc->x;
		p[1] = fc->x>>8;
		p[2] = fc->top;
		p[3] = fc->bottom;
		p[4] = fc->left;
		p[5] = fc->width;
		fc++;
		p += 6;
	}
}

int
writesubfont(int fd, Subfont *f)
{
	char hdr[3*12+1];
	uchar *data;
	int nb;

	sprint(hdr, "%11d %11d %11d ", f->n, f->height, f->ascent);
	if(write(fd, hdr, 3*12) != 3*12){
   Err:
		werrstr("writesubfont: bad write: %r");
		return -1;
	}
	nb = 6*(f->n+1);
	data = malloc(nb);
	if(data == nil)
		return -1;
	packinfo(f->info, data, f->n);
	if(write(fd, data, nb) != nb)
		goto Err;
	free(data);
	return 0;
}
