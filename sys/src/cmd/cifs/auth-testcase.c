/*
 * Beware the LM hash is easy to crack (google for l0phtCrack)
 * and though NTLM is more secure it is still breakable.
 * Ntlmv2 is better and seen as good enough by the Windows community.
 * For real security use Kerberos.
 */
#include <u.h>
#include <libc.h>
#include <mp.h>
#include <auth.h>
#include <libsec.h>
#include <ctype.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include "cifs.h"

#define NTLMV2_TEST	1
#define DEF_AUTH 	"ntlmv2"

static enum {
	MACkeylen	= 40,	/* MAC key len */
	MAClen		= 8,	/* signature length */
	MACoff		= 14,	/* sign. offset from start of SMB (not netbios) pkt */
	Bliplen		= 8,	/* size of LMv2 client nonce */
};

static void
dmp(char *s, int seq, void *buf, int n)
{
	int i;
	char *p = buf;

	print("%s %3d      ", s, seq);
	while(n > 0){
		for(i = 0; i < 16 && n > 0; i++, n--)
			print("%02x ", *p++ & 0xff);
		if(n > 0)
			print("\n");
	}
	print("\n");
}

static Auth *
auth_plain(char *windom, char *keyp, uchar *chal, int len)
{
	UserPasswd *up;
	static Auth *ap;

	USED(chal, len);

	up = auth_getuserpasswd(auth_getkey, "windom=%s proto=pass service=cifs %s",
		windom, keyp);
	if(! up)
		sysfatal("cannot get key - %r");

	ap = emalloc9p(sizeof(Auth));
	memset(ap, 0, sizeof(ap));
	ap->user = estrdup9p(up->user);
	ap->windom = estrdup9p(windom);

	ap->resp[0] = estrdup9p(up->passwd);
	ap->len[0] = strlen(up->passwd);
	memset(up->passwd, 0, strlen(up->passwd));
	free(up);

	return ap;
}

static Auth *
auth_lm_and_ntlm(char *windom, char *keyp, uchar *chal, int len)
{
	int err;
	Auth *ap;
	char user[64];
	MSchapreply mcr;

	err = auth_respond(chal, len, user, sizeof user, &mcr, sizeof mcr,
		auth_getkey, "windom=%s proto=mschap role=client service=cifs %s",
		windom, keyp);
	if(err == -1)
		sysfatal("cannot get key - %r");

	ap = emalloc9p(sizeof(Auth));
	memset(ap, 0, sizeof(ap));
	ap->user = estrdup9p(user);
	ap->windom = estrdup9p(windom);

	/* LM response */
	ap->len[0] = sizeof(mcr.LMresp);
	ap->resp[0] = emalloc9p(ap->len[0]);
	memcpy(ap->resp[0], mcr.LMresp, ap->len[0]);

	/* NTLM response */
	ap->len[1] = sizeof(mcr.NTresp);
	ap->resp[1] = emalloc9p(ap->len[1]);
	memcpy(ap->resp[1], mcr.NTresp, ap->len[1]);

	return ap;
}

/*
 * NTLM response only, the LM response is a just
 * copy of the NTLM one.  We do this because the lm
 * response is easily reversed - Google for l0pht for more info.
 */
static Auth *
auth_ntlm(char *windom, char *keyp, uchar *chal, int len)
{
	Auth *ap;

	if((ap = auth_lm_and_ntlm(windom, keyp, chal, len)) == nil)
		return nil;

	free(ap->resp[0]);
	ap->len[0] = ap->len[1];
	ap->resp[0] = emalloc9p(ap->len[0]);
	memcpy(ap->resp[0], ap->resp[1], ap->len[0]);
	return ap;
}

/*
 * This is not really nescessary as all fields hmac_md5'ed
 * in the ntlmv2 protocol are less than 64 bytes long, however
 * I still do this for completeness.
 */
static DigestState *
hmac_t64(uchar *data, ulong dlen, uchar *key, ulong klen, uchar *digest,
	DigestState *state)
{
	if(klen > 64)
		klen = 64;
	return hmac_md5(data, dlen, key, klen, digest, state);
}


static int
ntv2_blob(uchar *blob, int len, char *windom)
{
	int n;
	uvlong nttime;
	Rune r;
	char *d;
	uchar *p;
	enum {			/* name types */
		Beof,		/* end of name list */
		Bnetbios,	/* Netbios machine name */
		Bdomain,	/* Windows Domain name (NT) */
		Bdnsfqdn,	/* DNS Fully Qualified Domain Name */
		Bdnsname,	/* DNS machine name (win2k) */
	};

	p = blob;
	*p++ = 1;		/* response type */
	*p++ = 1;		/* max response type understood by client */

	*p++ = 0;
	*p++ = 0;		/* 2 bytes reserved */

	*p++ = 0;
	*p++ = 0;
	*p++ = 0;
	*p++ = 0;		/* 4 bytes unknown */

#ifdef NTLMV2_TEST
	*p++ = 0xf0;
	*p++ = 0x20;
	*p++ = 0xd0;
	*p++ = 0xb6;
	*p++ = 0xc2;
	*p++ = 0x92;
	*p++ = 0xbe;
	*p++ = 0x01;
#else
	nttime = time(nil);	/* nt time now */
	nttime = nttime + 11644473600LL;
	nttime = nttime * 10000000LL;
	*p++ = nttime & 0xff;
	*p++ = (nttime >> 8) & 0xff;
	*p++ = (nttime >> 16) & 0xff;
	*p++ = (nttime >> 24) & 0xff;
	*p++ = (nttime >> 32) & 0xff;
	*p++ = (nttime >> 40) & 0xff;
	*p++ = (nttime >> 48) & 0xff;
	*p++ = (nttime >> 56) & 0xff;
#endif
#ifdef NTLMV2_TEST
	*p++ = 0x05;
	*p++ = 0x83;
	*p++ = 0x32;
	*p++ = 0xec;
	*p++ = 0xfa;
	*p++ = 0xe4;
	*p++ = 0xf3;
	*p++ = 0x6d;
#else
	genrandom(p, 8);
	p += 8;			/* client nonce */
#endif
	*p++ = 0x6f;
	*p++ = 0;
	*p++ = 0x6e;
	*p++ = 0;		/* unknown data */

	*p++ = Bdomain;
	*p++ = 0;		/* name type */

	n = utflen(windom) * 2;
	*p++ = n;
	*p++ = n >> 8;		/* name length */

	d = windom;
	while(*d && p - blob < len - 8){
		d += chartorune(&r, d);
		r = toupperrune(r);
		*p++ = r;
		*p++ = r >> 8;
	}

	*p++ = 0;
	*p++ = Beof;		/* name type */

	*p++ = 0;
	*p++ = 0;		/* name length */

	*p++ = 0x65;
	*p++ = 0;
	*p++ = 0;
	*p++ = 0;		/* unknown data */
	return p - blob;
}

static Auth *
auth_ntlmv2(char *windom, char *keyp, uchar *chal, int len)
{
	int i, n;
	Rune r;
	char *p, *u;
	uchar c, lm_hmac[MD5dlen], nt_hmac[MD5dlen], nt_sesskey[MD5dlen];
	uchar lm_sesskey[MD5dlen];
	uchar v1hash[MD5dlen], blip[Bliplen], blob[1024], v2hash[MD5dlen];
	DigestState *ds;
	UserPasswd *up;
	static Auth *ap;

	up = auth_getuserpasswd(auth_getkey, "windom=%s proto=pass service=cifs-ntlmv2 %s",
		windom, keyp);
	if(! up)
		sysfatal("cannot get key - %r");

#ifdef NTLMV2_TEST
{
	static uchar srvchal[] = { 0x52, 0xaa, 0xc8, 0xe8, 0x2c, 0x06, 0x7f, 0xa1 };
	up->user = "ADMINISTRATOR";
	windom = "rocknroll";
	chal = srvchal;
}
#endif
	ap = emalloc9p(sizeof(Auth));
	memset(ap, 0, sizeof(ap));

	/* Standard says unlimited length, experience says 128 max */
	if((n = strlen(up->passwd)) > 128)
		n = 128;

	ds = md4(nil, 0, nil, nil);
	for(i = 0, p = up->passwd; i < n; i++) {
		p += chartorune(&r, p);
		c = r;
		md4(&c, 1, nil, ds);
		c = r >> 8;
		md4(&c, 1, nil, ds);
	}
	md4(nil, 0, v1hash, ds);

#ifdef NTLMV2_TEST
{
	uchar v1[] = {
		0x0c, 0xb6, 0x94, 0x88, 0x05, 0xf7, 0x97, 0xbf,
		0x2a, 0x82, 0x80, 0x79, 0x73, 0xb8, 0x95, 0x37
	;
	memcpy(v1hash, v1, sizeof(v1));
}
#endif
	/*
	 * Some documentation insists that the username must be forced to
	 * uppercase, but the domain name should not be. Other shows both
	 * being forced to uppercase.  I am pretty sure this is irrevevant as
	 * the domain name passed from the remote server always seems to be in
	 * uppercase already.
	 */
        ds = hmac_t64(nil, 0, v1hash, MD5dlen, nil, nil);
	u = up->user;
	while(*u){
		u += chartorune(&r, u);
		r = toupperrune(r);
		c = r & 0xff;
        	hmac_t64(&c, 1, v1hash, MD5dlen, nil, ds);
		c = r >> 8;
        	hmac_t64(&c, 1, v1hash, MD5dlen, nil, ds);
	}
	u = windom;

	while(*u){
		u += chartorune(&r, u);
		c = r;
        	hmac_t64(&c, 1, v1hash, MD5dlen, nil, ds);
		c = r >> 8;
        	hmac_t64(&c, 1, v1hash, MD5dlen, nil, ds);
	}
        hmac_t64(nil, 0, v1hash, MD5dlen, v2hash, ds);
#ifdef NTLMV2_TEST
	print("want:               40 e1 b3 24...\n");
	dmp("v2hash==kr", 0, v2hash, MD5dlen);
#endif
	ap->user = estrdup9p(up->user);
	ap->windom = estrdup9p(windom);

	/* LM v2 */

	genrandom(blip, Bliplen);
#ifdef NTLMV2_TEST
{
	uchar t[] = { 0x05, 0x83, 0x32, 0xec, 0xfa, 0xe4, 0xf3, 0x6d };
	memcpy(blip, t, 8);
}
#endif
        ds = hmac_t64(chal, len, v2hash, MD5dlen, nil, nil);
	hmac_t64(blip, Bliplen, v2hash, MD5dlen, lm_hmac, ds);
	ap->len[0] = MD5dlen+Bliplen;
	ap->resp[0] = emalloc9p(ap->len[0]);
	memcpy(ap->resp[0], lm_hmac, MD5dlen);
	memcpy(ap->resp[0]+MD5dlen, blip, Bliplen);
#ifdef NTLMV2_TEST
	print("want:               38 6b ae...\n");
	dmp("lmv2 resp ", 0, lm_hmac, MD5dlen);
#endif

	/* LM v2 session key */
	hmac_t64(lm_hmac, MD5dlen, v2hash, MD5dlen, lm_sesskey, nil);

	/* LM v2 MAC key */
	ap->mackey[0] = emalloc9p(MACkeylen);
	memcpy(ap->mackey[0], lm_sesskey, MD5dlen);
	memcpy(ap->mackey[0]+MD5dlen, ap->resp[0], MACkeylen-MD5dlen);

	/* NTLM v2 */
	n = ntv2_blob(blob, sizeof(blob), windom);
        ds = hmac_t64(chal, len, v2hash, MD5dlen, nil, nil);
	hmac_t64(blob, n, v2hash, MD5dlen, nt_hmac, ds);
	ap->len[1] = MD5dlen+n;
	ap->resp[1] = emalloc9p(ap->len[1]);
	memcpy(ap->resp[1], nt_hmac, MD5dlen);
	memcpy(ap->resp[1]+MD5dlen, blob, n);
#ifdef NTLMV2_TEST
	print("want:               1a ad 55...\n");
	dmp("ntv2 resp ", 0, nt_hmac, MD5dlen);
#endif

	/* NTLM v2 session key */
	hmac_t64(nt_hmac, MD5dlen, v2hash, MD5dlen, nt_sesskey, nil);

	/* NTLM v2 MAC key */
	ap->mackey[1] = emalloc9p(MACkeylen);
	memcpy(ap->mackey[1], nt_sesskey, MD5dlen);
	memcpy(ap->mackey[1]+MD5dlen, ap->resp[1], MACkeylen-MD5dlen);
	free(up);

	return ap;
}

struct {
	char	*name;
	Auth	*(*func)(char *, char *, uchar *, int);
} methods[] = {
	{ "plain",	auth_plain },
	{ "lm+ntlm",	auth_lm_and_ntlm },
	{ "ntlm",	auth_ntlm },
	{ "ntlmv2",	auth_ntlmv2 },
//	{ "kerberos",	auth_kerberos },
};

void
autherr(void)
{
	int i;

	fprint(2, "supported auth methods:\t");
	for(i = 0; i < nelem(methods); i++)
		fprint(2, "%s ", methods[i].name);
	fprint(2, "\n");
	exits("usage");
}

Auth *
getauth(char *name, char *windom, char *keyp, int secmode, uchar *chal, int len)
{
	int i;
	Auth *ap;

	if(name == nil){
		name = DEF_AUTH;
		if((secmode & SECMODE_PW_ENCRYPT) == 0)
			sysfatal("plaintext authentication required, use '-a plain'");
	}

	ap = nil;
	for(i = 0; i < nelem(methods); i++)
		if(strcmp(methods[i].name, name) == 0){
			ap = methods[i].func(windom, keyp, chal, len);
			break;
		}

	if(! ap){
		fprint(2, "%s: %s - unknown auth method\n", argv0, name);
		autherr();		/* never returns */
	}
	return ap;
}

static int
genmac(uchar *buf, int len, int seq, uchar key[MACkeylen], uchar mine[MAClen])
{
	DigestState *ds;
	uchar *sig, digest[MD5dlen], their[MAClen];

	sig = buf+MACoff;
	memcpy(their, sig, MAClen);
	memset(sig, 0, MAClen);
	sig[0] = seq;
	sig[1] = seq >> 8;
	sig[2] = seq >> 16;
	sig[3] = seq >> 24;

	ds = md5(key, MACkeylen, nil, nil);
	md5(buf, len, nil, ds);
	md5(nil, 0, digest, ds);
	memcpy(mine, digest, MAClen);

	return memcmp(their, mine, MAClen);
}

int
macsign(Pkt *p)
{
	int i, len;
	uchar *sig, *buf, mac[MAClen], zeros[MACkeylen];

	sig = p->buf + NBHDRLEN + MACoff;
	buf = p->buf + NBHDRLEN;
	len = (p->pos - p->buf) - NBHDRLEN;

	for(i = -3; i < 4; i++){
		memset(zeros, 0, sizeof(zeros));
		if(genmac(buf, len, p->seq+i, zeros, mac) == 0){
			dmp("got", 0, buf, len);
			dmp("Zero OK", p->seq, mac, MAClen);
			return 0;
		}

		if(genmac(buf, len, p->seq+i, p->s->auth->mackey[0], mac) == 0){
			dmp("got", 0, buf, len);
			dmp("LM-hash OK", p->seq, mac, MAClen);
			return 0;
		}

		if(genmac(buf, len, p->seq+i, p->s->auth->mackey[1], mac) == 0){
			dmp("got", 0, buf, len);
			dmp("NT-hash OK", p->seq, mac, MAClen);
			return 0;
		}
	}
	genmac(buf, len, p->seq, p->s->auth->mackey[0], mac);

	memcpy(sig, mac, MAClen);
	return -1;
}
