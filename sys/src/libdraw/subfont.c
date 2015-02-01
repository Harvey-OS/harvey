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

Subfont*
allocsubfont(char *name, int n, int height, int ascent, Fontchar *info, Image *i)
{
	Subfont *f;

	assert(height != 0 /* allocsubfont */);

	f = malloc(sizeof(Subfont));
	if(f == 0)
		return 0;
	f->n = n;
	f->height = height;
	f->ascent = ascent;
	f->info = info;
	f->bits = i;
	f->ref = 1;
	if(name){
		f->name = strdup(name);
		if(lookupsubfont(i->display, name) == 0)
			installsubfont(name, f);
	}else
		f->name = 0;
	return f;
}
