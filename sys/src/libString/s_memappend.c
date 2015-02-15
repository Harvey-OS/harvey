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

/* append a char array ( of up to n characters) to a String */
String *
s_memappend(String *to, char *from, int n)
{
	char *e;

	if (to == 0)
		to = s_new();
	if (from == 0)
		return to;
	for(e = from + n; from < e; from++)
		s_putc(to, *from);
	s_terminate(to);
	return to;
}

