#include <u.h>
#include <libc.h>
#include <bio.h>
#include "snap.h"

void*
emalloc(ulong n)
{
	void *v;
	v = malloc(n);
	if(v == nil){
		fprint(2, "out of memory\n");
		exits("memory");
	}
	memset(v, 0, n);
	return v;
}

void*
erealloc(void *v, ulong n)
{
	v = realloc(v, n);
	if(v == nil) {
		fprint(2, "out of memory\n");
		exits("memory");
	}
	return v;
}

char*
estrdup(char *s)
{
	s = strdup(s);
	if(s == nil) {
		fprint(2, "out of memory\n");
		exits("memory");
	}
	return s;
}
