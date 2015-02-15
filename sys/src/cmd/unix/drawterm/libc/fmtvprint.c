/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

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
	VA_COPY(va,f->args);
	VA_END(f->args);
	VA_COPY(f->args,args);
	n = dofmt(f, fmt);
	f->flags = 0;
	f->width = 0;
	f->prec = 0;
	VA_END(f->args);
	VA_COPY(f->args,va);
	VA_END(va);
	if(n >= 0)
		return 0;
	return n;
}

