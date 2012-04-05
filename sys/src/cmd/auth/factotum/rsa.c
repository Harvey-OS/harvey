/*
 * RSA authentication.
 *
 * Old ssh client protocol:
 *	read public key
 *		if you don't like it, read another, repeat
 *	write challenge
 *	read response
 *
 * all numbers are hexadecimal biginits parsable with strtomp.
 *
 * Sign (PKCS #1 using hash=sha1 or hash=md5)
 *	write hash(msg)
 *	read signature(hash(msg))
 *
 * Verify:
 *	write hash(msg)
 *	write signature(hash(msg))
 *	read ok or fail
 */

#include "dat.h"

enum {
	CHavePub,
	CHaveResp,
	VNeedHash,
	VNeedSig,
	VHaveResp,
	SNeedHash,
	SHaveResp,
	Maxphase,
};

static char *phasenames[] = {
[CHavePub]	"CHavePub",
[CHaveResp]	"CHaveResp",
[VNeedHash]	"VNeedHash",
[VNeedSig]	"VNeedSig",
[VHaveResp]	"VHaveResp",
[SNeedHash]	"SNeedHash",
[SHaveResp]	"SHaveResp",
};

struct State
{
	RSApriv *priv;
	mpint *resp;
	int off;
	Key *key;
	mpint *digest;
	int sigresp;
};

static mpint* mkdigest(RSApub *key, char *hashalg, uchar *hash, uint dlen);

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
	if(k->privattr == nil)		/* only public half */
		return priv;
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
	Keyinfo ki;
	State *s;
	char *role;

	if((role = _strfindattr(fss->attr, "role")) == nil)
		return failure(fss, "rsa role not specified");
	if(strcmp(role, "client") == 0)
		fss->phase = CHavePub;
	else if(strcmp(role, "sign") == 0)
		fss->phase = SNeedHash;
	else if(strcmp(role, "verify") == 0)
		fss->phase = VNeedHash;
	else
		return failure(fss, "rsa role %s unimplemented", role);

	s = emalloc(sizeof *s);
	fss->phasename = phasenames;
	fss->maxphase = Maxphase;
	fss->ps = s;

	switch(fss->phase){
	case SNeedHash:
	case VNeedHash:
		mkkeyinfo(&ki, fss, nil);
		if(findkey(&s->key, &ki, nil) != RpcOk)
			return failure(fss, nil);
		/* signing needs private key */
		if(fss->phase == SNeedHash && s->key->privattr == nil)
			return failure(fss,
				"missing private half of key -- cannot sign");
	}
	return RpcOk;
}

static int
rsaread(Fsstate *fss, void *va, uint *n)
{
	RSApriv *priv;
	State *s;
	mpint *m;
	Keyinfo ki;
	int len, r;

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
	case SHaveResp:
		priv = s->key->priv;
		len = (mpsignif(priv->pub.n)+7)/8;
		if(len > *n)
			return failure(fss, "signature buffer too short");
		m = rsadecrypt(priv, s->digest, nil);
		r = mptobe(m, (uchar*)va, len, nil);
		if(r < len){
			memmove((uchar*)va+len-r, va, r);
			memset(va, 0, len-r);
		}
		*n = len;
		mpfree(m);
		fss->phase = Established;
		return RpcOk;
	case VHaveResp:
		*n = snprint(va, *n, "%s", s->sigresp == 0? "ok":
			"signature does not verify");
		fss->phase = Established;
		return RpcOk;
	}
}

static int
rsawrite(Fsstate *fss, void *va, uint n)
{
	RSApriv *priv;
	mpint *m, *mm;
	State *s;
	char *hash;
	int dlen;

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
	case SNeedHash:
	case VNeedHash:
		/* get hash type from key */
		hash = _strfindattr(s->key->attr, "hash");
		if(hash == nil)
			hash = "sha1";
		if(strcmp(hash, "sha1") == 0)
			dlen = SHA1dlen;
		else if(strcmp(hash, "md5") == 0)
			dlen = MD5dlen;
		else
			return failure(fss, "unknown hash function %s", hash);
		if(n != dlen)
			return failure(fss, "hash length %d should be %d",
				n, dlen);
		priv = s->key->priv;
		s->digest = mkdigest(&priv->pub, hash, (uchar *)va, n);
		if(s->digest == nil)
			return failure(fss, nil);
		if(fss->phase == VNeedHash)
			fss->phase = VNeedSig;
		else
			fss->phase = SHaveResp;
		return RpcOk;
	case VNeedSig:
		priv = s->key->priv;
		m = betomp((uchar*)va, n, nil);
		mm = rsaencrypt(&priv->pub, m, nil);
		s->sigresp = mpcmp(s->digest, mm);
		mpfree(m);
		mpfree(mm);
		fss->phase = VHaveResp;
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
	if(s->digest)
		mpfree(s->digest);
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

/*
 * Simple ASN.1 encodings.
 * Lengths < 128 are encoded as 1-bytes constants,
 * making our life easy.
 */

/*
 * Hash OIDs
 *
 * SHA1 = 1.3.14.3.2.26
 * MDx = 1.2.840.113549.2.x
 */
#define O0(a,b)	((a)*40+(b))
#define O2(x)	\
	(((x)>> 7)&0x7F)|0x80, \
	((x)&0x7F)
#define O3(x)	\
	(((x)>>14)&0x7F)|0x80, \
	(((x)>> 7)&0x7F)|0x80, \
	((x)&0x7F)
uchar oidsha1[] = { O0(1, 3), 14, 3, 2, 26 };
uchar oidmd2[] = { O0(1, 2), O2(840), O3(113549), 2, 2 };
uchar oidmd5[] = { O0(1, 2), O2(840), O3(113549), 2, 5 };

/*
 *	DigestInfo ::= SEQUENCE {
 *		digestAlgorithm AlgorithmIdentifier,
 *		digest OCTET STRING
 *	}
 *
 * except that OpenSSL seems to sign
 *
 *	DigestInfo ::= SEQUENCE {
 *		SEQUENCE{ digestAlgorithm AlgorithmIdentifier, NULL }
 *		digest OCTET STRING
 *	}
 *
 * instead.  Sigh.
 */
static int
mkasn1(uchar *asn1, char *alg, uchar *d, uint dlen)
{
	uchar *obj, *p;
	uint olen;

	if(strcmp(alg, "sha1") == 0){
		obj = oidsha1;
		olen = sizeof(oidsha1);
	}else if(strcmp(alg, "md5") == 0){
		obj = oidmd5;
		olen = sizeof(oidmd5);
	}else{
		sysfatal("bad alg in mkasn1");
		return -1;
	}

	p = asn1;
	*p++ = 0x30;		/* sequence */
	p++;

	*p++ = 0x30;		/* another sequence */
	p++;

	*p++ = 0x06;		/* object id */
	*p++ = olen;
	memmove(p, obj, olen);
	p += olen;

	*p++ = 0x05;		/* null */
	*p++ = 0;

	asn1[3] = p - (asn1+4);	/* end of inner sequence */

	*p++ = 0x04;		/* octet string */
	*p++ = dlen;
	memmove(p, d, dlen);
	p += dlen;

	asn1[1] = p - (asn1+2);	/* end of outer sequence */
	return p - asn1;
}

static mpint*
mkdigest(RSApub *key, char *hashalg, uchar *hash, uint dlen)
{
	mpint *m;
	uchar asn1[512], *buf;
	int len, n, pad;

	/*
	 * Create ASN.1
	 */
	n = mkasn1(asn1, hashalg, hash, dlen);

	/*
	 * PKCS#1 padding
	 */
	len = (mpsignif(key->n)+7)/8 - 1;
	if(len < n+2){
		werrstr("rsa key too short");
		return nil;
	}
	pad = len - (n+2);
	buf = emalloc(len);
	buf[0] = 0x01;
	memset(buf+1, 0xFF, pad);
	buf[1+pad] = 0x00;
	memmove(buf+1+pad+1, asn1, n);
	m = betomp(buf, len, nil);
	free(buf);
	return m;
}
