/*
 * p9cr, vnc - textual challenge/response authentication
 *
 * Client protocol:	[currently unimplemented]
 *	write challenge
 *	read response
 *
 * Server protocol:
 *	write user
 *	read challenge
 * 	write response
 */

#include "dat.h"

enum
{
	Maxchal=	64,
};

typedef struct State State;
struct State
{
	Key	*key;
	int	astype;
	int	asfd;
	Ticket	t;
	Ticketreq tr;
	char	chal[Maxchal];
	int	challen;
};

enum
{
	CNeedChal,
	CHaveResp,

	SHaveChal,
	SNeedResp,

	Maxphase,
};

static char *phasenames[Maxphase] =
{
[CNeedChal]	"CNeedChal",
[CHaveResp]	"CHaveResp",

[SHaveChal]	"SHaveChal",
[SNeedResp]	"SNeedResp",
};

static void
p9crclose(Fsstate *fss)
{
	State *s;

	s = fss->ps;
	if(s->asfd >= 0){
		close(s->asfd);
		s->asfd = -1;
	}
	free(s);
}

static int getchal(State*, Fsstate*);

static int
p9crinit(Proto *p, Fsstate *fss)
{
	int iscli, ret;
	char *user;
	State *s;

	if((iscli = isclient(_str_findattr(fss->attr, "role"))) < 0)
		return failure(fss, nil);
	if(iscli)
		return failure(fss, "%s client not implemented", p->name);
	
	s = emalloc(sizeof(*s));
	s->asfd = -1;
	if(p == &p9cr){
		s->astype = AuthChal;
		s->challen = NETCHLEN;
	}else if(p == &vnc){
		s->astype = AuthVNC;
		s->challen = Maxchal;
	}else
		abort();

	if((ret = findp9authkey(&s->key, fss)) != RpcOk){
		free(s);
		return ret;
	}
	fss->phasename = phasenames;
	fss->maxphase = Maxphase;
	if((user = _str_findattr(fss->attr, "user")) == nil){
		free(s);
		return failure(fss, "no user name specified in start msg");
	}
	if(strlen(user) >= sizeof s->tr.uid){
		free(s);
		return failure(fss, "user name too long");
	}
	fss->ps = s;
	strcpy(s->tr.uid, user);
	ret = getchal(s, fss);
	if(ret != RpcOk){
		p9crclose(fss);	/* frees s */
		fss->ps = nil;
	}
	return ret;
}

static int
p9crread(Fsstate *fss, void *va, uint *n)
{
	int m;
	State *s;

	s = fss->ps;
	switch(fss->phase){
	default:
		return phaseerror(fss, "read");

	case SHaveChal:
		if(s->astype == AuthChal)
			m = strlen(s->chal);	/* ascii string */
		else
			m = s->challen;		/* fixed length binary */
		if(m > *n)
			return toosmall(fss, m);
		*n = m;
		memmove(va, s->chal, m);
		fss->phase = SNeedResp;
		return RpcOk;
	}
}

static int
p9crwrite(Fsstate *fss, void *va, uint n)
{
	char tbuf[TICKETLEN+AUTHENTLEN];
	State *s;
	char *data = va;
	Authenticator a;
	char resp[Maxchal];

	s = fss->ps;
	switch(fss->phase){
	default:
		return phaseerror(fss, "write");

	case SNeedResp:
		/* send response to auth server and get ticket */
		if(n > sizeof(resp))
			return failure(fss, Ebadarg);
		memset(resp, 0, sizeof resp);
		memmove(resp, data, n);
		if(write(s->asfd, resp, s->challen) != s->challen)
			return failure(fss, Easproto);

		/* get ticket plus authenticator from auth server */
		if(_asrdresp(s->asfd, tbuf, TICKETLEN+AUTHENTLEN) < 0)
			return failure(fss, nil);

		/* check ticket */
		convM2T(tbuf, &s->t, s->key->priv);
		if(s->t.num != AuthTs
		|| memcmp(s->t.chal, s->tr.chal, sizeof(s->t.chal)) != 0)
			return failure(fss, Easproto);
		convM2A(tbuf+TICKETLEN, &a, s->t.key);
		if(a.num != AuthAc
		|| memcmp(a.chal, s->tr.chal, sizeof(a.chal)) != 0
		|| a.id != 0)
			return failure(fss, Easproto);

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
getchal(State *s, Fsstate *fss)
{
	char trbuf[TICKREQLEN];
	int n;
	safecpy(s->tr.hostid, _str_findattr(s->key->attr, "user"), sizeof(s->tr.hostid));
	safecpy(s->tr.authid, _str_findattr(s->key->attr, "user"), sizeof(s->tr.authid));
	safecpy(s->tr.authdom, _str_findattr(s->key->attr, "dom"), sizeof(s->tr.authdom));
	memrandom(s->tr.chal, sizeof s->tr.chal);
	s->tr.type = s->astype;
	convTR2M(&s->tr, trbuf);

	/* get challenge from auth server */
	s->asfd = _authdial(nil, _str_findattr(s->key->attr, "dom"));
	if(s->asfd < 0)
		return failure(fss, Easproto);
	if(write(s->asfd, trbuf, TICKREQLEN) != TICKREQLEN)
		return failure(fss, Easproto);
	n = _asrdresp(s->asfd, s->chal, s->challen);
	if(n <= 0){
		if(n == 0)
			werrstr("_asrdresp short read");
		return failure(fss, nil);
	}
	s->challen = n;
	fss->phase = SHaveChal;
	return RpcOk;
}

Proto p9cr =
{
.name=		"p9cr",
.init=		p9crinit,
.write=		p9crwrite,
.read=		p9crread,
.close=		p9crclose,
};

Proto vnc =
{
.name=		"vnc",
.init=		p9crinit,
.write=		p9crwrite,
.read=		p9crread,
.close=		p9crclose,
};
