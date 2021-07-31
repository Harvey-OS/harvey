/*
 *  devssl - secure sockets layer emulation
 */
#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

#include	<libsec.h>

#define NOSPOOKS 1

typedef struct OneWay OneWay;
struct OneWay
{
	QLock	q;
	QLock	ctlq;

	void	*state;		/* encryption state */
	int	slen;		/* hash data length */
	uchar	*secret;	/* secret */
	ulong	mid;		/* message id */
};

enum
{
	/* connection states */
	Sincomplete=	0,
	Sclear=		1,
	Sencrypting=	2,
	Sdigesting=	4,
	Sdigenc=	Sencrypting|Sdigesting,

	/* encryption algorithms */
	Noencryption=	0,
	DESCBC=		1,
	DESECB=		2,
	RC4=		3
};

typedef struct Dstate Dstate;
struct Dstate
{
	Chan	*c;		/* io channel */
	uchar	state;		/* state of connection */
	int	ref;		/* serialized by dslock for atomic destroy */

	uchar	encryptalg;	/* encryption algorithm */
	ushort	blocklen;	/* blocking length */

	ushort	diglen;		/* length of digest */
	DigestState *(*hf)(uchar*, ulong, uchar*, DigestState*);	/* hash func */

	/* for SSL format */
	int	max;		/* maximum unpadded data per msg */
	int	maxpad;		/* maximum padded data per msg */

	/* input side */
	OneWay	in;
	Block	*processed;
	Block	*unprocessed;

	/* output side */
	OneWay	out;

	/* protections */
	char	user[NAMELEN];
	int	perm;
};

Lock	dslock;
int	dshiwat;
int	maxdstate = 128;
Dstate** dstate;
char	*encalgs;
char	*hashalgs;

enum
{
	Maxdmsg=	1<<16,
	Maxdstate=	64
};

enum{
	Qtopdir		= 1,	/* top level directory */
	Qprotodir,
	Qclonus,
	Qconvdir,		/* directory for a conversation */
	Qdata,
	Qctl,
	Qsecretin,
	Qsecretout,
	Qencalgs,
	Qhashalgs,
};

#define TYPE(x) 	((x).path & 0xf)
#define CONV(x) 	(((x).path >> 5)&(Maxdstate-1))
#define QID(c, y) 	(((c)<<5) | (y))

static void	ensure(Dstate*, Block**, int);
static void	consume(Block**, uchar*, int);
static void	setsecret(OneWay*, uchar*, int);
static Block*	encryptb(Dstate*, Block*, int);
static Block*	decryptb(Dstate*, Block*);
static Block*	digestb(Dstate*, Block*, int);
static void	checkdigestb(Dstate*, Block*);
static Chan*	buftochan(char*);
static void	sslhangup(Dstate*);
static Dstate*	dsclone(Chan *c);
static void	dsnew(Chan *c, Dstate **);

char *sslnames[] = {
[Qclonus]	"clone",
[Qdata]		"data",
[Qctl]		"ctl",
[Qsecretin]	"secretin",
[Qsecretout]	"secretout",
[Qencalgs]	"encalgs",
[Qhashalgs]	"hashalgs",
};

static int
sslgen(Chan *c, Dirtab *d, int nd, int s, Dir *dp)
{
	Qid q;
	Dstate *ds;
	char name[16], *p, *nm;

	USED(nd);
	USED(d);
	q.vers = 0;
	switch(TYPE(c->qid)) {
	case Qtopdir:
		if(s == DEVDOTDOT){
			q.path = QID(0, Qtopdir)|CHDIR;
			devdir(c, q, "#D", 0, eve, CHDIR|0555, dp);
			return 1;
		}
		if(s > 0)
			return -1;
		q.path = QID(0, Qprotodir)|CHDIR;
		devdir(c, q, "ssl", 0, eve, CHDIR|0555, dp);
		return 1;
	case Qprotodir:
		if(s == DEVDOTDOT){
			q.path = QID(0, Qtopdir)|CHDIR;
			devdir(c, q, ".", 0, eve, CHDIR|0555, dp);
			return 1;
		}
		if(s < dshiwat) {
			sprint(name, "%d", s);
			q.path = QID(s, Qconvdir)|CHDIR;
			ds = dstate[s];
			if(ds != 0)
				nm = ds->user;
			else
				nm = eve;
			devdir(c, q, name, 0, nm, CHDIR|0555, dp);
			return 1;
		}
		if(s > dshiwat)
			return -1;
		q.path = QID(0, Qclonus);
		devdir(c, q, "clone", 0, eve, 0555, dp);
		return 1;
	case Qconvdir:
		if(s == DEVDOTDOT){
			q.path = QID(0, Qprotodir)|CHDIR;
			devdir(c, q, "ssl", 0, eve, CHDIR|0555, dp);
			return 1;
		}
		ds = dstate[CONV(c->qid)];
		if(ds != 0)
			nm = ds->user;
		else
			nm = eve;
		switch(s) {
		default:
			return -1;
		case 0:
			q.path = QID(CONV(c->qid), Qctl);
			p = "ctl";
			break;
		case 1:
			q.path = QID(CONV(c->qid), Qdata);
			p = "data";
			break;
		case 2:
			q.path = QID(CONV(c->qid), Qsecretin);
			p = "secretin";
			break;
		case 3:
			q.path = QID(CONV(c->qid), Qsecretout);
			p = "secretout";
			break;
		case 4:
			q.path = QID(CONV(c->qid), Qencalgs);
			p = "encalgs";
			break;
		case 5:
			q.path = QID(CONV(c->qid), Qhashalgs);
			p = "hashalgs";
			break;
		}
		devdir(c, q, p, 0, nm, 0660, dp);
		return 1;
	case Qclonus:
		devdir(c, c->qid, sslnames[TYPE(c->qid)], 0, eve, 0555, dp);
		return 1;
	default:
		ds = dstate[CONV(c->qid)];
		devdir(c, c->qid, sslnames[TYPE(c->qid)], 0, ds->user, 0660, dp);
		return 1;
	}
	return -1;
}

static Chan*
sslattach(char *spec)
{
	Chan *c;

	c = devattach('D', spec);
	c->qid.path = QID(0, Qtopdir)|CHDIR;
	c->qid.vers = 0;
	return c;
}

static int
sslwalk(Chan *c, char *name)
{
	return devwalk(c, name, 0, 0, sslgen);
}

static void
sslstat(Chan *c, char *db)
{
	devstat(c, db, 0, 0, sslgen);
}

static Chan*
sslopen(Chan *c, int omode)
{
	Dstate *s, **pp;
	int perm;

	perm = 0;
	omode &= 3;
	switch(omode) {
	case OREAD:
		perm = 4;
		break;
	case OWRITE:
		perm = 2;
		break;
	case ORDWR:
		perm = 6;
		break;
	}

	switch(TYPE(c->qid)) {
	default:
		panic("sslopen");
	case Qtopdir:
	case Qprotodir:
	case Qconvdir:
		if(omode != OREAD)
			error(Eperm);
		break;
	case Qclonus:
		s = dsclone(c);
		if(s == 0)
			error(Enodev);
		break;
	case Qctl:
	case Qdata:
	case Qsecretin:
	case Qsecretout:
		if(waserror()) {
			unlock(&dslock);
			nexterror();
		}
		lock(&dslock);
		pp = &dstate[CONV(c->qid)];
		s = *pp;
		if(s == 0)
			dsnew(c, pp);
		else {
			if((perm & (s->perm>>6)) != perm
			   && (strcmp(up->user, s->user) != 0
			     || (perm & s->perm) != perm))
				error(Eperm);

			s->ref++;
		}
		unlock(&dslock);
		poperror();
		break;
	case Qencalgs:
	case Qhashalgs:
		if(omode != OREAD)
			error(Eperm);
		break;
	}
	c->mode = openmode(omode);
	c->flag |= COPEN;
	c->offset = 0;
	return c;
}

static void
sslwstat(Chan *c, char *dp)
{
	Dir d;
	Dstate *s;

	convM2D(dp, &d);

	s = dstate[CONV(c->qid)];
	if(s == 0)
		error(Ebadusefd);
	if(strcmp(s->user, up->user) != 0)
		error(Eperm);

	memmove(s->user, d.uid, NAMELEN);
	s->perm = d.mode;
}

static void
sslclose(Chan *c)
{
	Dstate *s;

	switch(TYPE(c->qid)) {
	case Qctl:
	case Qdata:
	case Qsecretin:
	case Qsecretout:
		if((c->flag & COPEN) == 0)
			break;

		s = dstate[CONV(c->qid)];
		if(s == 0)
			break;

		lock(&dslock);
		if(--s->ref > 0) {
			unlock(&dslock);
			break;
		}
		dstate[CONV(c->qid)] = 0;
		unlock(&dslock);

		sslhangup(s);
		if(s->c)
			cclose(s->c);
		if(s->in.secret)
			free(s->in.secret);
		if(s->out.secret)
			free(s->out.secret);
		if(s->in.state)
			free(s->in.state);
		if(s->out.state)
			free(s->out.state);
		free(s);

	}
}

/*
 *  make sure we have at least 'n' bytes in list 'l'
 */
static void
ensure(Dstate *s, Block **l, int n)
{
	int sofar, i;
	Block *b, *bl;

	sofar = 0;
	for(b = *l; b; b = b->next){
		sofar += BLEN(b);
		if(sofar >= n)
			return;
		l = &b->next;
	}

	while(sofar < n){
		bl = devtab[s->c->type]->bread(s->c, Maxdmsg, 0);
		if(bl == 0)
			error(Ehungup);
		*l = bl;
		i = 0;
		for(b = bl; b; b = b->next){
			i += BLEN(b);
			l = &b->next;
		}
		if(i == 0)
			error(Ehungup);
		sofar += i;
	}
}

/*
 *  copy 'n' bytes from 'l' into 'p' and free
 *  the bytes in 'l'
 */
static void
consume(Block **l, uchar *p, int n)
{
	Block *b;
	int i;

	for(; *l && n > 0; n -= i){
		b = *l;
		i = BLEN(b);
		if(i > n)
			i = n;
		memmove(p, b->rp, i);
		b->rp += i;
		p += i;
		if(BLEN(b) < 0)
			panic("consume");
		if(BLEN(b))
			break;
		*l = b->next;
		freeb(b);
	}
}

/*
 *  give back n bytes
 */
static void
regurgitate(Dstate *s, uchar *p, int n)
{
	Block *b;

	if(n <= 0)
		return;
	b = s->unprocessed;
	if(s->unprocessed == nil || b->rp - b->base < n) {
		b = allocb(n);
		memmove(b->wp, p, n);
		b->wp += n;
		b->next = s->unprocessed;
		s->unprocessed = b;
	} else {
		b->rp -= n;
		memmove(b->rp, p, n);
	}
}

/*
 *  remove at most n bytes from the queue, if discard is set
 *  dump the remainder
 */
static Block*
qremove(Block **l, int n, int discard)
{
	Block *nb, *b, *first;
	int i;

	first = *l;
	for(b = first; b; b = b->next){
		i = BLEN(b);
		if(i == n){
			if(discard){
				freeblist(b->next);
				*l = 0;
			} else
				*l = b->next;
			b->next = 0;
			return first;
		} else if(i > n){
			i -= n;
			if(discard){
				freeblist(b->next);
				b->wp -= i;
				*l = 0;
			} else {
				nb = allocb(i);
				memmove(nb->wp, b->rp+n, i);
				nb->wp += i;
				b->wp -= i;
				nb->next = b->next;
				*l = nb;
			}
			b->next = 0;
			if(BLEN(b) < 0)
				panic("qremove");
			return first;
		} else
			n -= i;
		if(BLEN(b) < 0)
			panic("qremove");
	}
	*l = 0;
	return first;
}

/*
 *  We can't let Eintr's lose data since the program
 *  doing the read may be able to handle it.  The only
 *  places Eintr is possible is during the read's in consume.
 *  Therefore, we make sure we can always put back the bytes
 *  consumed before the last ensure.
 */
static Block*
sslbread(Chan *c, long n, ulong)
{
	volatile struct { Dstate *s; } s;
	Block *b;
	uchar consumed[3];
	int nconsumed;
	int len, pad;

	s.s = dstate[CONV(c->qid)];
	if(s.s == 0)
		panic("sslbread");
	if(s.s->state == Sincomplete)
		error(Ebadusefd);

	nconsumed = 0;
	if(waserror()){
		if(strcmp(up->error, Eintr) != 0)
			regurgitate(s.s, consumed, nconsumed);
		qunlock(&s.s->in.q);
		nexterror();
	}
	qlock(&s.s->in.q);

	if(s.s->processed == 0){
		/* read in the whole message */
		ensure(s.s, &s.s->unprocessed, 2);
		consume(&s.s->unprocessed, consumed, 2);
		nconsumed = 2;
		if(consumed[0] & 0x80){
			len = ((consumed[0] & 0x7f)<<8) | consumed[1];
			ensure(s.s, &s.s->unprocessed, len);
			pad = 0;
		} else {
			len = ((consumed[0] & 0x3f)<<8) | consumed[1];
			ensure(s.s, &s.s->unprocessed, len+1);
			consume(&s.s->unprocessed, &consumed[2], 1);
			pad = consumed[2];
			if(pad > len){
				print("pad %d buf len %d\n", pad, len);
				error("bad pad in ssl message");
			}
		}
		USED(nconsumed);
		nconsumed = 0;

		/*  if an Eintr happens after this, we're screwed.  Make
		 *  sure nothing we call can sleep.  Luckily, allocb
		 *  won't sleep, it'll just error out.
		 */

		/* grab the next message and decode/decrypt it */
		b = qremove(&s.s->unprocessed, len, 0);

		if(waserror()){
			qunlock(&s.s->in.ctlq);
			if(b != nil)
				freeb(b);
			nexterror();
		}
		qlock(&s.s->in.ctlq);
		switch(s.s->state){
		case Sencrypting:
			b = decryptb(s.s, b);
			break;
		case Sdigesting:
			b = pullupblock(b, s.s->diglen);
			if(b == nil)
				error("ssl message too short");
			checkdigestb(s.s, b);
			b->rp += s.s->diglen;
			break;
		case Sdigenc:
			b = decryptb(s.s, b);
			b = pullupblock(b, s.s->diglen);
			if(b == nil)
				error("ssl message too short");
			checkdigestb(s.s, b);
			b->rp += s.s->diglen;
			len -= s.s->diglen;
			break;
		}

		/* remove pad */
		if(pad)
			s.s->processed = qremove(&b, len - pad, 1);
		else
			s.s->processed = b;
		b = nil;
		s.s->in.mid++;
		qunlock(&s.s->in.ctlq);
		poperror();
		USED(nconsumed);
	}

	/* return at most what was asked for */
	b = qremove(&s.s->processed, n, 0);

	qunlock(&s.s->in.q);
	poperror();

	return b;
}

static long
sslread(Chan *c, void *a, long n, vlong off)
{
	volatile struct { Block *b; } b;
	Block *nb;
	uchar *va;
	int i;
	char buf[128];
	ulong offset = off;

	if(c->qid.path & CHDIR)
		return devdirread(c, a, n, 0, 0, sslgen);

	switch(TYPE(c->qid)) {
	default:
		error(Ebadusefd);
	case Qctl:
		sprint(buf, "%lud", CONV(c->qid));
		return readstr(offset, a, n, buf);
	case Qdata:
		b.b = sslbread(c, n, offset);
		break;
	case Qencalgs:
		return readstr(offset, a, n, encalgs);
		break;
	case Qhashalgs:
		return readstr(offset, a, n, hashalgs);
		break;
	}

	if(waserror()){
		freeblist(b.b);
		nexterror();
	}

	n = 0;
	va = a;
	for(nb = b.b; nb; nb = nb->next){
		i = BLEN(nb);
		memmove(va+n, nb->rp, i);
		n += i;
	}

	freeblist(b.b);
	poperror();

	return n;
}

/*
 *  this algorithm doesn't have to be great since we're just
 *  trying to obscure the block fill
 */
static void
randfill(uchar *buf, int len)
{
	while(len-- > 0)
		*buf++ = nrand(256);
}

/*
 *  use SSL record format, add in count, digest and/or encrypt.
 *  the write is interruptable.  if it is interrupted, we'll
 *  get out of sync with the far side.  not much we can do about
 *  it since we don't know if any bytes have been written.
 */
static long
sslbwrite(Chan *c, Block *b, ulong offset)
{
	volatile struct { Dstate *s; } s;
	volatile struct { Block *b; } bb;
	Block *nb;
	int h, n, m, pad, rv;
	uchar *p;

	bb.b = b;
	s.s = dstate[CONV(c->qid)];
	if(s.s == 0)
		panic("sslbwrite");
	if(s.s->state == Sincomplete){
		freeb(b);
		error(Ebadusefd);
	}

	if(waserror()){
		qunlock(&s.s->out.q);
		if(bb.b != nil)
			freeb(bb.b);
		nexterror();
	}
	qlock(&s.s->out.q);

	rv = 0;
	while(bb.b){
		m = n = BLEN(bb.b);
		h = s.s->diglen + 2;

		/* trim to maximum block size */
		pad = 0;
		if(m > s.s->max){
			m = s.s->max;
		} else if(s.s->blocklen != 1){
			pad = (m + s.s->diglen)%s.s->blocklen;
			if(pad){
				if(m > s.s->maxpad){
					pad = 0;
					m = s.s->maxpad;
				} else {
					pad = s.s->blocklen - pad;
					h++;
				}
			}
		}

		rv += m;
		if(m != n){
			nb = allocb(m + h + pad);
			memmove(nb->wp + h, bb.b->rp, m);
			nb->wp += m + h;
			bb.b->rp += m;
		} else {
			/* add header space */
			nb = padblock(bb.b, h);
			bb.b = 0;
		}
		m += s.s->diglen;

		/* SSL style count */
		if(pad){
			nb = padblock(nb, -pad);
			randfill(nb->wp, pad);
			nb->wp += pad;
			m += pad;

			p = nb->rp;
			p[0] = (m>>8);
			p[1] = m;
			p[2] = pad;
			offset = 3;
		} else {
			p = nb->rp;
			p[0] = (m>>8) | 0x80;
			p[1] = m;
			offset = 2;
		}

		switch(s.s->state){
		case Sencrypting:
			nb = encryptb(s.s, nb, offset);
			break;
		case Sdigesting:
			nb = digestb(s.s, nb, offset);
			break;
		case Sdigenc:
			nb = digestb(s.s, nb, offset);
			nb = encryptb(s.s, nb, offset);
			break;
		}

		s.s->out.mid++;

		m = BLEN(nb);
		devtab[s.s->c->type]->bwrite(s.s->c, nb, s.s->c->offset);
		s.s->c->offset += m;
	}
	qunlock(&s.s->out.q);
	poperror();

	return rv;
}

static void
setsecret(OneWay *w, uchar *secret, int n)
{
	if(w->secret)
		free(w->secret);

	w->secret = smalloc(n);
	memmove(w->secret, secret, n);
	w->slen = n;
}

static void
initDESkey(OneWay *w)
{
	if(w->state){
		free(w->state);
		w->state = 0;
	}

	w->state = smalloc(sizeof(DESstate));
	if(w->slen >= 16)
		setupDESstate(w->state, w->secret, w->secret+8);
	else if(w->slen >= 8)
		setupDESstate(w->state, w->secret, 0);
	else
		error("secret too short");
}

/*
 *  40 bit DES is the same as 56 bit DES.  However,
 *  16 bits of the key are masked to zero.
 */
static void
initDESkey_40(OneWay *w)
{
	uchar key[8];

	if(w->state){
		free(w->state);
		w->state = 0;
	}

	if(w->slen >= 8){
		memmove(key, w->secret, 8);
		key[0] &= 0x0f;
		key[2] &= 0x0f;
		key[4] &= 0x0f;
		key[6] &= 0x0f;
	}

	w->state = malloc(sizeof(DESstate));
	if(w->slen >= 16)
		setupDESstate(w->state, key, w->secret+8);
	else if(w->slen >= 8)
		setupDESstate(w->state, key, 0);
	else
		error("secret too short");
}

static void
initRC4key(OneWay *w)
{
	if(w->state){
		free(w->state);
		w->state = 0;
	}

	w->state = smalloc(sizeof(RC4state));
	setupRC4state(w->state, w->secret, w->slen);
}

/*
 *  40 bit RC4 is the same as n-bit RC4.  However,
 *  we ignore all but the first 40 bits of the key.
 */
static void
initRC4key_40(OneWay *w)
{
	if(w->state){
		free(w->state);
		w->state = 0;
	}

	if(w->slen > 5)
		w->slen = 5;

	w->state = malloc(sizeof(RC4state));
	setupRC4state(w->state, w->secret, w->slen);
}

/*
 *  128 bit RC4 is the same as n-bit RC4.  However,
 *  we ignore all but the first 128 bits of the key.
 */
static void
initRC4key_128(OneWay *w)
{
	if(w->state){
		free(w->state);
		w->state = 0;
	}

	if(w->slen > 16)
		w->slen = 16;

	w->state = malloc(sizeof(RC4state));
	setupRC4state(w->state, w->secret, w->slen);
}


typedef struct Hashalg Hashalg;
struct Hashalg
{
	char	*name;
	int	diglen;
	DigestState *(*hf)(uchar*, ulong, uchar*, DigestState*);
};

Hashalg hashtab[] =
{
	{ "md4", MD4dlen, md4, },
	{ "md5", MD5dlen, md5, },
	{ "sha1", SHA1dlen, sha1, },
	{ "sha", SHA1dlen, sha1, },
	{ 0 }
};

static int
parsehashalg(char *p, Dstate *s)
{
	Hashalg *ha;

	for(ha = hashtab; ha->name; ha++){
		if(strcmp(p, ha->name) == 0){
			s->hf = ha->hf;
			s->diglen = ha->diglen;
			s->state &= ~Sclear;
			s->state |= Sdigesting;
			return 0;
		}
	}
	return -1;
}

typedef struct Encalg Encalg;
struct Encalg
{
	char	*name;
	int	blocklen;
	int	alg;
	void	(*keyinit)(OneWay*);
};

#ifdef NOSPOOKS
Encalg encrypttab[] =
{
	{ "descbc", 8, DESCBC, initDESkey, },           /* DEPRECATED -- use des_56_cbc */
	{ "desecb", 8, DESECB, initDESkey, },           /* DEPRECATED -- use des_56_ecb */
	{ "des_56_cbc", 8, DESCBC, initDESkey, },
	{ "des_56_ecb", 8, DESECB, initDESkey, },
	{ "des_40_cbc", 8, DESCBC, initDESkey_40, },
	{ "des_40_ecb", 8, DESECB, initDESkey_40, },
	{ "rc4", 1, RC4, initRC4key_40, },              /* DEPRECATED -- use rc4_X      */
	{ "rc4_256", 1, RC4, initRC4key, },
	{ "rc4_128", 1, RC4, initRC4key_128, },
	{ "rc4_40", 1, RC4, initRC4key_40, },
	{ 0 }
};
#else
Encalg encrypttab[] =
{
	{ "des_40_cbc", 8, DESCBC, initDESkey_40, },
	{ "des_40_ecb", 8, DESECB, initDESkey_40, },
	{ "rc4", 1, RC4, initRC4key_40, },              /* DEPRECATED -- use rc4_X      */
	{ "rc4_40", 1, RC4, initRC4key_40, },
	{ 0 }
};
#endif NOSPOOKS

static int
parseencryptalg(char *p, Dstate *s)
{
	Encalg *ea;

	for(ea = encrypttab; ea->name; ea++){
		if(strcmp(p, ea->name) == 0){
			s->encryptalg = ea->alg;
			s->blocklen = ea->blocklen;
			(*ea->keyinit)(&s->in);
			(*ea->keyinit)(&s->out);
			s->state &= ~Sclear;
			s->state |= Sencrypting;
			return 0;
		}
	}
	return -1;
}

static long
sslwrite(Chan *c, void *a, long n, vlong off)
{
	volatile struct { Dstate *s; } s;
	volatile struct { Block *b; } b;
	int m, t;
	char *p, *np, *e, buf[128];
	uchar *x;
	ulong offset = off;

	s.s = dstate[CONV(c->qid)];
	if(s.s == 0)
		panic("sslwrite");

	t = TYPE(c->qid);
	if(t == Qdata){
		if(s.s->state == Sincomplete)
			error(Ebadusefd);

		p = a;
		e = p + n;
		do {
			m = e - p;
			if(m > s.s->max)
				m = s.s->max;

			b.b = allocb(m);
			if(waserror()){
				freeb(b.b);
				nexterror();
			}
			memmove(b.b->wp, p, m);
			poperror();
			b.b->wp += m;

			sslbwrite(c, b.b, offset);

			p += m;
		} while(p < e);
		return n;
	}

	/* mutex with operations using what we're about to change */
	if(waserror()){
		qunlock(&s.s->in.ctlq);
		qunlock(&s.s->out.q);
		nexterror();
	}
	qlock(&s.s->in.ctlq);
	qlock(&s.s->out.q);

	switch(t){
	default:
		panic("sslwrite");
	case Qsecretin:
		setsecret(&s.s->in, a, n);
		goto out;
	case Qsecretout:
		setsecret(&s.s->out, a, n);
		goto out;
	case Qctl:
		break;
	}

	if(n >= sizeof(buf))
		error("arg too long");
	strncpy(buf, a, n);
	buf[n] = 0;
	p = strchr(buf, '\n');
	if(p)
		*p = 0;
	p = strchr(buf, ' ');
	if(p)
		*p++ = 0;

	if(strcmp(buf, "fd") == 0){
		s.s->c = buftochan(p);

		/* default is clear (msg delimiters only) */
		s.s->state = Sclear;
		s.s->blocklen = 1;
		s.s->diglen = 0;
		s.s->maxpad = s.s->max = (1<<15) - s.s->diglen - 1;
		s.s->in.mid = 0;
		s.s->out.mid = 0;
	} else if(strcmp(buf, "alg") == 0 && p != 0){
		s.s->blocklen = 1;
		s.s->diglen = 0;

		if(s.s->c == 0)
			error("must set fd before algorithm");

		s.s->state = Sclear;
		s.s->maxpad = s.s->max = (1<<15) - s.s->diglen - 1;
		if(strcmp(p, "clear") == 0){
			goto out;
		}

		if(s.s->in.secret && s.s->out.secret == 0)
			setsecret(&s.s->out, s.s->in.secret, s.s->in.slen);
		if(s.s->out.secret && s.s->in.secret == 0)
			setsecret(&s.s->in, s.s->out.secret, s.s->out.slen);
		if(s.s->in.secret == 0 || s.s->out.secret == 0)
			error("algorithm but no secret");

		s.s->hf = 0;
		s.s->encryptalg = Noencryption;
		s.s->blocklen = 1;

		for(;;){
			np = strchr(p, ' ');
			if(np)
				*np++ = 0;

			if(parsehashalg(p, s.s) < 0)
			if(parseencryptalg(p, s.s) < 0)
				error("bad algorithm");

			if(np == 0)
				break;
			p = np;
		}

		if(s.s->hf == 0 && s.s->encryptalg == Noencryption)
			error("bad algorithm");

		if(s.s->blocklen != 1){
			s.s->max = (1<<15) - s.s->diglen - 1;
			s.s->max -= s.s->max % s.s->blocklen;
			s.s->maxpad = (1<<14) - s.s->diglen - 1;
			s.s->maxpad -= s.s->maxpad % s.s->blocklen;
		} else
			s.s->maxpad = s.s->max = (1<<15) - s.s->diglen - 1;
	} else if(strcmp(buf, "secretin") == 0 && p != 0) {
		m = (strlen(p)*3)/2;
		x = smalloc(m);
		n = dec64(x, m, p, strlen(p));
		setsecret(&s.s->in, x, n);
		free(x);
	} else if(strcmp(buf, "secretout") == 0 && p != 0) {
		m = (strlen(p)*3)/2 + 1;
		x = smalloc(m);
		n = dec64(x, m, p, strlen(p));
		setsecret(&s.s->out, x, n);
		free(x);
	} else
		error(Ebadarg);

out:
	qunlock(&s.s->in.ctlq);
	qunlock(&s.s->out.q);
	poperror();
	return n;
}

static void
sslinit(void)
{
	struct Encalg *e;
	struct Hashalg *h;
	int n;
	char *cp;

	if((dstate = smalloc(sizeof(Dstate*) * maxdstate)) == 0)
		panic("sslinit");

	n = 1;
	for(e = encrypttab; e->name != nil; e++)
		n += strlen(e->name) + 1;
	cp = encalgs = smalloc(n);
	for(e = encrypttab;;){
		strcpy(cp, e->name);
		cp += strlen(e->name);
		e++;
		if(e->name == nil)
			break;
		*cp++ = ' ';
	}
	*cp = 0;

	n = 1;
	for(h = hashtab; h->name != nil; h++)
		n += strlen(h->name) + 1;
	cp = hashalgs = smalloc(n);
	for(h = hashtab;;){
		strcpy(cp, h->name);
		cp += strlen(h->name);
		h++;
		if(h->name == nil)
			break;
		*cp++ = ' ';
	}
	*cp = 0;
}

Dev ssldevtab = {
	'D',
	"ssl",

	devreset,
	sslinit,
	sslattach,
	devclone,
	sslwalk,
	sslstat,
	sslopen,
	devcreate,
	sslclose,
	sslread,
	sslbread,
	sslwrite,
	sslbwrite,
	devremove,
	sslwstat,
};

static Block*
encryptb(Dstate *s, Block *b, int offset)
{
	uchar *p, *ep, *p2, *ip, *eip;
	DESstate *ds;

	switch(s->encryptalg){
	case DESECB:
		ds = s->out.state;
		ep = b->rp + BLEN(b);
		for(p = b->rp + offset; p < ep; p += 8)
			block_cipher(ds->expanded, p, 0);
		break;
	case DESCBC:
		ds = s->out.state;
		ep = b->rp + BLEN(b);
		for(p = b->rp + offset; p < ep; p += 8){
			p2 = p;
			ip = ds->ivec;
			for(eip = ip+8; ip < eip; )
				*p2++ ^= *ip++;
			block_cipher(ds->expanded, p, 0);
			memmove(ds->ivec, p, 8);
		}
		break;
	case RC4:
		rc4(s->out.state, b->rp + offset, BLEN(b) - offset);
		break;
	}
	return b;
}

static Block*
decryptb(Dstate *s, Block *bin)
{
	Block *b, **l;
	uchar *p, *ep, *tp, *ip, *eip;
	DESstate *ds;
	uchar tmp[8];
	int i;

	l = &bin;
	for(b = bin; b; b = b->next){
		/* make sure we have a multiple of s->blocklen */
		if(s->blocklen > 1){
			i = BLEN(b);
			if(i % s->blocklen){
				*l = b = pullupblock(b, i + s->blocklen - (i%s->blocklen));
				if(b == 0)
					error("ssl encrypted message too short");
			}
		}
		l = &b->next;

		/* decrypt */
		switch(s->encryptalg){
		case DESECB:
			ds = s->in.state;
			ep = b->rp + BLEN(b);
			for(p = b->rp; p < ep; p += 8)
				block_cipher(ds->expanded, p, 1);
			break;
		case DESCBC:
			ds = s->in.state;
			ep = b->rp + BLEN(b);
			for(p = b->rp; p < ep;){
				memmove(tmp, p, 8);
				block_cipher(ds->expanded, p, 1);
				tp = tmp;
				ip = ds->ivec;
				for(eip = ip+8; ip < eip; ){
					*p++ ^= *ip;
					*ip++ = *tp++;
				}
			}
			break;
		case RC4:
			rc4(s->in.state, b->rp, BLEN(b));
			break;
		}
	}
	return bin;
}

static Block*
digestb(Dstate *s, Block *b, int offset)
{
	uchar *p;
	DigestState ss;
	uchar msgid[4];
	ulong n, h;
	OneWay *w;

	w = &s->out;

	memset(&ss, 0, sizeof(ss));
	h = s->diglen + offset;
	n = BLEN(b) - h;

	/* hash secret + message */
	(*s->hf)(w->secret, w->slen, 0, &ss);
	(*s->hf)(b->rp + h, n, 0, &ss);

	/* hash message id */
	p = msgid;
	n = w->mid;
	*p++ = n>>24;
	*p++ = n>>16;
	*p++ = n>>8;
	*p = n;
	(*s->hf)(msgid, 4, b->rp + offset, &ss);

	return b;
}

static void
checkdigestb(Dstate *s, Block *bin)
{
	uchar *p;
	DigestState ss;
	uchar msgid[4];
	int n, h;
	OneWay *w;
	uchar digest[128];
	Block *b;

	w = &s->in;

	memset(&ss, 0, sizeof(ss));

	/* hash secret */
	(*s->hf)(w->secret, w->slen, 0, &ss);

	/* hash message */
	h = s->diglen;
	for(b = bin; b; b = b->next){
		n = BLEN(b) - h;
		if(n < 0)
			panic("checkdigestb");
		(*s->hf)(b->rp + h, n, 0, &ss);
		h = 0;
	}

	/* hash message id */
	p = msgid;
	n = w->mid;
	*p++ = n>>24;
	*p++ = n>>16;
	*p++ = n>>8;
	*p = n;
	(*s->hf)(msgid, 4, digest, &ss);

	if(memcmp(digest, bin->rp, s->diglen) != 0)
		error("bad digest");
}

/* get channel associated with an fd */
static Chan*
buftochan(char *p)
{
	Chan *c;
	int fd;

	if(p == 0)
		error(Ebadarg);
	fd = strtoul(p, 0, 0);
	if(fd < 0)
		error(Ebadarg);
	c = fdtochan(fd, -1, 0, 1);	/* error check and inc ref */
	return c;
}

/* hand up a digest connection */
static void
sslhangup(Dstate *s)
{
	Block *b;

	qlock(&s->in.q);
	for(b = s->processed; b; b = s->processed){
		s->processed = b->next;
		freeb(b);
	}
	if(s->unprocessed){
		freeb(s->unprocessed);
		s->unprocessed = 0;
	}
	s->state = Sincomplete;
	qunlock(&s->in.q);
}

static Dstate*
dsclone(Chan *ch)
{
	Dstate **pp, **ep, **np;
	int newmax;

	if(waserror()) {
		unlock(&dslock);
		nexterror();
	}
	lock(&dslock);
	ep = &dstate[maxdstate];
	for(pp = dstate; pp < ep; pp++) {
		if(*pp == 0) {
			dsnew(ch, pp);
			break;
		}
	}
	if(pp >= ep) {
		if(maxdstate >= Maxdstate) {
			unlock(&dslock);
			poperror();
			return 0;
		}
		newmax = 2 * maxdstate;
		if(newmax > Maxdstate)
			newmax = Maxdstate;
		np = smalloc(sizeof(Dstate*) * newmax);
		if(np == 0)
			error(Enomem);
		memmove(np, dstate, sizeof(Dstate*) * maxdstate);
		dstate = np;
		pp = &dstate[maxdstate];
		memset(pp, 0, sizeof(Dstate*)*(newmax - maxdstate));
		maxdstate = newmax;
		dsnew(ch, pp);
	}
	unlock(&dslock);
	poperror();
	return *pp;
}

static void
dsnew(Chan *ch, Dstate **pp)
{
	Dstate *s;
	int t;

	*pp = s = malloc(sizeof(*s));
	if(!s)
		error(Enomem);
	if(pp - dstate >= dshiwat)
		dshiwat++;
	memset(s, 0, sizeof(*s));
	s->state = Sincomplete;
	s->ref = 1;
	strncpy(s->user, up->user, sizeof(s->user));
	s->perm = 0660;
	t = TYPE(ch->qid);
	if(t == Qclonus)
		t = Qctl;
	ch->qid.path = QID(pp - dstate, t);
	ch->qid.vers = 0;
}
