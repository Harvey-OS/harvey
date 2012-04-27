/*
 * 4th Edition p9any/p9sk1 authentication based on auth9p1.c
 * Nigel Roles (nigel@9fs.org) 2003
 */

#include <plan9.h>
#include <fcall.h>
#include <u9fs.h>
#include <stdlib.h>	/* for random stuff */

typedef struct	Ticket		Ticket;
typedef struct	Ticketreq	Ticketreq;
typedef struct	Authenticator	Authenticator;

enum
{
	DOMLEN=		48,		/* length of an authentication domain name */
	CHALLEN=	8		/* length of a challenge */
};

enum {
	HaveProtos,
	NeedProto,
	NeedChal,
	HaveTreq,
	NeedTicket,
	HaveAuth,
	Established,
};

/* encryption numberings (anti-replay) */
enum
{
	AuthTreq=1,	/* ticket request */
	AuthChal=2,	/* challenge box request */
	AuthPass=3,	/* change password */
	AuthOK=4,	/* fixed length reply follows */
	AuthErr=5,	/* error follows */
	AuthMod=6,	/* modify user */
	AuthApop=7,	/* apop authentication for pop3 */
	AuthOKvar=9,	/* variable length reply follows */
	AuthChap=10,	/* chap authentication for ppp */
	AuthMSchap=11,	/* MS chap authentication for ppp */
	AuthCram=12,	/* CRAM verification for IMAP (RFC2195 & rfc2104) */
	AuthHttp=13,	/* http domain login */
	AuthVNC=14,	/* http domain login */


	AuthTs=64,	/* ticket encrypted with server's key */
	AuthTc,		/* ticket encrypted with client's key */
	AuthAs,		/* server generated authenticator */
	AuthAc,		/* client generated authenticator */
	AuthTp,		/* ticket encrypted with client's key for password change */
	AuthHr		/* http reply */
};

struct Ticketreq
{
	char	type;
	char	authid[NAMELEN];	/* server's encryption id */
	char	authdom[DOMLEN];	/* server's authentication domain */
	char	chal[CHALLEN];		/* challenge from server */
	char	hostid[NAMELEN];	/* host's encryption id */
	char	uid[NAMELEN];		/* uid of requesting user on host */
};
#define	TICKREQLEN	(3*NAMELEN+CHALLEN+DOMLEN+1)

struct Ticket
{
	char	num;			/* replay protection */
	char	chal[CHALLEN];		/* server challenge */
	char	cuid[NAMELEN];		/* uid on client */
	char	suid[NAMELEN];		/* uid on server */
	char	key[DESKEYLEN];		/* nonce DES key */
};
#define	TICKETLEN	(CHALLEN+2*NAMELEN+DESKEYLEN+1)

struct Authenticator
{
	char	num;			/* replay protection */
	char	chal[CHALLEN];
	ulong	id;			/* authenticator id, ++'d with each auth */
};
#define	AUTHENTLEN	(CHALLEN+4+1)

extern int chatty9p;

static	int	convT2M(Ticket*, char*, char*);
static	void	convM2T(char*, Ticket*, char*);
static	void	convM2Tnoenc(char*, Ticket*);
static	int	convA2M(Authenticator*, char*, char*);
static	void	convM2A(char*, Authenticator*, char*);
static	int	convTR2M(Ticketreq*, char*);
static	void	convM2TR(char*, Ticketreq*);
static	int	passtokey(char*, char*);

/*
 * destructively encrypt the buffer, which
 * must be at least 8 characters long.
 */
static int
encrypt9p(void *key, void *vbuf, int n)
{
	char ekey[128], *buf;
	int i, r;

	if(n < 8)
		return 0;
	key_setup(key, ekey);
	buf = vbuf;
	n--;
	r = n % 7;
	n /= 7;
	for(i = 0; i < n; i++){
		block_cipher(ekey, buf, 0);
		buf += 7;
	}
	if(r)
		block_cipher(ekey, buf - 7 + r, 0);
	return 1;
}

/*
 * destructively decrypt the buffer, which
 * must be at least 8 characters long.
 */
static int
decrypt9p(void *key, void *vbuf, int n)
{
	char ekey[128], *buf;
	int i, r;

	if(n < 8)
		return 0;
	key_setup(key, ekey);
	buf = vbuf;
	n--;
	r = n % 7;
	n /= 7;
	buf += n * 7;
	if(r)
		block_cipher(ekey, buf - 7 + r, 1);
	for(i = 0; i < n; i++){
		buf -= 7;
		block_cipher(ekey, buf, 1);
	}
	return 1;
}

#define	CHAR(x)		*p++ = f->x
#define	SHORT(x)	p[0] = f->x; p[1] = f->x>>8; p += 2
#define	VLONG(q)	p[0] = (q); p[1] = (q)>>8; p[2] = (q)>>16; p[3] = (q)>>24; p += 4
#define	LONG(x)		VLONG(f->x)
#define	STRING(x,n)	memmove(p, f->x, n); p += n

static int
convTR2M(Ticketreq *f, char *ap)
{
	int n;
	uchar *p;

	p = (uchar*)ap;
	CHAR(type);
	STRING(authid, NAMELEN);
	STRING(authdom, DOMLEN);
	STRING(chal, CHALLEN);
	STRING(hostid, NAMELEN);
	STRING(uid, NAMELEN);
	n = p - (uchar*)ap;
	return n;
}

static int
convT2M(Ticket *f, char *ap, char *key)
{
	int n;
	uchar *p;

	p = (uchar*)ap;
	CHAR(num);
	STRING(chal, CHALLEN);
	STRING(cuid, NAMELEN);
	STRING(suid, NAMELEN);
	STRING(key, DESKEYLEN);
	n = p - (uchar*)ap;
	if(key)
		encrypt9p(key, ap, n);
	return n;
}

int
convA2M(Authenticator *f, char *ap, char *key)
{
	int n;
	uchar *p;

	p = (uchar*)ap;
	CHAR(num);
	STRING(chal, CHALLEN);
	LONG(id);
	n = p - (uchar*)ap;
	if(key)
		encrypt9p(key, ap, n);
	return n;
}

#undef CHAR
#undef SHORT
#undef VLONG
#undef LONG
#undef STRING

#define	CHAR(x)		f->x = *p++
#define	SHORT(x)	f->x = (p[0] | (p[1]<<8)); p += 2
#define	VLONG(q)	q = (p[0] | (p[1]<<8) | (p[2]<<16) | (p[3]<<24)); p += 4
#define	LONG(x)		VLONG(f->x)
#define	STRING(x,n)	memmove(f->x, p, n); p += n

void
convM2A(char *ap, Authenticator *f, char *key)
{
	uchar *p;

	if(key)
		decrypt9p(key, ap, AUTHENTLEN);
	p = (uchar*)ap;
	CHAR(num);
	STRING(chal, CHALLEN);
	LONG(id);
	USED(p);
}

void
convM2T(char *ap, Ticket *f, char *key)
{
	uchar *p;

	if(key)
		decrypt9p(key, ap, TICKETLEN);
	p = (uchar*)ap;
	CHAR(num);
	STRING(chal, CHALLEN);
	STRING(cuid, NAMELEN);
	f->cuid[NAMELEN-1] = 0;
	STRING(suid, NAMELEN);
	f->suid[NAMELEN-1] = 0;
	STRING(key, DESKEYLEN);
	USED(p);
}

#undef CHAR
#undef SHORT
#undef LONG
#undef VLONG
#undef STRING

static int
passtokey(char *key, char *p)
{
	uchar buf[NAMELEN], *t;
	int i, n;

	n = strlen(p);
	if(n >= NAMELEN)
		n = NAMELEN-1;
	memset(buf, ' ', 8);
	t = buf;
	strncpy((char*)t, p, n);
	t[n] = 0;
	memset(key, 0, DESKEYLEN);
	for(;;){
		for(i = 0; i < DESKEYLEN; i++)
			key[i] = (t[i] >> i) + (t[i+1] << (8 - (i+1)));
		if(n <= 8)
			return 1;
		n -= 8;
		t += 8;
		if(n < 8){
			t -= 8 - n;
			n = 8;
		}
		encrypt9p(key, t, 8);
	}
	return 1;	/* not reached */
}

static char authkey[DESKEYLEN];
static char *authid;
static char *authdom;
static char *haveprotosmsg;
static char *needprotomsg;

static void
p9anyinit(void)
{
	int n, fd;
	char abuf[200];
	char *af, *f[4];

	af = autharg;
	if(af == nil)
		af = "/etc/u9fs.key";

	if((fd = open(af, OREAD)) < 0)
		sysfatal("can't open key file '%s'", af);

	if((n = readn(fd, abuf, sizeof(abuf)-1)) < 0)
		sysfatal("can't read key file '%s'", af);
	if (n > 0 && abuf[n - 1] == '\n')
		n--;
	abuf[n] = '\0';

	if(getfields(abuf, f, nelem(f), 0, "\n") != 3)
		sysfatal("key file '%s' not exactly 3 lines", af);

	passtokey(authkey, f[0]);
	authid = strdup(f[1]);
	authdom = strdup(f[2]);
	haveprotosmsg = malloc(strlen("p9sk1") + 1 + strlen(authdom) + 1);
	sprint(haveprotosmsg, "p9sk1@%s", authdom);
	needprotomsg = malloc(strlen("p9sk1") + 1 + strlen(authdom) + 1);
	sprint(needprotomsg, "p9sk1 %s", authdom);
}

typedef struct AuthSession {
	int state;
	char *uname;
	char *aname;
	char cchal[CHALLEN];
	Ticketreq tr;
	Ticket t;
} AuthSession;

static char*
p9anyauth(Fcall *rx, Fcall *tx)
{
	AuthSession *sp;
	Fid *f;
	char *ep;

	sp = malloc(sizeof(AuthSession));
	f = newauthfid(rx->afid, sp, &ep);
	if (f == nil) {
		free(sp);
		return ep;
	}
	if (chatty9p)
		fprint(2, "p9anyauth: afid %d\n", rx->afid);
	sp->state = HaveProtos;
	sp->uname = strdup(rx->uname);
	sp->aname = strdup(rx->aname);
	tx->aqid.type = QTAUTH;
	tx->aqid.path = 1;
	tx->aqid.vers = 0;
	return nil;
}

static char *
p9anyattach(Fcall *rx, Fcall *tx)
{
	AuthSession *sp;
	Fid *f;
	char *ep;

	f = oldauthfid(rx->afid, (void **)&sp, &ep);
	if (f == nil)
		return ep;
	if (chatty9p)
		fprint(2, "p9anyattach: afid %d state %d\n", rx->afid, sp->state);
	if (sp->state == Established && strcmp(rx->uname, sp->uname) == 0
		&& strcmp(rx->aname, sp->aname) == 0)
		return nil;
	return "authentication failed";
}

static int
readstr(Fcall *rx, Fcall *tx, char *s, int len)
{
	if (rx->offset >= len)
		return 0;
	tx->count = len - rx->offset;
	if (tx->count > rx->count)
		tx->count = rx->count;
	memcpy(tx->data, s + rx->offset, tx->count);
	return tx->count;
}

static char *
p9anyread(Fcall *rx, Fcall *tx)
{
	AuthSession *sp;
	char *ep;

	Fid *f;
	f = oldauthfid(rx->afid, (void **)&sp, &ep);
	if (f == nil)
		return ep;
	if (chatty9p)
		fprint(2, "p9anyread: afid %d state %d\n", rx->fid, sp->state);
	switch (sp->state) {
	case HaveProtos:
		readstr(rx, tx, haveprotosmsg, strlen(haveprotosmsg) + 1);
		if (rx->offset + tx->count == strlen(haveprotosmsg) + 1)
			sp->state = NeedProto;
		return nil;
	case HaveTreq:
		if (rx->count != TICKREQLEN)
			goto botch;
		convTR2M(&sp->tr, tx->data);
		tx->count = TICKREQLEN;
		sp->state = NeedTicket;
		return nil;
	case HaveAuth: {
		Authenticator a;
		if (rx->count != AUTHENTLEN)
			goto botch;
		a.num = AuthAs;
		memmove(a.chal, sp->cchal, CHALLEN);
		a.id = 0;
		convA2M(&a, (char*)tx->data, sp->t.key);
		memset(sp->t.key, 0, sizeof(sp->t.key));
		tx->count = rx->count;
		sp->state = Established;
		return nil;
	}
	default:
	botch:
		return "protocol botch";
	}
}

static char *
p9anywrite(Fcall *rx, Fcall *tx)
{
	AuthSession *sp;
	char *ep;

	Fid *f;

	f = oldauthfid(rx->afid, (void **)&sp, &ep);
	if (f == nil)
		return ep;
	if (chatty9p)
		fprint(2, "p9anywrite: afid %d state %d\n", rx->fid, sp->state);
	switch (sp->state) {
	case NeedProto:
		if (rx->count != strlen(needprotomsg) + 1)
			return "protocol response wrong length";
		if (memcmp(rx->data, needprotomsg, rx->count) != 0)
			return "unacceptable protocol";
		sp->state = NeedChal;
		tx->count = rx->count;
		return nil;
	case NeedChal:
		if (rx->count != CHALLEN)
			goto botch;
		memmove(sp->cchal, rx->data, CHALLEN);
		sp->tr.type = AuthTreq;
		safecpy(sp->tr.authid, authid, sizeof(sp->tr.authid));
		safecpy(sp->tr.authdom, authdom, sizeof(sp->tr.authdom));
		randombytes((uchar *)sp->tr.chal, CHALLEN);
		safecpy(sp->tr.hostid, "", sizeof(sp->tr.hostid));
		safecpy(sp->tr.uid, "", sizeof(sp->tr.uid));
		tx->count = rx->count;
		sp->state = HaveTreq;
		return nil;
	case NeedTicket: {
		Authenticator a;

		if (rx->count != TICKETLEN + AUTHENTLEN) {
			fprint(2, "bad length in attach");
			goto botch;
		}
		convM2T((char*)rx->data, &sp->t, authkey);
		if (sp->t.num != AuthTs) {
			fprint(2, "bad AuthTs in attach\n");
			goto botch;
		}
		if (memcmp(sp->t.chal, sp->tr.chal, CHALLEN) != 0) {
			fprint(2, "bad challenge in attach\n");
			goto botch;
		}
		convM2A((char*)rx->data + TICKETLEN, &a, sp->t.key);
		if (a.num != AuthAc) {
			fprint(2, "bad AuthAs in attach\n");
			goto botch;
		}
		if(memcmp(a.chal, sp->tr.chal, CHALLEN) != 0) {
			fprint(2, "bad challenge in attach 2\n");
			goto botch;
		}
		sp->state = HaveAuth;
		tx->count = rx->count;
		return nil;
	}
	default:
	botch:
		return "protocol botch";
	}
}

static void
safefree(char *p)
{
	if (p) {
		memset(p, 0, strlen(p));
		free(p);
	}
}

static char *
p9anyclunk(Fcall *rx, Fcall *tx)
{
	Fid *f;
	AuthSession *sp;
	char *ep;

	f = oldauthfid(rx->afid, (void **)&sp, &ep);
	if (f == nil)
		return ep;
	if (chatty9p)
		fprint(2, "p9anyclunk: afid %d\n", rx->fid);
	safefree(sp->uname);
	safefree(sp->aname);
	memset(sp, 0, sizeof(sp));
	free(sp);
	return nil;
}

Auth authp9any = {
	"p9any",
	p9anyauth,
	p9anyattach,
	p9anyinit,
	p9anyread,
	p9anywrite,
	p9anyclunk,
};
