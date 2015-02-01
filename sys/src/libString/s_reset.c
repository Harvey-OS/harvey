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

String*
s_reset(String *s)
{
	if(s != nil){
		s = s_unique(s);
		s->ptr = s->base;
		*s->ptr = '\0';
	} else
		s = s_new();
	return s;
}

String*
s_restart(String *s)
{
	s = s_unique(s);
	s->ptr = s->base;
	return s;
}
