/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "all.h"

/* this is all stubbed out; the NFS authentication stuff is now disabled - rob */

Xfid *
xfauth(Xfile *, String *)
{
	return 0;
}

int32_t
xfauthread(Xfid *xf, int32_t, uint8_t *, int32_t)
{

	chat("xfauthread %s...", xf->uid);
	return 0;
}

int32_t
xfauthwrite(Xfid *xf, int32_t, uint8_t *, int32_t)
{
	chat("xfauthwrite %s...", xf->uid);
	return 0;
}

int
xfauthremove(Xfid *, int8_t *)
{
	chat("authremove...");
	return -1;
}
