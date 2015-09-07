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
#include <ndb.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include <ctype.h>
#include "dat.h"
#include "fns.h"

void*
erealloc(void *a, uint n)
{
	a = realloc(a, n);
	if(a == nil)
		sysfatal("realloc %d: out of memory", n);
	setrealloctag(a, getcallerpc());
	return a;
}

void*
emalloc(uint n)
{
	void *a;

	a = mallocz(n, 1);
	if(a == nil)
		sysfatal("malloc %d: out of memory", n);
	setmalloctag(a, getcallerpc());
	return a;
}

char*
estrdup(char *s)
{
	s = strdup(s);
	if(s == nil)
		sysfatal("strdup: out of memory");
	setmalloctag(s, getcallerpc());
	return s;
}

char*
estredup(char *s, char *e)
{
	char *t;

	t = emalloc(e-s+1);
	memmove(t, s, e-s);
	t[e-s] = '\0';
	setmalloctag(t, getcallerpc());
	return t;
}

char*
estrmanydup(char *s, ...)
{
	char *p, *t;
	int len;
	va_list arg;

	len = strlen(s);
	va_start(arg, s);
	while((p = va_arg(arg, char*)) != nil)
		len += strlen(p);
	len++;

	t = emalloc(len);
	strcpy(t, s);
	va_start(arg, s);
	while((p = va_arg(arg, char*)) != nil)
		strcat(t, p);
	return t;
}

char*
strlower(char *s)
{
	char *t;

	for(t=s; *t; t++)
		if('A' <= *t && *t <= 'Z')
			*t += 'a'-'A';
	return s;
}
