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

enum {
	ARgiveup = 100,
};

static int
dorpc(AuthRpc *rpc, char *verb, char *val, int len, AuthGetkey *getkey)
{
	int ret;

	for(;;){
		if((ret = auth_rpc(rpc, verb, val, len)) != ARneedkey && ret != ARbadkey)
			return ret;
		if(getkey == nil)
			return ARgiveup;	/* don't know how */
		if((*getkey)(rpc->arg) < 0)
			return ARgiveup;	/* user punted */
	}
}

int
auth_respond(void *chal, uint nchal, char *user, uint nuser, void *resp, uint nresp, AuthGetkey *getkey, char *fmt, ...)
{
	char *p, *s;
	va_list arg;
	int afd;
	AuthRpc *rpc;
	Attr *a;

	if((afd = open("/mnt/factotum/rpc", ORDWR)) < 0)
		return -1;
	
	if((rpc = auth_allocrpc(afd)) == nil){
		close(afd);
		return -1;
	}

	quotefmtinstall();	/* just in case */
	va_start(arg, fmt);
	p = vsmprint(fmt, arg);
	va_end(arg);

	if(p==nil
	|| dorpc(rpc, "start", p, strlen(p), getkey) != ARok
	|| dorpc(rpc, "write", chal, nchal, getkey) != ARok
	|| dorpc(rpc, "read", nil, 0, getkey) != ARok){
		free(p);
		close(afd);
		auth_freerpc(rpc);
		return -1;
	}
	free(p);

	if(rpc->narg < nresp)
		nresp = rpc->narg;
	memmove(resp, rpc->arg, nresp);

	if((a = auth_attr(rpc)) != nil
	&& (s = _strfindattr(a, "user")) != nil && strlen(s) < nuser)
		strcpy(user, s);
	else if(nuser > 0)
		user[0] = '\0';

	_freeattr(a);
	close(afd);
	auth_freerpc(rpc);
	return nresp;	
}
