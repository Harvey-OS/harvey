#include <u.h>
#include <libc.h>
#include <draw.h>
#include <plumb.h>
#include "faces.h"

void*
emalloc(ulong sz)
{
	void *v;
	v = malloc(sz);
	if(v == nil) {
		fprint(2, "out of memory allocating %ld\n", sz);
		exits("mem");
	}
	memset(v, 0, sz);
	return v;
}

void*
erealloc(void *v, ulong sz)
{
	v = realloc(v, sz);
	if(v == nil) {
		fprint(2, "out of memory allocating %ld\n", sz);
		exits("mem");
	}
	return v;
}

char*
estrdup(char *s)
{
	char *t;
	if((t = strdup(s)) == nil) {
		fprint(2, "out of memory in strdup(%.10s)\n", s);
		exits("mem");
	}
	return t;
}

Face*
faceunpack(Plumbmsg *m)
{
	char *p, *q;
	char *ep;
	int nstr;
	Face *f;

	if(m == nil)
		return nil;

	f = emalloc(sizeof *f);
	p = m->data;
	ep = p+m->ndata;

	for(nstr=0; nstr<Nstring; nstr++) {
		if((q = memchr(p, '\n', ep-p)) == nil)
			break;

		*q++ = 0;
		f->str[nstr] = estrdup(p);
		p = q;
	}

	if(nstr < Nstring)
		while(nstr < Nstring)
			f->str[nstr++] = estrdup("");

	f->ntimes = 1;
	return f;			
}

