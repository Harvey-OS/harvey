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
#include <mp.h>
#include <libsec.h>
#include <ctype.h>

#include "iso9660.h"

typedef struct Stringtab	Stringtab;
struct Stringtab {
	Stringtab *link;
	char *str;
};

static Stringtab *stab[1024];

static uint
hash(char *s)
{
	uint h;
	uint8_t *p;

	h = 0;
	for(p=(uint8_t*)s; *p; p++)
		h = h*37 + *p;
	return h;
}

static char*
estrdup(char *s)
{
	if((s = strdup(s)) == nil)
		sysfatal("strdup(%.10s): out of memory", s);
	return s;
}

char*
atom(char *str)
{
	uint h;
	Stringtab *tab;

	h = hash(str) % nelem(stab);
	for(tab=stab[h]; tab; tab=tab->link)
		if(strcmp(str, tab->str) == 0)
			return tab->str;

	tab = emalloc(sizeof *tab);
	tab->str = estrdup(str);
	tab->link = stab[h];
	stab[h] = tab;
	return tab->str;
}

void*
emalloc(uint32_t n)
{
	void *p;

	if((p = malloc(n)) == nil)
		sysfatal("malloc(%lu): out of memory", n);
	memset(p, 0, n);
	return p;
}

void*
erealloc(void *v, uint32_t n)
{
	if((v = realloc(v, n)) == nil)
		sysfatal("realloc(%p, %lu): out of memory", v, n);
	return v;
}

char*
struprcpy(char *p, char *s)
{
	char *op;

	op = p;
	for(; *s; s++)
		*p++ = toupper(*s);
	*p = '\0';

	return op;
}

int
chat(char *fmt, ...)
{
	va_list arg;

	if(!chatty)
		return 0;
	va_start(arg, fmt);
	vfprint(2, fmt, arg);
	va_end(arg);
	return 1;
}
