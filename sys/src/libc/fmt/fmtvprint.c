#include <u.h>
#include <libc.h>
#include "fmtdef.h"


/*
 * format a string into the output buffer
 * designed for formats which themselves call fmt,
 * but ignore any width flags
 */
int
fmtvprint(Fmt *f, char *fmt, va_list args)
{
	va_list va;
	int n;

	f->flags = 0;
	f->width = 0;
	f->prec = 0;
	va = f->args;
	f->args = args;
	n = dofmt(f, fmt);
	f->flags = 0;
	f->width = 0;
	f->prec = 0;
	f->args = va;
	if(n >= 0)
		return 0;
	return n;
}

