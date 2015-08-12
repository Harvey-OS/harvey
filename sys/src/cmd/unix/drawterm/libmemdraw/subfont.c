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

Memsubfont*
allocmemsubfont(char* name, int n, int height, int ascent, Fontchar* info,
                Memimage* i)
{
	Memsubfont* f;

	f = malloc(sizeof(Memsubfont));
	if(f == 0)
		return 0;
	f->n = n;
	f->height = height;
	f->ascent = ascent;
	f->info = info;
	f->bits = i;
	if(name)
		f->name = strdup(name);
	else
		f->name = 0;
	return f;
}

void
freememsubfont(Memsubfont* f)
{
	if(f == 0)
		return;
	free(f->info); /* note: f->info must have been malloc'ed! */
	freememimage(f->bits);
	free(f);
}
