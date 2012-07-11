#include <u.h>
#include <libc.h>
#include "fmtdef.h"


/*
 * format a string into the output buffer
 * designed for formats which themselves call fmt
 */
int
fmtprint(Fmt *f, char *fmt, ...)
{
	va_list va;
	int n;

	va_start(va, fmt);
	n = fmtvprint(f, fmt, va);
	va_end(va);
	return n;
}

