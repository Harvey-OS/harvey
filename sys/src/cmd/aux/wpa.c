#include <u.h>
#include <libc.h>
#include <ip.h>
#include <mp.h>
#include <libsec.h>
#include <auth.h>

enum {
	PMKlen = 256/8,
	PTKlen = 512/8,
	GTKlen = 256/8,

	MIClen = 16,

	Noncelen = 32,
	Eaddrlen = 6,
};

enum {
	Fptk	= 1<<3,
	Fins	= 1<<6,
	Fack	= 1<<7,
	Fmic	= 1<<8,
	Fsec	= 1<<9,
	Ferr	= 1<<10,
	Freq	= 1<<11,
	Fenc	= 1<<12,

	Keydescrlen = 1+2+2+8+32+16+8+8+16+2,
};

typedef struct Keydescr Keydescr;
struct Keydescr
{
	uchar	type[1];
	uchar	flags[2];
	uchar	keylen[2];
	uchar	repc[8];
	uchar	nonce[32];
	uchar	eapoliv[16];
	uchar	rsc[8];
	uchar	id[8];
	uchar	mic[16];
	uchar	datalen[2];
	uchar	data[];
};

typedef struct Cipher Cipher;
struct Cipher
{
	char	*name;
	int	keylen;
};

typedef struct Eapconn Eapconn;
typedef struct TLStunn TLStunn;

struct Eapconn
{
	int	fd;
	int	version;

	uchar	type;
	uchar	smac[Eaddrlen];
	uchar	amac[Eaddrlen];

	TLStunn	*tunn;

	void	(*write)(Eapconn*, uchar *data, int datalen);
};

struct TLStunn
{
	int	fd;

	int	clientpid;
	int	readerpid;

	uchar	id;
	uchar	tp;
};

Cipher	tkip = { "tkip", 32 };
Cipher	ccmp = { "ccmp", 16 };

Cipher	*peercipher;
Cipher	*groupcipher;

int	forked;
int	prompt;
int	debug;
int	fd, cfd;
char	*dev;
enum {
	AuthNone,
	AuthPSK,
	AuthWPA,
};
int	authtype;
char	devdir[40];
uchar	ptk[PTKlen];
char	essid[32+1];
uvlong	lastrepc;

uchar rsntkipoui[4] = {0x00, 0x0F, 0xAC, 0x02};
uchar rsnccmpoui[4] = {0x00, 0x0F, 0xAC, 0x04};
uchar rsnapskoui[4] = {0x00, 0x0F, 0xAC, 0x02};
uchar rsnawpaoui[4] = {0x00, 0x0F, 0xAC, 0x01};

uchar	rsnie[] = {
	0x30,			/* RSN */
	0x14,			/* length */
	0x01, 0x00,		/* version 1 */
	0x00, 0x0F, 0xAC, 0x04,	/* group cipher suite CCMP */
	0x01, 0x00,		/* pairwise cipher suite count 1 */
	0x00, 0x0F, 0xAC, 0x04,	/* pairwise cipher suite CCMP */
	0x01, 0x00,		/* authentication suite count 1 */
	0x00, 0x0F, 0xAC, 0x02,	/* authentication suite PSK */
	0x00, 0x00,		/* capabilities */
};

uchar wpa1oui[4]    = {0x00, 0x50, 0xF2, 0x01};
uchar wpatkipoui[4] = {0x00, 0x50, 0xF2, 0x02};
uchar wpaapskoui[4] = {0x00, 0x50, 0xF2, 0x02};
uchar wpaawpaoui[4] = {0x00, 0x50, 0xF2, 0x01};

uchar	wpaie[] = {
	0xdd,			/* vendor specific */
	0x16,			/* length */
	0x00, 0x50, 0xf2, 0x01,	/* WPAIE type 1 */
	0x01, 0x00,		/* version 1 */
	0x00, 0x50, 0xf2, 0x02,	/* group cipher suite TKIP */
	0x01, 0x00,		/* pairwise cipher suite count 1 */
	0x00, 0x50, 0xf2, 0x02,	/* pairwise cipher suite TKIP */
	0x01, 0x00,		/* authentication suite count 1 */
	0x00, 0x50, 0xf2, 0x02,	/* authentication suite PSK */
};

void*
emalloc(int len)
{
	void *v;

	if((v = mallocz(len, 1)) == nil)
		sysfatal("malloc: %r");
	return v;
}

int
hextob(char *s, char **sp, uchar *b, int n)
{
	int r;

	n <<= 1;
	for(r = 0; r < n && *s; s++){
		*b <<= 4;
		if(*s >= '0' && *s <= '9')
			*b |= (*s - '0');
		else if(*s >= 'a' && *s <= 'f')
			*b |= 10+(*s - 'a');
		else if(*s >= 'A' && *s <= 'F')
			*b |= 10+(*s - 'A');
		else break;
		if((++r & 1) == 0)
			b++;
	}
	if(sp != nil)
		*sp = s;
	return r >> 1;
}

char*
getifstats(char *key, char *val, int nval)
{
	char buf[8*1024], *f[2], *p, *e;
	int fd, n;

	snprint(buf, sizeof(buf), "%s/ifstats", devdir);
	if((fd = open(buf, OREAD)) < 0)
		return nil;
	n = readn(fd, buf, sizeof(buf)-1);
	close(fd);
	if(n <= 0)
		return nil;
	buf[n] = 0;
	for(p = buf; (e = strchr(p, '\n')) != nil; p = e){
		*e++ = 0;
		if(gettokens(p, f, 2, "\t\r\n ") != 2)
			continue;
		if(strcmp(f[0], key) != 0)
			continue;
		strncpy(val, f[1], nval);
		val[nval-1] = 0;
		return val;
	}
	return nil;
}

char*
getessid(void)
{
	return getifstats("essid:", essid, sizeof(essid));
}

int
getbssid(uchar mac[Eaddrlen])
{
	char buf[64];

	if(getifstats("bssid:", buf, sizeof(buf)) != nil)
		return parseether(mac, buf);
	return -1;
}

int
connected(void)
{
	char status[1024];

	if(getifstats("status:", status, sizeof(status)) == nil)
		return 0;
	if(strcmp(status, "connecting") == 0)
		return 0;
	if(strcmp(status, "unauthenticated") == 0)
		return 0;
	if(debug)
		fprint(2, "status: %s\n", status);
	return 1;
}

int
buildrsne(uchar rsne[258])
{
	char buf[1024];
	uchar brsne[258];
	int brsnelen;
	uchar *p, *w, *e;
	int i, n;

	if(getifstats("brsne:", buf, sizeof(buf)) == nil)
		return 0;	/* not an error, might be old kernel */

	brsnelen = hextob(buf, nil, brsne, sizeof(brsne));
	if(brsnelen <= 4){
trunc:		sysfatal("invalid or truncated RSNE; brsne: %s", buf);
		return 0;
	}

	w = rsne;
	p = brsne;
	e = p + brsnelen;
	if(p[0] == 0x30){
		p += 2;

		/* RSN */
		*w++ = 0x30;
		*w++ = 0;	/* length */
	} else if(p[0] == 0xDD){
		p += 2;
		if((e - p) < 4 || memcmp(p, wpa1oui, 4) != 0){
			sysfatal("unrecognized WPAIE type; brsne: %s", buf);
			return 0;
		}

		/* WPA */
		*w++ = 0xDD;
		*w++ = 0;	/* length */

		memmove(w, wpa1oui, 4);
		w += 4;
		p += 4;
	} else {
		sysfatal("unrecognized RSNE type; brsne: %s", buf);
		return 0;
	}

	if((e - p) < 6)
		goto trunc;

	*w++ = *p++;		/* version */
	*w++ = *p++;

	if(rsne[0] == 0x30){
		if(memcmp(p, rsnccmpoui, 4) == 0)
			groupcipher = &ccmp;
		else if(memcmp(p, rsntkipoui, 4) == 0)
			groupcipher = &tkip;
		else {
			sysfatal("unrecognized RSN group cipher; brsne: %s", buf);
			return 0;
		}
	} else {
		if(memcmp(p, wpatkipoui, 4) != 0){
			sysfatal("unrecognized WPA group cipher; brsne: %s", buf);
			return 0;
		}
		groupcipher = &tkip;
	}

	memmove(w, p, 4);	/* group cipher */
	w += 4;
	p += 4;

	if((e - p) < 6)
		goto trunc;

	*w++ = 0x01;		/* # of peer ciphers */
	*w++ = 0x00;
	n = *p++;
	n |= *p++ << 8;

	if(n <= 0)
		goto trunc;

	peercipher = &tkip;
	for(i=0; i<n; i++){
		if((e - p) < 4)
			goto trunc;

		if(rsne[0] == 0x30 && memcmp(p, rsnccmpoui, 4) == 0 && peercipher == &tkip)
			peercipher = &ccmp;
		p += 4;
	}
	if(peercipher == &ccmp)
		memmove(w, rsnccmpoui, 4);
	else if(rsne[0] == 0x30)
		memmove(w, rsntkipoui, 4);
	else
		memmove(w, wpatkipoui, 4);
	w += 4;

	if((e - p) < 6)
		goto trunc;

	*w++ = 0x01;		/* # of auth suites */
	*w++ = 0x00;
	n = *p++;
	n |= *p++ << 8;

	if(n <= 0)
		goto trunc;

	for(i=0; i<n; i++){
		if((e - p) < 4)
			goto trunc;

		if(rsne[0] == 0x30){
			/* look for PSK oui */
			if(memcmp(p, rsnapskoui, 4) == 0)
				break;
			/* look for WPA oui */
			if(memcmp(p, rsnawpaoui, 4) == 0){
				authtype = AuthWPA;
				break;
			}
		} else {
			/* look for PSK oui */
			if(memcmp(p, wpaapskoui, 4) == 0)
				break;
			/* look for WPA oui */
			if(memcmp(p, wpaawpaoui, 4) == 0){
				authtype = AuthWPA;
				break;
			}
		}
		p += 4;
	}
	if(i >= n){
		sysfatal("auth suite is not PSK or WPA; brsne: %s", buf);
		return 0;
	}

	memmove(w, p, 4);
	w += 4;

	if(rsne[0] == 0x30){
		/* RSN caps */
		*w++ = 0x00;
		*w++ = 0x00;
	}

	rsne[1] = (w - rsne) - 2;
	return w - rsne;
}

char*
factotumattr(char *attr, char *fmt, ...)
{
	char buf[1024];
	va_list list;
	AuthRpc *rpc;
	char *val;
	Attr *a;
	int afd;

	if((afd = open("/mnt/factotum/rpc", ORDWR)) < 0)
		return nil;
	if((rpc = auth_allocrpc(afd)) == nil){
		close(afd);
		return nil;
	}
	va_start(list, fmt);
	vsnprint(buf, sizeof(buf), fmt, list);
	va_end(list);
	val = nil;
	if(auth_rpc(rpc, "start", buf, strlen(buf)) == 0){
		if((a = auth_attr(rpc)) != nil){
			if((val = _strfindattr(a, attr)) != nil)
				val = strdup(val);
			_freeattr(a);
		}
	}
	auth_freerpc(rpc);
	close(afd);

	return val;
}

void
freeup(UserPasswd *up)
{
	memset(up->user, 0, strlen(up->user));
	memset(up->passwd, 0, strlen(up->passwd));
	free(up);
}

char*
getidentity(void)
{
	static char *identity;
	char *s;

	s = nil;
	for(;;){
		if(getessid() == nil)
			break;
		if((s = factotumattr("user", "proto=pass service=wpa essid=%q", essid)) != nil)
			break;
		if((s = factotumattr("user", "proto=mschapv2 role=client service=wpa essid=%q", essid)) != nil)
			break;
		break;
	}
	if(s != nil){
		free(identity);
		identity = s;
	} else if(identity == nil)
		identity = strdup("anonymous");
	if(debug)
		fprint(2, "identity: %s\n", identity);
	return identity;
}

int
factotumctl(char *fmt, ...)
{
	va_list list;
	int fd, r, n;
	char *s;

	r = -1;
	if((fd = open("/mnt/factotum/ctl", OWRITE)) >= 0){
		va_start(list, fmt);
		s = vsmprint(fmt, list);
		va_end(list);
		if(s != nil){
			n = strlen(s);
			r = write(fd, s, n);
			memset(s, 0, n);
			free(s);
		}
		close(fd);
	}
	return r;
}

int
setpmk(uchar pmk[PMKlen])
{
	if(getessid() == nil)
		return -1;
	return factotumctl("key proto=wpapsk role=client essid=%q !password=%.*H\n", essid, PMKlen, pmk);
}

int
getptk(AuthGetkey *getkey,
	uchar smac[Eaddrlen], uchar amac[Eaddrlen], 
	uchar snonce[Noncelen],  uchar anonce[Noncelen], 
	uchar ptk[PTKlen])
{
	uchar buf[2*Eaddrlen + 2*Noncelen], *p;
	AuthRpc *rpc;
	int afd, ret;
	char *s;

	ret = -1;
	s = nil;
	rpc = nil;
	if((afd = open("/mnt/factotum/rpc", ORDWR)) < 0)
		goto out;
	if((rpc = auth_allocrpc(afd)) == nil)
		goto out;
	if((s = getessid()) == nil)
		goto out;
	if((s = smprint("proto=wpapsk role=client essid=%q", s)) == nil)
		goto out;
	if((ret = auth_rpc(rpc, "start", s, strlen(s))) != ARok)
		goto out;
	p = buf;
	memmove(p, smac, Eaddrlen); p += Eaddrlen;
	memmove(p, amac, Eaddrlen); p += Eaddrlen;
	memmove(p, snonce, Noncelen); p += Noncelen;
	memmove(p, anonce, Noncelen); p += Noncelen;
	if((ret = auth_rpc(rpc, "write", buf, p - buf)) != ARok)
		goto out;
	if((ret = auth_rpc(rpc, "read", nil, 0)) != ARok)
		goto out;
	if(rpc->narg != PTKlen){
		ret = -1;
		goto out;
	}
	memmove(ptk, rpc->arg, PTKlen);
	ret = 0;
out:
	if(getkey != nil){
		switch(ret){
		case ARneedkey:
		case ARbadkey:
			(*getkey)(rpc->arg);
			break;
		}
	}
	free(s);
	if(afd >= 0) close(afd);
	if(rpc != nil) auth_freerpc(rpc);
	return ret;
}

void
dumpkeydescr(Keydescr *kd)
{
	static struct {
		int	flag;
		char	*name;
	} flags[] = {
		Fptk,	"ptk",
		Fins,	"ins",
		Fack,	"ack",
		Fmic,	"mic",
		Fsec,	"sec",
		Ferr,	"err",
		Freq,	"req",
		Fenc,	"enc",
	};
	int i, f;

	f = kd->flags[0]<<8 | kd->flags[1];
	fprint(2, "type=%.*H vers=%d flags=%.*H ( ",
		sizeof(kd->type), kd->type, kd->flags[1] & 7,
		sizeof(kd->flags), kd->flags);
	for(i=0; i<nelem(flags); i++)
		if(flags[i].flag & f)
			fprint(2, "%s ", flags[i].name);
	fprint(2, ") len=%.*H\nrepc=%.*H nonce=%.*H\neapoliv=%.*H rsc=%.*H id=%.*H mic=%.*H\n",
		sizeof(kd->keylen), kd->keylen,
		sizeof(kd->repc), kd->repc,
		sizeof(kd->nonce), kd->nonce,
		sizeof(kd->eapoliv), kd->eapoliv,
		sizeof(kd->rsc), kd->rsc,
		sizeof(kd->id), kd->id,
		sizeof(kd->mic), kd->mic);
	i = kd->datalen[0]<<8 | kd->datalen[1];
	fprint(2, "data[%.4x]=%.*H\n", i, i, kd->data);
}

int
rc4unwrap(uchar key[16], uchar iv[16], uchar *data, int len)
{
	uchar seed[32];
	RC4state rs;

	memmove(seed, iv, 16);
	memmove(seed+16, key, 16);
	setupRC4state(&rs, seed, sizeof(seed));
	rc4skip(&rs, 256);
	rc4(&rs, data, len);
	return len;
}

int
aesunwrap(uchar *key, int nkey, uchar *data, int len)
{
	static uchar IV[8] = { 0xa6, 0xa6, 0xa6, 0xa6, 0xa6, 0xa6, 0xa6, 0xa6, };
	uchar B[16], *R;
	AESstate s;
	uint t;
	int n;

	len -= 8;
	if(len < 16 || (len % 8) != 0)
		return -1;
	n = len/8;
	t = n*6;
	setupAESstate(&s, key, nkey, 0);
	memmove(B, data, 8);
	memmove(data, data+8, n*8);
	do {
		for(R = data + (n - 1)*8; R >= data; t--, R -= 8){
			memmove(B+8, R, 8);
			B[7] ^= (t >> 0);
			B[6] ^= (t >> 8);
			B[5] ^= (t >> 16);
			B[4] ^= (t >> 24);
			aes_decrypt(s.dkey, s.rounds, B, B);
			memmove(R, B+8, 8);
		}
	} while(t > 0);
	if(memcmp(B, IV, 8) != 0)
		return -1;
	return n*8;
}

int
calcmic(Keydescr *kd, uchar *msg, int msglen)
{
	int vers;

	vers = kd->flags[1] & 7;
	memset(kd->mic, 0, MIClen);
	if(vers == 1){
		uchar digest[MD5dlen];

		hmac_md5(msg, msglen, ptk, 16, digest, nil);
		memmove(kd->mic, digest, MIClen);
		return 0;
	}
	if(vers == 2){
		uchar digest[SHA1dlen];

		hmac_sha1(msg, msglen, ptk, 16, digest, nil);
		memmove(kd->mic, digest, MIClen);
		return 0;
	}
	return -1;
}

int
checkmic(Keydescr *kd, uchar *msg, int msglen)
{
	uchar tmp[MIClen];

	memmove(tmp, kd->mic, MIClen);
	if(calcmic(kd, msg, msglen) != 0)
		return -1;
	return memcmp(tmp, kd->mic, MIClen) != 0;
}

void
fdwrite(Eapconn *conn, uchar *data, int len)
{
	if(write(conn->fd, data, len) != len)
		sysfatal("write: %r");
}

void
etherwrite(Eapconn *conn, uchar *data, int len)
{
	uchar *buf, *p;
	int n;

	if(debug)
		fprint(2, "\nreply(v%d,t%d) %E -> %E: ", conn->version, conn->type, conn->smac, conn->amac);
	n = 2*Eaddrlen + 2 + len;
	if(n < 60) n = 60;	/* ETHERMINTU */
	p = buf = emalloc(n);
	/* ethernet header */
	memmove(p, conn->amac, Eaddrlen); p += Eaddrlen;
	memmove(p, conn->smac, Eaddrlen); p += Eaddrlen;
	*p++ = 0x88;
	*p++ = 0x8e;
	/* eapol data */
	memmove(p, data, len);
	fdwrite(conn, buf, n);
	free(buf);
}

void
eapwrite(Eapconn *conn, uchar *data, int len)
{
	uchar *buf, *p;

	p = buf = emalloc(len + 4);
	/* eapol header */
	*p++ = conn->version;
	*p++ = conn->type;
	*p++ = len >> 8;
	*p++ = len;
	/* eap data */
	memmove(p, data, len); p += len;
	etherwrite(conn, buf, p - buf);
	free(buf);
}

void
replykey(Eapconn *conn, int flags, Keydescr *kd, uchar *data, int datalen)
{
	uchar buf[4096], *p = buf;

	/* eapol hader */
	*p++ = conn->version;
	*p++ = conn->type;
	datalen += Keydescrlen;
	*p++ = datalen >> 8;
	*p++ = datalen;
	datalen -= Keydescrlen;
	/* key header */
	memmove(p, kd, Keydescrlen);
	kd = (Keydescr*)p;
	kd->flags[0] = flags >> 8;
	kd->flags[1] = flags;
	kd->datalen[0] = datalen >> 8;
	kd->datalen[1] = datalen;
	/* key data */
	p = kd->data;
	memmove(p, data, datalen);
	p += datalen;
	/* mic */
	memset(kd->mic, 0, MIClen);
	if(flags & Fmic)
		calcmic(kd, buf, p - buf);
	etherwrite(conn, buf, p - buf);
	if(debug)
		dumpkeydescr(kd);
}

void
eapresp(Eapconn *conn, int code, int id, uchar *data, int len)
{
	uchar *buf, *p;

	len += 4;
	p = buf = emalloc(len);
	/* eap header */
	*p++ = code;
	*p++ = id;
	*p++ = len >> 8;
	*p++ = len;
	memmove(p, data, len-4);
	(*conn->write)(conn, buf, len);
	free(buf);

	if(debug)
		fprint(2, "eapresp(code=%d, id=%d, data=%.*H)\n", code, id, len-4, data);
}

void
tlsreader(TLStunn *tunn, Eapconn *conn)
{
	enum {
		Tlshdrsz = 5,
		TLStunnhdrsz = 6,
	};
	uchar *rec, *w, *p;
	int fd, n, css;

	fd = tunn->fd;
	rec = nil;
	css = 0;
Reset:
	w = rec;
	w += TLStunnhdrsz;
	for(;;w += n){
		if((p = realloc(rec, (w - rec) + Tlshdrsz)) == nil)
			break;
		w = p + (w - rec), rec = p;
		if(readn(fd, w, Tlshdrsz) != Tlshdrsz)
			break;
		n = w[3]<<8 | w[4];
		if(n < 1)
			break;
		if((p = realloc(rec, (w - rec) + Tlshdrsz+n)) == nil)
			break;
		w = p + (w - rec), rec = p;
		if(readn(fd, w+Tlshdrsz, n) != n)
			break;
		n += Tlshdrsz;	
		
		/* batch records that need to be send together */
		if(!css){
			/* Client Certificate */
			if(w[0] == 22 && w[5] == 11)
				continue;
			/* Client Key Exchange */
			if(w[0] == 22 && w[5] == 16)
				continue;
			/* Change Cipher Spec */
			if(w[0] == 20){
				css = 1;
				continue;
			}
		}

		/* do not forward alert, close connection */
		if(w[0] == 21)
			break;

		/* check if we'r still the tunnel for this connection */
		if(conn->tunn != tunn)
			break;

		/* flush records in encapsulation */
		p = rec + TLStunnhdrsz;
		w += n;
		n = w - p;
		*(--p) = n;
		*(--p) = n >> 8;
		*(--p) = n >> 16;
		*(--p) = n >> 24;
		*(--p) = 0x80;	/* flags: Length included */
		*(--p) = tunn->tp;

		eapresp(conn, 2, tunn->id, p, w - p);
		goto Reset;
	}
	free(rec);
}

void ttlsclient(int);
void peapclient(int);

void
eapreset(Eapconn *conn)
{
	TLStunn *tunn;

	tunn = conn->tunn;
	if(tunn == nil)
		return;
	if(debug)
		fprint(2, "eapreset: kill client %d\n", tunn->clientpid);
	conn->tunn = nil;
	postnote(PNPROC, tunn->clientpid, "kill");
}

int
tlswrap(int fd, char *label)
{
	TLSconn *tls;

	tls = emalloc(sizeof(TLSconn));
	if(debug)
		tls->trace = print;
	if(label != nil){
		/* tls client computes the 1024 bit MSK for us */
		tls->sessionType = "ttls";
		tls->sessionConst = label;
		tls->sessionKeylen = 128;
		tls->sessionKey = emalloc(tls->sessionKeylen);
	}
	fd = tlsClient(fd, tls);
	if(fd < 0)
		sysfatal("tlsClient: %r");
	if(label != nil && tls->sessionKey != nil){
		/*
		 * PMK is derived from MSK by taking the first 256 bits.
		 * we store the PMK into factotum with setpmk() associated
		 * with the current essid.
		 */
		if(setpmk(tls->sessionKey) < 0)
			sysfatal("setpmk: %r");

		/* destroy session key */
		memset(tls->sessionKey, 0, tls->sessionKeylen);
	}
	free(tls->cert);	/* TODO: check cert */
	free(tls->sessionID);
	free(tls->sessionKey);
	free(tls);
	return fd;
}

void
eapreq(Eapconn *conn, int code, int id, uchar *data, int datalen)
{
	TLStunn *tunn;
	int tp, frag;
	char *user;

	if(debug)
		fprint(2, "eapreq(code=%d, id=%d, data=%.*H)\n", code, id, datalen, data);

	switch(code){
	case 1:	/* Request */
		break;
	case 4:	/* NAK */
	case 3:	/* Success */
		eapreset(conn);
		if(code == 4 || debug)
			fprint(2, "%s: eap code %s\n", argv0, code == 3 ? "Success" : "NAK");
		return;
	default:
	unhandled:
		if(debug)
			fprint(2, "unhandled: %.*H\n", datalen < 0 ? 0 : datalen, data);
		return;
	}
	if(datalen < 1)
		goto unhandled;

	tp = data[0];
	switch(tp){
	case 1:		/* Identity */
		user = getidentity();
		datalen = 1+strlen(user);
		memmove(data+1, user, datalen-1);
		eapresp(conn, 2, id, data, datalen);
		return;
	case 2:
		fprint(2, "%s: eap error: %.*s\n",
			argv0, utfnlen((char*)data+1, datalen-1), (char*)data+1);
		return;
	case 33:	/* EAP Extensions (AVP) */
		if(debug)
			fprint(2, "eap extension: %.*H\n", datalen, data);
		eapresp(conn, 2, id, data, datalen);
		return;
	case 26:	/* MS-CHAP-V2 */
		data++;
		datalen--;
		if(datalen < 1)
			break;

		/* OpCode */	
		switch(data[0]){
		case 1:	/* Challenge */
			if(datalen > 4) {
				uchar cid, chal[16], resp[48];
				char user[256+1];
				int len;

				cid = data[1];
				len = data[2]<<8 | data[3];
				if(data[4] != sizeof(chal))
					break;
				if(len > datalen || (5 + data[4]) > len)
					break;
				memmove(chal, data+5, sizeof(chal));
				memset(user, 0, sizeof(user));
				memset(resp, 0, sizeof(resp));
				if(auth_respond(chal, sizeof(chal), user, sizeof(user), resp, sizeof(resp), nil,
					"proto=mschapv2 role=client service=wpa essid=%q", essid) < 0){
					fprint(2, "%s: eap mschapv2: auth_respond: %r\n", argv0);
					break;
				}
				len = 5 + sizeof(resp) + 1 + strlen(user);
				data[0] = 2;		/* OpCode - Response */
				data[1] = cid;		/* Identifier */
				data[2] = len >> 8;
				data[3] = len;
				data[4] = sizeof(resp)+1;	/* ValueSize */
				memmove(data+5, resp, sizeof(resp));
				data[5 + sizeof(resp)] = 0;	/* flags */
				strcpy((char*)&data[5 + sizeof(resp) + 1], user);

				*(--data) = tp, len++;
				eapresp(conn, 2, id, data, len);
				return;
			}
			break;

		case 3:	/* Success */
		case 4:	/* Failure */
			if(debug || data[0] == 4)
				fprint(2, "%s: eap mschapv2 %s: %.*s\n", argv0,
					data[0] == 3 ? "Success" : "Failure",
					datalen < 4 ? 0 : utfnlen((char*)data+4, datalen-4), (char*)data+4);
			*(--data) = tp;
			eapresp(conn, 2, id, data, 2);
			return;
		}
		break;

	case 21:	/* EAP-TTLS */
	case 25:	/* PEAP */
		if(datalen < 2)
			break;
		datalen -= 2;
		data++;
		tunn = conn->tunn;
		if(*data & 0x20){	/* flags: start */
			int p[2], pid;

			if(tunn != nil){
				if(tunn->id == id && tunn->tp == tp)
					break;	/* is retransmit, ignore */
				eapreset(conn);
			}
			if(pipe(p) < 0)
				sysfatal("pipe: %r");
			if((pid = fork()) == -1)
				sysfatal("fork: %r");
			if(pid == 0){
				close(p[0]);
				switch(tp){
				case 21:
					ttlsclient(p[1]);
					break;
				case 25:
					peapclient(p[1]);
					break;
				}
				exits(nil);
			}
			close(p[1]);
			tunn = emalloc(sizeof(TLStunn));
			tunn->tp = tp;
			tunn->id = id;
			tunn->fd = p[0];
			tunn->clientpid = pid;
			conn->tunn = tunn;
			if((pid = rfork(RFPROC|RFMEM)) == -1)
				sysfatal("fork: %r");
			if(pid == 0){
				tunn->readerpid = getpid();
				tlsreader(tunn, conn);
				if(conn->tunn == tunn)
					conn->tunn = nil;
				close(tunn->fd);
				free(tunn);
				exits(nil);
			}
			return;
		}
		if(tunn == nil)
			break;
		if(id <= tunn->id || tunn->tp != tp)
			break;
		tunn->id = id;
		frag = *data & 0x40;	/* flags: more fragments */
		if(*data & 0x80){	/* flags: length included */
			datalen -= 4;
			data += 4;
		}
		data++;
		if(datalen > 0)
			write(tunn->fd, data, datalen);
		if(frag || (tp == 25 && data[0] == 20)){	/* ack change cipher spec */
			data -= 2;
			data[0] = tp;
			data[1] = 0;
			eapresp(conn, 2, id, data, 2);
		}
		return;
	}
	goto unhandled;
}

int
avp(uchar *p, int n, int code, void *val, int len, int pad)
{
	pad = 8 + ((len + pad) & ~pad);	/* header + data + data pad */
	assert(((pad + 3) & ~3) <= n);
	p[0] = code >> 24;
	p[1] = code >> 16;
	p[2] = code >> 8;
	p[3] = code;
	p[4] = 2;
	p[5] = pad >> 16;
	p[6] = pad >> 8;
	p[7] = pad;
	memmove(p+8, val, len);
	len += 8;
	pad = (pad + 3) & ~3;	/* packet padding */
	memset(p+len, 0, pad - len);
	return pad;
}

enum {
	/* Avp Code */
	AvpUserName = 1,
	AvpUserPass = 2,
	AvpChapPass = 3,
	AvpChapChal = 60,
};

void
ttlsclient(int fd)
{
	uchar buf[4096];
	UserPasswd *up;
	int n;

	fd = tlswrap(fd, "ttls keying material");
	if((up = auth_getuserpasswd(nil, "proto=pass service=wpa essid=%q", essid)) == nil)
		sysfatal("auth_getuserpasswd: %r");
	n = avp(buf, sizeof(buf), AvpUserName, up->user, strlen(up->user), 0);
	n += avp(buf+n, sizeof(buf)-n, AvpUserPass, up->passwd, strlen(up->passwd), 15);
	freeup(up);
	write(fd, buf, n);
	memset(buf, 0, n);
}

void
peapwrite(Eapconn *conn, uchar *data, int len)
{
	assert(len >= 4);
	fdwrite(conn, data + 4, len - 4);
}

void
peapclient(int fd)
{
	static Eapconn conn;
	uchar buf[4096], *p;
	int n, id, code;

	conn.fd = fd = tlswrap(fd, "client EAP encryption");
	while((n = read(fd, p = buf, sizeof(buf))) > 0){
		if(n > 4 && (p[2] << 8 | p[3]) == n && p[4] == 33){
			code = p[0];
			id = p[1];
			p += 4, n -= 4;
			conn.write = fdwrite;
		} else {
			code = 1;
			id = 0;
			conn.write = peapwrite;
		}
		eapreq(&conn, code, id, p, n);
	}
}

void
usage(void)
{
	fprint(2, "%s: [-dp12] [-s essid] dev\n", argv0);
	exits("usage");
}

void
background(void)
{
	if(forked || debug)
		return;
	switch(rfork(RFNOTEG|RFREND|RFPROC|RFNOWAIT)){
	default:
		exits(nil);
	case -1:
		sysfatal("fork: %r");
		return;
	case 0:
		break;
	}
	forked = 1;
}

void
main(int argc, char *argv[])
{
	uchar mac[Eaddrlen], buf[4096], snonce[Noncelen], anonce[Noncelen];
	static uchar brsne[258];
	static Eapconn conn;
	char addr[128];
	uchar *rsne;
	int newptk; /* gate key reinstallation */
	int rsnelen;
	int n, try;

	quotefmtinstall();
	fmtinstall('H', encodefmt);
	fmtinstall('E', eipfmt);

	rsne = nil;
	rsnelen = -1;
	peercipher = nil;
	groupcipher = nil;

	ARGBEGIN {
	case 'd':
		debug = 1;
		break;
	case 'p':
		prompt = 1;
		break;
	case 's':
		strncpy(essid, EARGF(usage()), 32);
		break;
	case '1':
		rsne = wpaie;
		rsnelen = sizeof(wpaie);
		peercipher = &tkip;
		groupcipher = &tkip;
		break;
	case '2':
		rsne = rsnie;
		rsnelen = sizeof(rsnie);
		peercipher = &ccmp;
		groupcipher = &ccmp;
		break;
	default:
		usage();
	} ARGEND;

	if(*argv != nil)
		dev = *argv++;

	if(*argv != nil || dev == nil)
		usage();

	if(myetheraddr(mac, dev) < 0)
		sysfatal("can't get mac address: %r");

	snprint(addr, sizeof(addr), "%s!0x888e", dev);
	if((fd = dial(addr, nil, devdir, &cfd)) < 0)
		sysfatal("dial: %r");

	if(essid[0] != 0){
		if(fprint(cfd, "essid %q", essid) < 0)
			sysfatal("write essid: %r");
	} else if(prompt) {
		getessid();
		if(essid[0] == 0)
			sysfatal("no essid set");
	}
	if(!prompt)
		background();

Connect:
 	/* bss scan might not be complete yet, so check for 10 seconds.	*/
	for(try = 100; (forked || try >= 0) && !connected(); try--)
		sleep(100);

	authtype = AuthPSK;
	if(rsnelen <= 0 || rsne == brsne){
		rsne = brsne;
		rsnelen = buildrsne(rsne);
	}
	if(rsnelen > 0){
		if(debug)
			fprint(2, "rsne: %.*H\n", rsnelen, rsne);
		/*
		 * we use write() instead of fprint so the message gets written
		 * at once and not chunked up on fprint buffer.
		 */
		n = sprint((char*)buf, "auth %.*H", rsnelen, rsne);
		if(write(cfd, buf, n) != n)
			sysfatal("write auth: %r");
	} else {
		authtype = AuthNone;
	}

	conn.fd = fd;
	conn.write = eapwrite;
	conn.type = 1;	/* Start */
	conn.version = 1;
	memmove(conn.smac, mac, Eaddrlen);
	getbssid(conn.amac);

	if(prompt){
		UserPasswd *up;
		prompt = 0;
		switch(authtype){
		case AuthNone:
			print("no authentication required\n");
			break;
		case AuthPSK:
			/* dummy to for factotum keyprompt */
			genrandom(anonce, sizeof(anonce));
			genrandom(snonce, sizeof(snonce));
			getptk(auth_getkey, conn.smac, conn.amac, snonce, anonce, ptk);
			break;
		case AuthWPA:
			up = auth_getuserpasswd(auth_getkey, "proto=pass service=wpa essid=%q", essid);
			if(up != nil){
				factotumctl("key proto=mschapv2 role=client service=wpa"
					" essid=%q user=%q !password=%q\n",
					essid, up->user, up->passwd);
				freeup(up);
			}
			break;
		}
		background();
	}

	genrandom(ptk, sizeof(ptk));
	newptk = 0;

	lastrepc = 0ULL;
	for(;;){
		uchar *p, *e, *m;
		int proto, flags, vers, datalen;
		uvlong repc, rsc, tsc;
		Keydescr *kd;

		if((n = read(fd, buf, sizeof(buf))) < 0)
			sysfatal("read: %r");

		if(n == 0){
			if(debug)
				fprint(2, "got deassociation\n");
			eapreset(&conn);
			goto Connect;
		}

		p = buf;
		e = buf+n;
		if(n < 2*Eaddrlen + 2)
			continue;

		memmove(conn.smac, p, Eaddrlen); p += Eaddrlen;
		memmove(conn.amac, p, Eaddrlen); p += Eaddrlen;
		proto = p[0]<<8 | p[1]; p += 2;

		if(proto != 0x888e || memcmp(conn.smac, mac, Eaddrlen) != 0)
			continue;

		m = p;
		n = e - p;
		if(n < 4)
			continue;

		conn.version = p[0];
		if(conn.version != 0x01 && conn.version != 0x02)
			continue;
		conn.type = p[1];
		n = p[2]<<8 | p[3];
		p += 4;
		if(p+n > e)
			continue;
		e = p + n;

		if(debug)
			fprint(2, "\nrecv(v%d,t%d) %E <- %E: ", conn.version, conn.type, conn.smac, conn.amac);

		if(authtype == AuthNone)
			continue;

		if(conn.type == 0x00 && authtype == AuthWPA){
			uchar code, id;

			if(n < 4)
				continue;
			code = p[0];
			id = p[1];
			n = p[3] | p[2]<<8;
			if(n < 4 || p + n > e)
				continue;
			p += 4, n -= 4;
			eapreq(&conn, code, id, p, n);
			continue;
		}

		if(conn.type != 0x03)
			continue;

		if(n < Keydescrlen){
			if(debug)
				fprint(2, "bad kd size\n");
			continue;
		}
		kd = (Keydescr*)p;
		if(debug)
			dumpkeydescr(kd);

		if(kd->type[0] != 0xFE && kd->type[0] != 0x02)
			continue;

		vers = kd->flags[1] & 7;
		flags = kd->flags[0]<<8 | kd->flags[1];
		datalen = kd->datalen[0]<<8 | kd->datalen[1];
		if(kd->data + datalen > e)
			continue;

		if((flags & Fmic) == 0){
			if((flags & (Fptk|Fack)) != (Fptk|Fack))
				continue;

			memmove(anonce, kd->nonce, sizeof(anonce));
			genrandom(snonce, sizeof(snonce));
			if(getptk(nil, conn.smac, conn.amac, snonce, anonce, ptk) != 0){
				if(debug)
					fprint(2, "getptk: %r\n");
				continue;
			}
			/* allow installation of new keys */	
			newptk = 1;

			/* ack key exchange with mic */
			memset(kd->rsc, 0, sizeof(kd->rsc));
			memset(kd->eapoliv, 0, sizeof(kd->eapoliv));
			memmove(kd->nonce, snonce, sizeof(kd->nonce));
			replykey(&conn, (flags & ~(Fack|Fins)) | Fmic, kd, rsne, rsnelen);
		} else {
			uchar gtk[GTKlen];
			int gtklen, gtkkid;

			if(checkmic(kd, m, e - m) != 0){
				if(debug)
					fprint(2, "bad mic\n");
				continue;
			}

			repc =	(uvlong)kd->repc[7] |
				(uvlong)kd->repc[6]<<8 |
				(uvlong)kd->repc[5]<<16 |
				(uvlong)kd->repc[4]<<24 |
				(uvlong)kd->repc[3]<<32 |
				(uvlong)kd->repc[2]<<40 |
				(uvlong)kd->repc[1]<<48 |
				(uvlong)kd->repc[0]<<56;
			if(repc <= lastrepc){
				if(debug)
					fprint(2, "bad repc: %llux <= %llux\n", repc, lastrepc);
				continue;
			}
			lastrepc = repc;

			rsc =	(uvlong)kd->rsc[0] |
				(uvlong)kd->rsc[1]<<8 |
				(uvlong)kd->rsc[2]<<16 |
				(uvlong)kd->rsc[3]<<24 |
				(uvlong)kd->rsc[4]<<32 |
				(uvlong)kd->rsc[5]<<40;

			if(datalen > 0 && (flags & Fenc) != 0){
				if(vers == 1)
					datalen = rc4unwrap(ptk+16, kd->eapoliv, kd->data, datalen);
				else
					datalen = aesunwrap(ptk+16, 16, kd->data, datalen);
				if(datalen <= 0){
					if(debug)
						fprint(2, "bad keywrap\n");
					continue;
				}
				if(debug)
					fprint(2, "unwraped keydata[%.4x]=%.*H\n", datalen, datalen, kd->data);
			}

			gtklen = 0;
			gtkkid = -1;

			if(kd->type[0] != 0xFE || (flags & (Fptk|Fack)) == (Fptk|Fack)){
				uchar *p, *x, *e;

				p = kd->data;
				e = p + datalen;
				for(; p+2 <= e; p = x){
					if((x = p+2+p[1]) > e)
						break;
					if(debug)
						fprint(2, "ie=%.2x data[%.2x]=%.*H\n", p[0], p[1], p[1], p+2);
					if(p[0] == 0x30){ /* RSN */
					}
					if(p[0] == 0xDD){ /* WPA */
						static uchar oui[] = { 0x00, 0x0f, 0xac, 0x01, };

						if(p+2+sizeof(oui) > x || memcmp(p+2, oui, sizeof(oui)) != 0)
							continue;
						if((flags & Fenc) == 0)
							continue;	/* ignore gorup key if unencrypted */
						gtklen = x - (p + 8);
						if(gtklen <= 0)
							continue;
						if(gtklen > sizeof(gtk))
							gtklen = sizeof(gtk);
						memmove(gtk, p + 8, gtklen);
						gtkkid = p[6] & 3;
					}
				}
			}

			if((flags & (Fptk|Fack)) == (Fptk|Fack)){
				if(!newptk)	/* a retransmit, already installed PTK */
					continue;
				if(vers != 1)	/* in WPA2, RSC is for group key only */
					tsc = 0LL;
				else {
					tsc = rsc;
					rsc = 0LL;
				}
				/* install pairwise receive key (PTK) */
				if(fprint(cfd, "rxkey %E %s:%.*H@%llux", conn.amac,
					peercipher->name, peercipher->keylen, ptk+32, tsc) < 0)
					sysfatal("write rxkey: %r");

				memset(kd->rsc, 0, sizeof(kd->rsc));
				memset(kd->eapoliv, 0, sizeof(kd->eapoliv));
				memset(kd->nonce, 0, sizeof(kd->nonce));
				replykey(&conn, flags & ~(Fack|Fenc|Fins), kd, nil, 0);
				sleep(100);

				tsc = 0LL;
				/* install pairwise transmit key (PTK) */ 
				if(fprint(cfd, "txkey %E %s:%.*H@%llux", conn.amac,
					peercipher->name, peercipher->keylen, ptk+32, tsc) < 0)
					sysfatal("write txkey: %r");
				newptk = 0; /* prevent PTK re-installation on (replayed) retransmits */
			} else
			if((flags & (Fptk|Fsec|Fack)) == (Fsec|Fack)){
				if(kd->type[0] == 0xFE){
					/* WPA always RC4 encrypts the GTK, even tho the flag isnt set */
					if((flags & Fenc) == 0)
						datalen = rc4unwrap(ptk+16, kd->eapoliv, kd->data, datalen);
					gtklen = datalen;
					if(gtklen > sizeof(gtk))
						gtklen = sizeof(gtk);
					memmove(gtk, kd->data, gtklen);
					gtkkid = (flags >> 4) & 3;
				}
				memset(kd->rsc, 0, sizeof(kd->rsc));
				memset(kd->eapoliv, 0, sizeof(kd->eapoliv));
				memset(kd->nonce, 0, sizeof(kd->nonce));
				replykey(&conn, flags & ~(Fenc|Fack), kd, nil, 0);
			} else
				continue;
			/* install group key (GTK) */
			if(gtklen >= groupcipher->keylen && gtkkid != -1)
				if(fprint(cfd, "rxkey%d %E %s:%.*H@%llux",
					gtkkid, conn.amac, 
					groupcipher->name, groupcipher->keylen, gtk, rsc) < 0)
					sysfatal("write rxkey%d: %r", gtkkid);
		}
	}
}
