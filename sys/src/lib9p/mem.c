#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include <thread.h>
#include "9p.h"

static void
outofmem(vlong sz)
{
	fprint(2, "out of memory allocating %llud\n", sz);
	exits("mem");
}

void*
emalloc9p(uintptr sz)
{
	void *v;

	if((v = malloc(sz)) == nil)
		outofmem(sz);
	memset(v, 0, sz);
	setmalloctag(v, getcallerpc(&sz));
	return v;
}

void*
erealloc9p(void *v, uintptr sz)
{
	void *nv;

	if((nv = realloc(v, sz)) == nil)
		outofmem(sz);
	if(v == nil)
		setmalloctag(nv, getcallerpc(&v));
	setrealloctag(nv, getcallerpc(&v));
	return nv;
}

char*
estrdup9p(char *s)
{
	char *t;

	if((t = strdup(s)) == nil) {
		fprint(2, "out of memory in strdup(%.10s)\n", s);
		exits("mem");
	}
	setmalloctag(t, getcallerpc(&s));
	return t;
}
