/*
 * CHAP, MSCHAP
 * 
 * The client does not authenticate the server, hence no CAI
 *
 * Client protocol:
 *	unimplemented
 *
 * Server protocol:
 *	read challenge: 8 bytes binary
 *	write user: utf8
 *	write response: Chapreply or MSchapreply structure
 */

#include "dat.h"

enum {
	ChapChallen = 8,
};

static int dochal(State*);
static int doreply(State*, void*, int);

struct State
{
	char *protoname;
	int astype;
	int asfd;
	Key *key;
	Ticket	t;
	Ticketreq	tr;
	char chal[ChapChallen];
	char	err[ERRMAX];
	char user[64];
	uchar secret[16];	/* for mschap */
	int nsecret;
};

enum
{
	SHaveChal,
	SNeedUser,
	SNeedResp,
	SHaveZero,
	SHaveCAI,

	Maxphase
};

static char *phasenames[Maxphase] =
{
[SHaveChal]	"SHaveChal",
[SNeedUser]	"SNeedUser",
[SNeedResp]	"SNeedResp",
[SHaveZero]	"SHaveZero",
[SHaveCAI]	"SHaveCAI",
};

static int
chapinit(Proto *p, Fsstate *fss)
{
	int iscli, ret;
	State *s;

	if((iscli = isclient(_str_findattr(fss->attr, "role"))) < 0)
		return failure(fss, nil);
	if(iscli)
		return failure(fss, "%s client not supported", p->name);

	s = emalloc(sizeof *s);
	fss->phasename = phasenames;
	fss->maxphase = Maxphase;
	s->asfd = -1;
	if(p == &chap){
		s->astype = AuthChap;
		s->protoname = "chap";
	}else{
		s->astype = AuthMSchap;
		s->protoname = "mschap";
	}
	if((ret = findp9authkey(&s->key, fss)) != RpcOk){
		free(s);
		return ret;
	}
	if(dochal(s) < 0){
		free(s);
		return failure(fss, nil);
	}
	fss->phase = SHaveChal;
	fss->ps = s;
	return RpcOk;
}

static void
chapclose(Fsstate *fss)
{
	State *s;

	s = fss->ps;
	if(s->asfd >= 0){
		close(s->asfd);
		s->asfd = -1;
	}
	free(s);
}

static int
chapwrite(Fsstate *fss, void *va, uint n)
{
	int nreply;
	void *reply;
	State *s;
	Chapreply cr;
	MSchapreply mcr;
	OChapreply ocr;
	OMSchapreply omcr;

	s = fss->ps;
	switch(fss->phase){
	default:
		return phaseerror(fss, "write");

	case SNeedUser:
		if(n >= sizeof s->user)
			return failure(fss, "user name too long");
		memmove(s->user, va, n);
		s->user[n] = '\0';
		fss->phase = SNeedResp;
		return RpcOk;

	case SNeedResp:
		switch(s->astype){
		default:
			return failure(fss, "chap internal botch");
		case AuthChap:
			if(n != sizeof(Chapreply))
				return failure(fss, "did not get Chapreply");
			memmove(&cr, va, sizeof cr);
			ocr.id = cr.id;
			memmove(ocr.resp, cr.resp, sizeof ocr.resp);
			strecpy(ocr.uid, ocr.uid+sizeof ocr.uid, s->user);
			reply = &ocr;
			nreply = sizeof ocr;
			break;
		case AuthMSchap:
			if(n != sizeof(MSchapreply))
				return failure(fss, "did not get MSchapreply");
			memmove(&mcr, va, sizeof mcr);
			memmove(omcr.LMresp, mcr.LMresp, sizeof omcr.LMresp);
			memmove(omcr.NTresp, mcr.NTresp, sizeof omcr.NTresp);
			strecpy(omcr.uid, omcr.uid+sizeof omcr.uid, s->user);
			reply = &omcr;
			nreply = sizeof omcr;
			break;
		}
		if(doreply(s, reply, nreply) < 0)
			return failure(fss, nil);
		fss->phase = Established;
		fss->ai.cuid = s->t.cuid;
		fss->ai.suid = s->t.suid;
		fss->ai.secret = s->secret;
		fss->ai.nsecret = s->nsecret;
		fss->haveai = 1;
		return RpcOk;
	}
}

static int
chapread(Fsstate *fss, void *va, uint *n)
{
	State *s;

	s = fss->ps;
	switch(fss->phase){
	default:
		return phaseerror(fss, "read");

	case SHaveChal:
		if(*n > sizeof s->chal)
			*n = sizeof s->chal;
		memmove(va, s->chal, *n);
		fss->phase = SNeedUser;
		return RpcOk;
	}
}

static int
dochal(State *s)
{
	char *dom, *user;
	char trbuf[TICKREQLEN];

	s->asfd = -1;

	/* send request to authentication server and get challenge */
	if((dom = _str_findattr(s->key->attr, "dom")) == nil
	|| (user = _str_findattr(s->key->attr, "user")) == nil){
		werrstr("chap/dochal cannot happen");
		goto err;
	}
	s->asfd = _authdial(nil, dom);
	if(s->asfd < 0)
		goto err;
	
	memset(&s->tr, 0, sizeof(s->tr));
	s->tr.type = s->astype;
	safecpy(s->tr.authdom, dom, sizeof s->tr.authdom);
	safecpy(s->tr.hostid, user, sizeof(s->tr.hostid));
	convTR2M(&s->tr, trbuf);

	if(write(s->asfd, trbuf, TICKREQLEN) != TICKREQLEN)
		goto err;

	/* readn, not _asrdresp.  needs to match auth.srv.c. */
	if(readn(s->asfd, s->chal, sizeof s->chal) != sizeof s->chal)
		goto err;
	return 0;

err:
	if(s->asfd >= 0)
		close(s->asfd);
	s->asfd = -1;
	return -1;
}

static int
doreply(State *s, void *reply, int nreply)
{
	char ticket[TICKETLEN+AUTHENTLEN];
	int n;
	Authenticator a;

	if((n=write(s->asfd, reply, nreply)) != nreply){
		if(n >= 0)
			werrstr("short write to auth server");
		goto err;
	}

	if(_asrdresp(s->asfd, ticket, TICKETLEN+AUTHENTLEN) < 0){
		/* leave connection open so we can try again */
		return -1;
	}
	s->nsecret = readn(s->asfd, s->secret, sizeof s->secret);
	if(s->nsecret < 0)
		s->nsecret = 0;
	close(s->asfd);
	s->asfd = -1;
	convM2T(ticket, &s->t, s->key->priv);
	if(s->t.num != AuthTs
	|| memcmp(s->t.chal, s->tr.chal, sizeof(s->t.chal)) != 0){
		werrstr(Easproto);
		return -1;
	}
	convM2A(ticket+TICKETLEN, &a, s->t.key);
	if(a.num != AuthAc
	|| memcmp(a.chal, s->tr.chal, sizeof(a.chal)) != 0
	|| a.id != 0){
		werrstr(Easproto);
		return -1;
	}

	return 0;
err:
	if(s->asfd >= 0)
		close(s->asfd);
	s->asfd = -1;
	return -1;
}

Proto chap = {
.name=	"chap",
.init=	chapinit,
.write=	chapwrite,
.read=	chapread,
.close=	chapclose,
};

Proto mschap = {
.name=	"mschap",
.init=	chapinit,
.write=	chapwrite,
.read=	chapread,
.close=	chapclose,
};

