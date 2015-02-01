/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "ssh.h"

static AuthInfo*
authsrvpasswordfn(Conn *c, Msg *m)
{
	char *pass;
	AuthInfo *ai;

	pass = getstring(m);
	ai = auth_userpasswd(c->user, pass);
	free(m);
	return ai;
}

Authsrv authsrvpassword =
{
	SSH_AUTH_PASSWORD,
	"password",
	SSH_CMSG_AUTH_PASSWORD,
	authsrvpasswordfn,
};

