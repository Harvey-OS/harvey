#include <u.h>
#include <libc.h>
#include <thread.h>
#include "usb.h"

void *
emalloc(ulong size)
{
	void *x;

	x = malloc(size);
	if(x == nil)
		sysfatal("malloc: %r");
	return x;
}

void *
emallocz(ulong size, int zero)
{
	void *x;

	x = malloc(size);
	if(x == nil)
		sysfatal("malloc: %r");
	if(zero)
		memset(x, 0, size);
	return x;
}
