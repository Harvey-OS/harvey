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
#include <draw.h>
#include <html.h>
#include "dat.h"

void*
emalloc(uint32_t n)
{
	void* p;

	p = malloc(n);
	if(p == nil)
		error("can't malloc: %r");
	memset(p, 0, n);
	return p;
}

void*
erealloc(void* p, uint32_t n)
{
	p = realloc(p, n);
	if(p == nil)
		error("can't malloc: %r");
	return p;
}

char*
estrdup(char* s)
{
	char* t;

	t = emalloc(strlen(s) + 1);
	strcpy(t, s);
	return t;
}

char*
estrstrdup(char* s, char* t)
{
	int32_t ns, nt;
	char* u;

	ns = strlen(s);
	nt = strlen(t);
	/* use malloc to avoid memset */
	u = malloc(ns + nt + 1);
	if(u == nil)
		error("can't malloc: %r");
	memmove(u, s, ns);
	memmove(u + ns, t, nt);
	u[ns + nt] = '\0';
	return u;
}

char*
eappend(char* s, char* sep, char* t)
{
	int32_t ns, nsep, nt;
	char* u;

	if(t == nil)
		u = estrstrdup(s, sep);
	else {
		ns = strlen(s);
		nsep = strlen(sep);
		nt = strlen(t);
		/* use malloc to avoid memset */
		u = malloc(ns + nsep + nt + 1);
		if(u == nil)
			error("can't malloc: %r");
		memmove(u, s, ns);
		memmove(u + ns, sep, nsep);
		memmove(u + ns + nsep, t, nt);
		u[ns + nsep + nt] = '\0';
	}
	free(s);
	return u;
}

char*
egrow(char* s, char* sep, char* t)
{
	s = eappend(s, sep, t);
	free(t);
	return s;
}

void
error(char* fmt, ...)
{
	va_list arg;
	char buf[256];
	Fmt f;

	fmtfdinit(&f, 2, buf, sizeof buf);
	fmtprint(&f, "Mail: ");
	va_start(arg, fmt);
	fmtvprint(&f, fmt, arg);
	va_end(arg);
	fmtprint(&f, "\n");
	fmtfdflush(&f);
	exits(fmt);
}

void
growbytes(Bytes* b, char* s, int32_t ns)
{
	if(b->nalloc < b->n + ns + 1) {
		b->nalloc = b->n + ns + 8000;
		/* use realloc to avoid memset */
		b->b = realloc(b->b, b->nalloc);
		if(b->b == nil)
			error("growbytes: can't realloc: %r");
	}
	memmove(b->b + b->n, s, ns);
	b->n += ns;
	b->b[b->n] = '\0';
}
