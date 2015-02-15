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
#include <libsec.h>

#include "iso9660.h"

Rune*
strtorune(Rune *r, char *s)
{
	Rune *or;

	if(s == nil)
		return nil;

	or = r;
	while(*s)
		s += chartorune(r++, s);
	*r = L'\0';
	return or;
}

Rune*
runechr(Rune *s, Rune c)
{
	for(; *s; s++)
		if(*s == c)
			return s;
	return nil;
}

int
runecmp(Rune *s, Rune *t)
{
	while(*s && *t && *s == *t)
		s++, t++;
	return *s - *t;
}

