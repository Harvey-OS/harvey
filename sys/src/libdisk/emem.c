#include <u.h>
#include <libc.h>
#include <disk.h>

void*
emalloc(long sz)
{
	void *v;

	v = malloc(sz);
	if(v == nil)
		sysfatal("malloc %lud fails\n", sz);
	memset(v, 0, sz);
	return v;
}

void*
erealloc(void *v, long sz)
{
	v = realloc(v, sz);
	if(v == nil)
		sysfatal("realloc %lud fails\n", sz);
	return v;
}

char*
estrdup(char *s)
{
	s = strdup(s);
	if(s == nil)
		sysfatal("strdup fails\n");
	return s;
}

