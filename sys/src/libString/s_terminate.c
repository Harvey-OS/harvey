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
#include "String.h"

void
s_terminate(String *s)
{
	if(s->ref > 1)
		sysfatal("can't s_terminate a shared string");
	if (s->ptr >= s->end)
		s_grow(s, 1);
	*s->ptr = 0;
}
