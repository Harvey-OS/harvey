#include <u.h>
#include <libc.h>
#include "httpd.h"

void*
ezalloc(ulong n)
{
	void *p;

	p = malloc(n);
	if(p == nil)
		sysfatal("out of memory");
	memset(p, 0, n);
	return p;
}

char*
estrdup(char *s)
{
	s = strdup(s);
	if(s == nil)
		sysfatal("out of memory");
	return s;
}

