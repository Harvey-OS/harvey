#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/netif.h"
#include "../port/error.h"

#include	<libsec.h>
#include "../port/thwack.h"

/*
 * sdp - secure datagram protocol
 */

typedef struct Sdp Sdp;
typedef struct Conv Conv;
typedef struct OneWay OneWay;
typedef struct Stats Stats;
typedef struct AckPkt AckPkt;
typedef struct Algorithm Algorithm;
typedef struct CipherRc4 CipherRc4;

enum
{
	Qtopdir=	1,		/* top level directory */

	Qsdpdir,			/* sdp directory */
	Qclone,
	Qlog,

	Qconvdir,			/* directory per conversation */
	Qctl,
	Qdata,				/* unreliable packet channel */
	Qcontrol,			/* reliable control channel */
	Qstatus,
	Qstats,
	Qrstats,

	MaxQ,

	Maxconv= 256,		// power of 2
	Nfs= 4,			// number of file systems
	MaxRetries=	12,
	KeepAlive = 300,	// keep alive in seconds - should probably be about 60 but is higher to avoid linksys bug
	SecretLength= 32,	// a secret per direction
	SeqMax = (1<<24),
	SeqWindow = 32,
	NCompStats = 8,
};

#define TYPE(x) 	(((ulong)(x).path) & 0xff)
#define CONV(x) 	((((ulong)(x).path) >> 8)&(Maxconv-1))
#define QID(x, y) 	(((x)<<8) | (y))

struct Stats
{
	ulong	outPackets;
	ulong	outDataPackets;
	ulong	outDataBytes;
	ulong	outCompDataBytes;
	ulong	outCompBytes;
	ulong	outCompStats[NCompStats];
	ulong	inPackets;
	ulong	inDataPackets;
	ulong	inDataBytes;
	ulong	inCompDataBytes;
	ulong	inMissing;
	ulong	inDup;
	ulong	inReorder;
	ulong	inBadComp;
	ulong	inBadAuth;
	ulong	inBadSeq;
	ulong	inBadOther;
};

struct OneWay
{
	Rendez	statsready;

	ulong	seqwrap;	// number of wraps of the sequence number
	ulong	seq;
	ulong	window;

	uchar	secret[SecretLength];

	QLock	controllk;
	Rendez	controlready;
	Block	*controlpkt;		// control channel
	ulong	controlseq;

	void	*cipherstate;	// state cipher
	int		cipherivlen;	// initial vector length
	int		cipherblklen;	// block length
	int		(*cipher)(OneWay*, uchar *buf, int len);

	void	*authstate;		// auth state
	int		authlen;		// auth data length in bytes
	int		(*auth)(OneWay*, uchar *buf, int len);

	void	*compstate;
	int		(*comp)(Conv*, int subtype, ulong seq, Block **);
};

// conv states
enum {
	CFree,
	CInit,
	CDial,
	CAccept,
	COpen,
	CLocalClose,
	CRemoteClose,
	CClosed,
};

struct Conv {
	QLock;
	Sdp	*sdp;
	int	id;

	int ref;	// holds conv up

	int state;

	int dataopen;	// ref count of opens on Qdata
	int controlopen;	// ref count of opens on Qcontrol
	int reader;		// reader proc has been started

	Stats	lstats;
	Stats	rstats;
	
	ulong	lastrecv;	// time last packet was received 
	ulong	timeout;
	int		retries;

	// the following pair uniquely define conversation on this port
	ulong dialid;
	ulong acceptid;

	QLock readlk;		// protects readproc
	Proc *readproc;

	Chan *chan;		// packet channel
	char *channame;

	char owner[KNAMELEN];		/* protections */
	int	perm;

	Algorithm *auth;
	Algorithm *cipher;
	Algorithm *comp;

	int drop;

	OneWay	in;
	OneWay	out;
};

struct Sdp {
	QLock;
	Log;
	int	nconv;
	Conv *conv[Maxconv];
	int ackproc;
};

enum {
	TConnect,
	TControl,
	TData,
	TCompData,
};

enum {
	ControlMesg,
	ControlAck,
};

enum {
	ThwackU,
	ThwackC,
};

enum {
	ConOpenRequest,
	ConOpenAck,
	ConOpenAckAck,
	ConClose,
	ConCloseAck,
	ConReset,
};

struct AckPkt
{
	uchar	cseq[4];
	uchar	outPackets[4];
	uchar	outDataPackets[4];
	uchar	outDataBytes[4];
	uchar	outCompDataBytes[4];
	uchar	outCompStats[4*NCompStats];
	uchar	inPackets[4];
	uchar	inDataPackets[4];
	uchar	inDataBytes[4];
	uchar	inCompDataBytes[4];
	uchar	inMissing[4];
	uchar	inDup[4];
	uchar	inReorder[4];
	uchar	inBadComp[4];
	uchar	inBadAuth[4];
	uchar	inBadSeq[4];
	uchar	inBadOther[4];
};

struct Algorithm
{
	char 	*name;
	int		keylen;		// in bytes
	void	(*init)(Conv*);
};

enum {
	RC4forward	= 10*1024*1024,	// maximum skip forward
	RC4back = 100*1024,		// maximum look back
};

struct CipherRc4
{
	ulong cseq;	// current byte sequence number
	RC4state current;

	int ovalid;	// old is valid
	ulong lgseq; // last good sequence
	ulong oseq;	// old byte sequence number
	RC4state old;
};

static Dirtab sdpdirtab[]={
	"log",		{Qlog},		0,	0666,
	"clone",	{Qclone},		0,	0666,
};

static Dirtab convdirtab[]={
	"ctl",		{Qctl},	0,	0666,
	"data",		{Qdata},	0,	0666,
	"control",	{Qcontrol},	0,	0666,
	"status",	{Qstatus},	0,	0444,
	"stats",	{Qstats},	0,	0444,
	"rstats",	{Qrstats},	0,	0444,
};

static int m2p[] = {
	[OREAD]		4,
	[OWRITE]	2,
	[ORDWR]		6
};

enum {
	Logcompress=	(1<<0),
	Logauth=	(1<<1),
	Loghmac=	(1<<2),
};

static Logflag logflags[] =
{
	{ "compress",	Logcompress, },
	{ "auth",	Logauth, },
	{ "hmac",	Loghmac, },
	{ nil,		0, },
};

static Dirtab	*dirtab[MaxQ];
static Sdp sdptab[Nfs];
static char *convstatename[] = {
	[CFree]		"Free",
	[CInit]		"Init",
	[CDial]		"Dial",
	[CAccept]	"Accept",
	[COpen]		"Open",
	[CLocalClose] "LocalClose",
	[CRemoteClose] "RemoteClose",
	[CClosed]	"Closed",
};

static int sdpgen(Chan *c, char*, Dirtab*, int, int s, Dir *dp);
static Conv *sdpclone(Sdp *sdp);
static void sdpackproc(void *a);
static void onewaycleanup(OneWay *ow);
static int readready(void *a);
static int controlread();
static void convsetstate(Conv *c, int state);
static Block *readcontrol(Conv *c, int n);
static void writecontrol(Conv *c, void *p, int n, int wait);
static Block *readdata(Conv *c, int n);
static long writedata(Conv *c, Block *b);
static void convderef(Conv *c);
static Block *conviput(Conv *c, Block *b, int control);
static void conviconnect(Conv *c, int op, Block *b);
static void convicontrol(Conv *c, int op, Block *b);
static Block *convicomp(Conv *c, int op, ulong, Block *b);
static void convoput(Conv *c, int type, int subtype, Block *b);
static void convoconnect(Conv *c, int op, ulong dialid, ulong acceptid);
static void convopenchan(Conv *c, char *path);
static void convstats(Conv *c, int local, char *buf, int n);
static void convreader(void *a);

static void setalg(Conv *c, char *name, Algorithm *tab, Algorithm **);
static void setsecret(OneWay *cc, char *secret);

static void nullcipherinit(Conv*c);
static void descipherinit(Conv*c);
static void rc4cipherinit(Conv*c);
static void nullauthinit(Conv*c);
static void shaauthinit(Conv*c);
static void md5authinit(Conv*c);
static void nullcompinit(Conv*c);
static void thwackcompinit(Conv*c);

static Algorithm cipheralg[] =
{
	"null",			0,	nullcipherinit,
	"des_56_cbc",	7,	descipherinit,
	"rc4_128",		16,	rc4cipherinit,
	"rc4_256",		32,	rc4cipherinit,
	nil,			0,	nil,
};

static Algorithm authalg[] =
{
	"null",			0,	nullauthinit,
	"hmac_sha1_96",	16,	shaauthinit,
	"hmac_md5_96",	16,	md5authinit,
	nil,			0,	nil,
};

static Algorithm compalg[] =
{
	"null",			0,	nullcompinit,
	"thwack",		0,	thwackcompinit,
	nil,			0,	nil,
};


static void
sdpinit(void)
{
	int i;
	Dirtab *dt;
	
	// setup dirtab with non directory entries
	for(i=0; i<nelem(sdpdirtab); i++) {
		dt = sdpdirtab + i;
		dirtab[TYPE(dt->qid)] = dt;
	}

	for(i=0; i<nelem(convdirtab); i++) {
		dt = convdirtab + i;
		dirtab[TYPE(dt->qid)] = dt;
	}

}

static Chan*
sdpattach(char* spec)
{
	Chan *c;
	int dev;
	char buf[100];
	Sdp *sdp;
	int start;

	dev = atoi(spec);
	if(dev<0 || dev >= Nfs)
		error("bad specification");

	c = devattach('E', spec);
	c->qid = (Qid){QID(0, Qtopdir), 0, QTDIR};
	c->dev = dev;

	sdp = sdptab + dev;
	qlock(sdp);
	start = sdp->ackproc == 0;
	sdp->ackproc = 1;
	qunlock(sdp);

	if(start) {
		snprint(buf, sizeof(buf), "sdpackproc%d", dev);
		kproc(buf, sdpackproc, sdp);
	}
	
	return c;
}

static Walkqid*
sdpwalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, 0, 0, sdpgen);
}

static int
sdpstat(Chan* c, uchar* db, int n)
{
	return devstat(c, db, n, nil, 0, sdpgen);
}

static Chan*
sdpopen(Chan* ch, int omode)
{
	int perm;
	Sdp *sdp;
	Conv *c;

	omode &= 3;
	perm = m2p[omode];
	USED(perm);

	sdp = sdptab + ch->dev;

	switch(TYPE(ch->qid)) {
	default:
		break;
	case Qtopdir:
	case Qsdpdir:
	case Qconvdir:
		if(omode != OREAD)
			error(Eperm);
		break;
	case Qlog:
		logopen(sdp);
		break;
	case Qclone:
		c = sdpclone(sdp);
		if(c == nil)
			error(Enodev);
		ch->qid.path = QID(c->id, Qctl);
		break;
	case Qdata:
	case Qctl:
	case Qstatus:
	case Qcontrol:
	case Qstats:
	case Qrstats:
		c = sdp->conv[CONV(ch->qid)];
		qlock(c);
		if(waserror()) {
			qunlock(c);
			nexterror();
		}
		if((perm & (c->perm>>6)) != perm)
		if(strcmp(up->user, c->owner) != 0 || (perm & c->perm) != perm)
				error(Eperm);

		c->ref++;
		if(TYPE(ch->qid) == Qdata) {
			c->dataopen++;
			// kill reader if Qdata is opened for the first time
			if(c->dataopen == 1)
			if(c->readproc != nil)
				postnote(c->readproc, 1, "interrupt", 0);
		} else if(TYPE(ch->qid) == Qcontrol) {	
			c->controlopen++;
		}
		qunlock(c);
		poperror();
		break;
	}
	ch->mode = openmode(omode);
	ch->flag |= COPEN;
	ch->offset = 0;
	return ch;
}

static void
sdpclose(Chan* ch)
{
	Sdp *sdp  = sdptab + ch->dev;
	Conv *c;

	if(!(ch->flag & COPEN))
		return;
	switch(TYPE(ch->qid)) {
	case Qlog:
		logclose(sdp);
		break;
	case Qctl:
	case Qstatus:
	case Qstats:
	case Qrstats:
		c = sdp->conv[CONV(ch->qid)];
		qlock(c);
		convderef(c);
		qunlock(c);
		break;

	case Qdata:
		c = sdp->conv[CONV(ch->qid)];
		qlock(c);
		c->dataopen--;
		convderef(c);
		if(c->dataopen == 0)
		if(c->reader == 0)
		if(c->chan != nil)
		if(!waserror()) {
			kproc("convreader", convreader, c);
			c->reader = 1;
			c->ref++;
			poperror();
		}
		qunlock(c);
		break;

	case Qcontrol:
		c = sdp->conv[CONV(ch->qid)];
		qlock(c);
		c->controlopen--;
		convderef(c);
		if(c->controlopen == 0 && c->ref != 0) {
			switch(c->state) {
			default:
				convsetstate(c, CClosed);
				break;
			case CAccept:
			case COpen:
				convsetstate(c, CLocalClose);
				break;
			}
		}
		qunlock(c);
		break;
	}
}

static long
sdpread(Chan *ch, void *a, long n, vlong off)
{
	char buf[256];
	char *s;
	Sdp *sdp = sdptab + ch->dev;
	Conv *c;
	Block *b;
	int rv;

	USED(off);
	switch(TYPE(ch->qid)) {
	default:
		error(Eperm);
	case Qtopdir:
	case Qsdpdir:
	case Qconvdir:
		return devdirread(ch, a, n, 0, 0, sdpgen);
	case Qlog:
		return logread(sdp, a, off, n);
	case Qstatus:
		c = sdp->conv[CONV(ch->qid)];
		qlock(c);
		n = readstr(off, a, n, convstatename[c->state]);
		qunlock(c);
		return n;
	case Qctl:
		snprint(buf, sizeof buf, "%lud", CONV(ch->qid));
		return readstr(off, a, n, buf);
	case Qcontrol:
		b = readcontrol(sdp->conv[CONV(ch->qid)], n);
		if(b == nil)
			return 0;
		if(BLEN(b) < n)
			n = BLEN(b);
		memmove(a, b->rp, n);
		freeb(b);
		return n;
	case Qdata:
		b = readdata(sdp->conv[CONV(ch->qid)], n);
		if(b == nil)
			return 0;
		if(BLEN(b) < n)
			n = BLEN(b);
		memmove(a, b->rp, n);
		freeb(b);
		return n;
	case Qstats:
	case Qrstats:
		c = sdp->conv[CONV(ch->qid)];
		s = smalloc(1000);
		convstats(c, TYPE(ch->qid) == Qstats, s, 1000);
		rv = readstr(off, a, n, s);
		free(s);
		return rv;
	}
}

static Block*
sdpbread(Chan* ch, long n, ulong offset)
{
	Sdp *sdp = sdptab + ch->dev;

	if(TYPE(ch->qid) != Qdata)
		return devbread(ch, n, offset);
	return readdata(sdp->conv[CONV(ch->qid)], n);
}


static long
sdpwrite(Chan *ch, void *a, long n, vlong off)
{
	Sdp *sdp = sdptab + ch->dev;
	Cmdbuf *cb;
	char *arg0;
	char *p;
	Conv *c;
	Block *b;
	
	USED(off);
	switch(TYPE(ch->qid)) {
	default:
		error(Eperm);
	case Qctl:
		c = sdp->conv[CONV(ch->qid)];
		cb = parsecmd(a, n);
		qlock(c);
		if(waserror()) {
			qunlock(c);
			free(cb);
			nexterror();
		}
		if(cb->nf == 0)
			error("short write");
		arg0 = cb->f[0];
		if(strcmp(arg0, "accept") == 0) {
			if(cb->nf != 2)
				error("usage: accept file");
			convopenchan(c, cb->f[1]);
		} else if(strcmp(arg0, "dial") == 0) {
			if(cb->nf != 2)
				error("usage: dial file");
			convopenchan(c, cb->f[1]);
			convsetstate(c, CDial);
		} else if(strcmp(arg0, "drop") == 0) {
			if(cb->nf != 2)
				error("usage: drop permil");
			c->drop = atoi(cb->f[1]);
		} else if(strcmp(arg0, "cipher") == 0) {
			if(cb->nf != 2)
				error("usage: cipher alg");
			setalg(c, cb->f[1], cipheralg, &c->cipher);
		} else if(strcmp(arg0, "auth") == 0) {
			if(cb->nf != 2)
				error("usage: auth alg");
			setalg(c, cb->f[1], authalg, &c->auth);
		} else if(strcmp(arg0, "comp") == 0) {
			if(cb->nf != 2)
				error("usage: comp alg");
			setalg(c, cb->f[1], compalg, &c->comp);
		} else if(strcmp(arg0, "insecret") == 0) {
			if(cb->nf != 2)
				error("usage: insecret secret");
			setsecret(&c->in, cb->f[1]);
			if(c->cipher)
				c->cipher->init(c);
			if(c->auth)
				c->auth->init(c);
		} else if(strcmp(arg0, "outsecret") == 0) {
			if(cb->nf != 2)
				error("usage: outsecret secret");
			setsecret(&c->out, cb->f[1]);
			if(c->cipher)
				c->cipher->init(c);
			if(c->auth)
				c->auth->init(c);
		} else
			error("unknown control request");
		poperror();
		qunlock(c);
		free(cb);
		return n;
	case Qlog:
		cb = parsecmd(a, n);
		p = logctl(sdp, cb->nf, cb->f, logflags);
		free(cb);
		if(p != nil)
			error(p);
		return n;
	case Qcontrol:
		writecontrol(sdp->conv[CONV(ch->qid)], a, n, 0);
		return n;
	case Qdata:
		b = allocb(n);
		memmove(b->wp, a, n);
		b->wp += n;
		return writedata(sdp->conv[CONV(ch->qid)], b);
	}
}

long
sdpbwrite(Chan *ch, Block *bp, ulong offset)
{
	Sdp *sdp = sdptab + ch->dev;

	if(TYPE(ch->qid) != Qdata)
		return devbwrite(ch, bp, offset);
	return writedata(sdp->conv[CONV(ch->qid)], bp);
}

static int
sdpgen(Chan *c, char*, Dirtab*, int, int s, Dir *dp)
{
	Sdp *sdp = sdptab + c->dev;
	int type = TYPE(c->qid);
	Dirtab *dt;
	Qid qid;

	if(s == DEVDOTDOT){
		switch(TYPE(c->qid)){
		case Qtopdir:
		case Qsdpdir:
			snprint(up->genbuf, sizeof(up->genbuf), "#E%ld", c->dev);
			mkqid(&qid, Qtopdir, 0, QTDIR);
			devdir(c, qid, up->genbuf, 0, eve, 0555, dp);
			break;
		case Qconvdir:
			snprint(up->genbuf, sizeof(up->genbuf), "%d", s);
			mkqid(&qid, Qsdpdir, 0, QTDIR);
			devdir(c, qid, up->genbuf, 0, eve, 0555, dp);
			break;
		default:
			panic("sdpwalk %llux", c->qid.path);
		}
		return 1;
	}

	switch(type) {
	default:
		// non directory entries end up here
		if(c->qid.type & QTDIR)
			panic("sdpgen: unexpected directory");	
		if(s != 0)
			return -1;
		dt = dirtab[TYPE(c->qid)];
		if(dt == nil)
			panic("sdpgen: unknown type: %lud", TYPE(c->qid));
		devdir(c, c->qid, dt->name, dt->length, eve, dt->perm, dp);
		return 1;
	case Qtopdir:
		if(s != 0)
			return -1;
		mkqid(&qid, QID(0, Qsdpdir), 0, QTDIR);
		devdir(c, qid, "sdp", 0, eve, 0555, dp);
		return 1;
	case Qsdpdir:
		if(s<nelem(sdpdirtab)) {
			dt = sdpdirtab+s;
			devdir(c, dt->qid, dt->name, dt->length, eve, dt->perm, dp);
			return 1;
		}
		s -= nelem(sdpdirtab);
		if(s >= sdp->nconv)
			return -1;
		mkqid(&qid, QID(s, Qconvdir), 0, QTDIR);
		snprint(up->genbuf, sizeof(up->genbuf), "%d", s);
		devdir(c, qid, up->genbuf, 0, eve, 0555, dp);
		return 1;
	case Qconvdir:
		if(s>=nelem(convdirtab))
			return -1;
		dt = convdirtab+s;
		mkqid(&qid, QID(CONV(c->qid),TYPE(dt->qid)), 0, QTFILE);
		devdir(c, qid, dt->name, dt->length, eve, dt->perm, dp);
		return 1;
	}
}

static Conv*
sdpclone(Sdp *sdp)
{
	Conv *c, **pp, **ep;

	c = nil;
	ep = sdp->conv + nelem(sdp->conv);
	qlock(sdp);
	if(waserror()) {
		qunlock(sdp);
		nexterror();
	}
	for(pp = sdp->conv; pp < ep; pp++) {
		c = *pp;
		if(c == nil){
			c = malloc(sizeof(Conv));
			if(c == nil)
				error(Enomem);
			memset(c, 0, sizeof(Conv));
			qlock(c);
			c->sdp = sdp;
			c->id = pp - sdp->conv;
			*pp = c;
			sdp->nconv++;
			break;
		}
		if(c->ref == 0 && canqlock(c)){
			if(c->ref == 0)
				break;
			qunlock(c);
		}
	}
	poperror();
	qunlock(sdp);

	if(pp >= ep)
		return nil;

	assert(c->state == CFree);
	// set ref to 2 - 1 ref for open - 1 ref for channel state
	c->ref = 2;
	c->state = CInit;
	c->in.window = ~0;
	strncpy(c->owner, up->user, sizeof(c->owner));
	c->perm = 0660;
	qunlock(c);

	return c;
}

// assume c is locked
static void
convretryinit(Conv *c)
{
	c->retries = 0;
	// +2 to avoid rounding effects.
	c->timeout = TK2SEC(m->ticks) + 2;
}

// assume c is locked
static int
convretry(Conv *c, int reset)
{
	c->retries++;
	if(c->retries > MaxRetries) {
		if(reset)
			convoconnect(c, ConReset, c->dialid, c->acceptid);
		convsetstate(c, CClosed);
		return 0;
	}
	c->timeout = TK2SEC(m->ticks) + (c->retries+1);
	return 1;
}

// assumes c is locked
static void
convtimer(Conv *c, ulong sec)
{
	Block *b;

	if(c->timeout > sec)
		return;

	switch(c->state) {
	case CInit:
		break;
	case CDial:
		if(convretry(c, 1))
			convoconnect(c, ConOpenRequest, c->dialid, 0);
		break;
	case CAccept:
		if(convretry(c, 1))
			convoconnect(c, ConOpenAck, c->dialid, c->acceptid);
		break;
	case COpen:
		b = c->out.controlpkt;
		if(b != nil) {
			if(convretry(c, 1))
				convoput(c, TControl, ControlMesg, copyblock(b, blocklen(b)));
			break;
		}

		c->timeout = c->lastrecv + KeepAlive;
		if(c->timeout > sec)
			break;
		// keepalive - randomly spaced between KeepAlive and 2*KeepAlive
		if(c->timeout + KeepAlive > sec && nrand(c->lastrecv + 2*KeepAlive - sec) > 0)
			break;
		// can not use writecontrol
		b = allocb(4);
		c->out.controlseq++;
		hnputl(b->wp, c->out.controlseq);
		b->wp += 4;
		c->out.controlpkt = b;
		convretryinit(c);
		if(!waserror()) {
			convoput(c, TControl, ControlMesg, copyblock(b, blocklen(b)));
			poperror();
		}
		break;
	case CLocalClose:
		if(convretry(c, 0))
			convoconnect(c, ConClose, c->dialid, c->acceptid);
		break;
	case CRemoteClose:
	case CClosed:
		break;
	}
}


static void
sdpackproc(void *a)
{
	Sdp *sdp = a;
	ulong sec;
	int i;
	Conv *c;

	for(;;) {
		tsleep(&up->sleep, return0, 0, 1000);
		sec = TK2SEC(m->ticks);
		qlock(sdp);
		for(i=0; i<sdp->nconv; i++) {
			c = sdp->conv[i];
			if(c->ref == 0)
				continue;
			qunlock(sdp);
			qlock(c);
			if(c->ref > 0 && !waserror()) {
				convtimer(c, sec);
				poperror();
			}
			qunlock(c);
			qlock(sdp);
		}
		qunlock(sdp);
	}
}

Dev sdpdevtab = {
	'E',
	"sdp",

	devreset,
	sdpinit,
	devshutdown,
	sdpattach,
	sdpwalk,
	sdpstat,
	sdpopen,
	devcreate,
	sdpclose,
	sdpread,
	devbread,
	sdpwrite,
	devbwrite,
	devremove,
	devwstat,
};

// assume hold lock on c
static void
convsetstate(Conv *c, int state)
{

if(0)print("convsetstate %d: %s -> %s\n", c->id, convstatename[c->state], convstatename[state]);

	switch(state) {
	default:
		panic("setstate: bad state: %d", state);
	case CDial:
		assert(c->state == CInit);
		c->dialid = (rand()<<16) + rand();
		convretryinit(c);
		convoconnect(c, ConOpenRequest, c->dialid, 0);
		break;
	case CAccept:
		assert(c->state == CInit);
		c->acceptid = (rand()<<16) + rand();
		convretryinit(c);
		convoconnect(c, ConOpenAck, c->dialid, c->acceptid);
		break;
	case COpen:
		assert(c->state == CDial || c->state == CAccept);
		c->lastrecv = TK2SEC(m->ticks);
		if(c->state == CDial) {
			convretryinit(c);
			convoconnect(c, ConOpenAckAck, c->dialid, c->acceptid);
			hnputl(c->in.secret, c->acceptid);
			hnputl(c->in.secret+4, c->dialid);
			hnputl(c->out.secret, c->dialid);
			hnputl(c->out.secret+4, c->acceptid);
		} else {
			hnputl(c->in.secret, c->dialid);
			hnputl(c->in.secret+4, c->acceptid);
			hnputl(c->out.secret, c->acceptid);
			hnputl(c->out.secret+4, c->dialid);
		}
		setalg(c, "hmac_md5_96", authalg, &c->auth);
		break;
	case CLocalClose:
		assert(c->state == CAccept || c->state == COpen);
		convretryinit(c);
		convoconnect(c, ConClose, c->dialid, c->acceptid);
		break;
	case CRemoteClose:
		wakeup(&c->in.controlready);
		wakeup(&c->out.controlready);
		break;
	case CClosed:
		wakeup(&c->in.controlready);
		wakeup(&c->out.controlready);
		if(c->readproc)
			postnote(c->readproc, 1, "interrupt", 0);
		if(c->state != CClosed)
			convderef(c);
		break;
	}
	c->state = state;
}


//assumes c is locked
static void
convderef(Conv *c)
{
	c->ref--;
	if(c->ref > 0) {
		return;
	}
	assert(c->ref == 0);
	assert(c->dataopen == 0);
	assert(c->controlopen == 0);
if(0)print("convderef: %d: ref == 0!\n", c->id);
	c->state = CFree;
	if(c->chan) {	
		cclose(c->chan);
		c->chan = nil;
	}
	if(c->channame) {
		free(c->channame);
		c->channame = nil;
	}
	c->cipher = nil;
	c->auth = nil;
	c->comp = nil;
	strcpy(c->owner, "network");
	c->perm = 0660;
	c->dialid = 0;
	c->acceptid = 0;
	c->timeout = 0;
	c->retries = 0;
	c->drop = 0;
	onewaycleanup(&c->in);
	onewaycleanup(&c->out);
	memset(&c->lstats, 0, sizeof(Stats));
	memset(&c->rstats, 0, sizeof(Stats));
}

static void
onewaycleanup(OneWay *ow)
{
	if(ow->controlpkt)
		freeb(ow->controlpkt);
	if(ow->authstate)
		free(ow->authstate);
	if(ow->cipherstate)
		free(ow->cipherstate);
	if(ow->compstate)
		free(ow->compstate);
	memset(ow, 0, sizeof(OneWay));
}


// assumes conv is locked
static void
convopenchan(Conv *c, char *path)
{
	if(c->state != CInit || c->chan != nil)
		error("already connected");
	c->chan = namec(path, Aopen, ORDWR, 0);
	c->channame = smalloc(strlen(path)+1);
	strcpy(c->channame, path);
	if(waserror()) {
		cclose(c->chan);
		c->chan = nil;
		free(c->channame);
		c->channame = nil;
		nexterror();
	}
	kproc("convreader", convreader, c);

	assert(c->reader == 0 && c->ref > 0);
	// after kproc in case it fails
	c->reader = 1;
	c->ref++;

	poperror();
}

static void
convstats(Conv *c, int local, char *buf, int n)
{
	Stats *stats;
	char *p, *ep;
	int i;

	if(local) {
		stats = &c->lstats;
	} else {
		if(!waserror()) {
			writecontrol(c, 0, 0, 1);
			poperror();
		}
		stats = &c->rstats;
	}

	qlock(c);
	p = buf;
	ep = buf + n;
	p += snprint(p, ep-p, "outPackets: %lud\n", stats->outPackets);
	p += snprint(p, ep-p, "outDataPackets: %lud\n", stats->outDataPackets);
	p += snprint(p, ep-p, "outDataBytes: %lud\n", stats->outDataBytes);
	p += snprint(p, ep-p, "outCompDataBytes: %lud\n", stats->outCompDataBytes);
	for(i=0; i<NCompStats; i++) {
		if(stats->outCompStats[i] == 0)
			continue;
		p += snprint(p, ep-p, "outCompStats[%d]: %lud\n", i, stats->outCompStats[i]);
	}
	p += snprint(p, ep-p, "inPackets: %lud\n", stats->inPackets);
	p += snprint(p, ep-p, "inDataPackets: %lud\n", stats->inDataPackets);
	p += snprint(p, ep-p, "inDataBytes: %lud\n", stats->inDataBytes);
	p += snprint(p, ep-p, "inCompDataBytes: %lud\n", stats->inCompDataBytes);
	p += snprint(p, ep-p, "inMissing: %lud\n", stats->inMissing);
	p += snprint(p, ep-p, "inDup: %lud\n", stats->inDup);
	p += snprint(p, ep-p, "inReorder: %lud\n", stats->inReorder);
	p += snprint(p, ep-p, "inBadComp: %lud\n", stats->inBadComp);
	p += snprint(p, ep-p, "inBadAuth: %lud\n", stats->inBadAuth);
	p += snprint(p, ep-p, "inBadSeq: %lud\n", stats->inBadSeq);
	p += snprint(p, ep-p, "inBadOther: %lud\n", stats->inBadOther);
	USED(p);
	qunlock(c);
}

// c is locked
static void
convack(Conv *c)
{
	Block *b;
	AckPkt *ack;
	Stats *s;
	int i;

	b = allocb(sizeof(AckPkt));
	ack = (AckPkt*)b->wp;
	b->wp += sizeof(AckPkt);
	s = &c->lstats;
	hnputl(ack->cseq, c->in.controlseq);
	hnputl(ack->outPackets, s->outPackets);
	hnputl(ack->outDataPackets, s->outDataPackets);
	hnputl(ack->outDataBytes, s->outDataBytes);
	hnputl(ack->outCompDataBytes, s->outCompDataBytes);
	for(i=0; i<NCompStats; i++)
		hnputl(ack->outCompStats+i*4, s->outCompStats[i]);
	hnputl(ack->inPackets, s->inPackets);
	hnputl(ack->inDataPackets, s->inDataPackets);
	hnputl(ack->inDataBytes, s->inDataBytes);
	hnputl(ack->inCompDataBytes, s->inCompDataBytes);
	hnputl(ack->inMissing, s->inMissing);
	hnputl(ack->inDup, s->inDup);
	hnputl(ack->inReorder, s->inReorder);
	hnputl(ack->inBadComp, s->inBadComp);
	hnputl(ack->inBadAuth, s->inBadAuth);
	hnputl(ack->inBadSeq, s->inBadSeq);
	hnputl(ack->inBadOther, s->inBadOther);
	convoput(c, TControl, ControlAck, b);
}


// assume we hold lock for c
static Block *
conviput(Conv *c, Block *b, int control)
{
	int type, subtype;
	ulong seq, seqwrap;
	long seqdiff;
	int pad;

	c->lstats.inPackets++;

	if(BLEN(b) < 4) {
		c->lstats.inBadOther++;
		freeb(b);
		return nil;
	}
	
	type = b->rp[0] >> 4;
	subtype = b->rp[0] & 0xf;
	b->rp += 1;
	if(type == TConnect) {
		conviconnect(c, subtype, b);
		return nil;
	}

	switch(c->state) {
	case CInit:
	case CDial:
		c->lstats.inBadOther++;
		convoconnect(c, ConReset, c->dialid, c->acceptid);
		convsetstate(c, CClosed);
		break;
	case CAccept:
	case CRemoteClose:
	case CLocalClose:
		c->lstats.inBadOther++;
		freeb(b);
		return nil;
	}

	seq = (b->rp[0]<<16) + (b->rp[1]<<8) + b->rp[2];
	b->rp += 3;

	seqwrap = c->in.seqwrap;
	seqdiff = seq - c->in.seq;
	if(seqdiff < -(SeqMax*3/4)) {
		seqwrap++;
		seqdiff += SeqMax;
	} else if(seqdiff > SeqMax*3/4) {
		seqwrap--;
		seqdiff -= SeqMax;
	}

	if(seqdiff <= 0) {
		if(seqdiff <= -SeqWindow) {
if(0)print("old sequence number: %ld (%ld %ld)\n", seq, c->in.seqwrap, seqdiff);
			c->lstats.inBadSeq++;
			freeb(b);
			return nil;
		}

		if(c->in.window & (1<<-seqdiff)) {
if(0)print("dup sequence number: %ld (%ld %ld)\n", seq, c->in.seqwrap, seqdiff);
			c->lstats.inDup++;
			freeb(b);
			return nil;
		}

		c->lstats.inReorder++;
	}

	// ok the sequence number looks ok
if(0) print("coniput seq=%ulx\n", seq);
	if(c->in.auth != 0) {
		if(!(*c->in.auth)(&c->in, b->rp-4, BLEN(b)+4)) {
if(0)print("bad auth %ld\n", BLEN(b)+4);
			c->lstats.inBadAuth++;
			freeb(b);
			return nil;
		}
		b->wp -= c->in.authlen;
	}

	if(c->in.cipher != 0) {
		if(!(*c->in.cipher)(&c->in, b->rp, BLEN(b))) {
if(0)print("bad cipher\n");
			c->lstats.inBadOther++;
			freeb(b);
			return nil;
		}
		b->rp += c->in.cipherivlen;
		if(c->in.cipherblklen > 1) {
			pad = b->wp[-1];
			if(pad > BLEN(b)) {
if(0)print("pad too big\n");
				c->lstats.inBadOther++;
				freeb(b);
				return nil;
			}
			b->wp -= pad;
		}
	}

	// ok the packet is good
	if(seqdiff > 0) {
		while(seqdiff > 0 && c->in.window != 0) {
			if((c->in.window & (1<<(SeqWindow-1))) == 0) {
				c->lstats.inMissing++;
			}
			c->in.window <<= 1;
			seqdiff--;
		}
		if(seqdiff > 0) {
			c->lstats.inMissing += seqdiff;
			seqdiff = 0;
		}
		c->in.seq = seq;
		c->in.seqwrap = seqwrap;
	}
	c->in.window |= 1<<-seqdiff;
	c->lastrecv = TK2SEC(m->ticks);

	switch(type) {
	case TControl:
		convicontrol(c, subtype, b);
		return nil;
	case TData:
		c->lstats.inDataPackets++;
		c->lstats.inDataBytes += BLEN(b);
		if(control)
			break;
		return b;
	case TCompData:
		c->lstats.inDataPackets++;
		c->lstats.inCompDataBytes += BLEN(b);
		b = convicomp(c, subtype, seq, b);
		if(b == nil) {
			c->lstats.inBadComp++;
			return nil;
		}
		c->lstats.inDataBytes += BLEN(b);
		if(control)
			break;
		return b;
	}
if(0)print("dropping packet id=%d: type=%d n=%ld control=%d\n", c->id, type, BLEN(b), control);
	c->lstats.inBadOther++;
	freeb(b);
	return nil;
}

// assume hold conv lock
static void
conviconnect(Conv *c, int subtype, Block *b)
{
	ulong dialid;
	ulong acceptid;

	if(BLEN(b) != 8) {
		freeb(b);
		return;
	}
	dialid = nhgetl(b->rp);
	acceptid = nhgetl(b->rp + 4);
	freeb(b);

if(0)print("sdp: conviconnect: %s: %d %uld %uld\n", convstatename[c->state], subtype, dialid, acceptid);

	if(subtype == ConReset) {
		convsetstate(c, CClosed);
		return;
	}

	switch(c->state) {
	default:
		panic("unknown state: %d", c->state);
	case CInit:
		break;
	case CDial:
		if(dialid != c->dialid)
			goto Reset;
		break;
	case CAccept:
	case COpen:
	case CLocalClose:
	case CRemoteClose:
		if(dialid != c->dialid
		|| subtype != ConOpenRequest && acceptid != c->acceptid)
			goto Reset;
		break;
	case CClosed:
		goto Reset;
	}

	switch(subtype) {
	case ConOpenRequest:
		switch(c->state) {
		case CInit:
			c->dialid = dialid;
			convsetstate(c, CAccept);
			return;
		case CAccept:
		case COpen:
			// duplicate ConOpenRequest that we ignore
			return;
		}
		break;
	case ConOpenAck:
		switch(c->state) {
		case CDial:
			c->acceptid = acceptid;
			convsetstate(c, COpen);
			return;
		case COpen:
			// duplicate that we have to ack
			convoconnect(c, ConOpenAckAck, acceptid, dialid);
			return;
		}
		break;
	case ConOpenAckAck:
		switch(c->state) {
		case CAccept:
			convsetstate(c, COpen);
			return;
		case COpen:
		case CLocalClose:
		case CRemoteClose:
			// duplicate that we ignore
			return;
		}
		break;
	case ConClose:
		switch(c->state) {
		case COpen:
			convoconnect(c, ConCloseAck, dialid, acceptid);
			convsetstate(c, CRemoteClose);
			return;
		case CRemoteClose:
			// duplicate ConClose
			convoconnect(c, ConCloseAck, dialid, acceptid);
			return;
		}
		break;
	case ConCloseAck:
		switch(c->state) {
		case CLocalClose:
			convsetstate(c, CClosed);
			return;
		}
		break;
	}
Reset:
	// invalid connection message - reset to sender
if(1)print("sdp: invalid conviconnect - sending reset\n");
	convoconnect(c, ConReset, dialid, acceptid);
	convsetstate(c, CClosed);
}

static void
convicontrol(Conv *c, int subtype, Block *b)
{
	ulong cseq;
	AckPkt *ack;
	int i;

	if(BLEN(b) < 4)
		return;
	cseq = nhgetl(b->rp);
	
	switch(subtype){
	case ControlMesg:
		if(cseq == c->in.controlseq) {
if(0)print("duplicate control packet: %ulx\n", cseq);
			// duplicate control packet
			freeb(b);
			if(c->in.controlpkt == nil)
				convack(c);
			return;
		}

		if(cseq != c->in.controlseq+1)
			return;
		c->in.controlseq = cseq;
		b->rp += 4;
		if(BLEN(b) == 0) {
			// just a ping
			freeb(b);
			convack(c);
		} else {
			c->in.controlpkt = b;
if(0) print("recv %ld size=%ld\n", cseq, BLEN(b));
			wakeup(&c->in.controlready);
		}
		return;
	case ControlAck:
		if(cseq != c->out.controlseq)
			return;
		if(BLEN(b) < sizeof(AckPkt))
			return;
		ack = (AckPkt*)(b->rp);
		c->rstats.outPackets = nhgetl(ack->outPackets);
		c->rstats.outDataPackets = nhgetl(ack->outDataPackets);
		c->rstats.outDataBytes = nhgetl(ack->outDataBytes);
		c->rstats.outCompDataBytes = nhgetl(ack->outCompDataBytes);
		for(i=0; i<NCompStats; i++)
			c->rstats.outCompStats[i] = nhgetl(ack->outCompStats + 4*i);
		c->rstats.inPackets = nhgetl(ack->inPackets);
		c->rstats.inDataPackets = nhgetl(ack->inDataPackets);
		c->rstats.inDataBytes = nhgetl(ack->inDataBytes);
		c->rstats.inCompDataBytes = nhgetl(ack->inCompDataBytes);
		c->rstats.inMissing = nhgetl(ack->inMissing);
		c->rstats.inDup = nhgetl(ack->inDup);
		c->rstats.inReorder = nhgetl(ack->inReorder);
		c->rstats.inBadComp = nhgetl(ack->inBadComp);
		c->rstats.inBadAuth = nhgetl(ack->inBadAuth);
		c->rstats.inBadSeq = nhgetl(ack->inBadSeq);
		c->rstats.inBadOther = nhgetl(ack->inBadOther);
		freeb(b);
		freeb(c->out.controlpkt);
		c->out.controlpkt = nil;
		c->timeout = c->lastrecv + KeepAlive;
		wakeup(&c->out.controlready);
		return;
	}
}

static Block*
convicomp(Conv *c, int subtype, ulong seq, Block *b)
{
	if(c->in.comp == nil) {
		freeb(b);
		return nil;
	}
	if(!(*c->in.comp)(c, subtype, seq, &b))
		return nil;
	return b;
}

// c is locked
static void
convwriteblock(Conv *c, Block *b)
{
	// simulated errors
	if(c->drop && nrand(c->drop) == 0)
		return;

	if(waserror()) {
		convsetstate(c, CClosed);
		nexterror();
	}
	devtab[c->chan->type]->bwrite(c->chan, b, 0);
	poperror();
}


// assume hold conv lock
static void
convoput(Conv *c, int type, int subtype, Block *b)
{
	int pad;
	
	c->lstats.outPackets++;
	/* Make room for sdp trailer */
	if(c->out.cipherblklen > 1)
		pad = c->out.cipherblklen - (BLEN(b) + c->out.cipherivlen) % c->out.cipherblklen;
	else
		pad = 0;

	b = padblock(b, -(pad+c->out.authlen));

	if(pad) {
		memset(b->wp, 0, pad-1);
		b->wp[pad-1] = pad;
		b->wp += pad;
	}

	/* Make space to fit sdp header */
	b = padblock(b, 4 + c->out.cipherivlen);
	b->rp[0] = (type << 4) | subtype;
	c->out.seq++;
	if(c->out.seq == (1<<24)) {
		c->out.seq = 0;
		c->out.seqwrap++;
	}
	b->rp[1] = c->out.seq>>16;
	b->rp[2] = c->out.seq>>8;
	b->rp[3] = c->out.seq;
	
	if(c->out.cipher)
		(*c->out.cipher)(&c->out, b->rp+4, BLEN(b)-4);

	// auth
	if(c->out.auth) {
		b->wp += c->out.authlen;
		(*c->out.auth)(&c->out, b->rp, BLEN(b));
	}
	
	convwriteblock(c, b);
}

// assume hold conv lock
static void
convoconnect(Conv *c, int op, ulong dialid, ulong acceptid)
{
	Block *b;

	c->lstats.outPackets++;
	assert(c->chan != nil);
	b = allocb(9);
	b->wp[0] = (TConnect << 4) | op;
	hnputl(b->wp+1, dialid);
	hnputl(b->wp+5, acceptid);
	b->wp += 9;

	if(!waserror()) {
		convwriteblock(c, b);
		poperror();
	}
}

static Block *
convreadblock(Conv *c, int n)
{
	Block *b;
	Chan *ch;

	qlock(&c->readlk);
	if(waserror()) {
		c->readproc = nil;
		qunlock(&c->readlk);
		nexterror();
	}
	qlock(c);
	if(c->state == CClosed) {
		qunlock(c);
		error("closed");
	}
	c->readproc = up;
	ch = c->chan;
	assert(c->ref > 0);
	qunlock(c);

	b = devtab[ch->type]->bread(ch, n, 0);
	c->readproc = nil;
	poperror();
	qunlock(&c->readlk);

	return b;
}

static int
readready(void *a)
{
	Conv *c = a;

	return c->in.controlpkt != nil || (c->state == CClosed) || (c->state == CRemoteClose);
}

static Block *
readcontrol(Conv *c, int n)
{
	Block *b;

	USED(n);

	qlock(&c->in.controllk);
	if(waserror()) {
		qunlock(&c->in.controllk);
		nexterror();
	}
	qlock(c);	// this lock is not held during the sleep below

	for(;;) {
		if(c->chan == nil || c->state == CClosed) {
			qunlock(c);
if(0)print("readcontrol: return error - state = %s\n", convstatename[c->state]);
			error("conversation closed");
		}

		if(c->in.controlpkt != nil)
			break;

		if(c->state == CRemoteClose) {
			qunlock(c);
if(0)print("readcontrol: return nil - state = %s\n", convstatename[c->state]);
			poperror();
			return nil;
		}
		qunlock(c);
		sleep(&c->in.controlready, readready, c);
		qlock(c);
	}

	convack(c);

	b = c->in.controlpkt;
	c->in.controlpkt = nil;
	qunlock(c);
	poperror();
	qunlock(&c->in.controllk);
	return b;
}


static int
writeready(void *a)
{
	Conv *c = a;

	return c->out.controlpkt == nil || (c->state == CClosed) || (c->state == CRemoteClose);
}

// c is locked
static void
writewait(Conv *c)
{
	for(;;) {
		if(c->state == CFree || c->state == CInit ||
		   c->state == CClosed || c->state == CRemoteClose)
			error("conversation closed");

		if(c->state == COpen && c->out.controlpkt == nil)
			break;

		qunlock(c);
		if(waserror()) {
			qlock(c);
			nexterror();
		}
		sleep(&c->out.controlready, writeready, c);
		poperror();
		qlock(c);
	}
}

static void
writecontrol(Conv *c, void *p, int n, int wait)
{
	Block *b;

	qlock(&c->out.controllk);
	qlock(c);
	if(waserror()) {
		qunlock(c);
		qunlock(&c->out.controllk);
		nexterror();
	}
	writewait(c);
	b = allocb(4+n);
	c->out.controlseq++;
	hnputl(b->wp, c->out.controlseq);
	memmove(b->wp+4, p, n);
	b->wp += 4+n;
	c->out.controlpkt = b;
	convretryinit(c);
	convoput(c, TControl, ControlMesg, copyblock(b, blocklen(b)));
	if(wait)
		writewait(c);
	poperror();
	qunlock(c);
	qunlock(&c->out.controllk);
}

static Block *
readdata(Conv *c, int n)
{
	Block *b;
	int nn;

	for(;;) {

		// some slack for tunneling overhead
		nn = n + 100;

		// make sure size is big enough for control messages
		if(nn < 1000)
			nn = 1000;
		b = convreadblock(c, nn);
		if(b == nil)
			return nil;
		qlock(c);
		if(waserror()) {
			qunlock(c);
			return nil;
		}
		b = conviput(c, b, 0);
		poperror();
		qunlock(c);
		if(b != nil) {
			if(BLEN(b) > n)
				b->wp = b->rp + n;
			return b;
		}
	}
}

static long
writedata(Conv *c, Block *b)
{
	int n;
	ulong seq;
	int subtype;

	qlock(c);
	if(waserror()) {
		qunlock(c);
		nexterror();
	}

	if(c->state != COpen) {
		freeb(b);
		error("conversation not open");
	}

	n = BLEN(b);
	c->lstats.outDataPackets++;
	c->lstats.outDataBytes += n;

	if(c->out.comp != nil) {
		// must generate same value as convoput
		seq = (c->out.seq + 1) & (SeqMax-1);

		subtype = (*c->out.comp)(c, 0, seq, &b);
		c->lstats.outCompDataBytes += BLEN(b);
		convoput(c, TCompData, subtype, b);
	} else
		convoput(c, TData, 0, b);

	poperror();
	qunlock(c);
	return n;
}

static void
convreader(void *a)
{
	Conv *c = a;
	Block *b;

	qlock(c);
	assert(c->reader == 1);
	while(c->dataopen == 0 && c->state != CClosed) {
		qunlock(c);
		b = nil;
		if(!waserror()) {
			b = convreadblock(c, 2000);
			poperror();
		}
		qlock(c);
		if(b == nil) {
			if(strcmp(up->errstr, Eintr) != 0) {
				convsetstate(c, CClosed);
				break;
			}
		} else if(!waserror()) {
			conviput(c, b, 1);
			poperror();
		}
	}
	c->reader = 0;
	convderef(c);
	qunlock(c);
	pexit("hangup", 1);
}


/* ciphers, authenticators, and compressors  */

static void
setalg(Conv *c, char *name, Algorithm *alg, Algorithm **p)
{
	for(; alg->name; alg++)
		if(strcmp(name, alg->name) == 0)
			break;
	if(alg->name == nil)
		error("unknown algorithm");

	*p = alg;
	alg->init(c);
}

static void
setsecret(OneWay *ow, char *secret)
{
	char *p;
	int i, c;
	
	i = 0;
	memset(ow->secret, 0, sizeof(ow->secret));
	for(p=secret; *p; p++) {
		if(i >= sizeof(ow->secret)*2)
			break;
		c = *p;
		if(c >= '0' && c <= '9')
			c -= '0';
		else if(c >= 'a' && c <= 'f')
			c -= 'a'-10;
		else if(c >= 'A' && c <= 'F')
			c -= 'A'-10;
		else
			error("bad character in secret");
		if((i&1) == 0)
			c <<= 4;
		ow->secret[i>>1] |= c;
		i++;
	}
}

static void
setkey(uchar *key, int n, OneWay *ow, char *prefix)
{
	uchar ibuf[SHA1dlen], obuf[MD5dlen], salt[10];
	int i, round = 0;

	while(n > 0){
		for(i=0; i<round+1; i++)
			salt[i] = 'A'+round;
		sha1((uchar*)prefix, strlen(prefix), ibuf, sha1(salt, round+1, nil, nil));
		md5(ibuf, SHA1dlen, obuf, md5(ow->secret, sizeof(ow->secret), nil, nil));
		i = (n<MD5dlen) ? n : MD5dlen;
		memmove(key, obuf, i);
		key += i;
		n -= i;
		if(++round > sizeof salt)
			panic("setkey: you ask too much");
	}
}

static void
cipherfree(Conv *c)
{
	if(c->in.cipherstate) {
		free(c->in.cipherstate);
		c->in.cipherstate = nil;
	}
	if(c->out.cipherstate) {
		free(c->out.cipherstate);
		c->out.cipherstate = nil;
	}
	c->in.cipher = nil;
	c->in.cipherblklen = 0;
	c->out.cipherblklen = 0;
	c->in.cipherivlen = 0;
	c->out.cipherivlen = 0;
}

static void
authfree(Conv *c)
{
	if(c->in.authstate) {
		free(c->in.authstate);
		c->in.authstate = nil;
	}
	if(c->out.authstate) {
		free(c->out.authstate);
		c->out.authstate = nil;
	}
	c->in.auth = nil;
	c->in.authlen = 0;
	c->out.authlen = 0;
}

static void
compfree(Conv *c)
{
	if(c->in.compstate) {
		free(c->in.compstate);
		c->in.compstate = nil;
	}
	if(c->out.compstate) {
		free(c->out.compstate);
		c->out.compstate = nil;
	}
	c->in.comp = nil;
}

static void
nullcipherinit(Conv *c)
{
	cipherfree(c);
}

static int
desencrypt(OneWay *ow, uchar *p, int n)
{
	uchar *pp, *ip, *eip, *ep;
	DESstate *ds = ow->cipherstate;

	if(n < 8 || (n & 0x7 != 0))
		return 0;
	ep = p + n;
	memmove(p, ds->ivec, 8);
	for(p += 8; p < ep; p += 8){
		pp = p;
		ip = ds->ivec;
		for(eip = ip+8; ip < eip; )
			*pp++ ^= *ip++;
		block_cipher(ds->expanded, p, 0);
		memmove(ds->ivec, p, 8);
	}
	return 1;
}

static int
desdecrypt(OneWay *ow, uchar *p, int n)
{
	uchar tmp[8];
	uchar *tp, *ip, *eip, *ep;
	DESstate *ds = ow->cipherstate;

	if(n < 8 || (n & 0x7 != 0))
		return 0;
	ep = p + n;
	memmove(ds->ivec, p, 8);
	p += 8;
	while(p < ep){
		memmove(tmp, p, 8);
		block_cipher(ds->expanded, p, 1);
		tp = tmp;
		ip = ds->ivec;
		for(eip = ip+8; ip < eip; ){
			*p++ ^= *ip;
			*ip++ = *tp++;
		}
	}
	return 1;
}

static void
descipherinit(Conv *c)
{
	uchar key[8];
	uchar ivec[8];
	int i;
	int n = c->cipher->keylen;

	cipherfree(c);
	
	if(n > sizeof(key))
		n = sizeof(key);

	/* in */
	memset(key, 0, sizeof(key));
	setkey(key, n, &c->in, "cipher");
	memset(ivec, 0, sizeof(ivec));
	c->in.cipherblklen = 8;
	c->in.cipherivlen = 8;
	c->in.cipher = desdecrypt;
	c->in.cipherstate = smalloc(sizeof(DESstate));
	setupDESstate(c->in.cipherstate, key, ivec);
	
	/* out */
	memset(key, 0, sizeof(key));
	setkey(key, n, &c->out, "cipher");
	for(i=0; i<8; i++)
		ivec[i] = nrand(256);
	c->out.cipherblklen = 8;
	c->out.cipherivlen = 8;
	c->out.cipher = desencrypt;
	c->out.cipherstate = smalloc(sizeof(DESstate));
	setupDESstate(c->out.cipherstate, key, ivec);
}

static int
rc4encrypt(OneWay *ow, uchar *p, int n)
{
	CipherRc4 *cr = ow->cipherstate;

	if(n < 4)
		return 0;

	hnputl(p, cr->cseq);
	p += 4;
	n -= 4;
	rc4(&cr->current, p, n);
	cr->cseq += n;
	return 1;
}

static int
rc4decrypt(OneWay *ow, uchar *p, int n)
{
	CipherRc4 *cr = ow->cipherstate;
	RC4state tmpstate;
	ulong seq;
	long d, dd;

	if(n < 4)
		return 0;

	seq = nhgetl(p);
	p += 4;
	n -= 4;
	d = seq-cr->cseq;
	if(d == 0) {
		rc4(&cr->current, p, n);
		cr->cseq += n;
		if(cr->ovalid) {
			dd = cr->cseq - cr->lgseq;
			if(dd > RC4back)
				cr->ovalid = 0;
		}
	} else if(d > 0) {
//print("missing packet: %uld %ld\n", seq, d);
		// this link is hosed 
		if(d > RC4forward)
			return 0;
		cr->lgseq = seq;
		if(!cr->ovalid) {
			cr->ovalid = 1;
			cr->oseq = cr->cseq;
			memmove(&cr->old, &cr->current, sizeof(RC4state));
		}
		rc4skip(&cr->current, d);
		rc4(&cr->current, p, n);
		cr->cseq = seq+n;
	} else {
//print("reordered packet: %uld %ld\n", seq, d);
		dd = seq - cr->oseq;
		if(!cr->ovalid || -d > RC4back || dd < 0)
			return 0;
		memmove(&tmpstate, &cr->old, sizeof(RC4state));
		rc4skip(&tmpstate, dd);
		rc4(&tmpstate, p, n);
		return 1;
	}

	// move old state up
	if(cr->ovalid) {
		dd = cr->cseq - RC4back - cr->oseq;
		if(dd > 0) {
			rc4skip(&cr->old, dd);
			cr->oseq += dd;
		}
	}

	return 1;
}

static void
rc4cipherinit(Conv *c)
{
	uchar key[32];
	CipherRc4 *cr;
	int n;

	cipherfree(c);

	n = c->cipher->keylen;
	if(n > sizeof(key))
		n = sizeof(key);

	/* in */
	memset(key, 0, sizeof(key));
	setkey(key, n, &c->in, "cipher");
	c->in.cipherblklen = 1;
	c->in.cipherivlen = 4;
	c->in.cipher = rc4decrypt;
	cr = smalloc(sizeof(CipherRc4));
	memset(cr, 0, sizeof(*cr));
	setupRC4state(&cr->current, key, n);
	c->in.cipherstate = cr;

	/* out */
	memset(key, 0, sizeof(key));
	setkey(key, n, &c->out, "cipher");
	c->out.cipherblklen = 1;
	c->out.cipherivlen = 4;
	c->out.cipher = rc4encrypt;
	cr = smalloc(sizeof(CipherRc4));
	memset(cr, 0, sizeof(*cr));
	setupRC4state(&cr->current, key, n);
	c->out.cipherstate = cr;
}

static void
nullauthinit(Conv *c)
{
	authfree(c);
}

static void
shaauthinit(Conv *c)
{
	authfree(c);
}

static void
seanq_hmac_md5(uchar hash[MD5dlen], ulong wrap, uchar *t, long tlen, uchar *key, long klen)
{
	uchar ipad[65], opad[65], wbuf[4];
	int i;
	DigestState *digest;
	uchar innerhash[MD5dlen];

	for(i=0; i<64; i++){
		ipad[i] = 0x36;
		opad[i] = 0x5c;
	}
	ipad[64] = opad[64] = 0;
	for(i=0; i<klen; i++){
		ipad[i] ^= key[i];
		opad[i] ^= key[i];
	}
	hnputl(wbuf, wrap);
	digest = md5(ipad, 64, nil, nil);
	digest = md5(wbuf, sizeof(wbuf), nil, digest);
	md5(t, tlen, innerhash, digest);
	digest = md5(opad, 64, nil, nil);
	md5(innerhash, MD5dlen, hash, digest);
}

static int
md5auth(OneWay *ow, uchar *t, int tlen)
{
	uchar hash[MD5dlen];
	int r;

	if(tlen < ow->authlen)
		return 0;
	tlen -= ow->authlen;

	memset(hash, 0, MD5dlen);
	seanq_hmac_md5(hash, ow->seqwrap, t, tlen, (uchar*)ow->authstate, 16);
	r = memcmp(t+tlen, hash, ow->authlen) == 0;
	memmove(t+tlen, hash, ow->authlen);
	return r;
}

static void
md5authinit(Conv *c)
{
	int keylen;

	authfree(c);

	keylen = c->auth->keylen;
	if(keylen > 16)
		keylen = 16;

	/* in */
	c->in.authstate = smalloc(16);
	memset(c->in.authstate, 0, 16);
	setkey(c->in.authstate, keylen, &c->in, "auth");
	c->in.authlen = 12;
	c->in.auth = md5auth;
	
	/* out */
	c->out.authstate = smalloc(16);
	memset(c->out.authstate, 0, 16);
	setkey(c->out.authstate, keylen, &c->out, "auth");
	c->out.authlen = 12;
	c->out.auth = md5auth;
}

static void
nullcompinit(Conv *c)
{
	compfree(c);
}

static int
thwackcomp(Conv *c, int, ulong seq, Block **bp)
{
	Block *b, *bb;
	int nn;
	ulong ackseq;
	uchar mask;

	// add ack info
	b = padblock(*bp, 4);

	ackseq = unthwackstate(c->in.compstate, &mask);
	b->rp[0] = mask;
	b->rp[1] = ackseq>>16;
	b->rp[2] = ackseq>>8;
	b->rp[3] = ackseq;

	bb = allocb(BLEN(b));
	nn = thwack(c->out.compstate, bb->wp, b->rp, BLEN(b), seq, c->lstats.outCompStats);
	if(nn < 0) {
		freeb(bb);
		*bp = b;
		return ThwackU;
	} else {
		bb->wp += nn;
		freeb(b);
		*bp = bb;
		return ThwackC;
	}
}

static int
thwackuncomp(Conv *c, int subtype, ulong seq, Block **bp)
{
	Block *b, *bb;
	ulong mask;
	ulong mseq;
	int n;

	switch(subtype) {
	default:
		return 0;
	case ThwackU:
		b = *bp;
		mask = b->rp[0];
		mseq = (b->rp[1]<<16) | (b->rp[2]<<8) | b->rp[3];
		b->rp += 4;
		thwackack(c->out.compstate, mseq, mask);
		return 1;
	case ThwackC:
		bb = *bp;
		b = allocb(ThwMaxBlock);
		n = unthwack(c->in.compstate, b->wp, ThwMaxBlock, bb->rp, BLEN(bb), seq);
		freeb(bb);
		*bp = nil;
		if(n < 0) {
if(0)print("unthwack failed: %d\n", n);
			freeb(b);
			return 0;
		}
		b->wp += n;
		mask = b->rp[0];
		mseq = (b->rp[1]<<16) | (b->rp[2]<<8) | b->rp[3];
		thwackack(c->out.compstate, mseq, mask);
		b->rp += 4;
		*bp = b;
		return 1;
	}
}

static void
thwackcompinit(Conv *c)
{
	compfree(c);

	c->in.compstate = malloc(sizeof(Unthwack));
	if(c->in.compstate == nil)
		error(Enomem);
	unthwackinit(c->in.compstate);
	c->out.compstate = malloc(sizeof(Thwack));
	if(c->out.compstate == nil)
		error(Enomem);
	thwackinit(c->out.compstate);
	c->in.comp = thwackuncomp;
	c->out.comp = thwackcomp;
}
