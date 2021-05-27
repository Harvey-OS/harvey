/*
 * WPA-PSK
 *
 * Client protocol:
 *	write challenge: smac[6] + amac[6] + snonce[32] + anonce[32]
 *	read response: ptk[64]
 *
 * Server protocol:
 *	unimplemented
 */
#include "dat.h"

enum {
	PMKlen = 256/8,
	PTKlen = 512/8,

	Eaddrlen = 6,
	Noncelen = 32,
};

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
	uchar	resp[PTKlen];
};

static void
pass2pmk(char *pass, char *ssid, uchar pmk[PMKlen])
{
	int npass = strlen(pass);
	if(npass == 2*PMKlen && dec16(pmk, PMKlen, pass, npass) == PMKlen)
		return;
	pbkdf2_x((uchar*)pass, npass, (uchar*)ssid, strlen(ssid), 4096, pmk, PMKlen, hmac_sha1, SHA1dlen);
}

static void
prfn(uchar *k, int klen, char *a, uchar *b, int blen, uchar *d, int dlen)
{
	uchar r[SHA1dlen], i;
	DigestState *ds;
	int n;

	i = 0;
	while(dlen > 0){
		ds = hmac_sha1((uchar*)a, strlen(a)+1, k, klen, nil, nil);
		hmac_sha1(b, blen, k, klen, nil, ds);
		hmac_sha1(&i, 1, k, klen, r, ds);
		i++;
		n = dlen;
		if(n > sizeof(r))
			n = sizeof(r);
		memmove(d, r, n); d += n;
		dlen -= n;
	}
}

static void
calcptk(uchar pmk[PMKlen], uchar smac[Eaddrlen], uchar amac[Eaddrlen], 
	uchar snonce[Noncelen],  uchar anonce[Noncelen], 
	uchar ptk[PTKlen])
{
	uchar b[2*Eaddrlen + 2*Noncelen];

	if(memcmp(smac, amac, Eaddrlen) > 0){
		memmove(b + Eaddrlen*0, amac, Eaddrlen);
		memmove(b + Eaddrlen*1, smac, Eaddrlen);
	} else {
		memmove(b + Eaddrlen*0, smac, Eaddrlen);
		memmove(b + Eaddrlen*1, amac, Eaddrlen);
	}
	if(memcmp(snonce, anonce, Eaddrlen) > 0){
		memmove(b + Eaddrlen*2 + Noncelen*0, anonce, Noncelen);
		memmove(b + Eaddrlen*2 + Noncelen*1, snonce, Noncelen);
	} else {
		memmove(b + Eaddrlen*2 + Noncelen*0, snonce, Noncelen);
		memmove(b + Eaddrlen*2 + Noncelen*1, anonce, Noncelen);
	}
	prfn(pmk, PMKlen, "Pairwise key expansion", b, sizeof(b), ptk, PTKlen);
}

static int
wpapskinit(Proto *p, Fsstate *fss)
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

static int
wpapskwrite(Fsstate *fss, void *va, uint n)
{
	uchar pmk[PMKlen], *smac, *amac, *snonce, *anonce;
	char *pass, *essid;
	State *s;
	int ret;
	Key *k;
	Keyinfo ki;
	Attr *attr;

	s = fss->ps;

	if(fss->phase != CNeedChal)
		return phaseerror(fss, "write");
	if(n != (2*Eaddrlen + 2*Noncelen))
		return phaseerror(fss, "bad write size");

	attr = _delattr(_copyattr(fss->attr), "role");
	mkkeyinfo(&ki, fss, attr);
	ret = findkey(&k, &ki, "%s", fss->proto->keyprompt);
	_freeattr(attr);
	if(ret != RpcOk)
		return ret;

	pass = _strfindattr(k->privattr, "!password");
	if(pass == nil)
		return failure(fss, "key has no password");
	essid = _strfindattr(k->attr, "essid");
	if(essid == nil)
		return failure(fss, "key has no essid");
	setattrs(fss->attr, k->attr);
	closekey(k);

	pass2pmk(pass, essid, pmk);

	smac = va;
	amac = smac + Eaddrlen;
	snonce = amac + Eaddrlen;
	anonce = snonce + Noncelen;
	calcptk(pmk, smac, amac, snonce, anonce, s->resp);

	fss->phase = CHaveResp;
	return RpcOk;
}

static int
wpapskread(Fsstate *fss, void *va, uint *n)
{
	State *s;

	s = fss->ps;
	if(fss->phase != CHaveResp)
		return phaseerror(fss, "read");
	if(*n > sizeof(s->resp))
		*n = sizeof(s->resp);
	memmove(va, s->resp, *n);
	fss->phase = Established;
	fss->haveai = 0;
	return RpcOk;
}

static void
wpapskclose(Fsstate *fss)
{
	State *s;
	s = fss->ps;
	free(s);
}

Proto wpapsk = {
.name=		"wpapsk",
.init=		wpapskinit,
.write=		wpapskwrite,
.read=		wpapskread,
.close=		wpapskclose,
.addkey=	replacekey,
.keyprompt=	"!password? essid?"
};
