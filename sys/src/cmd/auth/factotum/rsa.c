/*
 * SSH RSA authentication.
 * 
 * Client protocol:
 *	read public key
 *		if you don't like it, read another, repeat
 *	write challenge
 *	read response
 * all numbers are hexadecimal biginits parsable with strtomp.
 *
 */

#include "dat.h"

enum {
	CHavePub,
	CHaveResp,

	Maxphase,
};

static char *phasenames[] = {
[CHavePub]	"CHavePub",
[CHaveResp]	"CHaveResp",
};

struct State
{
	RSApriv *priv;
	mpint *resp;
	int off;
	Key *key;
};

static RSApriv*
readrsapriv(Key *k)
{
	char *a;
	RSApriv *priv;

	priv = rsaprivalloc();

	if((a=_strfindattr(k->attr, "ek"))==nil || (priv->pub.ek=strtomp(a, nil, 16, nil))==nil)
		goto Error;
	if((a=_strfindattr(k->attr, "n"))==nil || (priv->pub.n=strtomp(a, nil, 16, nil))==nil)
		goto Error;
	if((a=_strfindattr(k->privattr, "!p"))==nil || (priv->p=strtomp(a, nil, 16, nil))==nil)
		goto Error;
	if((a=_strfindattr(k->privattr, "!q"))==nil || (priv->q=strtomp(a, nil, 16, nil))==nil)
		goto Error;
	if((a=_strfindattr(k->privattr, "!kp"))==nil || (priv->kp=strtomp(a, nil, 16, nil))==nil)
		goto Error;
	if((a=_strfindattr(k->privattr, "!kq"))==nil || (priv->kq=strtomp(a, nil, 16, nil))==nil)
		goto Error;
	if((a=_strfindattr(k->privattr, "!c2"))==nil || (priv->c2=strtomp(a, nil, 16, nil))==nil)
		goto Error;
	if((a=_strfindattr(k->privattr, "!dk"))==nil || (priv->dk=strtomp(a, nil, 16, nil))==nil)
		goto Error;
	return priv;

Error:
	rsaprivfree(priv);
	return nil;
}

static int
rsainit(Proto*, Fsstate *fss)
{
	int iscli;
	State *s;

	if((iscli = isclient(_strfindattr(fss->attr, "role"))) < 0)
		return failure(fss, nil);
	if(iscli==0)
		return failure(fss, "rsa server unimplemented");

	s = emalloc(sizeof *s);
	fss->phasename = phasenames;
	fss->maxphase = Maxphase;
	fss->phase = CHavePub;
	fss->ps = s;
	return RpcOk;
}

static int
rsaread(Fsstate *fss, void *va, uint *n)
{
	RSApriv *priv;
	State *s;
	Keyinfo ki;

	s = fss->ps;
	switch(fss->phase){
	default:
		return phaseerror(fss, "read");
	case CHavePub:
		if(s->key){
			closekey(s->key);
			s->key = nil;
		}
		mkkeyinfo(&ki, fss, nil);
		ki.skip = s->off;
		ki.noconf = 1;
		if(findkey(&s->key, &ki, nil) != RpcOk)
			return failure(fss, nil);
		s->off++;
		priv = s->key->priv;
		*n = snprint(va, *n, "%B", priv->pub.n);
		return RpcOk;
	case CHaveResp:
		*n = snprint(va, *n, "%B", s->resp);
		fss->phase = Established;
		return RpcOk;
	}
}

static int
rsawrite(Fsstate *fss, void *va, uint)
{
	mpint *m;
	State *s;

	s = fss->ps;
	switch(fss->phase){
	default:
		return phaseerror(fss, "write");
	case CHavePub:
		if(s->key == nil)
			return failure(fss, "no current key");
		switch(canusekey(fss, s->key)){
		case -1:
			return RpcConfirm;
		case 0:
			return failure(fss, "confirmation denied");
		case 1:
			break;
		}
		m = strtomp(va, nil, 16, nil);
		if(m == nil)
			return failure(fss, "invalid challenge value");
		m = rsadecrypt(s->key->priv, m, m);
		s->resp = m;
		fss->phase = CHaveResp;
		return RpcOk;
	}
}

static void
rsaclose(Fsstate *fss)
{
	State *s;

	s = fss->ps;
	if(s->key)
		closekey(s->key);
	if(s->resp)
		mpfree(s->resp);
	free(s);
}

static int
rsaaddkey(Key *k, int before)
{
	fmtinstall('B', mpfmt);

	if((k->priv = readrsapriv(k)) == nil){
		werrstr("malformed key data");
		return -1;
	}
	return replacekey(k, before);
}

static void
rsaclosekey(Key *k)
{
	rsaprivfree(k->priv);
}

Proto rsa = {
.name=	"rsa",
.init=		rsainit,
.write=	rsawrite,
.read=	rsaread,
.close=	rsaclose,
.addkey=	rsaaddkey,
.closekey=	rsaclosekey,
};
