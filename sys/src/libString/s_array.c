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

extern String*	_s_alloc(void);

/* return a String containing a character array (this had better not grow) */
extern String *
s_array(char *cp, int len)
{
	String *sp = _s_alloc();

	sp->base = sp->ptr = cp;
	sp->end = sp->base + len;
	sp->fixed = 1;
	return sp;
}
