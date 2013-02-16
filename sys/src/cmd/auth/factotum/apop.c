/*
 * APOP, CRAM - MD5 challenge/response authentication
 *
 * The client does not authenticate the server, hence no CAI
 *
 * Client protocol:
 *	write challenge: randomstring@domain
 *	read response: 2*MD5dlen hex digits
 *
 * Server protocol:
 *	read challenge: randomstring@domain
 *	write user: user
 *	write response: 2*MD5dlen hex digits
 */

#include "dat.h"

struct State
{
	int asfd;
	int astype;
	Key *key;
	Ticket	t;
	Ticketreq	tr;
	char chal[128];
	char	resp[64];
	char *user;
};

enum
{
	CNeedChal,
	CHaveResp,

	SHaveChal,
	SNeedUser,
	SNeedResp,

	Maxphase,
};

static char *phasenames[Maxphase] = {
[CNeedChal]	"CNeedChal",
[CHaveResp]	"CHaveResp",

[SHaveChal]	"SHaveChal",
[SNeedUser]	"SNeedUser",
[SNeedResp]	"SNeedResp",
};

static int dochal(State*);
static int doreply(State*, char*, char*);

static int
apopinit(Proto *p, Fsstate *fss)
{
	int iscli, ret;
	State *s;

	if((iscli = isclient(_strfindattr(fss->attr, "role"))) < 0)
		return failure(fss, nil);

	s = emalloc(sizeof *s);
	fss->phasename = phasenames;
	fss->maxphase = Maxphase;
	s->asfd = -1;
	if(p == &apop)
		s->astype = AuthApop;
	else if(p == &cram)
		s->astype = AuthCram;
	else
		abort();

	if(iscli)
		fss->phase = CNeedChal;
	else{
		if((ret = findp9authkey(&s->key, fss)) != RpcOk){
			free(s);
			return ret;
		}
		if(dochal(s) < 0){
			free(s);
			return failure(fss, nil);
		}
		fss->phase = SHaveChal;
	}
	fss->ps = s;
	return RpcOk;
}

static int
apopwrite(Fsstate *fss, void *va, uint n)
{
	char *a, *v;
	int i, ret;
	uchar digest[MD5dlen];
	DigestState *ds;
	Key *k;
	State *s;
	Keyinfo ki;

	s = fss->ps;
	a = va;
	switch(fss->phase){
	default:
		return phaseerror(fss, "write");

	case CNeedChal:
		ret = findkey(&k, mkkeyinfo(&ki, fss, nil), "%s", fss->proto->keyprompt);
		if(ret != RpcOk)
			return ret;
		v = _strfindattr(k->privattr, "!password");
		if(v == nil)
			return failure(fss, "key has no password");
		setattrs(fss->attr, k->attr);
		switch(s->astype){
		default:
			abort();
		case AuthCram:
			hmac_md5((uchar*)a, n, (uchar*)v, strlen(v),
				digest, nil);
			snprint(s->resp, sizeof s->resp, "%.*H", MD5dlen, digest);
			break;
		case AuthApop:
			ds = md5((uchar*)a, n, nil, nil);
			md5((uchar*)v, strlen(v), digest, ds);
			for(i=0; i<MD5dlen; i++)
				sprint(&s->resp[2*i], "%2.2x", digest[i]);
			break;
		}
		closekey(k);
		fss->phase = CHaveResp;
		return RpcOk;
	
	case SNeedUser:
		if((v = _strfindattr(fss->attr, "user")) && strcmp(v, a) != 0)
			return failure(fss, "bad user");
		fss->attr = setattr(fss->attr, "user=%q", a);
		s->user = estrdup(a);
		fss->phase = SNeedResp;
		return RpcOk;

	case SNeedResp:
		if(n != 2*MD5dlen)
			return failure(fss, "response not MD5 digest");
		if(doreply(s, s->user, a) < 0){
			fss->phase = SNeedUser;
			return failure(fss, nil);
		}
		fss->haveai = 1;
		fss->ai.cuid = s->t.cuid;
		fss->ai.suid = s->t.suid;
		fss->ai.nsecret = 0;
		fss->ai.secret = nil;
		fss->phase = Established;
		return RpcOk;
	}
}

static int
apopread(Fsstate *fss, void *va, uint *n)
{
	State *s;

	s = fss->ps;
	switch(fss->phase){
	default:
		return phaseerror(fss, "read");

	case CHaveResp:
		if(*n > strlen(s->resp))
			*n = strlen(s->resp);
		memmove(va, s->resp, *n);
		fss->phase = Established;
		fss->haveai = 0;
		return RpcOk;

	case SHaveChal:
		if(*n > strlen(s->chal))
			*n = strlen(s->chal);
		memmove(va, s->chal, *n);
		fss->phase = SNeedUser;
		return RpcOk;
	}
}

static void
apopclose(Fsstate *fss)
{
	State *s;

	s = fss->ps;
	if(s->asfd >= 0){
		close(s->asfd);
		s->asfd = -1;
	}
	if(s->key != nil){
		closekey(s->key);
		s->key = nil;
	}
	if(s->user != nil){
		free(s->user);
		s->user = nil;
	}
	free(s);
}

static int
dochal(State *s)
{
	char *dom, *user, trbuf[TICKREQLEN];

	s->asfd = -1;

	/* send request to authentication server and get challenge */
	/* send request to authentication server and get challenge */
	if((dom = _strfindattr(s->key->attr, "dom")) == nil
	|| (user = _strfindattr(s->key->attr, "user")) == nil){
		werrstr("apop/dochal cannot happen");
		goto err;
	}

	s->asfd = _authdial(nil, dom);

	/* could generate our own challenge on error here */
	if(s->asfd < 0)
		goto err;

	memset(&s->tr, 0, sizeof(s->tr));
	s->tr.type = s->astype;
	safecpy(s->tr.authdom, dom, sizeof s->tr.authdom);
	safecpy(s->tr.hostid, user, sizeof(s->tr.hostid));
	convTR2M(&s->tr, trbuf);

	if(write(s->asfd, trbuf, TICKREQLEN) != TICKREQLEN)
		goto err;
	if(_asrdresp(s->asfd, s->chal, sizeof s->chal) <= 5)
		goto err;
	return 0;

err:
	if(s->asfd >= 0)
		close(s->asfd);
	s->asfd = -1;
	return -1;
}

static int
doreply(State *s, char *user, char *response)
{
	char ticket[TICKETLEN+AUTHENTLEN];
	char trbuf[TICKREQLEN];
	int n;
	Authenticator a;

	memrandom(s->tr.chal, CHALLEN);
	safecpy(s->tr.uid, user, sizeof(s->tr.uid));
	convTR2M(&s->tr, trbuf);
	if((n=write(s->asfd, trbuf, TICKREQLEN)) != TICKREQLEN){
		if(n >= 0)
			werrstr("short write to auth server");
		goto err;
	}
	/* send response to auth server */
	if(strlen(response) != MD5dlen*2){
		werrstr("response not MD5 digest");
		goto err;
	}
	if((n=write(s->asfd, response, MD5dlen*2)) != MD5dlen*2){
		if(n >= 0)
			werrstr("short write to auth server");
		goto err;
	}
	if(_asrdresp(s->asfd, ticket, TICKETLEN+AUTHENTLEN) < 0){
		/* leave connection open so we can try again */
		return -1;
	}
	close(s->asfd);
	s->asfd = -1;

	convM2T(ticket, &s->t, (char*)s->key->priv);
	if(s->t.num != AuthTs
	|| memcmp(s->t.chal, s->tr.chal, sizeof(s->t.chal)) != 0){
		if(s->key->successes == 0)
			disablekey(s->key);
		werrstr(Easproto);
		goto err;
	}
	s->key->successes++;
	convM2A(ticket+TICKETLEN, &a, s->t.key);
	if(a.num != AuthAc
	|| memcmp(a.chal, s->tr.chal, sizeof(a.chal)) != 0
	|| a.id != 0){
		werrstr(Easproto);
		goto err;
	}

	return 0;
err:
	if(s->asfd >= 0)
		close(s->asfd);
	s->asfd = -1;
	return -1;
}

Proto apop = {
.name=	"apop",
.init=		apopinit,
.write=	apopwrite,
.read=	apopread,
.close=	apopclose,
.addkey=	replacekey,
.keyprompt=	"!password?"
};

Proto cram = {
.name=	"cram",
.init=		apopinit,
.write=	apopwrite,
.read=	apopread,
.close=	apopclose,
.addkey=	replacekey,
.keyprompt=	"!password?"
};
