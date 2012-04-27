/*
 * Encapsulating Security Payload for IPsec for IPv4, rfc1827.
 * extended to IPv6.
 * rfc2104 defines hmac computation.
 *	currently only implements tunnel mode.
 * TODO: verify aes algorithms;
 *	transport mode (host-to-host)
 */
#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

#include	"ip.h"
#include	"ipv6.h"
#include	"libsec.h"

#define BITS2BYTES(bi) (((bi) + BI2BY - 1) / BI2BY)
#define BYTES2BITS(by)  ((by) * BI2BY)

typedef struct Algorithm Algorithm;
typedef struct Esp4hdr Esp4hdr;
typedef struct Esp6hdr Esp6hdr;
typedef struct Espcb Espcb;
typedef struct Esphdr Esphdr;
typedef struct Esppriv Esppriv;
typedef struct Esptail Esptail;
typedef struct Userhdr Userhdr;

enum {
	Encrypt,
	Decrypt,

	IP_ESPPROTO	= 50,	/* IP v4 and v6 protocol number */
	Esp4hdrlen	= IP4HDR + 8,
	Esp6hdrlen	= IP6HDR + 8,

	Esptaillen	= 2,	/* does not include pad or auth data */
	Userhdrlen	= 4,	/* user-visible header size - if enabled */

	Desblk	 = BITS2BYTES(64),
	Des3keysz = BITS2BYTES(192),

	Aesblk	 = BITS2BYTES(128),
	Aeskeysz = BITS2BYTES(128),
};

struct Esphdr
{
	uchar	espspi[4];	/* Security parameter index */
	uchar	espseq[4];	/* Sequence number */
	uchar	payload[];
};

/*
 * tunnel-mode (network-to-network, etc.) layout is:
 * new IP hdrs | ESP hdr |
 *	 enc { orig IP hdrs | TCP/UDP hdr | user data | ESP trailer } | ESP ICV
 *
 * transport-mode (host-to-host) layout would be:
 *	orig IP hdrs | ESP hdr |
 *			enc { TCP/UDP hdr | user data | ESP trailer } | ESP ICV
 */
struct Esp4hdr
{
	/* ipv4 header */
	uchar	vihl;		/* Version and header length */
	uchar	tos;		/* Type of service */
	uchar	length[2];	/* packet length */
	uchar	id[2];		/* Identification */
	uchar	frag[2];	/* Fragment information */
	uchar	Unused;
	uchar	espproto;	/* Protocol */
	uchar	espplen[2];	/* Header plus data length */
	uchar	espsrc[4];	/* Ip source */
	uchar	espdst[4];	/* Ip destination */

	Esphdr;
};

/* tunnel-mode layout */
struct Esp6hdr
{
	IPV6HDR;
	Esphdr;
};

struct Esptail
{
	uchar	pad;
	uchar	nexthdr;
};

/* IP-version-dependent data */
typedef struct Versdep Versdep;
struct Versdep
{
	ulong	version;
	ulong	iphdrlen;
	ulong	hdrlen;		/* iphdrlen + esp hdr len */
	ulong	spi;
	uchar	laddr[IPaddrlen];
	uchar	raddr[IPaddrlen];
};

/* header as seen by the user */
struct Userhdr
{
	uchar	nexthdr;	/* next protocol */
	uchar	unused[3];
};

struct Esppriv
{
	uvlong	in;
	ulong	inerrors;
};

/*
 *  protocol specific part of Conv
 */
struct Espcb
{
	int	incoming;
	int	header;		/* user-level header */
	ulong	spi;
	ulong	seq;		/* last seq sent */
	ulong	window;		/* for replay attacks */

	char	*espalg;
	void	*espstate;	/* other state for esp */
	int	espivlen;	/* in bytes */
	int	espblklen;
	int	(*cipher)(Espcb*, uchar *buf, int len);

	char	*ahalg;
	void	*ahstate;	/* other state for esp */
	int	ahlen;		/* auth data length in bytes */
	int	ahblklen;
	int	(*auth)(Espcb*, uchar *buf, int len, uchar *hash);
	DigestState *ds;
};

struct Algorithm
{
	char 	*name;
	int	keylen;		/* in bits */
	void	(*init)(Espcb*, char* name, uchar *key, unsigned keylen);
};

static	Conv* convlookup(Proto *esp, ulong spi);
static	char *setalg(Espcb *ecb, char **f, int n, Algorithm *alg);
static	void espkick(void *x);

static	void nullespinit(Espcb*, char*, uchar *key, unsigned keylen);
static	void des3espinit(Espcb*, char*, uchar *key, unsigned keylen);
static	void aescbcespinit(Espcb*, char*, uchar *key, unsigned keylen);
static	void aesctrespinit(Espcb*, char*, uchar *key, unsigned keylen);
static	void desespinit(Espcb *ecb, char *name, uchar *k, unsigned n);

static	void nullahinit(Espcb*, char*, uchar *key, unsigned keylen);
static	void shaahinit(Espcb*, char*, uchar *key, unsigned keylen);
static	void aesahinit(Espcb*, char*, uchar *key, unsigned keylen);
static	void md5ahinit(Espcb*, char*, uchar *key, unsigned keylen);

static Algorithm espalg[] =
{
	"null",		0,	nullespinit,
	"des3_cbc",	192,	des3espinit,	/* new rfc2451, des-ede3 */
	"aes_128_cbc",	128,	aescbcespinit,	/* new rfc3602 */
	"aes_ctr",	128,	aesctrespinit,	/* new rfc3686 */
	"des_56_cbc",	64,	desespinit,	/* rfc2405, deprecated */
	/* rc4 was never required, was used in original bandt */
//	"rc4_128",	128,	rc4espinit,
	nil,		0,	nil,
};

static Algorithm ahalg[] =
{
	"null",		0,	nullahinit,
	"hmac_sha1_96",	128,	shaahinit,	/* rfc2404 */
	"aes_xcbc_mac_96", 128,	aesahinit,	/* new rfc3566 */
	"hmac_md5_96",	128,	md5ahinit,	/* rfc2403 */
	nil,		0,	nil,
};

static char*
espconnect(Conv *c, char **argv, int argc)
{
	char *p, *pp, *e = nil;
	ulong spi;
	Espcb *ecb = (Espcb*)c->ptcl;

	switch(argc) {
	default:
		e = "bad args to connect";
		break;
	case 2:
		p = strchr(argv[1], '!');
		if(p == nil){
			e = "malformed address";
			break;
		}
		*p++ = 0;
		if (parseip(c->raddr, argv[1]) == -1) {
			e = Ebadip;
			break;
		}
		findlocalip(c->p->f, c->laddr, c->raddr);
		ecb->incoming = 0;
		ecb->seq = 0;
		if(strcmp(p, "*") == 0) {
			qlock(c->p);
			for(;;) {
				spi = nrand(1<<16) + 256;
				if(convlookup(c->p, spi) == nil)
					break;
			}
			qunlock(c->p);
			ecb->spi = spi;
			ecb->incoming = 1;
			qhangup(c->wq, nil);
		} else {
			spi = strtoul(p, &pp, 10);
			if(pp == p) {
				e = "malformed address";
				break;
			}
			ecb->spi = spi;
			qhangup(c->rq, nil);
		}
		nullespinit(ecb, "null", nil, 0);
		nullahinit(ecb, "null", nil, 0);
	}
	Fsconnected(c, e);

	return e;
}


static int
espstate(Conv *c, char *state, int n)
{
	return snprint(state, n, "%s", c->inuse?"Open\n":"Closed\n");
}

static void
espcreate(Conv *c)
{
	c->rq = qopen(64*1024, Qmsg, 0, 0);
	c->wq = qopen(64*1024, Qkick, espkick, c);
}

static void
espclose(Conv *c)
{
	Espcb *ecb;

	qclose(c->rq);
	qclose(c->wq);
	qclose(c->eq);
	ipmove(c->laddr, IPnoaddr);
	ipmove(c->raddr, IPnoaddr);

	ecb = (Espcb*)c->ptcl;
	free(ecb->espstate);
	free(ecb->ahstate);
	memset(ecb, 0, sizeof(Espcb));
}

static int
convipvers(Conv *c)
{
	if((memcmp(c->raddr, v4prefix, IPv4off) == 0 &&
	    memcmp(c->laddr, v4prefix, IPv4off) == 0) ||
	    ipcmp(c->raddr, IPnoaddr) == 0)
		return V4;
	else
		return V6;
}

static int
pktipvers(Fs *f, Block **bpp)
{
	if (*bpp == nil || BLEN(*bpp) == 0) {
		/* get enough to identify the IP version */
		*bpp = pullupblock(*bpp, IP4HDR);
		if(*bpp == nil) {
			netlog(f, Logesp, "esp: short packet\n");
			return 0;
		}
	}
	return (((Esp4hdr*)(*bpp)->rp)->vihl & 0xf0) == IP_VER4? V4: V6;
}

static void
getverslens(int version, Versdep *vp)
{
	vp->version = version;
	switch(vp->version) {
	case V4:
		vp->iphdrlen = IP4HDR;
		vp->hdrlen   = Esp4hdrlen;
		break;
	case V6:
		vp->iphdrlen = IP6HDR;
		vp->hdrlen   = Esp6hdrlen;
		break;
	default:
		panic("esp: getverslens version %d wrong", version);
	}
}

static void
getpktspiaddrs(uchar *pkt, Versdep *vp)
{
	Esp4hdr *eh4;
	Esp6hdr *eh6;

	switch(vp->version) {
	case V4:
		eh4 = (Esp4hdr*)pkt;
		v4tov6(vp->raddr, eh4->espsrc);
		v4tov6(vp->laddr, eh4->espdst);
		vp->spi = nhgetl(eh4->espspi);
		break;
	case V6:
		eh6 = (Esp6hdr*)pkt;
		ipmove(vp->raddr, eh6->src);
		ipmove(vp->laddr, eh6->dst);
		vp->spi = nhgetl(eh6->espspi);
		break;
	default:
		panic("esp: getpktspiaddrs vp->version %ld wrong", vp->version);
	}
}

/*
 * encapsulate next IP packet on x's write queue in IP/ESP packet
 * and initiate output of the result.
 */
static void
espkick(void *x)
{
	int nexthdr, payload, pad, align;
	uchar *auth;
	Block *bp;
	Conv *c = x;
	Esp4hdr *eh4;
	Esp6hdr *eh6;
	Espcb *ecb;
	Esptail *et;
	Userhdr *uh;
	Versdep vers;

	getverslens(convipvers(c), &vers);
	bp = qget(c->wq);
	if(bp == nil)
		return;

	qlock(c);
	ecb = c->ptcl;

	if(ecb->header) {
		/* make sure the message has a User header */
		bp = pullupblock(bp, Userhdrlen);
		if(bp == nil) {
			qunlock(c);
			return;
		}
		uh = (Userhdr*)bp->rp;
		nexthdr = uh->nexthdr;
		bp->rp += Userhdrlen;
	} else {
		nexthdr = 0;	/* what should this be? */
	}

	payload = BLEN(bp) + ecb->espivlen;

	/* Make space to fit ip header */
	bp = padblock(bp, vers.hdrlen + ecb->espivlen);
	getpktspiaddrs(bp->rp, &vers);

	align = 4;
	if(ecb->espblklen > align)
		align = ecb->espblklen;
	if(align % ecb->ahblklen != 0)
		panic("espkick: ahblklen is important after all");
	pad = (align-1) - (payload + Esptaillen-1)%align;

	/*
	 * Make space for tail
	 * this is done by calling padblock with a negative size
	 * Padblock does not change bp->wp!
	 */
	bp = padblock(bp, -(pad+Esptaillen+ecb->ahlen));
	bp->wp += pad+Esptaillen+ecb->ahlen;

	et = (Esptail*)(bp->rp + vers.hdrlen + payload + pad);

	/* fill in tail */
	et->pad = pad;
	et->nexthdr = nexthdr;

	/* encrypt the payload */
	ecb->cipher(ecb, bp->rp + vers.hdrlen, payload + pad + Esptaillen);
	auth = bp->rp + vers.hdrlen + payload + pad + Esptaillen;

	/* fill in head; construct a new IP header and an ESP header */
	if (vers.version == V4) {
		eh4 = (Esp4hdr *)bp->rp;
		eh4->vihl = IP_VER4;
		v6tov4(eh4->espsrc, c->laddr);
		v6tov4(eh4->espdst, c->raddr);
		eh4->espproto = IP_ESPPROTO;
		eh4->frag[0] = 0;
		eh4->frag[1] = 0;

		hnputl(eh4->espspi, ecb->spi);
		hnputl(eh4->espseq, ++ecb->seq);
	} else {
		eh6 = (Esp6hdr *)bp->rp;
		eh6->vcf[0] = IP_VER6;
		ipmove(eh6->src, c->laddr);
		ipmove(eh6->dst, c->raddr);
		eh6->proto = IP_ESPPROTO;

		hnputl(eh6->espspi, ecb->spi);
		hnputl(eh6->espseq, ++ecb->seq);
	}

	/* compute secure hash */
	ecb->auth(ecb, bp->rp + vers.iphdrlen, (vers.hdrlen - vers.iphdrlen) +
		payload + pad + Esptaillen, auth);

	qunlock(c);
	/* print("esp: pass down: %uld\n", BLEN(bp)); */
	if (vers.version == V4)
		ipoput4(c->p->f, bp, 0, c->ttl, c->tos, c);
	else
		ipoput6(c->p->f, bp, 0, c->ttl, c->tos, c);
}

/*
 * decapsulate IP packet from IP/ESP packet in bp and
 * pass the result up the spi's Conv's read queue.
 */
void
espiput(Proto *esp, Ipifc*, Block *bp)
{
	int payload, nexthdr;
	uchar *auth, *espspi;
	Conv *c;
	Espcb *ecb;
	Esptail *et;
	Fs *f;
	Userhdr *uh;
	Versdep vers;

	f = esp->f;

	getverslens(pktipvers(f, &bp), &vers);

	bp = pullupblock(bp, vers.hdrlen + Esptaillen);
	if(bp == nil) {
		netlog(f, Logesp, "esp: short packet\n");
		return;
	}
	getpktspiaddrs(bp->rp, &vers);

	qlock(esp);
	/* Look for a conversation structure for this port */
	c = convlookup(esp, vers.spi);
	if(c == nil) {
		qunlock(esp);
		netlog(f, Logesp, "esp: no conv %I -> %I!%lud\n", vers.raddr,
			vers.laddr, vers.spi);
		icmpnoconv(f, bp);
		freeblist(bp);
		return;
	}

	qlock(c);
	qunlock(esp);

	ecb = c->ptcl;
	/* too hard to do decryption/authentication on block lists */
	if(bp->next)
		bp = concatblock(bp);

	if(BLEN(bp) < vers.hdrlen + ecb->espivlen + Esptaillen + ecb->ahlen) {
		qunlock(c);
		netlog(f, Logesp, "esp: short block %I -> %I!%lud\n", vers.raddr,
			vers.laddr, vers.spi);
		freeb(bp);
		return;
	}

	auth = bp->wp - ecb->ahlen;
	espspi = vers.version == V4?	((Esp4hdr*)bp->rp)->espspi:
					((Esp6hdr*)bp->rp)->espspi;

	/* compute secure hash and authenticate */
	if(!ecb->auth(ecb, espspi, auth - espspi, auth)) {
		qunlock(c);
print("esp: bad auth %I -> %I!%ld\n", vers.raddr, vers.laddr, vers.spi);
		netlog(f, Logesp, "esp: bad auth %I -> %I!%lud\n", vers.raddr,
			vers.laddr, vers.spi);
		freeb(bp);
		return;
	}

	payload = BLEN(bp) - vers.hdrlen - ecb->ahlen;
	if(payload <= 0 || payload % 4 != 0 || payload % ecb->espblklen != 0) {
		qunlock(c);
		netlog(f, Logesp, "esp: bad length %I -> %I!%lud payload=%d BLEN=%lud\n",
			vers.raddr, vers.laddr, vers.spi, payload, BLEN(bp));
		freeb(bp);
		return;
	}

	/* decrypt payload */
	if(!ecb->cipher(ecb, bp->rp + vers.hdrlen, payload)) {
		qunlock(c);
print("esp: cipher failed %I -> %I!%ld: %s\n", vers.raddr, vers.laddr, vers.spi, up->errstr);
		netlog(f, Logesp, "esp: cipher failed %I -> %I!%lud: %s\n",
			vers.raddr, vers.laddr, vers.spi, up->errstr);
		freeb(bp);
		return;
	}

	payload -= Esptaillen;
	et = (Esptail*)(bp->rp + vers.hdrlen + payload);
	payload -= et->pad + ecb->espivlen;
	nexthdr = et->nexthdr;
	if(payload <= 0) {
		qunlock(c);
		netlog(f, Logesp, "esp: short packet after decrypt %I -> %I!%lud\n",
			vers.raddr, vers.laddr, vers.spi);
		freeb(bp);
		return;
	}

	/* trim packet */
	bp->rp += vers.hdrlen + ecb->espivlen; /* toss original IP & ESP hdrs */
	bp->wp = bp->rp + payload;
	if(ecb->header) {
		/* assume Userhdrlen < Esp4hdrlen < Esp6hdrlen */
		bp->rp -= Userhdrlen;
		uh = (Userhdr*)bp->rp;
		memset(uh, 0, Userhdrlen);
		uh->nexthdr = nexthdr;
	}

	/* ingress filtering here? */

	if(qfull(c->rq)){
		netlog(f, Logesp, "esp: qfull %I -> %I.%uld\n", vers.raddr,
			vers.laddr, vers.spi);
		freeblist(bp);
	}else {
//		print("esp: pass up: %uld\n", BLEN(bp));
		qpass(c->rq, bp);	/* pass packet up the read queue */
	}

	qunlock(c);
}

char*
espctl(Conv *c, char **f, int n)
{
	Espcb *ecb = c->ptcl;
	char *e = nil;

	if(strcmp(f[0], "esp") == 0)
		e = setalg(ecb, f, n, espalg);
	else if(strcmp(f[0], "ah") == 0)
		e = setalg(ecb, f, n, ahalg);
	else if(strcmp(f[0], "header") == 0)
		ecb->header = 1;
	else if(strcmp(f[0], "noheader") == 0)
		ecb->header = 0;
	else
		e = "unknown control request";
	return e;
}

/* called from icmp(v6) for unreachable hosts, time exceeded, etc. */
void
espadvise(Proto *esp, Block *bp, char *msg)
{
	Conv *c;
	Versdep vers;

	getverslens(pktipvers(esp->f, &bp), &vers);
	getpktspiaddrs(bp->rp, &vers);

	qlock(esp);
	c = convlookup(esp, vers.spi);
	if(c != nil) {
		qhangup(c->rq, msg);
		qhangup(c->wq, msg);
	}
	qunlock(esp);
	freeblist(bp);
}

int
espstats(Proto *esp, char *buf, int len)
{
	Esppriv *upriv;

	upriv = esp->priv;
	return snprint(buf, len, "%llud %lud\n",
		upriv->in,
		upriv->inerrors);
}

static int
esplocal(Conv *c, char *buf, int len)
{
	Espcb *ecb = c->ptcl;
	int n;

	qlock(c);
	if(ecb->incoming)
		n = snprint(buf, len, "%I!%uld\n", c->laddr, ecb->spi);
	else
		n = snprint(buf, len, "%I\n", c->laddr);
	qunlock(c);
	return n;
}

static int
espremote(Conv *c, char *buf, int len)
{
	Espcb *ecb = c->ptcl;
	int n;

	qlock(c);
	if(ecb->incoming)
		n = snprint(buf, len, "%I\n", c->raddr);
	else
		n = snprint(buf, len, "%I!%uld\n", c->raddr, ecb->spi);
	qunlock(c);
	return n;
}

static	Conv*
convlookup(Proto *esp, ulong spi)
{
	Conv *c, **p;
	Espcb *ecb;

	for(p=esp->conv; *p; p++){
		c = *p;
		ecb = c->ptcl;
		if(ecb->incoming && ecb->spi == spi)
			return c;
	}
	return nil;
}

static char *
setalg(Espcb *ecb, char **f, int n, Algorithm *alg)
{
	uchar *key;
	int c, nbyte, nchar;
	uint i;

	if(n < 2 || n > 3)
		return "bad format";
	for(; alg->name; alg++)
		if(strcmp(f[1], alg->name) == 0)
			break;
	if(alg->name == nil)
		return "unknown algorithm";

	nbyte = (alg->keylen + 7) >> 3;
	if (n == 2)
		nchar = 0;
	else
		nchar = strlen(f[2]);
	if(nchar != 2 * nbyte)			/* TODO: maybe < is ok */
		return "key not required length";
	/* convert hex digits from ascii, in place */
	for(i=0; i<nchar; i++) {
		c = f[2][i];
		if(c >= '0' && c <= '9')
			f[2][i] -= '0';
		else if(c >= 'a' && c <= 'f')
			f[2][i] -= 'a'-10;
		else if(c >= 'A' && c <= 'F')
			f[2][i] -= 'A'-10;
		else
			return "non-hex character in key";
	}
	/* collapse hex digits into complete bytes in reverse order in key */
	key = smalloc(nbyte);
	for(i = 0; i < nchar && i/2 < nbyte; i++) {
		c = f[2][nchar-i-1];
		if(i&1)
			c <<= 4;
		key[i/2] |= c;
	}

	alg->init(ecb, alg->name, key, alg->keylen);
	free(key);
	return nil;
}


/*
 * null encryption
 */

static int
nullcipher(Espcb*, uchar*, int)
{
	return 1;
}

static void
nullespinit(Espcb *ecb, char *name, uchar*, unsigned)
{
	ecb->espalg = name;
	ecb->espblklen = 1;
	ecb->espivlen = 0;
	ecb->cipher = nullcipher;
}

static int
nullauth(Espcb*, uchar*, int, uchar*)
{
	return 1;
}

static void
nullahinit(Espcb *ecb, char *name, uchar*, unsigned)
{
	ecb->ahalg = name;
	ecb->ahblklen = 1;
	ecb->ahlen = 0;
	ecb->auth = nullauth;
}


/*
 * sha1
 */

static void
seanq_hmac_sha1(uchar hash[SHA1dlen], uchar *t, long tlen, uchar *key, long klen)
{
	int i;
	uchar ipad[Hmacblksz+1], opad[Hmacblksz+1], innerhash[SHA1dlen];
	DigestState *digest;

	memset(ipad, 0x36, Hmacblksz);
	memset(opad, 0x5c, Hmacblksz);
	ipad[Hmacblksz] = opad[Hmacblksz] = 0;
	for(i = 0; i < klen; i++){
		ipad[i] ^= key[i];
		opad[i] ^= key[i];
	}
	digest = sha1(ipad, Hmacblksz, nil, nil);
	sha1(t, tlen, innerhash, digest);
	digest = sha1(opad, Hmacblksz, nil, nil);
	sha1(innerhash, SHA1dlen, hash, digest);
}

static int
shaauth(Espcb *ecb, uchar *t, int tlen, uchar *auth)
{
	int r;
	uchar hash[SHA1dlen];

	memset(hash, 0, SHA1dlen);
	seanq_hmac_sha1(hash, t, tlen, (uchar*)ecb->ahstate, BITS2BYTES(128));
	r = memcmp(auth, hash, ecb->ahlen) == 0;
	memmove(auth, hash, ecb->ahlen);
	return r;
}

static void
shaahinit(Espcb *ecb, char *name, uchar *key, unsigned klen)
{
	if(klen != 128)
		panic("shaahinit: bad keylen");
	klen /= BI2BY;

	ecb->ahalg = name;
	ecb->ahblklen = 1;
	ecb->ahlen = BITS2BYTES(96);
	ecb->auth = shaauth;
	ecb->ahstate = smalloc(klen);
	memmove(ecb->ahstate, key, klen);
}


/*
 * aes
 */

/* ah_aes_xcbc_mac_96, rfc3566 */
static int
aesahauth(Espcb *ecb, uchar *t, int tlen, uchar *auth)
{
	int r;
	uchar hash[AESdlen];

	memset(hash, 0, AESdlen);
	ecb->ds = hmac_aes(t, tlen, (uchar*)ecb->ahstate, BITS2BYTES(96), hash,
		ecb->ds);
	r = memcmp(auth, hash, ecb->ahlen) == 0;
	memmove(auth, hash, ecb->ahlen);
	return r;
}

static void
aesahinit(Espcb *ecb, char *name, uchar *key, unsigned klen)
{
	if(klen != 128)
		panic("aesahinit: keylen not 128");
	klen /= BI2BY;

	ecb->ahalg = name;
	ecb->ahblklen = 1;
	ecb->ahlen = BITS2BYTES(96);
	ecb->auth = aesahauth;
	ecb->ahstate = smalloc(klen);
	memmove(ecb->ahstate, key, klen);
}

static int
aescbccipher(Espcb *ecb, uchar *p, int n)	/* 128-bit blocks */
{
	uchar tmp[AESbsize], q[AESbsize];
	uchar *pp, *tp, *ip, *eip, *ep;
	AESstate *ds = ecb->espstate;

	ep = p + n;
	if(ecb->incoming) {
		memmove(ds->ivec, p, AESbsize);
		p += AESbsize;
		while(p < ep){
			memmove(tmp, p, AESbsize);
			aes_decrypt(ds->dkey, ds->rounds, p, q);
			memmove(p, q, AESbsize);
			tp = tmp;
			ip = ds->ivec;
			for(eip = ip + AESbsize; ip < eip; ){
				*p++ ^= *ip;
				*ip++ = *tp++;
			}
		}
	} else {
		memmove(p, ds->ivec, AESbsize);
		for(p += AESbsize; p < ep; p += AESbsize){
			pp = p;
			ip = ds->ivec;
			for(eip = ip + AESbsize; ip < eip; )
				*pp++ ^= *ip++;
			aes_encrypt(ds->ekey, ds->rounds, p, q);
			memmove(ds->ivec, q, AESbsize);
			memmove(p, q, AESbsize);
		}
	}
	return 1;
}

static void
aescbcespinit(Espcb *ecb, char *name, uchar *k, unsigned n)
{
	uchar key[Aeskeysz], ivec[Aeskeysz];
	int i;

	n = BITS2BYTES(n);
	if(n > Aeskeysz)
		n = Aeskeysz;
	memset(key, 0, sizeof(key));
	memmove(key, k, n);
	for(i = 0; i < Aeskeysz; i++)
		ivec[i] = nrand(256);
	ecb->espalg = name;
	ecb->espblklen = Aesblk;
	ecb->espivlen = Aesblk;
	ecb->cipher = aescbccipher;
	ecb->espstate = smalloc(sizeof(AESstate));
	setupAESstate(ecb->espstate, key, n /* keybytes */, ivec);
}

static int
aesctrcipher(Espcb *ecb, uchar *p, int n)	/* 128-bit blocks */
{
	uchar tmp[AESbsize], q[AESbsize];
	uchar *pp, *tp, *ip, *eip, *ep;
	AESstate *ds = ecb->espstate;

	ep = p + n;
	if(ecb->incoming) {
		memmove(ds->ivec, p, AESbsize);
		p += AESbsize;
		while(p < ep){
			memmove(tmp, p, AESbsize);
			aes_decrypt(ds->dkey, ds->rounds, p, q);
			memmove(p, q, AESbsize);
			tp = tmp;
			ip = ds->ivec;
			for(eip = ip + AESbsize; ip < eip; ){
				*p++ ^= *ip;
				*ip++ = *tp++;
			}
		}
	} else {
		memmove(p, ds->ivec, AESbsize);
		for(p += AESbsize; p < ep; p += AESbsize){
			pp = p;
			ip = ds->ivec;
			for(eip = ip + AESbsize; ip < eip; )
				*pp++ ^= *ip++;
			aes_encrypt(ds->ekey, ds->rounds, p, q);
			memmove(ds->ivec, q, AESbsize);
			memmove(p, q, AESbsize);
		}
	}
	return 1;
}

static void
aesctrespinit(Espcb *ecb, char *name, uchar *k, unsigned n)
{
	uchar key[Aesblk], ivec[Aesblk];
	int i;

	n = BITS2BYTES(n);
	if(n > Aeskeysz)
		n = Aeskeysz;
	memset(key, 0, sizeof(key));
	memmove(key, k, n);
	for(i = 0; i < Aesblk; i++)
		ivec[i] = nrand(256);
	ecb->espalg = name;
	ecb->espblklen = Aesblk;
	ecb->espivlen = Aesblk;
	ecb->cipher = aesctrcipher;
	ecb->espstate = smalloc(sizeof(AESstate));
	setupAESstate(ecb->espstate, key, n /* keybytes */, ivec);
}


/*
 * md5
 */

static void
seanq_hmac_md5(uchar hash[MD5dlen], uchar *t, long tlen, uchar *key, long klen)
{
	int i;
	uchar ipad[Hmacblksz+1], opad[Hmacblksz+1], innerhash[MD5dlen];
	DigestState *digest;

	memset(ipad, 0x36, Hmacblksz);
	memset(opad, 0x5c, Hmacblksz);
	ipad[Hmacblksz] = opad[Hmacblksz] = 0;
	for(i = 0; i < klen; i++){
		ipad[i] ^= key[i];
		opad[i] ^= key[i];
	}
	digest = md5(ipad, Hmacblksz, nil, nil);
	md5(t, tlen, innerhash, digest);
	digest = md5(opad, Hmacblksz, nil, nil);
	md5(innerhash, MD5dlen, hash, digest);
}

static int
md5auth(Espcb *ecb, uchar *t, int tlen, uchar *auth)
{
	uchar hash[MD5dlen];
	int r;

	memset(hash, 0, MD5dlen);
	seanq_hmac_md5(hash, t, tlen, (uchar*)ecb->ahstate, BITS2BYTES(128));
	r = memcmp(auth, hash, ecb->ahlen) == 0;
	memmove(auth, hash, ecb->ahlen);
	return r;
}

static void
md5ahinit(Espcb *ecb, char *name, uchar *key, unsigned klen)
{
	if(klen != 128)
		panic("md5ahinit: bad keylen");
	klen = BITS2BYTES(klen);
	ecb->ahalg = name;
	ecb->ahblklen = 1;
	ecb->ahlen = BITS2BYTES(96);
	ecb->auth = md5auth;
	ecb->ahstate = smalloc(klen);
	memmove(ecb->ahstate, key, klen);
}


/*
 * des, single and triple
 */

static int
descipher(Espcb *ecb, uchar *p, int n)
{
	DESstate *ds = ecb->espstate;

	if(ecb->incoming) {
		memmove(ds->ivec, p, Desblk);
		desCBCdecrypt(p + Desblk, n - Desblk, ds);
	} else {
		memmove(p, ds->ivec, Desblk);
		desCBCencrypt(p + Desblk, n - Desblk, ds);
	}
	return 1;
}

static int
des3cipher(Espcb *ecb, uchar *p, int n)
{
	DES3state *ds = ecb->espstate;

	if(ecb->incoming) {
		memmove(ds->ivec, p, Desblk);
		des3CBCdecrypt(p + Desblk, n - Desblk, ds);
	} else {
		memmove(p, ds->ivec, Desblk);
		des3CBCencrypt(p + Desblk, n - Desblk, ds);
	}
	return 1;
}

static void
desespinit(Espcb *ecb, char *name, uchar *k, unsigned n)
{
	uchar key[Desblk], ivec[Desblk];
	int i;

	n = BITS2BYTES(n);
	if(n > Desblk)
		n = Desblk;
	memset(key, 0, sizeof(key));
	memmove(key, k, n);
	for(i = 0; i < Desblk; i++)
		ivec[i] = nrand(256);
	ecb->espalg = name;
	ecb->espblklen = Desblk;
	ecb->espivlen = Desblk;

	ecb->cipher = descipher;
	ecb->espstate = smalloc(sizeof(DESstate));
	setupDESstate(ecb->espstate, key, ivec);
}

static void
des3espinit(Espcb *ecb, char *name, uchar *k, unsigned n)
{
	uchar key[3][Desblk], ivec[Desblk];
	int i;

	n = BITS2BYTES(n);
	if(n > Des3keysz)
		n = Des3keysz;
	memset(key, 0, sizeof(key));
	memmove(key, k, n);
	for(i = 0; i < Desblk; i++)
		ivec[i] = nrand(256);
	ecb->espalg = name;
	ecb->espblklen = Desblk;
	ecb->espivlen = Desblk;

	ecb->cipher = des3cipher;
	ecb->espstate = smalloc(sizeof(DES3state));
	setupDES3state(ecb->espstate, key, ivec);
}


/*
 * interfacing to devip
 */
void
espinit(Fs *fs)
{
	Proto *esp;

	esp = smalloc(sizeof(Proto));
	esp->priv = smalloc(sizeof(Esppriv));
	esp->name = "esp";
	esp->connect = espconnect;
	esp->announce = nil;
	esp->ctl = espctl;
	esp->state = espstate;
	esp->create = espcreate;
	esp->close = espclose;
	esp->rcv = espiput;
	esp->advise = espadvise;
	esp->stats = espstats;
	esp->local = esplocal;
	esp->remote = espremote;
	esp->ipproto = IP_ESPPROTO;
	esp->nc = Nchans;
	esp->ptclsize = sizeof(Espcb);

	Fsproto(fs, esp);
}
