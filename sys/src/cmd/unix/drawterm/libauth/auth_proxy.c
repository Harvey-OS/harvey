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
#include <fcall.h>
#include <auth.h>
#include "authlocal.h"

enum { ARgiveup = 100,
};

static uint8_t*
gstring(uint8_t* p, uint8_t* ep, char** s)
{
	uint n;

	if(p == nil)
		return nil;
	if(p + BIT16SZ > ep)
		return nil;
	n = GBIT16(p);
	p += BIT16SZ;
	if(p + n > ep)
		return nil;
	*s = malloc(n + 1);
	memmove((*s), p, n);
	(*s)[n] = '\0';
	p += n;
	return p;
}

static uint8_t*
gcarray(uint8_t* p, uint8_t* ep, uint8_t** s, int* np)
{
	uint n;

	if(p == nil)
		return nil;
	if(p + BIT16SZ > ep)
		return nil;
	n = GBIT16(p);
	p += BIT16SZ;
	if(p + n > ep)
		return nil;
	*s = malloc(n);
	if(*s == nil)
		return nil;
	memmove((*s), p, n);
	*np = n;
	p += n;
	return p;
}

void
auth_freeAI(AuthInfo* ai)
{
	if(ai == nil)
		return;
	free(ai->cuid);
	free(ai->suid);
	free(ai->cap);
	free(ai->secret);
	free(ai);
}

static uint8_t*
convM2AI(uint8_t* p, int n, AuthInfo** aip)
{
	uint8_t* e = p + n;
	AuthInfo* ai;

	ai = mallocz(sizeof(*ai), 1);
	if(ai == nil)
		return nil;

	p = gstring(p, e, &ai->cuid);
	p = gstring(p, e, &ai->suid);
	p = gstring(p, e, &ai->cap);
	p = gcarray(p, e, &ai->secret, &ai->nsecret);
	if(p == nil)
		auth_freeAI(ai);
	else
		*aip = ai;
	return p;
}

AuthInfo*
auth_getinfo(AuthRpc* rpc)
{
	AuthInfo* a;

	if(auth_rpc(rpc, "authinfo", nil, 0) != ARok)
		return nil;
	a = nil;
	if(convM2AI((uint8_t*)rpc->arg, rpc->narg, &a) == nil) {
		werrstr("bad auth info from factotum");
		return nil;
	}
	return a;
}

static int
dorpc(AuthRpc* rpc, char* verb, char* val, int len, AuthGetkey* getkey)
{
	int ret;

	for(;;) {
		if((ret = auth_rpc(rpc, verb, val, len)) != ARneedkey &&
		   ret != ARbadkey)
			return ret;
		if(getkey == 0)
			return ARgiveup; /* don't know how */
		if((*getkey)(rpc->arg) < 0)
			return ARgiveup; /* user punted */
	}
}

/*
 *  this just proxies what the factotum tells it to.
 */
AuthInfo*
fauth_proxy(int fd, AuthRpc* rpc, AuthGetkey* getkey, char* params)
{
	char* buf;
	int m, n, ret;
	AuthInfo* a;
	char oerr[ERRMAX];

	rerrstr(oerr, sizeof oerr);
	werrstr("UNKNOWN AUTH ERROR");

	if(dorpc(rpc, "start", params, strlen(params), getkey) != ARok) {
		werrstr("fauth_proxy start: %r");
		return nil;
	}

	buf = malloc(AuthRpcMax);
	if(buf == nil)
		return nil;
	for(;;) {
		switch(dorpc(rpc, "read", nil, 0, getkey)) {
		case ARdone:
			free(buf);
			a = auth_getinfo(rpc);
			errstr(oerr,
			       sizeof oerr); /* no error, restore whatever was
			                        there */
			return a;
		case ARok:
			if(write(fd, rpc->arg, rpc->narg) != rpc->narg) {
				werrstr("auth_proxy write fd: %r");
				goto Error;
			}
			break;
		case ARphase:
			n = 0;
			memset(buf, 0, AuthRpcMax);
			while((ret = dorpc(rpc, "write", buf, n, getkey)) ==
			      ARtoosmall) {
				if(atoi(rpc->arg) > AuthRpcMax)
					break;
				m = read(fd, buf + n, atoi(rpc->arg) - n);
				if(m <= 0) {
					if(m == 0)
						werrstr(
						    "auth_proxy short read: %s",
						    buf);
					goto Error;
				}
				n += m;
			}
			if(ret != ARok) {
				werrstr("auth_proxy rpc write: %s: %r", buf);
				goto Error;
			}
			break;
		default:
			werrstr("auth_proxy rpc: %r");
			goto Error;
		}
	}
Error:
	free(buf);
	return nil;
}

AuthInfo*
auth_proxy(int fd, AuthGetkey* getkey, char* fmt, ...)
{
	int afd;
	char* p;
	va_list arg;
	AuthInfo* ai;
	AuthRpc* rpc;

	quotefmtinstall(); /* just in case */
	va_start(arg, fmt);
	p = vsmprint(fmt, arg);
	va_end(arg);

	afd = open("/mnt/factotum/rpc", ORDWR);
	if(afd < 0) {
		werrstr("opening /mnt/factotum/rpc: %r");
		free(p);
		return nil;
	}

	rpc = auth_allocrpc(afd);
	if(rpc == nil) {
		free(p);
		return nil;
	}

	ai = fauth_proxy(fd, rpc, getkey, p);
	free(p);
	auth_freerpc(rpc);
	close(afd);
	return ai;
}
