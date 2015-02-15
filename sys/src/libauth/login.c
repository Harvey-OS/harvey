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
#include <auth.h>

int
login(char *user, char *password, char *namespace)
{
	int rv;
	AuthInfo *ai;

	if((ai = auth_userpasswd(user, password)) == nil)
		return -1;

	rv = auth_chuid(ai, namespace);
	auth_freeAI(ai);
	return rv;
}
