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
	char	resp[Maxchal];
	int	resplen;
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
	Attr *attr;
	Keyinfo ki;

	if((iscli = isclient(_strfindattr(fss->attr, "role"))) < 0)
		return failure(fss, nil);
	
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

	if(iscli){
		fss->phase = CNeedChal;
		if(p == &p9cr)
			attr = setattr(_copyattr(fss->attr), "proto=p9sk1");
		else
			attr = nil;
		ret = findkey(&s->key, mkkeyinfo(&ki, fss, attr),
			"role=client %s", p->keyprompt);
		_freeattr(attr);
		if(ret != RpcOk){
			free(s);
			return ret;
		}
		fss->ps = s;
	}else{
		if((ret = findp9authkey(&s->key, fss)) != RpcOk){
			free(s);
			return ret;
		}
		if((user = _strfindattr(fss->attr, "user")) == nil){
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
	}
	fss->phasename = phasenames;
	fss->maxphase = Maxphase;
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

	case CHaveResp:
		if(s->resplen < *n)
			*n = s->resplen;
		memmove(va, s->resp, *n);
		fss->phase = Established;
		return RpcOk;

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
p9response(Fsstate *fss, State *s)
{
	char key[DESKEYLEN];
	uchar buf[8];
	ulong chal;
	char *pw;

	pw = _strfindattr(s->key->privattr, "!password");
	if(pw == nil)
		return failure(fss, "vncresponse cannot happen");
	passtokey(key, pw);
	memset(buf, 0, 8);
	snprint((char*)buf, sizeof buf, "%d", atoi(s->chal));
	if(encrypt(key, buf, 8) < 0)
		return failure(fss, "can't encrypt response");
	chal = (buf[0]<<24)+(buf[1]<<16)+(buf[2]<<8)+buf[3];
	s->resplen = snprint(s->resp, sizeof s->resp, "%.8lux", chal);
	return RpcOk;
}

static uchar tab[256];

/* VNC reverses the bits of each byte before using as a des key */
static void
mktab(void)
{
	int i, j, k;
	static int once;

	if(once)
		return;
	once = 1;

	for(i=0; i<256; i++) {
		j=i;
		tab[i] = 0;
		for(k=0; k<8; k++) {
			tab[i] = (tab[i]<<1) | (j&1);
			j >>= 1;
		}
	}
}

static int
vncaddkey(Key *k, int before)
{
	uchar *p;
	char *s;

	k->priv = emalloc(8+1);
	if(s = _strfindattr(k->privattr, "!password")){
		mktab();
		memset(k->priv, 0, 8+1);
		strncpy((char*)k->priv, s, 8);
		for(p=k->priv; *p; p++)
			*p = tab[*p];
	}else{
		werrstr("no key data");
		return -1;
	}
	return replacekey(k, before);
}

static void
vncclosekey(Key *k)
{
	free(k->priv);
}

static int
vncresponse(Fsstate*, State *s)
{
	DESstate des;

	memmove(s->resp, s->chal, sizeof s->chal);
	setupDESstate(&des, s->key->priv, nil);
	desECBencrypt((uchar*)s->resp, s->challen, &des);
	s->resplen = s->challen;
	return RpcOk;
}

static int
p9crwrite(Fsstate *fss, void *va, uint n)
{
	char tbuf[TICKETLEN+AUTHENTLEN];
	State *s;
	char *data = va;
	Authenticator a;
	char resp[Maxchal];
	int ret;

	s = fss->ps;
	switch(fss->phase){
	default:
		return phaseerror(fss, "write");

	case CNeedChal:
		if(n >= sizeof(s->chal))
			return failure(fss, Ebadarg);
		memset(s->chal, 0, sizeof s->chal);
		memmove(s->chal, data, n);
		s->challen = n;

		if(s->astype == AuthChal)
			ret = p9response(fss, s);
		else
			ret = vncresponse(fss, s);
		if(ret != RpcOk)
			return ret;
		fss->phase = CHaveResp;
		return RpcOk;

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
		|| memcmp(s->t.chal, s->tr.chal, sizeof(s->t.chal)) != 0){
			if (s->key->successes == 0)
				disablekey(s->key);
			return failure(fss, Easproto);
		}
		s->key->successes++;
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

	safecpy(s->tr.hostid, _strfindattr(s->key->attr, "user"), sizeof(s->tr.hostid));
	safecpy(s->tr.authdom, _strfindattr(s->key->attr, "dom"), sizeof(s->tr.authdom));
	s->tr.type = s->astype;
	convTR2M(&s->tr, trbuf);

	/* get challenge from auth server */
	s->asfd = _authdial(nil, _strfindattr(s->key->attr, "dom"));
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
.keyprompt=	"user? !password?",
};

Proto vnc =
{
.name=		"vnc",
.init=		p9crinit,
.write=		p9crwrite,
.read=		p9crread,
.close=		p9crclose,
.keyprompt=	"!password?",
.addkey=	vncaddkey,
};
