/*
 * p9sk1, p9sk2 - Plan 9 secret (private) key authentication.
 * p9sk2 is an incomplete flawed variant of p9sk1.
 *
 * Client protocol:
 *	write challenge[challen]	(p9sk1 only)
 *	read tickreq[tickreqlen]
 *	write ticket[ticketlen]
 *	read authenticator[authentlen]
 *
 * Server protocol:
 * 	read challenge[challen]	(p9sk1 only)
 *	write tickreq[tickreqlen]
 *	read ticket[ticketlen]
 *	write authenticator[authentlen]
 */

#include "dat.h"

struct State
{
	int vers;
	Key *key;
	Ticket t;
	Ticketreq tr;
	char cchal[CHALLEN];
	char tbuf[TICKETLEN+AUTHENTLEN];
	char authkey[DESKEYLEN];
	uchar *secret;
	int speakfor;
};

enum
{
	/* client phases */
	CHaveChal,
	CNeedTreq,
	CHaveTicket,
	CNeedAuth,

	/* server phases */
	SNeedChal,
	SHaveTreq,
	SNeedTicket,
	SHaveAuth,

	Maxphase,
};

static char *phasenames[Maxphase] =
{
[CHaveChal]		"CHaveChal",
[CNeedTreq]		"CNeedTreq",
[CHaveTicket]		"CHaveTicket",
[CNeedAuth]		"CNeedAuth",

[SNeedChal]		"SNeedChal",
[SHaveTreq]		"SHaveTreq",
[SNeedTicket]		"SNeedTicket",
[SHaveAuth]		"SHaveAuth",
};

static int gettickets(State*, char*, char*);

static int
p9skinit(Proto *p, Fsstate *fss)
{
	State *s;
	int iscli, ret;
	Key *k;
	Keyinfo ki;
	Attr *attr;

	if((iscli = isclient(_strfindattr(fss->attr, "role"))) < 0)
		return failure(fss, nil);

	s = emalloc(sizeof *s);
	fss = fss;
	fss->phasename = phasenames;
	fss->maxphase = Maxphase;
	if(p == &p9sk1)
		s->vers = 1;
	else if(p == &p9sk2)
		s->vers = 2;
	else
		abort();
	if(iscli){
		switch(s->vers){
		case 1:
			fss->phase = CHaveChal;
			memrandom(s->cchal, CHALLEN);
			break;
		case 2:
			fss->phase = CNeedTreq;
			break;
		}
	}else{
		s->tr.type = AuthTreq;
		attr = setattr(_copyattr(fss->attr), "proto=p9sk1");
		mkkeyinfo(&ki, fss, attr);
		ki.user = nil;
		ret = findkey(&k, &ki, "user? dom?");
		_freeattr(attr);
		if(ret != RpcOk){
			free(s);
			return ret;
		}
		safecpy(s->tr.authid, _strfindattr(k->attr, "user"), sizeof(s->tr.authid));
		safecpy(s->tr.authdom, _strfindattr(k->attr, "dom"), sizeof(s->tr.authdom));
		s->key = k;
		memrandom(s->tr.chal, sizeof s->tr.chal);
		switch(s->vers){
		case 1:
			fss->phase = SNeedChal;
			break;
		case 2:
			fss->phase = SHaveTreq;
			memmove(s->cchal, s->tr.chal, CHALLEN);
			break;
		}
	}
	fss->ps = s;
	return RpcOk;
}

static int
p9skread(Fsstate *fss, void *a, uint *n)
{
	int m;
	State *s;

	s = fss->ps;
	switch(fss->phase){
	default:
		return phaseerror(fss, "read");

	case CHaveChal:
		m = CHALLEN;
		if(*n < m)
			return toosmall(fss, m);
		*n = m;
		memmove(a, s->cchal, m);
		fss->phase = CNeedTreq;
		return RpcOk;

	case SHaveTreq:
		m = TICKREQLEN;
		if(*n < m)
			return toosmall(fss, m);
		*n = m;
		convTR2M(&s->tr, a);
		fss->phase = SNeedTicket;
		return RpcOk;

	case CHaveTicket:
		m = TICKETLEN+AUTHENTLEN;
		if(*n < m)
			return toosmall(fss, m);
		*n = m;
		memmove(a, s->tbuf, m);
		fss->phase = CNeedAuth;
		return RpcOk;

	case SHaveAuth:
		m = AUTHENTLEN;
		if(*n < m)
			return toosmall(fss, m);
		*n = m;
		memmove(a, s->tbuf+TICKETLEN, m);
		fss->ai.cuid = s->t.cuid;
		fss->ai.suid = s->t.suid;
		s->secret = emalloc(8);
		des56to64((uchar*)s->t.key, s->secret);
		fss->ai.secret = s->secret;
		fss->ai.nsecret = 8;
		fss->haveai = 1;
		fss->phase = Established;
		return RpcOk;
	}
}

static int
p9skwrite(Fsstate *fss, void *a, uint n)
{
	int m, ret, sret;
	char tbuf[2*TICKETLEN], trbuf[TICKREQLEN], *user;
	Attr *attr;
	Authenticator auth;
	State *s;
	Key *srvkey;
	Keyinfo ki;

	s = fss->ps;
	switch(fss->phase){
	default:
		return phaseerror(fss, "write");

	case SNeedChal:
		m = CHALLEN;
		if(n < m)
			return toosmall(fss, m);
		memmove(s->cchal, a, m);
		fss->phase = SHaveTreq;
		return RpcOk;

	case CNeedTreq:
		m = TICKREQLEN;
		if(n < m)
			return toosmall(fss, m);

		/* remember server's chal */
		convM2TR(a, &s->tr);
		if(s->vers == 2)
			memmove(s->cchal, s->tr.chal, CHALLEN);

		if(s->key != nil)
			closekey(s->key);

		attr = _delattr(_delattr(_copyattr(fss->attr), "role"), "user");
		attr = setattr(attr, "proto=p9sk1");
		user = _strfindattr(fss->attr, "user");
		/*
		 * If our client is the user who started factotum (client==owner), then
		 * he can use whatever keys we have to speak as whoever he pleases.
		 * If, on the other hand, we're speaking on behalf of someone else,
		 * we will only vouch for their name on the local system.
		 *
		 * We do the sysuser findkey second so that if we return RpcNeedkey,
		 * the correct key information gets asked for.
		 */
		srvkey = nil;
		s->speakfor = 0;
		sret = RpcFailure;
		if(user==nil || strcmp(user, fss->sysuser) == 0){
			mkkeyinfo(&ki, fss, attr);
			ki.user = nil;
			sret = findkey(&srvkey, &ki,
				"role=speakfor dom=%q user?", s->tr.authdom);
		}
		if(user != nil)
			attr = setattr(attr, "user=%q", user);
		mkkeyinfo(&ki, fss, attr);
		ret = findkey(&s->key, &ki,
			"role=client dom=%q %s", s->tr.authdom, p9sk1.keyprompt);
		if(ret == RpcOk)
			closekey(srvkey);
		else if(sret == RpcOk){
			s->key = srvkey;
			s->speakfor = 1;
		}else if(ret == RpcConfirm || sret == RpcConfirm){
			_freeattr(attr);
			return RpcConfirm;
		}else{
			_freeattr(attr);
			return ret;
		}

		/* fill in the rest of the request */
		s->tr.type = AuthTreq;
		safecpy(s->tr.hostid, _strfindattr(s->key->attr, "user"), sizeof s->tr.hostid);
		if(s->speakfor)
			safecpy(s->tr.uid, fss->sysuser, sizeof s->tr.uid);
		else
			safecpy(s->tr.uid, s->tr.hostid, sizeof s->tr.uid);

		convTR2M(&s->tr, trbuf);

		/* get tickets, from auth server or invent if we can */
		if(gettickets(s, trbuf, tbuf) < 0){
			_freeattr(attr);
			return failure(fss, nil);
		}

		convM2T(tbuf, &s->t, (char*)s->key->priv);
		if(s->t.num != AuthTc){
			if(s->key->successes == 0 && !s->speakfor)
				disablekey(s->key);
			if(askforkeys && !s->speakfor){
				snprint(fss->keyinfo, sizeof fss->keyinfo,
					"%A %s", attr, p9sk1.keyprompt);
				_freeattr(attr);
				return RpcNeedkey;
			}else{
				_freeattr(attr);
				return failure(fss, Ebadkey);
			}
		}
		s->key->successes++;
		_freeattr(attr);
		memmove(s->tbuf, tbuf+TICKETLEN, TICKETLEN);

		auth.num = AuthAc;
		memmove(auth.chal, s->tr.chal, CHALLEN);
		auth.id = 0;
		convA2M(&auth, s->tbuf+TICKETLEN, s->t.key);
		fss->phase = CHaveTicket;
		return RpcOk;

	case SNeedTicket:
		m = TICKETLEN+AUTHENTLEN;
		if(n < m)
			return toosmall(fss, m);
		convM2T(a, &s->t, (char*)s->key->priv);
		if(s->t.num != AuthTs
		|| memcmp(s->t.chal, s->tr.chal, CHALLEN) != 0)
			return failure(fss, Easproto);
		convM2A((char*)a+TICKETLEN, &auth, s->t.key);
		if(auth.num != AuthAc
		|| memcmp(auth.chal, s->tr.chal, CHALLEN) != 0
		|| auth.id != 0)
			return failure(fss, Easproto);
		auth.num = AuthAs;
		memmove(auth.chal, s->cchal, CHALLEN);
		auth.id = 0;
		convA2M(&auth, s->tbuf+TICKETLEN, s->t.key);
		fss->phase = SHaveAuth;
		return RpcOk;

	case CNeedAuth:
		m = AUTHENTLEN;
		if(n < m)
			return toosmall(fss, m);
		convM2A(a, &auth, s->t.key);
		if(auth.num != AuthAs
		|| memcmp(auth.chal, s->cchal, CHALLEN) != 0
		|| auth.id != 0)
			return failure(fss, Easproto);
		fss->ai.cuid = s->t.cuid;
		fss->ai.suid = s->t.suid;
		s->secret = emalloc(8);
		des56to64((uchar*)s->t.key, s->secret);
		fss->ai.secret = s->secret;
		fss->ai.nsecret = 8;
		fss->haveai = 1;
		fss->phase = Established;
		return RpcOk;
	}
}

static void
p9skclose(Fsstate *fss)
{
	State *s;

	s = fss->ps;
	if(s->secret != nil){
		free(s->secret);
		s->secret = nil;
	}
	if(s->key != nil){
		closekey(s->key);
		s->key = nil;
	}
	free(s);
}

static int
unhex(char c)
{
	if('0' <= c && c <= '9')
		return c-'0';
	if('a' <= c && c <= 'f')
		return c-'a'+10;
	if('A' <= c && c <= 'F')
		return c-'A'+10;
	abort();
	return -1;
}

static int
hexparse(char *hex, uchar *dat, int ndat)
{
	int i;

	if(strlen(hex) != 2*ndat)
		return -1;
	if(hex[strspn(hex, "0123456789abcdefABCDEF")] != '\0')
		return -1;
	for(i=0; i<ndat; i++)
		dat[i] = (unhex(hex[2*i])<<4)|unhex(hex[2*i+1]);
	return 0;
}

static int
p9skaddkey(Key *k, int before)
{
	char *s;

	k->priv = emalloc(DESKEYLEN);
	if(s = _strfindattr(k->privattr, "!hex")){
		if(hexparse(s, k->priv, 7) < 0){
			free(k->priv);
			k->priv = nil;
			werrstr("malformed key data");
			return -1;
		}
	}else if(s = _strfindattr(k->privattr, "!password")){
		passtokey((char*)k->priv, s);
	}else{
		werrstr("no key data");
		free(k->priv);
		k->priv = nil;
		return -1;
	}
	return replacekey(k, before);
}

static void
p9skclosekey(Key *k)
{
	free(k->priv);
}

static int
getastickets(State *s, char *trbuf, char *tbuf)
{
	int asfd, rv;
	char *dom;

	if((dom = _strfindattr(s->key->attr, "dom")) == nil){
		werrstr("auth key has no domain");
		return -1;
	}
	asfd = _authdial(nil, dom);
	if(asfd < 0)
		return -1;
	rv = _asgetticket(asfd, trbuf, tbuf);
	close(asfd);
	return rv;
}

static int
mkserverticket(State *s, char *tbuf)
{
	Ticketreq *tr = &s->tr;
	Ticket t;

	if(strcmp(tr->authid, tr->hostid) != 0)
		return -1;
/* this keeps creating accounts on martha from working.  -- presotto
	if(strcmp(tr->uid, "none") == 0)
		return -1;
*/
	memset(&t, 0, sizeof(t));
	memmove(t.chal, tr->chal, CHALLEN);
	strcpy(t.cuid, tr->uid);
	strcpy(t.suid, tr->uid);
	memrandom(t.key, DESKEYLEN);
	t.num = AuthTc;
	convT2M(&t, tbuf, s->key->priv);
	t.num = AuthTs;
	convT2M(&t, tbuf+TICKETLEN, s->key->priv);
	return 0;
}

static int
gettickets(State *s, char *trbuf, char *tbuf)
{
/*
	if(mktickets(s, trbuf, tbuf) >= 0)
		return 0;
*/
	if(getastickets(s, trbuf, tbuf) >= 0)
		return 0;
	return mkserverticket(s, tbuf);
}

Proto p9sk1 = {
.name=	"p9sk1",
.init=		p9skinit,
.write=	p9skwrite,
.read=	p9skread,
.close=	p9skclose,
.addkey=	p9skaddkey,
.closekey=	p9skclosekey,
.keyprompt=	"user? !password?"
};

Proto p9sk2 = {
.name=	"p9sk2",
.init=		p9skinit,
.write=	p9skwrite,
.read=	p9skread,
.close=	p9skclose,
};

