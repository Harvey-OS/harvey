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
#include <venti.h>

int
vtscorefmt(Fmt *f)
{
	uint8_t *v;
	int i;

	v = va_arg(f->args, uint8_t*);
	if(v == nil)
		fmtprint(f, "*");
	else
		for(i = 0; i < VtScoreSize; i++)
			fmtprint(f, "%2.2x", v[i]);
	return 0;
}
