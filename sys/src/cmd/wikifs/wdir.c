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
#include <bio.h>
#include <String.h>
#include <thread.h>
#include "wiki.h"

/* open, create relative to wiki dir */
int8_t *wikidir;

static int8_t*
wname(int8_t *s)
{
	int8_t *t;

	t = emalloc(strlen(wikidir)+1+strlen(s)+1);
	strcpy(t, wikidir);
	strcat(t, "/");
	strcat(t, s);
	return t;
}

int
wopen(int8_t *fn, int mode)
{
	int rv;

	fn = wname(fn);
	rv = open(fn, mode);
	free(fn);
	return rv;
}

int
wcreate(int8_t *fn, int mode, int32_t perm)
{
	int rv;

	fn = wname(fn);
	rv = create(fn, mode, perm);
	free(fn);
	return rv;
}

Biobuf*
wBopen(int8_t *fn, int mode)
{
	Biobuf *rv;

	fn = wname(fn);
	rv = Bopen(fn, mode);
	free(fn);
	return rv;
}

int
waccess(int8_t *fn, int mode)
{
	int rv;

	fn = wname(fn);
	rv = access(fn, mode);
	free(fn);
	return rv;
}

Dir*
wdirstat(int8_t *fn)
{
	Dir *d;

	fn = wname(fn);
	d = dirstat(fn);
	free(fn);
	return d;
}
