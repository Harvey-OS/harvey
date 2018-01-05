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
 * designed for formats which themselves call fmt
 */
int
fmtvprint(Fmt *f, const char *fmt, va_list args)
{
	va_list va;
	int n;

	//va = f->args;
	//f->args = args;
	va_copy(va,f->args);
        va_end(f->args);
        va_copy(f->args,args);
	n = dofmt(f, fmt);
	va_end(f->args);
        va_copy(f->args,va);
        va_end(va);
	//f->args = va;
	if(n >= 0)
		return 0;
	return n;
}

