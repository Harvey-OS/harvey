#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include	"../ip/ip.h"

enum
{
	Qtopdir=	1,		/* top level directory */
	Qtopbase,
	Qarp=		Qtopbase,
	Qbootp,
	Qndb,
	Qiproute,
	Qipselftab,
	Qlog,

	Qprotodir,			/* directory for a protocol */
	Qprotobase,
	Qclone=		Qprotobase,
	Qstats,

	Qconvdir,			/* directory for a conversation */
	Qconvbase,
	Qctl=		Qconvbase,
	Qdata,
	Qerr,
	Qlisten,
	Qlocal,
	Qremote,
	Qstatus,
	Qsnoop,

	Logtype=	5,
	Masktype=	(1<<Logtype)-1,
	Logconv=	12,
	Maskconv=	(1<<Logconv)-1,
	Shiftconv=	Logtype,
	Logproto=	8,
	Maskproto=	(1<<Logproto)-1,
	Shiftproto=	Logtype + Logconv,

	Nfs=		128,
};
#define TYPE(x) 	( ((ulong)(x).path) & Masktype )
#define CONV(x) 	( (((ulong)(x).path) >> Shiftconv) & Maskconv )
#define PROTO(x) 	( (((ulong)(x).path) >> Shiftproto) & Maskproto )
#define QID(p, c, y) 	( ((p)<<(Shiftproto)) | ((c)<<Shiftconv) | (y) )

static char network[] = "network";

QLock	fslock;
Fs	*ipfs[Nfs];	/* attached fs's */
Queue	*qlog;

extern	void nullmediumlink(void);
extern	void pktmediumlink(void);
	long ndbwrite(Fs *f, char *a, ulong off, int n);

static int
ip3gen(Chan *c, int i, Dir *dp)
{
	Qid q;
	Conv *cv;
	char *p;

	cv = ipfs[c->dev]->p[PROTO(c->qid)]->conv[CONV(c->qid)];
	if(cv->owner == nil)
		kstrdup(&cv->owner, eve);
	mkqid(&q, QID(PROTO(c->qid), CONV(c->qid), i), 0, QTFILE);

	switch(i) {
	default:
		return -1;
	case Qctl:
		devdir(c, q, "ctl", 0, cv->owner, cv->perm, dp);
		return 1;
	case Qdata:
		devdir(c, q, "data", qlen(cv->rq), cv->owner, cv->perm, dp);
		return 1;
	case Qerr:
		devdir(c, q, "err", qlen(cv->eq), cv->owner, cv->perm, dp);
		return 1;
	case Qlisten:
		devdir(c, q, "listen", 0, cv->owner, cv->perm, dp);
		return 1;
	case Qlocal:
		p = "local";
		break;
	case Qremote:
		p = "remote";
		break;
	case Qsnoop:
		if(strcmp(cv->p->name, "ipifc") != 0)
			return -1;
		devdir(c, q, "snoop", qlen(cv->sq), cv->owner, 0400, dp);
		return 1;
	case Qstatus:
		p = "status";
		break;
	}
	devdir(c, q, p, 0, cv->owner, 0444, dp);
	return 1;
}

static int
ip2gen(Chan *c, int i, Dir *dp)
{
	Qid q;

	switch(i) {
	case Qclone:
		mkqid(&q, QID(PROTO(c->qid), 0, Qclone), 0, QTFILE);
		devdir(c, q, "clone", 0, network, 0666, dp);
		return 1;
	case Qstats:
		mkqid(&q, QID(PROTO(c->qid), 0, Qstats), 0, QTFILE);
		devdir(c, q, "stats", 0, network, 0444, dp);
		return 1;
	}
	return -1;
}

static int
ip1gen(Chan *c, int i, Dir *dp)
{
	Qid q;
	char *p;
	int prot;
	int len = 0;
	Fs *f;
	extern ulong	kerndate;

	f = ipfs[c->dev];

	prot = 0666;
	mkqid(&q, QID(0, 0, i), 0, QTFILE);
	switch(i) {
	default:
		return -1;
	case Qarp:
		p = "arp";
		prot = 0664;
		break;
	case Qbootp:
		p = "bootp";
		break;
	case Qndb:
		p = "ndb";
		len = strlen(f->ndb);
		q.vers = f->ndbvers;
		break;
	case Qiproute:
		p = "iproute";
		prot = 0664;
		break;
	case Qipselftab:
		p = "ipselftab";
		prot = 0444;
		break;
	case Qlog:
		p = "log";
		break;
	}
	devdir(c, q, p, len, network, prot, dp);
	if(i == Qndb && f->ndbmtime > kerndate)
		dp->mtime = f->ndbmtime;
	return 1;
}

static int
ipgen(Chan *c, char*, Dirtab*, int, int s, Dir *dp)
{
	Qid q;
	Conv *cv;
	Fs *f;

	f = ipfs[c->dev];

	switch(TYPE(c->qid)) {
	case Qtopdir:
		if(s == DEVDOTDOT){
			mkqid(&q, QID(0, 0, Qtopdir), 0, QTDIR);
			snprint(up->genbuf, sizeof up->genbuf, "#I%lud", c->dev);
			devdir(c, q, up->genbuf, 0, network, 0555, dp);
			return 1;
		}
		if(s < f->np) {
			if(f->p[s]->connect == nil)
				return 0;	/* protocol with no user interface */
			mkqid(&q, QID(s, 0, Qprotodir), 0, QTDIR);
			devdir(c, q, f->p[s]->name, 0, network, 0555, dp);
			return 1;
		}
		s -= f->np;
		return ip1gen(c, s+Qtopbase, dp);
	case Qarp:
	case Qbootp:
	case Qndb:
	case Qlog:
	case Qiproute:
	case Qipselftab:
		return ip1gen(c, TYPE(c->qid), dp);
	case Qprotodir:
		if(s == DEVDOTDOT){
			mkqid(&q, QID(0, 0, Qtopdir), 0, QTDIR);
			snprint(up->genbuf, sizeof up->genbuf, "#I%lud", c->dev);
			devdir(c, q, up->genbuf, 0, network, 0555, dp);
			return 1;
		}
		if(s < f->p[PROTO(c->qid)]->ac) {
			cv = f->p[PROTO(c->qid)]->conv[s];
			snprint(up->genbuf, sizeof up->genbuf, "%d", s);
			mkqid(&q, QID(PROTO(c->qid), s, Qconvdir), 0, QTDIR);
			devdir(c, q, up->genbuf, 0, cv->owner, 0555, dp);
			return 1;
		}
		s -= f->p[PROTO(c->qid)]->ac;
		return ip2gen(c, s+Qprotobase, dp);
	case Qclone:
	case Qstats:
		return ip2gen(c, TYPE(c->qid), dp);
	case Qconvdir:
		if(s == DEVDOTDOT){
			s = PROTO(c->qid);
			mkqid(&q, QID(s, 0, Qprotodir), 0, QTDIR);
			devdir(c, q, f->p[s]->name, 0, network, 0555, dp);
			return 1;
		}
		return ip3gen(c, s+Qconvbase, dp);
	case Qctl:
	case Qdata:
	case Qerr:
	case Qlisten:
	case Qlocal:
	case Qremote:
	case Qstatus:
	case Qsnoop:
		return ip3gen(c, TYPE(c->qid), dp);
	}
	return -1;
}

static void
ipreset(void)
{
	nullmediumlink();
	pktmediumlink();

	fmtinstall('i', eipfmt);
	fmtinstall('I', eipfmt);
	fmtinstall('E', eipfmt);
	fmtinstall('V', eipfmt);
	fmtinstall('M', eipfmt);
}

static Fs*
ipgetfs(int dev)
{
	extern void (*ipprotoinit[])(Fs*);
	Fs *f;
	int i;

	if(dev >= Nfs)
		return nil;

	qlock(&fslock);
	if(ipfs[dev] == nil){
		f = smalloc(sizeof(Fs));
		ip_init(f);
		arpinit(f);
		netloginit(f);
		for(i = 0; ipprotoinit[i]; i++)
			ipprotoinit[i](f);
		f->dev = dev;
		ipfs[dev] = f;
	}
	qunlock(&fslock);

	return ipfs[dev];
}

IPaux*
newipaux(char *owner, char *tag)
{
	IPaux *a;
	int n;

	a = smalloc(sizeof(*a));
	kstrdup(&a->owner, owner);
	memset(a->tag, ' ', sizeof(a->tag));
	n = strlen(tag);
	if(n > sizeof(a->tag))
		n = sizeof(a->tag);
	memmove(a->tag, tag, n);
	return a;
}

#define ATTACHER(c) (((IPaux*)((c)->aux))->owner)

static Chan*
ipattach(char* spec)
{
	Chan *c;
	int dev;

	dev = atoi(spec);
	if(dev >= Nfs)
		error("bad specification");

	ipgetfs(dev);
	c = devattach('I', spec);
	mkqid(&c->qid, QID(0, 0, Qtopdir), 0, QTDIR);
	c->dev = dev;

	c->aux = newipaux(commonuser(), "none");

	return c;
}

static Walkqid*
ipwalk(Chan* c, Chan *nc, char **name, int nname)
{
	IPaux *a = c->aux;
	Walkqid* w;

	w = devwalk(c, nc, name, nname, nil, 0, ipgen);
	if(w != nil && w->clone != nil)
		w->clone->aux = newipaux(a->owner, a->tag);
	return w;
}


static int
ipstat(Chan* c, uchar* db, int n)
{
	return devstat(c, db, n, nil, 0, ipgen);
}

static int
incoming(void* arg)
{
	Conv *conv;

	conv = arg;
	return conv->incall != nil;
}

static int m2p[] = {
	[OREAD]		4,
	[OWRITE]	2,
	[ORDWR]		6
};

static Chan*
ipopen(Chan* c, int omode)
{
	Conv *cv, *nc;
	Proto *p;
	int perm;
	Fs *f;

	perm = m2p[omode&3];

	f = ipfs[c->dev];

	switch(TYPE(c->qid)) {
	default:
		break;
	case Qndb:
		if(omode & (OWRITE|OTRUNC) && !iseve())
			error(Eperm);
		if((omode & (OWRITE|OTRUNC)) == (OWRITE|OTRUNC))
			f->ndb[0] = 0;
		break;
	case Qlog:
		netlogopen(f);
		break;
	case Qiproute:
	case Qarp:
		if(omode != OREAD && !iseve())
			error(Eperm);
		break;
	case Qtopdir:
	case Qprotodir:
	case Qconvdir:
	case Qstatus:
	case Qremote:
	case Qlocal:
	case Qstats:
	case Qbootp:
	case Qipselftab:
		if(omode != OREAD)
			error(Eperm);
		break;
	case Qsnoop:
		if(omode != OREAD)
			error(Eperm);
		p = f->p[PROTO(c->qid)];
		cv = p->conv[CONV(c->qid)];
		if(strcmp(ATTACHER(c), cv->owner) != 0 && !iseve())
			error(Eperm);
		incref(&cv->snoopers);
		break;
	case Qclone:
		p = f->p[PROTO(c->qid)];
		qlock(p);
		if(waserror()){
			qunlock(p);
			nexterror();
		}
		cv = Fsprotoclone(p, ATTACHER(c));
		qunlock(p);
		poperror();
		if(cv == nil) {
			error(Enodev);
			break;
		}
		mkqid(&c->qid, QID(p->x, cv->x, Qctl), 0, QTFILE);
		break;
	case Qdata:
	case Qctl:
	case Qerr:
		p = f->p[PROTO(c->qid)];
		qlock(p);
		cv = p->conv[CONV(c->qid)];
		qlock(cv);
		if(waserror()) {
			qunlock(cv);
			qunlock(p);
			nexterror();
		}
		if((perm & (cv->perm>>6)) != perm) {
			if(strcmp(ATTACHER(c), cv->owner) != 0)
				error(Eperm);
		 	if((perm & cv->perm) != perm)
				error(Eperm);

		}
		cv->inuse++;
		if(cv->inuse == 1){
			kstrdup(&cv->owner, ATTACHER(c));
			cv->perm = 0660;
		}
		qunlock(cv);
		qunlock(p);
		poperror();
		break;
	case Qlisten:
		cv = f->p[PROTO(c->qid)]->conv[CONV(c->qid)];
		if((perm & (cv->perm>>6)) != perm) {
			if(strcmp(ATTACHER(c), cv->owner) != 0)
				error(Eperm);
		 	if((perm & cv->perm) != perm)
				error(Eperm);

		}

		if(cv->state != Announced)
			error("not announced");

		if(waserror()){
			closeconv(cv);
			nexterror();
		}
		qlock(cv);
		cv->inuse++;
		qunlock(cv);

		nc = nil;
		while(nc == nil) {
			/* give up if we got a hangup */
			if(qisclosed(cv->rq))
				error("listen hungup");

			qlock(&cv->listenq);
			if(waserror()) {
				qunlock(&cv->listenq);
				nexterror();
			}

			/* wait for a connect */
			sleep(&cv->listenr, incoming, cv);

			qlock(cv);
			nc = cv->incall;
			if(nc != nil){
				cv->incall = nc->next;
				mkqid(&c->qid, QID(PROTO(c->qid), nc->x, Qctl), 0, QTFILE);
				kstrdup(&cv->owner, ATTACHER(c));
			}
			qunlock(cv);

			qunlock(&cv->listenq);
			poperror();
		}
		closeconv(cv);
		poperror();
		break;
	}
	c->mode = openmode(omode);
	c->flag |= COPEN;
	c->offset = 0;
	return c;
}

static void
ipcreate(Chan*, char*, int, ulong)
{
	error(Eperm);
}

static void
ipremove(Chan*)
{
	error(Eperm);
}

static int
ipwstat(Chan *c, uchar *dp, int n)
{
	Dir d;
	Conv *cv;
	Fs *f;
	Proto *p;

	f = ipfs[c->dev];
	switch(TYPE(c->qid)) {
	default:
		error(Eperm);
		break;
	case Qctl:
	case Qdata:
		break;
	}

	n = convM2D(dp, n, &d, nil);
	if(n > 0){
		p = f->p[PROTO(c->qid)];
		cv = p->conv[CONV(c->qid)];
		if(!iseve() && strcmp(ATTACHER(c), cv->owner) != 0)
			error(Eperm);
		if(d.uid[0])
			kstrdup(&cv->owner, d.uid);
		cv->perm = d.mode & 0777;
	}
	return n;
}

void
closeconv(Conv *cv)
{
	Conv *nc;
	Ipmulti *mp;

	qlock(cv);

	if(--cv->inuse > 0) {
		qunlock(cv);
		return;
	}

	/* close all incoming calls since no listen will ever happen */
	for(nc = cv->incall; nc; nc = cv->incall){
		cv->incall = nc->next;
		closeconv(nc);
	}
	cv->incall = nil;

	kstrdup(&cv->owner, network);
	cv->perm = 0660;

	while((mp = cv->multi) != nil)
		ipifcremmulti(cv, mp->ma, mp->ia);

	cv->r = nil;
	cv->rgen = 0;
	cv->p->close(cv);
	cv->state = Idle;
	qunlock(cv);
}

static void
ipclose(Chan* c)
{
	Fs *f;

	f = ipfs[c->dev];
	switch(TYPE(c->qid)) {
	default:
		break;
	case Qlog:
		if(c->flag & COPEN)
			netlogclose(f);
		break;
	case Qdata:
	case Qctl:
	case Qerr:
		if(c->flag & COPEN)
			closeconv(f->p[PROTO(c->qid)]->conv[CONV(c->qid)]);
		break;
	case Qsnoop:
		if(c->flag & COPEN)
			decref(&f->p[PROTO(c->qid)]->conv[CONV(c->qid)]->snoopers);
		break;
	}
	free(((IPaux*)c->aux)->owner);
	free(c->aux);
}

enum
{
	Statelen=	32*1024,
};

static long
ipread(Chan *ch, void *a, long n, vlong off)
{
	Conv *c;
	Proto *x;
	char *buf, *p;
	long rv;
	Fs *f;
	ulong offset = off;

	f = ipfs[ch->dev];

	p = a;
	switch(TYPE(ch->qid)) {
	default:
		error(Eperm);
	case Qtopdir:
	case Qprotodir:
	case Qconvdir:
		return devdirread(ch, a, n, 0, 0, ipgen);
	case Qarp:
		return arpread(f->arp, a, offset, n);
 	case Qbootp:
 		return bootpread(a, offset, n);
 	case Qndb:
		return readstr(offset, a, n, f->ndb);
	case Qiproute:
		return routeread(f, a, offset, n);
	case Qipselftab:
		return ipselftabread(f, a, offset, n);
	case Qlog:
		return netlogread(f, a, offset, n);
	case Qctl:
		buf = smalloc(16);
		snprint(buf, 16, "%lud", CONV(ch->qid));
		rv = readstr(offset, p, n, buf);
		free(buf);
		return rv;
	case Qremote:
		buf = smalloc(Statelen);
		x = f->p[PROTO(ch->qid)];
		c = x->conv[CONV(ch->qid)];
		if(x->remote == nil) {
			snprint(buf, Statelen, "%I!%d\n", c->raddr, c->rport);
		} else {
			(*x->remote)(c, buf, Statelen-2);
		}
		rv = readstr(offset, p, n, buf);
		free(buf);
		return rv;
	case Qlocal:
		buf = smalloc(Statelen);
		x = f->p[PROTO(ch->qid)];
		c = x->conv[CONV(ch->qid)];
		if(x->local == nil) {
			snprint(buf, Statelen, "%I!%d\n", c->laddr, c->lport);
		} else {
			(*x->local)(c, buf, Statelen-2);
		}
		rv = readstr(offset, p, n, buf);
		free(buf);
		return rv;
	case Qstatus:
		buf = smalloc(Statelen);
		x = f->p[PROTO(ch->qid)];
		c = x->conv[CONV(ch->qid)];
		(*x->state)(c, buf, Statelen-2);
		rv = readstr(offset, p, n, buf);
		free(buf);
		return rv;
	case Qdata:
		c = f->p[PROTO(ch->qid)]->conv[CONV(ch->qid)];
		return qread(c->rq, a, n);
	case Qerr:
		c = f->p[PROTO(ch->qid)]->conv[CONV(ch->qid)];
		return qread(c->eq, a, n);
	case Qsnoop:
		c = f->p[PROTO(ch->qid)]->conv[CONV(ch->qid)];
		return qread(c->sq, a, n);
	case Qstats:
		x = f->p[PROTO(ch->qid)];
		if(x->stats == nil)
			error("stats not implemented");
		buf = smalloc(Statelen);
		(*x->stats)(x, buf, Statelen);
		rv = readstr(offset, p, n, buf);
		free(buf);
		return rv;
	}
}

static Block*
ipbread(Chan* ch, long n, ulong offset)
{
	Conv *c;
	Proto *x;
	Fs *f;

	switch(TYPE(ch->qid)){
	case Qdata:
		f = ipfs[ch->dev];
		x = f->p[PROTO(ch->qid)];
		c = x->conv[CONV(ch->qid)];
		return qbread(c->rq, n);
	default:
		return devbread(ch, n, offset);
	}
}

/*
 *  set local address to be that of the ifc closest to remote address
 */
static void
setladdr(Conv* c)
{
	findlocalip(c->p->f, c->laddr, c->raddr);
}

/*
 *  set a local port making sure the quad of raddr,rport,laddr,lport is unique
 */
char*
setluniqueport(Conv* c, int lport)
{
	Proto *p;
	Conv *xp;
	int x;

	p = c->p;

	qlock(p);
	for(x = 0; x < p->nc; x++){
		xp = p->conv[x];
		if(xp == nil)
			break;
		if(xp == c)
			continue;
		if((xp->state == Connected || xp->state == Announced)
		&& xp->lport == lport
		&& xp->rport == c->rport
		&& ipcmp(xp->raddr, c->raddr) == 0
		&& ipcmp(xp->laddr, c->laddr) == 0){
			qunlock(p);
			return "address in use";
		}
	}
	c->lport = lport;
	qunlock(p);
	return nil;
}

/*
 * is lport in use by anyone?
 */
static int
lportinuse(Proto *p, ushort lport)
{
	int x;

	for(x = 0; x < p->nc && p->conv[x]; x++)
		if(p->conv[x]->lport == lport)
			return 1;
	return 0;
}

/*
 *  pick a local port and set it
 */
char *
setlport(Conv* c)
{
	Proto *p;
	int i, port;

	p = c->p;
	qlock(p);
	if(c->restricted){
		/* Restricted ports cycle between 600 and 1024. */
		for(i=0; i<1024-600; i++){
			if(p->nextrport >= 1024 || p->nextrport < 600)
				p->nextrport = 600;
			port = p->nextrport++;
			if(!lportinuse(p, port))
				goto chosen;
		}
	}else{
		/*
		 * Unrestricted ports are chosen randomly
		 * between 2^15 and 2^16.  There are at most
		 * 4*Nchan = 4096 ports in use at any given time,
		 * so even in the worst case, a random probe has a
		 * 1 - 4096/2^15 = 87% chance of success.
		 * If 64 successive probes fail, there is a bug somewhere
		 * (or a once in 10^58 event has happened, but that's
		 * less likely than a venti collision).
		 */
		for(i=0; i<64; i++){
			port = (1<<15) + nrand(1<<15);
			if(!lportinuse(p, port))
				goto chosen;
		}
	}
	qunlock(p);
	/*
	 * debugging: let's see if we ever get this.
	 * if we do (and we're a cpu server), we might as well restart
	 * since we're now unable to service new connections.
	 */
	panic("setlport: out of ports");
	return "no ports available";

chosen:
	c->lport = port;
	qunlock(p);
	return nil;
}

/*
 *  set a local address and port from a string of the form
 *	[address!]port[!r]
 */
char*
setladdrport(Conv* c, char* str, int announcing)
{
	char *p;
	char *rv;
	ushort lport;
	uchar addr[IPaddrlen];

	/*
	 *  ignore restricted part if it exists.  it's
	 *  meaningless on local ports.
	 */
	p = strchr(str, '!');
	if(p != nil){
		*p++ = 0;
		if(strcmp(p, "r") == 0)
			p = nil;
	}

	c->lport = 0;
	if(p == nil){
		if(announcing)
			ipmove(c->laddr, IPnoaddr);
		else
			setladdr(c);
		p = str;
	} else {
		if(strcmp(str, "*") == 0)
			ipmove(c->laddr, IPnoaddr);
		else {
			if(parseip(addr, str) == -1)
				return Ebadip;
			if(ipforme(c->p->f, addr))
				ipmove(c->laddr, addr);
			else
				return "not a local IP address";
		}
	}

	/* one process can get all connections */
	if(announcing && strcmp(p, "*") == 0){
		if(!iseve())
			error(Eperm);
		return setluniqueport(c, 0);
	}

	lport = atoi(p);
	if(lport <= 0)
		rv = setlport(c);
	else
		rv = setluniqueport(c, lport);
	return rv;
}

static char*
setraddrport(Conv* c, char* str)
{
	char *p;

	p = strchr(str, '!');
	if(p == nil)
		return "malformed address";
	*p++ = 0;
	if (parseip(c->raddr, str) == -1)
		return Ebadip;
	c->rport = atoi(p);
	p = strchr(p, '!');
	if(p){
		if(strstr(p, "!r") != nil)
			c->restricted = 1;
	}
	return nil;
}

/*
 *  called by protocol connect routine to set addresses
 */
char*
Fsstdconnect(Conv *c, char *argv[], int argc)
{
	char *p;

	switch(argc) {
	default:
		return "bad args to connect";
	case 2:
		p = setraddrport(c, argv[1]);
		if(p != nil)
			return p;
		setladdr(c);
		p = setlport(c);
		if (p != nil)
			return p;
		break;
	case 3:
		p = setraddrport(c, argv[1]);
		if(p != nil)
			return p;
		p = setladdrport(c, argv[2], 0);
		if(p != nil)
			return p;
	}

	if( (memcmp(c->raddr, v4prefix, IPv4off) == 0 &&
		memcmp(c->laddr, v4prefix, IPv4off) == 0)
		|| ipcmp(c->raddr, IPnoaddr) == 0)
		c->ipversion = V4;
	else
		c->ipversion = V6;

	return nil;
}
/*
 *  initiate connection and sleep till its set up
 */
static int
connected(void* a)
{
	return ((Conv*)a)->state == Connected;
}
static void
connectctlmsg(Proto *x, Conv *c, Cmdbuf *cb)
{
	char *p;

	if(c->state != 0)
		error(Econinuse);
	c->state = Connecting;
	c->cerr[0] = '\0';
	if(x->connect == nil)
		error("connect not supported");
	p = x->connect(c, cb->f, cb->nf);
	if(p != nil)
		error(p);

	qunlock(c);
	if(waserror()){
		qlock(c);
		nexterror();
	}
	sleep(&c->cr, connected, c);
	qlock(c);
	poperror();

	if(c->cerr[0] != '\0')
		error(c->cerr);
}

/*
 *  called by protocol announce routine to set addresses
 */
char*
Fsstdannounce(Conv* c, char* argv[], int argc)
{
	memset(c->raddr, 0, sizeof(c->raddr));
	c->rport = 0;
	switch(argc){
	default:
		break;
	case 2:
		return setladdrport(c, argv[1], 1);
	}
	return "bad args to announce";
}

/*
 *  initiate announcement and sleep till its set up
 */
static int
announced(void* a)
{
	return ((Conv*)a)->state == Announced;
}
static void
announcectlmsg(Proto *x, Conv *c, Cmdbuf *cb)
{
	char *p;

	if(c->state != 0)
		error(Econinuse);
	c->state = Announcing;
	c->cerr[0] = '\0';
	if(x->announce == nil)
		error("announce not supported");
	p = x->announce(c, cb->f, cb->nf);
	if(p != nil)
		error(p);

	qunlock(c);
	if(waserror()){
		qlock(c);
		nexterror();
	}
	sleep(&c->cr, announced, c);
	qlock(c);
	poperror();

	if(c->cerr[0] != '\0')
		error(c->cerr);
}

/*
 *  called by protocol bind routine to set addresses
 */
char*
Fsstdbind(Conv* c, char* argv[], int argc)
{
	switch(argc){
	default:
		break;
	case 2:
		return setladdrport(c, argv[1], 0);
	}
	return "bad args to bind";
}

static void
bindctlmsg(Proto *x, Conv *c, Cmdbuf *cb)
{
	char *p;

	if(x->bind == nil)
		p = Fsstdbind(c, cb->f, cb->nf);
	else
		p = x->bind(c, cb->f, cb->nf);
	if(p != nil)
		error(p);
}

static void
tosctlmsg(Conv *c, Cmdbuf *cb)
{
	if(cb->nf < 2)
		c->tos = 0;
	else
		c->tos = atoi(cb->f[1]);
}

static void
ttlctlmsg(Conv *c, Cmdbuf *cb)
{
	if(cb->nf < 2)
		c->ttl = MAXTTL;
	else
		c->ttl = atoi(cb->f[1]);
}

static long
ipwrite(Chan* ch, void *v, long n, vlong off)
{
	Conv *c;
	Proto *x;
	char *p;
	Cmdbuf *cb;
	uchar ia[IPaddrlen], ma[IPaddrlen];
	Fs *f;
	char *a;
	ulong offset = off;

	a = v;
	f = ipfs[ch->dev];

	switch(TYPE(ch->qid)){
	default:
		error(Eperm);
	case Qdata:
		x = f->p[PROTO(ch->qid)];
		c = x->conv[CONV(ch->qid)];

		if(c->wq == nil)
			error(Eperm);

		qwrite(c->wq, a, n);
		break;
	case Qarp:
		return arpwrite(f, a, n);
	case Qiproute:
		return routewrite(f, ch, a, n);
	case Qlog:
		netlogctl(f, a, n);
		return n;
	case Qndb:
		return ndbwrite(f, a, offset, n);
		break;
	case Qctl:
		x = f->p[PROTO(ch->qid)];
		c = x->conv[CONV(ch->qid)];
		cb = parsecmd(a, n);

		qlock(c);
		if(waserror()) {
			qunlock(c);
			free(cb);
			nexterror();
		}
		if(cb->nf < 1)
			error("short control request");
		if(strcmp(cb->f[0], "connect") == 0)
			connectctlmsg(x, c, cb);
		else if(strcmp(cb->f[0], "announce") == 0)
			announcectlmsg(x, c, cb);
		else if(strcmp(cb->f[0], "bind") == 0)
			bindctlmsg(x, c, cb);
		else if(strcmp(cb->f[0], "ttl") == 0)
			ttlctlmsg(c, cb);
		else if(strcmp(cb->f[0], "tos") == 0)
			tosctlmsg(c, cb);
		else if(strcmp(cb->f[0], "ignoreadvice") == 0)
			c->ignoreadvice = 1;
		else if(strcmp(cb->f[0], "addmulti") == 0){
			if(cb->nf < 2)
				error("addmulti needs interface address");
			if(cb->nf == 2){
				if(!ipismulticast(c->raddr))
					error("addmulti for a non multicast address");
				if (parseip(ia, cb->f[1]) == -1)
					error(Ebadip);
				ipifcaddmulti(c, c->raddr, ia);
			} else {
				if (parseip(ia, cb->f[1]) == -1 ||
				    parseip(ma, cb->f[2]) == -1)
					error(Ebadip);
				if(!ipismulticast(ma))
					error("addmulti for a non multicast address");
				ipifcaddmulti(c, ma, ia);
			}
		} else if(strcmp(cb->f[0], "remmulti") == 0){
			if(cb->nf < 2)
				error("remmulti needs interface address");
			if(!ipismulticast(c->raddr))
				error("remmulti for a non multicast address");
			if (parseip(ia, cb->f[1]) == -1)
				error(Ebadip);
			ipifcremmulti(c, c->raddr, ia);
		} else if(strcmp(cb->f[0], "maxfragsize") == 0){
			if(cb->nf < 2)
				error("maxfragsize needs size");

			c->maxfragsize = (int)strtol(cb->f[1], nil, 0);
			
		} else if(x->ctl != nil) {
			p = x->ctl(c, cb->f, cb->nf);
			if(p != nil)
				error(p);
		} else
			error("unknown control request");
		qunlock(c);
		free(cb);
		poperror();
	}
	return n;
}

static long
ipbwrite(Chan* ch, Block* bp, ulong offset)
{
	Conv *c;
	Proto *x;
	Fs *f;
	int n;

	switch(TYPE(ch->qid)){
	case Qdata:
		f = ipfs[ch->dev];
		x = f->p[PROTO(ch->qid)];
		c = x->conv[CONV(ch->qid)];

		if(c->wq == nil)
			error(Eperm);

		if(bp->next)
			bp = concatblock(bp);
		n = BLEN(bp);
		qbwrite(c->wq, bp);
		return n;
	default:
		return devbwrite(ch, bp, offset);
	}
}

Dev ipdevtab = {
	'I',
	"ip",

	ipreset,
	devinit,
	devshutdown,
	ipattach,
	ipwalk,
	ipstat,
	ipopen,
	ipcreate,
	ipclose,
	ipread,
	ipbread,
	ipwrite,
	ipbwrite,
	ipremove,
	ipwstat,
};

int
Fsproto(Fs *f, Proto *p)
{
	if(f->np >= Maxproto)
		return -1;

	p->f = f;

	if(p->ipproto > 0){
		if(f->t2p[p->ipproto] != nil)
			return -1;
		f->t2p[p->ipproto] = p;
	}

	p->qid.type = QTDIR;
	p->qid.path = QID(f->np, 0, Qprotodir);
	p->conv = malloc(sizeof(Conv*)*(p->nc+1));
	if(p->conv == nil)
		panic("Fsproto");

	p->x = f->np;
	p->nextrport = 600;
	f->p[f->np++] = p;

	return 0;
}

/*
 *  return true if this protocol is
 *  built in
 */
int
Fsbuiltinproto(Fs* f, uchar proto)
{
	return f->t2p[proto] != nil;
}

/*
 *  called with protocol locked
 */
Conv*
Fsprotoclone(Proto *p, char *user)
{
	Conv *c, **pp, **ep;

retry:
	c = nil;
	ep = &p->conv[p->nc];
	for(pp = p->conv; pp < ep; pp++) {
		c = *pp;
		if(c == nil){
			c = malloc(sizeof(Conv));
			if(c == nil)
				error(Enomem);
			qlock(c);
			c->p = p;
			c->x = pp - p->conv;
			if(p->ptclsize != 0){
				c->ptcl = malloc(p->ptclsize);
				if(c->ptcl == nil) {
					free(c);
					error(Enomem);
				}
			}
			*pp = c;
			p->ac++;
			c->eq = qopen(1024, Qmsg, 0, 0);
			(*p->create)(c);
			break;
		}
		if(canqlock(c)){
			/*
			 *  make sure both processes and protocol
			 *  are done with this Conv
			 */
			if(c->inuse == 0 && (p->inuse == nil || (*p->inuse)(c) == 0))
				break;

			qunlock(c);
		}
	}
	if(pp >= ep) {
		if(p->gc)
			print("Fsprotoclone: garbage collecting Convs\n");
		if(p->gc != nil && (*p->gc)(p))
			goto retry;
		/* debugging: do we ever get here? */
		if (cpuserver)
			panic("Fsprotoclone: all conversations in use");
		return nil;
	}

	c->inuse = 1;
	kstrdup(&c->owner, user);
	c->perm = 0660;
	c->state = Idle;
	ipmove(c->laddr, IPnoaddr);
	ipmove(c->raddr, IPnoaddr);
	c->r = nil;
	c->rgen = 0;
	c->lport = 0;
	c->rport = 0;
	c->restricted = 0;
	c->maxfragsize = 0;
	c->ttl = MAXTTL;
	qreopen(c->rq);
	qreopen(c->wq);
	qreopen(c->eq);

	qunlock(c);
	return c;
}

int
Fsconnected(Conv* c, char* msg)
{
	if(msg != nil && *msg != '\0')
		strncpy(c->cerr, msg, ERRMAX-1);

	switch(c->state){

	case Announcing:
		c->state = Announced;
		break;

	case Connecting:
		c->state = Connected;
		break;
	}

	wakeup(&c->cr);
	return 0;
}

Proto*
Fsrcvpcol(Fs* f, uchar proto)
{
	if(f->ipmux)
		return f->ipmux;
	else
		return f->t2p[proto];
}

Proto*
Fsrcvpcolx(Fs *f, uchar proto)
{
	return f->t2p[proto];
}

/*
 *  called with protocol locked
 */
Conv*
Fsnewcall(Conv *c, uchar *raddr, ushort rport, uchar *laddr, ushort lport, uchar version)
{
	Conv *nc;
	Conv **l;
	int i;

	qlock(c);
	i = 0;
	for(l = &c->incall; *l; l = &(*l)->next)
		i++;
	if(i >= Maxincall) {
		static int beenhere;

		qunlock(c);
		if (!beenhere) {
			beenhere = 1;
			print("Fsnewcall: incall queue full (%d) on port %d\n",
				i, c->lport);
		}
		return nil;
	}

	/* find a free conversation */
	nc = Fsprotoclone(c->p, network);
	if(nc == nil) {
		qunlock(c);
		return nil;
	}
	ipmove(nc->raddr, raddr);
	nc->rport = rport;
	ipmove(nc->laddr, laddr);
	nc->lport = lport;
	nc->next = nil;
	*l = nc;
	nc->state = Connected;
	nc->ipversion = version;

	qunlock(c);

	wakeup(&c->listenr);

	return nc;
}

long
ndbwrite(Fs *f, char *a, ulong off, int n)
{
	if(off > strlen(f->ndb))
		error(Eio);
	if(off+n >= sizeof(f->ndb))
		error(Eio);
	memmove(f->ndb+off, a, n);
	f->ndb[off+n] = 0;
	f->ndbvers++;
	f->ndbmtime = seconds();
	return n;
}

ulong
scalednconv(void)
{
	if(cpuserver && conf.npage*BY2PG >= 128*MB)
		return Nchans*4;
	return Nchans;
}
