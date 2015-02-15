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
#include <authsrv.h>
#include <bio.h>
#include "authcmdlib.h"

static int
getkey(char *authkey)
{
	Nvrsafe safe;

	if(readnvram(&safe, 0) < 0)
		return -1;
	memmove(authkey, safe.machkey, DESKEYLEN);
	memset(&safe, 0, sizeof safe);
	return 0;
}

int
getauthkey(char *authkey)
{
	if(getkey(authkey) == 0)
		return 1;
	print("can't read NVRAM, please enter machine key\n");
	getpass(authkey, nil, 0, 1);
	return 1;
}
