/*
 * CHAP, MSCHAP
 * 
 * The client does not authenticate the server, hence no CAI
 *
 * Client protocol:
 *	write Chapchal 
 *	read response Chapreply or MSchaprely structure
 *
 * Server protocol:
 *	read challenge: 8 bytes binary
 *	write user: utf8
 *	write response: Chapreply or MSchapreply structure
 */

#include <ctype.h>
#include "dat.h"

enum {
	ChapChallen = 8,
	ChapResplen = 16,
	MSchapResplen = 24,
};

static int dochal(State*);
static int doreply(State*, void*, int);
static void doLMchap(char *, uchar [ChapChallen], uchar [MSchapResplen]);
static void doNTchap(char *, uchar [ChapChallen], uchar [MSchapResplen]);
static void dochap(char *, int, char [ChapChallen], uchar [ChapResplen]);


struct State
{
	char *protoname;
	int astype;
	int asfd;
	Key *key;
	Ticket	t;
	Ticketreq	tr;
	char chal[ChapChallen];
	MSchapreply mcr;
	char cr[ChapResplen];
	char err[ERRMAX];
	char user[64];
	uchar secret[16];	/* for mschap */
	int nsecret;
};

enum
{
	CNeedChal,
	CHaveResp,

	SHaveChal,
	SNeedUser,
	SNeedResp,
	SHaveZero,
	SHaveCAI,

	Maxphase
};

static char *phasenames[Maxphase] =
{
[CNeedChal]	"CNeedChal",
[CHaveResp]	"CHaveResp",

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

	if((iscli = isclient(_strfindattr(fss->attr, "role"))) < 0)
		return failure(fss, nil);

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
	int ret, nreply;
	char *a, *v;
	void *reply;
	Key *k;
	Keyinfo ki;
	State *s;
	Chapreply cr;
	MSchapreply mcr;
	OChapreply ocr;
	OMSchapreply omcr;

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
		case AuthMSchap:
			doLMchap(v, (uchar *)a, (uchar *)s->mcr.LMresp);
			doNTchap(v, (uchar *)a, (uchar *)s->mcr.NTresp);
			break;
		case AuthChap:
			dochap(v, *a, a+1, (uchar *)s->cr);
			break;
		}
		closekey(k);
		fss->phase = CHaveResp;
		return RpcOk;

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
			memset(omcr.uid, 0, sizeof(omcr.uid));
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
			memset(omcr.uid, 0, sizeof(omcr.uid));
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

	case CHaveResp:
		switch(s->astype){
		default:
			phaseerror(fss, "write");
			break;
		case AuthMSchap:
			if(*n > sizeof(MSchapreply))
				*n = sizeof(MSchapreply);
			memmove(va, &s->mcr, *n);
			break;
		case AuthChap:
			if(*n > ChapResplen)
				*n = ChapResplen;
			memmove(va, s->cr, ChapResplen);
			break;
		}
		fss->phase = Established;
		fss->haveai = 0;
		return RpcOk;

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
	if((dom = _strfindattr(s->key->attr, "dom")) == nil
	|| (user = _strfindattr(s->key->attr, "user")) == nil){
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
		if(s->key->successes == 0)
			disablekey(s->key);
		werrstr(Easproto);
		return -1;
	}
	s->key->successes++;
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
.addkey= replacekey,
.keyprompt= "!password?"
};

Proto mschap = {
.name=	"mschap",
.init=	chapinit,
.write=	chapwrite,
.read=	chapread,
.close=	chapclose,
.addkey= replacekey,
.keyprompt= "!password?"
};

static void
hash(uchar pass[16], uchar c8[ChapChallen], uchar p24[MSchapResplen])
{
	int i;
	uchar p21[21];
	ulong schedule[32];

	memset(p21, 0, sizeof p21 );
	memmove(p21, pass, 16);

	for(i=0; i<3; i++) {
		key_setup(p21+i*7, schedule);
		memmove(p24+i*8, c8, 8);
		block_cipher(schedule, p24+i*8, 0);
	}
}

static void
doNTchap(char *pass, uchar chal[ChapChallen], uchar reply[MSchapResplen])
{
	Rune r;
	int i, n;
	uchar digest[MD4dlen];
	uchar *w, unipass[256];

	// Standard says unlimited length, experience says 128 max
	if ((n = strlen(pass)) > 128)
		n = 128;

	for(i=0, w=unipass; i < n; i++) {
		pass += chartorune(&r, pass);
		*w++ = r & 0xff;
		*w++ = r >> 8;
	}

	memset(digest, 0, sizeof digest);
	md4(unipass, w-unipass, digest, nil);
	memset(unipass, 0, sizeof unipass);
	hash(digest, chal, reply);
}

static void
doLMchap(char *pass, uchar chal[ChapChallen], uchar reply[MSchapResplen])
{
	int i;
	ulong schedule[32];
	uchar p14[15], p16[16];
	uchar s8[8] = {0x4b, 0x47, 0x53, 0x21, 0x40, 0x23, 0x24, 0x25};
	int n = strlen(pass);

	if(n > 14){
		// let prudent people avoid the LM vulnerability
		//   and protect the loop below from buffer overflow
		memset(reply, 0, MSchapResplen);
		return;
	}

	// Spec says space padded, experience says otherwise
	memset(p14, 0, sizeof p14 -1);
	p14[sizeof p14 - 1] = '\0';

	// NT4 requires uppercase, Win XP doesn't care
	for (i = 0; pass[i]; i++)
		p14[i] = islower(pass[i])? toupper(pass[i]): pass[i];

	for(i=0; i<2; i++) {
		key_setup(p14+i*7, schedule);
		memmove(p16+i*8, s8, 8);
		block_cipher(schedule, p16+i*8, 0);
	}

	memset(p14, 0, sizeof p14);
	hash(p16, chal, reply);
}

static void
dochap(char *pass, int id, char chal[ChapChallen], uchar resp[ChapResplen])
{
	char buf[1+ChapChallen+MAXNAMELEN+1];
	int n = strlen(pass);

	*buf = id;
	if (n > MAXNAMELEN)
		n = MAXNAMELEN-1;
	memset(buf, 0, sizeof buf);
	strncpy(buf+1, pass, n);
	memmove(buf+1+n, chal, ChapChallen);
	md5((uchar*)buf, 1+n+ChapChallen, resp, nil);
}

