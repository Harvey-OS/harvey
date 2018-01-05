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
#include <authsrv.h>
#include "authlocal.h"

/*
 * compute the proper response.  We encrypt the ascii of
 * challenge number, with trailing binary zero fill.
 * This process was derived empirically.
 * this was copied from inet's guard.
 */
static void
netresp(char *key, int32_t chal, char *answer)
{
	uint8_t buf[8];

	memset(buf, 0, sizeof buf);
	snprint((char *)buf, sizeof buf, "%lu", chal);
	if(encrypt(key, buf, 8) < 0)
		abort();
	sprint(answer, "%.8x", buf[0]<<24 | buf[1]<<16 | buf[2]<<8 | buf[3]);
}

AuthInfo*
auth_userpasswd(char *user, char *passwd)
{
	char key[DESKEYLEN], resp[16];
	AuthInfo *ai;
	Chalstate *ch;

	/*
	 * Probably we should have a factotum protocol
	 * to check a raw password.  For now, we use
	 * p9cr, which is simplest to speak.
	 */
	if((ch = auth_challenge("user=%q proto=p9cr role=server", user)) == nil)
		return nil;

	passtokey(key, passwd);
	netresp(key, atol(ch->chal), resp);
	memset(key, 0, sizeof key);

	ch->resp = resp;
	ch->nresp = strlen(resp);
	ai = auth_response(ch);
	auth_freechal(ch);
	return ai;
}
