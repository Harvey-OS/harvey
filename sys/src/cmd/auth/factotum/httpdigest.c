/*
 * HTTPDIGEST - MD5 challenge/response authentication (RFC 2617)
 *
 * Client protocol:
 *	write challenge: nonce method uri 
 *	read response: 2*MD5dlen hex digits
 *
 * Server protocol:
 *	unimplemented
 */
#include "dat.h"

enum
{
	CNeedChal,
	CHaveResp,

	Maxphase,
};

static char *phasenames[Maxphase] = {
[CNeedChal]	"CNeedChal",
[CHaveResp]	"CHaveResp",
};

struct State
{
	char resp[MD5dlen*2+1];
};

static int
hdinit(Proto *p, Fsstate *fss)
{
	int iscli;
	State *s;

	if((iscli = isclient(_strfindattr(fss->attr, "role"))) < 0)
		return failure(fss, nil);
	if(!iscli)
		return failure(fss, "%s server not supported", p->name);

	s = emalloc(sizeof *s);
	fss->phasename = phasenames;
	fss->maxphase = Maxphase;
	fss->phase = CNeedChal;
	fss->ps = s;
	return RpcOk;
}

static void
strtolower(char *s)
{
	while(*s){
		*s = tolower(*s);
		s++;
	}
}

static void
digest(char *user, char *realm, char *passwd,
	char *nonce, char *method, char *uri,
	char *dig)
{
	uchar b[MD5dlen];
	char ha1[MD5dlen*2+1];
	char ha2[MD5dlen*2+1];
	DigestState *s;

	/*
	 *  H(A1) = MD5(uid + ":" + realm ":" + passwd)
	 */
	s = md5((uchar*)user, strlen(user), nil, nil);
	md5((uchar*)":", 1, nil, s);
	md5((uchar*)realm, strlen(realm), nil, s);
	md5((uchar*)":", 1, nil, s);
	md5((uchar*)passwd, strlen(passwd), b, s);
	enc16(ha1, sizeof(ha1), b, MD5dlen);
	strtolower(ha1);

	/*
	 *  H(A2) = MD5(method + ":" + uri)
	 */
	s = md5((uchar*)method, strlen(method), nil, nil);
	md5((uchar*)":", 1, nil, s);
	md5((uchar*)uri, strlen(uri), b, s);
	enc16(ha2, sizeof(ha2), b, MD5dlen);
	strtolower(ha2);

	/*
	 *  digest = MD5(H(A1) + ":" + nonce + ":" + H(A2))
	 */
	s = md5((uchar*)ha1, MD5dlen*2, nil, nil);
	md5((uchar*)":", 1, nil, s);
	md5((uchar*)nonce, strlen(nonce), nil, s);
	md5((uchar*)":", 1, nil, s);
	md5((uchar*)ha2, MD5dlen*2, b, s);
	enc16(dig, MD5dlen*2+1, b, MD5dlen);
	strtolower(dig);
}

static int
hdwrite(Fsstate *fss, void *va, uint n)
{
	State *s;
	int ret;
	char *a, *p, *r, *u, *t;
	char *tok[4];
	Key *k;
	Keyinfo ki;
	Attr *attr;

	s = fss->ps;
	a = va;

	if(fss->phase != CNeedChal)
		return phaseerror(fss, "write");

	attr = _delattr(_copyattr(fss->attr), "role");
	mkkeyinfo(&ki, fss, attr);
	ret = findkey(&k, &ki, "%s", fss->proto->keyprompt);
	_freeattr(attr);
	if(ret != RpcOk)
		return ret;
	p = _strfindattr(k->privattr, "!password");
	if(p == nil)
		return failure(fss, "key has no password");
	r = _strfindattr(k->attr, "realm");
	if(r == nil)
		return failure(fss, "key has no realm");
	u = _strfindattr(k->attr, "user");
	if(u == nil)
		return failure(fss, "key has no user");
	setattrs(fss->attr, k->attr);

	/* copy in case a is not null-terminated */
	t = emalloc(n+1);
	memcpy(t, a, n);
	t[n] = 0;

	/* get nonce, method, uri */
	if(tokenize(t, tok, 4) != 3)
		return failure(fss, "bad challenge");

	digest(u, r, p, tok[0], tok[1], tok[2], s->resp);

	free(t);
	closekey(k);
	fss->phase = CHaveResp;
	return RpcOk;
}

static int
hdread(Fsstate *fss, void *va, uint *n)
{
	State *s;

	s = fss->ps;
	if(fss->phase != CHaveResp)
		return phaseerror(fss, "read");
	if(*n > strlen(s->resp))
		*n = strlen(s->resp);
	memmove(va, s->resp, *n);
	fss->phase = Established;
	fss->haveai = 0;
	return RpcOk;
}

static void
hdclose(Fsstate *fss)
{
	State *s;
	s = fss->ps;
	free(s);
}

Proto httpdigest = {
.name=		"httpdigest",
.init=		hdinit,
.write=		hdwrite,
.read=		hdread,
.close=		hdclose,
.addkey=	replacekey,
.keyprompt=	"user? realm? !password?"
};
