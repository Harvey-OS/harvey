/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "common.h"
#include "send.h"

#define isspace(c) ((c)==' ' || (c)=='\t' || (c)=='\n')

/*
 *  Translate the last component of the sender address.  If the translation
 *  yields the same address, replace the sender with its last component.
 */
extern void
gateway(message *mp)
{
	char *base;
	String *s;

	/* first remove all systems equivalent to us */
	base = skipequiv(s_to_c(mp->sender));
	if(base != s_to_c(mp->sender)){
		s = mp->sender;
		mp->sender = s_copy(base);
		s_free(s);
	}
}
