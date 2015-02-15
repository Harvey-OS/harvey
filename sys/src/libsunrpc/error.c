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
#include <thread.h>
#include <sunrpc.h>

static struct {
	SunStatus status;
	char *msg;
} tab[] = {
	SunProgUnavail,	"program unavailable",
	SunProgMismatch,	"program mismatch",
	SunProcUnavail,	"procedure unavailable",
	SunGarbageArgs,	"garbage args",
	SunSystemErr,		"system error",
	SunRpcMismatch,	"rpc mismatch",
	SunAuthBadCred,	"bad auth cred",
	SunAuthRejectedCred,	"rejected auth cred",
	SunAuthBadVerf,	"bad auth verf",
	SunAuthRejectedVerf,	"rejected auth verf",
	SunAuthTooWeak,	"auth too weak",
	SunAuthInvalidResp,	"invalid auth response",
	SunAuthFailed,		"auth failed",
};

void
sunErrstr(SunStatus status)
{
	int i;

	for(i=0; i<nelem(tab); i++){
		if(tab[i].status == status){
			werrstr(tab[i].msg);
			return;
		}
	}
	werrstr("unknown sun error %d", (int)status);
}
