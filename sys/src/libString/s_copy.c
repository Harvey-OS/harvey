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


/* return a String containing a copy of the passed char array */
extern String*
s_copy(char *cp)
{
	String *sp;
	int len;

	len = strlen(cp)+1;
	sp = s_newalloc(len);
	setmalloctag(sp, getcallerpc(&cp));
	strcpy(sp->base, cp);
	sp->ptr = sp->base + len - 1;		/* point to 0 terminator */
	return sp;
}
