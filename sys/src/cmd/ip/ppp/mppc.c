#include <u.h>
#include <libc.h>
#include <libsec.h>
#include <ip.h>
#include <auth.h>
#include "ppp.h"

enum {
	HistorySize=	8*1024,
	Cminmatch	= 3,		/* sintest match possible */
	Chshift		= 4,		/* nice compromise between space & time */
	Cnhash		= 1<<(Chshift*Cminmatch),
	HMASK		= Cnhash-1,
};

typedef struct Carena Carena;
struct Carena
{
	uchar	*pos;			/* current place, also amount of history filled */
	uchar	buf[HistorySize];
};

typedef struct Cstate Cstate;
struct Cstate
{
	QLock;
	int	count;
	int	reset;		/* compressor has been reset */
	int	front;		/* move to begining of history */
	ulong	sreg;		/* output shift reg */
	int	bits;		/* number of bits in sreg */
	Block	*b; 		/* output block */

	/*
	 * state for hashing compressor
	 */
	Carena	arenas[2];
	Carena	*hist;
	Carena	*ohist;
	ulong	hash[Cnhash];
	int	h;
	ulong	me;
	ulong	split;

	int	encrypt;
	uchar	startkey[16];
	uchar	key[16];
	RC4state rc4key;
};

typedef struct Uncstate Uncstate;
struct Uncstate
{
	int	count;	 	/* packet count - detects missing packets */
	int	resetid;	/* id of reset requests */
	uchar	his[HistorySize];
	int	indx;		/* current indx in history */
	int	size;		/* current history size */
	uchar	startkey[16];
	uchar	key[16];
	RC4state rc4key;
};

/* packet flags */
enum {
	Preset=		(1<<15),	/* reset history */
	Pfront=		(1<<14),	/* move packet to front of history */
	Pcompress=	(1<<13),	/* packet is compressed */
	Pencrypt=	(1<<12),	/* packet is encrypted */
};

enum {
	Lit7,		/* seven bit literal */
	Lit8,		/* eight bit literal */
	Off6,		/* six bit offset */
	Off8,		/* eight bit offset */
	Off13,		/* thirteen bit offset */
};

/* decode first four bits */
int decode[16] = {
	Lit7,	
	Lit7,
	Lit7,
	Lit7,
	Lit7,
	Lit7,
	Lit7,
	Lit7,
	Lit8,
	Lit8,	
	Lit8,
	Lit8,	
	Off13,
	Off13,
	Off8,
	Off6,
};
	

static	void		*compinit(PPP*);
static	Block*		comp(PPP*, ushort, Block*, int*);
static	void		comp2(Cstate*, uchar*, int);
static	Block		*compresetreq(void*, Block*);
static	void		compfini(void*);
static	void		complit(Cstate*, int);
static	void		compcopy(Cstate*, int, int);
static	void		compout(Cstate*, ulong, int);
static	void		compfront(Cstate*);
static	void		hashcheck(Cstate*);
static	void		compreset(Cstate*);
static	int		hashit(uchar*);


static	void		*uncinit(PPP*);
static	Block*		uncomp(PPP*, Block*, int *protop, Block**);
static	Block		*uncomp2(Uncstate *s, Block*, ushort);
static	void		uncfini(void*);
static	void		uncresetack(void*, Block*);
static  int		ipcheck(uchar*, int);
static  void		hischeck(Uncstate*);

static	void		setkey(uchar *key, uchar *startkey);

Comptype cmppc = {
	compinit,
	comp,
	compresetreq,
	compfini
};

Uncomptype uncmppc = {
	uncinit,
	uncomp,
	uncresetack,
	uncfini
};

static void *
compinit(PPP *ppp)
{
	Cstate *cs;

	cs = mallocz(sizeof(Cstate), 1);
	cs->hist = &cs->arenas[0];
	cs->ohist = &cs->arenas[1];
	compreset(cs);
	/*
	 * make reset clear the hash table
	 */
	cs->me = ~0;
	compreset(cs);

	cs->reset = 0;

	if(ppp->sendencrypted) {
		cs->encrypt = 1;
		memmove(cs->startkey, ppp->key, 16);
		memmove(cs->key, ppp->key, 16);
		setkey(cs->key, cs->startkey);
		setupRC4state(&cs->rc4key, cs->key, 16);
	}

	return cs;
}

static void
compfini(void *as)
{
	Cstate *cs;

	cs = as;
	free(cs);
}


static Block*
comp(PPP *ppp, ushort proto, Block *b, int *protop)
{
	Cstate *s;
	int n, n2;
	ushort count;

	s = ppp->cstate;
	*protop = 0;

	qlock(s);

	/* put protocol into b */
	b->rptr -= 2;
	if(b->rptr < b->base)
		sysfatal("mppc: not enough header in block");
	b->rptr[0] = proto>>8;
	b->rptr[1] = proto;

	n = BLEN(b);
	s->bits = 0;
	s->b = allocb(n*9/8+20);
	s->b->wptr += 2;	/* leave room for mppc header */

	comp2(s, b->rptr, n);
		
	/* flush sreg */
	if(s->bits)
		*s->b->wptr++ = s->sreg<<(8-s->bits);
	if(s->b->wptr > s->b->lim)
		sysfatal("mppc: comp: output block overflowed");

	n2 = BLEN(s->b);

	if(n2 > n-2 && !s->encrypt) {
		/* expened and not excrypting so send as a regular packet */
//netlog("mppc: comp: expanded\n"); 
		compreset(s);
		freeb(s->b);
		b->rptr += 2;
		qunlock(s);
		*protop = proto;
		return b;
	}

	count = s->count++;
	s->count &= 0xfff;
	if(s->front)
		count |= Pfront;
	if(s->reset)
		count |= Preset;
	s->reset = 0;
	s->front = 0;

	if(n2 > n) {
//netlog("mppc: comp: expanded\n"); 
		freeb(s->b);
		/* make room for count */
		compreset(s);
		b->rptr -= 2;
	} else {
		freeb(b);
		b = s->b;
		count |= Pcompress;
	}
	s->b = nil;

	if(s->encrypt) {
		count |= Pencrypt;
		if((count&0xff) == 0xff) {
//netlog("mppc: comp: changing key\n"); 
			setkey(s->key, s->startkey);
			setupRC4state(&s->rc4key, s->key, 16);
			rc4(&s->rc4key, s->key, 16);
			setupRC4state(&s->rc4key, s->key, 16);
		} else if(count&Preset)
			setupRC4state(&s->rc4key, s->key, 16);
		rc4(&s->rc4key, b->rptr+2, BLEN(b)-2);
//netlog("mppc: encrypt %ux\n", count);
	}

	b->rptr[0] = count>>8;
	b->rptr[1] = count;
	
	qunlock(s);

	*protop = Pcdata;
	return b;
}

static Block *
compresetreq(void *as, Block *b)
{
	Cstate *cs;

	cs = as;
netlog("mppc: comp: reset request\n");
	qlock(cs);
	compreset(cs);
	qunlock(cs);

	freeb(b);

	return nil;
}

static void
comp2(Cstate *cs, uchar *p, int n)
{
	Carena *hist, *ohist;
	ulong *hash, me, split, you, last;
	uchar *s, *t, *et, *buf, *obuf, *pos, *opos;
	int i, h, m;

	/*
	 * check for wrap
	 */
	if(cs->me + n < cs->me)
		compreset(cs);

	if(cs->hist->pos + n > cs->hist->buf + HistorySize)
		compfront(cs);

	hist = cs->hist;
	ohist = cs->ohist;

	hash = cs->hash;
	me = cs->me;
	split = cs->split;

	memmove(hist->pos, p, n);
	p = hist->pos;
	hist->pos = pos = p + n;

	m = Cminmatch;
	if(m > n)
		m = n;
	h = cs->h;
	for(i = 0; i < m; i++) {
		h = (((h)<<Chshift) ^ p[i]) & HMASK;
		last = me + (i - (Cminmatch-1));
		if(last >= split && last != me)
			hash[h] = last;
	}

	buf = hist->buf - split;
	obuf = ohist->buf + HistorySize - split;
	opos = ohist->pos;
	while(p < pos) {
		you = hash[h];
		if(you < split) {
			if(me - you >= HistorySize)
				t = opos;
			else
				t = obuf + you;
			et = opos;
		} else {
			t = buf + you;
			et = pos;
		}
		m = pos - p;
		if(m < et - t)
			et = t + m;
		for(s = p; t < et; t++) {
			if(*s != *t)
				break;
			s++;
		}
		m = s - p;
		if(m < Cminmatch) {
			complit(cs, *p);
			s = p + 1;
		} else
			compcopy(cs, me - you, m);

		for(; p != s; p++) {
			if(p + Cminmatch <= pos) {
				hash[h] = me;
				if(p + Cminmatch < pos)
					h = (((h)<<Chshift) ^ p[Cminmatch]) & HMASK;
			}
			me++;
		}
	}

	cs->h = h;
	cs->me = me;
}

static void
compfront(Cstate *cs)
{
	Carena *th;

	cs->front = 1;

	th = cs->ohist;
	cs->ohist = cs->hist;
	cs->hist = th;
	cs->hist->pos = cs->hist->buf;
	cs->h = 0;
	cs->me = cs->split + HistorySize;
	cs->split = cs->me;
}

static void
compreset(Cstate *cs)
{
	ulong me;

	cs->reset = 1;

	me = cs->me;
	if(me + 2 * HistorySize < me){
		me = 0;
		memset(cs->hash, 0, sizeof(cs->hash));
	}
	cs->me = me + 2 * HistorySize;
	cs->split = cs->me;
	cs->hist->pos = cs->hist->buf;
	cs->ohist->pos = cs->ohist->buf;
}

static void
complit(Cstate *s, int c)
{
	if(c&0x80)
		compout(s, 0x100|(c&0x7f), 9);
	else
		compout(s, c, 8);
}

static void
compcopy(Cstate *s, int off, int len)
{
	int i;
	ulong mask;

	if(off<64)
		compout(s, 0x3c0|off, 10);
	else if(off<320)
		compout(s, 0xe00|(off-64), 12);
	else
		compout(s, 0xc000|(off-320), 16);
	if(len < 3)
		sysfatal("compcopy: bad len: %d", len);
	if(len == 3)
		compout(s, 0, 1);
	else {
		for(i=3; (1<<i) <= len; i++)
			;
		mask = (1<<(i-1))-1;
		compout(s, (((1<<(i-2))-1)<<i) | len&mask, (i-1)<<1);
	}
}

static void
compout(Cstate *s, ulong data, int bits)
{
	ulong sreg;

	sreg = s->sreg;
	sreg <<= bits;
	sreg |= data;
	bits += s->bits;
	while(bits >= 8) {
		*s->b->wptr++ = sreg>>(bits-8);
		bits -= 8;
	}
	s->sreg = sreg;
	s->bits = bits;
}

void
printkey(uchar *key)
{
	char buf[200], *p;
	int i;

	p = buf;
	for(i=0; i<16; i++)
		p += sprint(p, "%.2ux ", key[i]);
//netlog("key = %s\n", buf);
}

static	void *
uncinit(PPP *ppp)
{
	Uncstate *s;

	s = mallocz(sizeof(Uncstate), 1);

	s->count = 0xfff;	/* count of non existant last packet */
	memmove(s->startkey, ppp->key, 16);
	memmove(s->key, ppp->key, 16);
	setkey(s->key, s->startkey);
	setupRC4state(&s->rc4key, s->key, 16);

	return s;
}

static	Block*
uncomp(PPP *ppp, Block *b, int *protop, Block **r)
{
	Uncstate *s;
	ushort proto;
	ushort count;
	Lcpmsg *m;

	*r = nil;
	*protop = 0;
	s = ppp->uncstate;
	if(BLEN(b) < 2){
		syslog(0, "ppp", ": mppc: short packet\n");
		freeb(b);
		return nil;
	}
	count = nhgets(b->rptr);
	b->rptr += 2;

	b = uncomp2(s, b, count);

	if(b == nil) {
//netlog("ppp: mppc: reset request\n");
		/* return reset request packet */
		*r = alloclcp(Lresetreq, s->resetid++, 4, &m);
		hnputs(m->len, 4);
		*protop = 0;
		return nil;
	}

	if(BLEN(b) < 2){
		syslog(0, "ppp", ": mppc: short packet\n");
		freeb(b);
		*protop = 0;
		return nil;
	}
	proto = nhgets(b->rptr);
	b->rptr += 2;

/*
	if(proto == 0x21)
		if(!ipcheck(b->rptr, BLEN(b)))
			hischeck(s);
*/

	*protop = proto;
	return b;
}

#define NEXTBYTE	sreg = (sreg<<8) | *p++; n--; bits += 8
int	maxoff;
	
static	Block*
uncomp2(Uncstate *s, Block *b, ushort count)
{
	int ecount, n, bits, off, len, ones;
	ulong sreg;
	int t;
	uchar *p, c, *hp, *hs, *he, *hq;

	if(count&Preset) {
//netlog("mppc reset\n");
		s->indx = 0;
		s->size = 0;
		setupRC4state(&s->rc4key, s->key, 16);
	} else {
		ecount = (s->count+1)&0xfff;
		if((count&0xfff) != ecount) {
netlog("******* bad count - got %ux expected %ux\n", count&0xfff, ecount);
			freeb(b);
			return nil;
		}
		if(count&Pfront) {
			s->indx = 0;
/*			netlog("ppp: mppc: frount flag set\n"); */
		}
	}

	/* update key */
	n = (((count+1)>>8)&0xf) - (((s->count+1)>>8)&0xf);
	if(n < 0)
		n += 16;
//netlog("mppc count = %ux oldcount %ux n = %d\n", count, s->count, n);
	if(n < 0 || n > 1) {
		syslog(0, "ppp", ": mppc bad count %ux, %ux", count, s->count);
		freeb(b);
		return nil;
	}
	if(n == 1) {
		setkey(s->key, s->startkey);
		setupRC4state(&s->rc4key, s->key, 16);
		rc4(&s->rc4key, s->key, 16);
		setupRC4state(&s->rc4key, s->key, 16);
	}
	
	s->count = count;

	n = BLEN(b);
	p = b->rptr;
	if(count & Pencrypt) {
//netlog("mppc unencrypt count = %ux\n", count);
		rc4(&s->rc4key, p, n);
	}

	if(!(count & Pcompress)) {
//netlog("uncompress blen = %d\n", BLEN(b));
		return  b;
	}

	bits = 0;
	sreg = 0;
	hs = s->his;		/* history start */
	hp = hs+s->indx;	/* write pointer in history */
	he = hs+sizeof(s->his);	/* hsitory end */
	for(;;) {
		if(bits<4) {
			if(n==0) goto Done;
			NEXTBYTE;
		}
		t = decode[(sreg>>(bits-4))&0xf];
		switch(t) {
		default:
			sysfatal("mppc: bad decode!");
		case Lit7:
			bits -= 1;
			if(bits<7) {
				if(n==0) goto Done;
				NEXTBYTE;
			}
			c = (sreg>>(bits-7))&0x7f;
			bits -= 7;
			if(hp >= he) goto His;
			*hp++ = c;
/* netlog("\tlit7 %.2ux\n", c); */
			continue;
		case Lit8:
			bits -= 2;
			if(bits<7) {
				if(n==0) goto Eof;
				NEXTBYTE;
			}
			c = 0x80 | ((sreg>>(bits-7))&0x7f);
			bits -= 7;
			if(hp >= he) goto His;
			*hp++ = c;
/* netlog("\tlit8 %.2ux\n", c); */
			continue;
		case Off6:
			bits -= 4;
			if(bits<6) {
				if(n==0) goto Eof;
				NEXTBYTE;
			}
			off = (sreg>>(bits-6))&0x3f;
			bits -= 6;
			break;
		case Off8:
			bits -= 4;
			if(bits<8) {
				if(n==0) goto Eof;
				NEXTBYTE;
			}
			off = ((sreg>>(bits-8))&0xff)+64;
			bits -= 8;
			break;
		case Off13:
			bits -= 3;
			while(bits<13) {
				if(n==0) goto Eof;
				NEXTBYTE;
			}
			off = ((sreg>>(bits-13))&0x1fff)+320;
			bits -= 13;
/* netlog("\toff=%d bits = %d sreg = %ux t = %x\n", off, bits, sreg, t); */
			break;
		}
		for(ones=0;;ones++) {
			if(bits == 0) {
				if(n==0) goto Eof;
				NEXTBYTE;
			}
			bits--;
			if(!(sreg&(1<<bits)))
				break;
		}
		if(ones>11) {
netlog("ppp: mppc: bad length %d\n", ones);
			freeb(b);
			return nil;
		}
		if(ones == 0) {
			len = 3;
		} else {
			ones++;
			while(bits<ones) {
				if(n==0) goto Eof;
				NEXTBYTE;
			}
			len = (1<<ones) | ((sreg>>(bits-ones))&((1<<ones)-1));
			bits -= ones;
		}

		hq = hp-off;
		if(hq < hs) {
			hq += sizeof(s->his);
			if(hq-hs+len > s->size) 
				goto His;
		}
		if(hp+len > he) goto His;
		while(len) {
			*hp++ = *hq++;
			len--;
		}
	}
Done:
	freeb(b);

	/* build up return block */
	hq = hs+s->indx;
	len = hp-hq;
	b = allocb(len);
	memmove(b->wptr, hq, len);
	b->wptr += len;
netlog("ppp: mppc: len %d bits = %d n=%d\n", len, bits, n);
	
	s->indx += len;
	if(s->indx > s->size)
		s->size = s->indx;
	
	return b;
Eof:
netlog("*****unexpected end of data\n");
	freeb(b);
	return nil;
His:
netlog("*****bad history\n");
	freeb(b);
	return nil;
}

static	void
uncresetack(void*, Block*)
{
}

static	void
uncfini(void *as)
{
	Uncstate *s;
	
	s = as;	
	free(s);
}

static void
setkey(uchar *key, uchar *startkey)
{
	uchar pad[40];
	SHAstate *s;
	uchar digest[SHA1dlen];

	s = sha1(startkey, 16, nil, nil);
	memset(pad, 0, 40);
	sha1(pad, 40, nil, s);
	sha1(key, 16, nil, s);
	memset(pad, 0xf2, 40);
	sha1(pad, 40, digest, s);
	memmove(key, digest, 16);
}


/* code to check if IP packet looks good */

typedef struct Iphdr Iphdr;
struct Iphdr
{
	uchar	vihl;		/* Version and header length */
	uchar	tos;		/* Type of service */
	uchar	length[2];	/* packet length */
	uchar	id[2];		/* Identification */
	uchar	frag[2];	/* Fragment information */
	uchar	ttl;		/* Time to live */
	uchar	proto;		/* Protocol */
	uchar	cksum[2];	/* Header checksum */
	uchar	src[4];		/* Ip source */
	uchar	dst[4];		/* Ip destination */
};

enum
{
	QMAX		= 64*1024-1,
	IP_TCPPROTO	= 6,
	TCP_IPLEN	= 8,
	TCP_PHDRSIZE	= 12,
	TCP_HDRSIZE	= 20,
	TCP_PKT		= TCP_IPLEN+TCP_PHDRSIZE,
};

enum
{
	UDP_PHDRSIZE	= 12,
	UDP_HDRSIZE	= 20,
	UDP_IPHDR	= 8,
	IP_UDPPROTO	= 17,
	UDP_USEAD	= 12,
	UDP_RELSIZE	= 16,

	Udprxms		= 200,
	Udptickms	= 100,
	Udpmaxxmit	= 10,
};

typedef struct UDPhdr UDPhdr;
struct UDPhdr
{
	/* ip header */
	uchar	vihl;		/* Version and header length */
	uchar	tos;		/* Type of service */
	uchar	length[2];	/* packet length */
	uchar	id[2];		/* Identification */
	uchar	frag[2];	/* Fragment information */
	uchar	Unused;	
	uchar	udpproto;	/* Protocol */
	uchar	udpplen[2];	/* Header plus data length */
	uchar	udpsrc[4];	/* Ip source */
	uchar	udpdst[4];	/* Ip destination */

	/* udp header */
	uchar	udpsport[2];	/* Source port */
	uchar	udpdport[2];	/* Destination port */
	uchar	udplen[2];	/* data length */
	uchar	udpcksum[2];	/* Checksum */
};

typedef struct TCPhdr TCPhdr;
struct TCPhdr
{
	uchar	vihl;		/* Version and header length */
	uchar	tos;		/* Type of service */
	uchar	length[2];	/* packet length */
	uchar	id[2];		/* Identification */
	uchar	frag[2];	/* Fragment information */
	uchar	Unused;
	uchar	proto;
	uchar	tcplen[2];
	uchar	tcpsrc[4];
	uchar	tcpdst[4];
	uchar	tcpsport[2];
	uchar	tcpdport[2];
	uchar	tcpseq[4];
	uchar	tcpack[4];
	uchar	tcpflag[2];
	uchar	tcpwin[2];
	uchar	tcpcksum[2];
	uchar	tcpurg[2];
	/* Options segment */
	uchar	tcpopt[2];
	uchar	tcpmss[2];
};

static void
hischeck(Uncstate *s)
{
	uchar *p;
	Iphdr *iph;
	int len;

	p = s->his;

netlog("***** history check\n");
	while(p < s->his+s->size) {
		if(p[0] != 0 || p[1] != 0x21) {
netlog("***** unknown protocol\n");
			return;
		}
		p += 2;
netlog("off = %ld ", p-s->his);
		iph = (Iphdr*)p;
		len = nhgets(iph->length);
		ipcheck(p, len);
		p += len;
	}
}


static int
ipcheck(uchar *p, int len)
{
	Iphdr *iph;
	TCPhdr *tcph;
	ushort length;
	UDPhdr *uh;
	Block *bp;
	ushort cksum;
	int good;

	bp = allocb(len);
	memmove(bp->wptr, p, len);
	bp->wptr += len;

	good = 1;

	iph = (Iphdr *)(bp->rptr);
/* netlog("ppp: mppc: ipcheck %I %I len %d proto %d\n", iph->src, iph->dst, BLEN(bp), iph->proto); */

	if(len != nhgets(iph->length)) {
		netlog("***** bad length! %d %d\n", len, nhgets(iph->length));
		good = 0;
	}

	cksum = ipcsum(&iph->vihl);
	if(cksum) {
		netlog("***** IP proto cksum!!! %I %ux\n", iph->src, cksum);
		good = 0;
	}

	switch(iph->proto) {
	default:
		break;
	case IP_TCPPROTO:

		tcph = (TCPhdr*)(bp->rptr);

		length = nhgets(tcph->length);

		tcph->Unused = 0;
		hnputs(tcph->tcplen, length-TCP_PKT);
		cksum = ptclcsum(bp, TCP_IPLEN, length-TCP_IPLEN);
		if(cksum) {
			netlog("***** bad tcp proto cksum %ux!!!\n", cksum);
			good = 0;
		}
		break;
	case IP_UDPPROTO:
		uh = (UDPhdr*)(bp->rptr);

		/* Put back pseudo header for checksum */
		uh->Unused = 0;
		len = nhgets(uh->udplen);
		hnputs(uh->udpplen, len);

		if(nhgets(uh->udpcksum)) {
			cksum = ptclcsum(bp, UDP_IPHDR, len+UDP_PHDRSIZE);
			if(cksum) {
				netlog("***** udp: proto cksum!!! %I %ux\n", uh->udpsrc, cksum);
				good = 0;
			}
		}
		break;
	}
	freeb(bp);
	return good;
}
