#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include <thread.h>
#include "9p.h"

void*
emalloc9p(ulong sz)
{
	void *v;

	if((v = malloc(sz)) == nil) {
		fprint(2, "out of memory allocating %lud\n", sz);
		exits("mem");
	}
	memset(v, 0, sz);
	setmalloctag(v, getcallerpc(&sz));
	return v;
}

void*
erealloc9p(void *v, ulong sz)
{
	if((v = realloc(v, sz)) == nil) {
		fprint(2, "out of memory allocating %lud\n", sz);
		exits("mem");
	}
	setrealloctag(v, getcallerpc(&v));
	return v;
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

