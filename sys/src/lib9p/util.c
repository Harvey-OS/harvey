#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include <thread.h>
#include "9p.h"
#include "impl.h"

void*
emalloc(ulong sz)
{
	void *v;

	if((v = malloc(sz)) == nil) {
		fprint(2, "out of memory allocating %lud\n", sz);
		exits("mem");
	}
	memset(v, 0, sz);
	return v;
}

void*
erealloc(void *v, ulong sz)
{
	if((v = realloc(v, sz)) == nil) {
		fprint(2, "out of memory allocating %lud\n", sz);
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


void
readbuf(vlong off, void *dst, long *dlen, void *src, long slen)
{
	if(off >= slen) {
		*dlen = 0;
		return;
	}
	if(off+*dlen > slen)
		*dlen = slen-off;
	memmove(dst, (char*)src+off, *dlen);
}
void
readstr(vlong off, void *dst, long *dlen, char *src)
{
	readbuf(off, dst, dlen, src, strlen(src));
}
