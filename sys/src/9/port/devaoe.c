/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 *	© 2005-7 coraid
 *	aoe storage initiator
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"

#include "../port/netif.h"
#include "../ip/ip.h"

#include "etherif.h"
#include "aoe.h"

#pragma varargck argpos eventlog 1

#define dprint(...)                    \
	if(debug)                      \
		eventlog(__VA_ARGS__); \
	else                           \
		USED(debug);
#define uprint(...) snprint(up->genbuf, sizeof up->genbuf, __VA_ARGS__);

enum {
	Maxunits = 0xff,
	Maxframes = 128,
	Ndevlink = 6,
	Nea = 6,
	Nnetlink = 6,

	//	ETHERMINTU	= 60,		/* minimum transmit size */
	//	ETHERMAXTU	= 1514,		/* maximum transmit size */
};

#define TYPE(q) ((uint32_t)(q).path & 0xf)
#define UNIT(q) (((uint32_t)(q).path >> 4) & 0xff)
#define L(q) (((uint32_t)(q).path >> 12) & 0xf)
#define QID(u, t) ((u) << 4 | (t))
#define Q3(l, u, t) ((l) << 8 | QID(u, t))
#define UP(d) ((d)->flag & Dup)

#define Ms2tk(t) (((t)*HZ) / 1000)
#define Tk2ms(t) (((t)*1000) / HZ)

enum {
	Qzero,
	Qtopdir = 1,
	Qtopbase,
	Qtopctl = Qtopbase,
	Qtoplog,
	Qtopend,

	Qunitdir,
	Qunitbase,
	Qctl = Qunitbase,
	Qdata,
	Qconfig,
	Qident,

	Qdevlinkdir,
	Qdevlinkbase,
	Qdevlink = Qdevlinkbase,
	Qdevlinkend,

	Qtopfiles = Qtopend - Qtopbase,
	Qdevlinkfiles = Qdevlinkend - Qdevlinkbase,

	Eventlen = 256,
	Nevents = 64,

	Fread = 0,
	Fwrite,
	Tfree = -1,
	Tmgmt,

	/* round trip bounds, timeouts, in ticks */
	Rtmax = Ms2tk(320),
	Rtmin = Ms2tk(20),
	Srbtimeout = 4 * 60 * HZ,

	Dbcnt = 1024,

	Crd = 0x20,
	Crdext = 0x24,
	Cwr = 0x30,
	Cwrext = 0x34,
	Cid = 0xec,
};

enum {
	Read,
	Write,
};

/*
 * unified set of flags
 * a Netlink + Aoedev most both be jumbo capable
 * to send jumbograms to that interface.
 */
enum {
	/* sync with ahci.h */
	Dllba = 1 << 0,
	Dsmart = 1 << 1,
	Dpower = 1 << 2,
	Dnop = 1 << 3,
	Datapi = 1 << 4,
	Datapi16 = 1 << 5,

	/* aoe specific */
	Dup = 1 << 6,
	Djumbo = 1 << 7,
};

static char *flagname[] = {
    "llba",
    "smart",
    "power",
    "nop",
    "atapi",
    "atapi16",

    "up",
    "jumbo",
};

typedef struct {
	uint8_t flag;
	uint8_t lostjumbo;
	int datamtu;

	Chan *cc;
	Chan *dc;
	Chan *mtu; /* open early to prevent bind issues. */
	char path[Maxpath];
	uint8_t ea[Eaddrlen];
} Netlink;

typedef struct {
	Netlink *nl;
	int nea;
	uint32_t eaidx;
	uint8_t eatab[Nea][Eaddrlen];
	uint32_t npkt;
	uint32_t resent;
	uint8_t flag;

	uint32_t rttavg;
	uint32_t mintimer;
} Devlink;

typedef struct Srb Srb;
struct Srb {
	Rendez;
	Srb *next;
	uint32_t ticksent;
	uint32_t len;
	int64_t sector;
	short write;
	short nout;
	char *error;
	void *dp;
	void *data;
};

typedef struct {
	int tag;
	uint32_t bcnt;
	uint32_t dlen;
	int64_t lba;
	uint32_t ticksent;
	int nhdr;
	uint8_t hdr[ETHERMINTU];
	void *dp;
	Devlink *dl;
	Netlink *nl;
	int eaidx;
	Srb *srb;
} Frame;

typedef struct Aoedev Aoedev;
struct Aoedev {
	QLock;
	Aoedev *next;

	uint32_t vers;

	int ndl;
	uint32_t dlidx;
	Devlink *dl;
	Devlink dltab[Ndevlink];

	uint16_t fwver;
	unsigned char flag;
	int nopen;
	int major;
	int minor;
	int unit;
	int lasttag;
	int nframes;
	Frame *frames;
	int64_t bsize;
	int64_t realbsize;

	uint maxbcnt;
	uint32_t lostjumbo;
	uint16_t nout;
	uint16_t maxout;
	uint32_t lastwadj;
	Srb *_head;
	Srb *_tail;
	Srb *inprocess;

	/* magic numbers 'R' us */
	char serial[20 + 1];
	char firmware[8 + 1];
	char model[40 + 1];
	int nconfig;
	unsigned char config[1024];
	unsigned char ident[512];
};

#pragma varargck type "æ" Aoedev *

static struct {
	Lock;
	QLock;
	Rendez _rendez;
	char buf[Eventlen * Nevents];
	char *rp;
	char *wp;
} events;

static struct {
	RWlock;
	int nd;
	Aoedev *d;
} devs;

static struct {
	Lock;
	int reader[Nnetlink];    /* reader is running. */
	Rendez rendez[Nnetlink]; /* confirm exit. */
	Netlink nl[Nnetlink];
} netlinks;

extern Dev aoedevtab;
static Ref units;
static Ref drivevers;
static int debug;
static int autodiscover = 1;
static int rediscover;

char Enotup[] = "aoe device is down";
char Echange[] = "media or partition has changed";

static Srb *
srballoc(uint32_t sz)
{
	Srb *srb;

	srb = malloc(sizeof *srb + sz);
	srb->dp = srb->data = srb + 1;
	srb->ticksent = sys->ticks;
	return srb;
}

static Srb *
srbkalloc(void *db, uint32_t n)
{
	Srb *srb;

	srb = malloc(sizeof *srb);
	srb->dp = srb->data = db;
	srb->ticksent = sys->ticks;
	return srb;
}

#define srbfree(srb) free(srb)

static void
srberror(Srb *srb, char *s)
{
	srb->error = s;
	srb->nout--;
	if(srb->nout == 0)
		wakeup(srb);
}

static void
frameerror(Aoedev *d, Frame *f, char *s)
{
	Srb *srb;

	srb = f->srb;
	if(f->tag == Tfree || !srb)
		return;
	f->srb = nil;
	f->tag = Tfree; /* don't get fooled by way-slow responses */
	srberror(srb, s);
	d->nout--;
}

static char *
unitname(Aoedev *d)
{
	Proc *up = externup();
	uprint("%d.%d", d->major, d->minor);
	return up->genbuf;
}

#if 0
static int
eventlogready(void* v)
{
	return *events.rp;
}
#endif

static int32_t
eventlogread(void *a, int32_t n)
{
	Proc *up = externup();
	int len;
	char *p, *buf;

	buf = smalloc(Eventlen);
	qlock(&events);
	lock(&events);
	p = events.rp;
	len = *p;
	if(len == 0) {
		n = 0;
		unlock(&events);
	} else {
		if(n > len)
			n = len;
		/* can't move directly into pageable space with events lock held */
		memmove(buf, p + 1, n);
		*p = 0;
		events.rp = p += Eventlen;
		if(p >= events.buf + sizeof events.buf)
			events.rp = events.buf;
		unlock(&events);

		/* the concern here is page faults in memmove below */
		if(waserror()) {
			free(buf);
			qunlock(&events);
			nexterror();
		}
		memmove(a, buf, n);
		poperror();
	}
	free(buf);
	qunlock(&events);
	return n;
}

static int
eventlog(char *fmt, ...)
{
	int dragrp, n;
	char *p;
	va_list arg;

	lock(&events);
	p = events.wp;
	dragrp = *p++;
	va_start(arg, fmt);
	n = vsnprint(p, Eventlen - 1, fmt, arg);
	*--p = n;
	p = events.wp += Eventlen;
	if(p >= events.buf + sizeof events.buf)
		p = events.wp = events.buf;
	if(dragrp)
		events.rp = p;
	unlock(&events);
	wakeup(&events._rendez);
	return n;
}

static int
eventcount(void)
{
	int n;

	lock(&events);
	if(*events.rp == 0)
		n = 0;
	else if(events.wp < events.rp)
		n = Nevents - (events.rp - events.wp);
	else
		n = events.wp - events.rp;
	unlock(&events);
	return n / Eventlen;
}

static int
tsince(int tag)
{
	int n;

	n = sys->ticks & 0xffff;
	n -= tag & 0xffff;
	if(n < 0)
		n += 1 << 16;
	return n;
}

static int
newtag(Aoedev *d)
{
	int t;

	do {
		t = ++d->lasttag << 16;
		t |= sys->ticks & 0xffff;
	} while(t == Tfree || t == Tmgmt);
	return t;
}

static void
downdev(Aoedev *d, char *err)
{
	Frame *f, *e;

	d->flag &= ~Dup;
	f = d->frames;
	e = f + d->nframes;
	for(; f < e; f->tag = Tfree, f->srb = nil, f++)
		frameerror(d, f, Enotup);
	d->inprocess = nil;
	eventlog("%æ: removed; %s\n", d, err);
}

static Block *
allocfb(Frame *f)
{
	int len;
	Block *b;

	len = f->nhdr + f->dlen;
	if(len < ETHERMINTU)
		len = ETHERMINTU;
	b = allocb(len);
	memmove(b->wp, f->hdr, f->nhdr);
	if(f->dlen)
		memmove(b->wp + f->nhdr, f->dp, f->dlen);
	b->wp += len;
	return b;
}

static void
putlba(Aoeata *a, int64_t lba)
{
	uint8_t *c;

	c = a->lba;
	c[0] = lba;
	c[1] = lba >> 8;
	c[2] = lba >> 16;
	c[3] = lba >> 24;
	c[4] = lba >> 32;
	c[5] = lba >> 40;
}

static Devlink *
pickdevlink(Aoedev *d)
{
	uint32_t i, n;
	Devlink *l;

	for(i = 0; i < d->ndl; i++) {
		n = d->dlidx++ % d->ndl;
		l = d->dl + n;
		if(l && l->flag & Dup)
			return l;
	}
	return 0;
}

static int
pickea(Devlink *l)
{
	if(l == 0)
		return -1;
	if(l->nea == 0)
		return -1;
	return l->eaidx++ % l->nea;
}

static int
hset(Aoedev *d, Frame *f, Aoehdr *h, int cmd)
{
	int i;
	Devlink *l;

	l = pickdevlink(d);
	i = pickea(l);
	if(i == -1) {
		downdev(d, "resend fails; no netlink/ea");
		return -1;
	}
	if(f->srb && sys->ticks - f->srb->ticksent > Srbtimeout) {
		eventlog("%æ: srb timeout\n", d);
		frameerror(d, f, Etimedout);
		return -1;
	}
	memmove(h->dst, l->eatab[i], Eaddrlen);
	memmove(h->src, l->nl->ea, sizeof h->src);
	hnputs(h->type, Aoetype);
	h->verflag = Aoever << 4;
	h->error = 0;
	hnputs(h->major, d->major);
	h->minor = d->minor;
	h->cmd = cmd;

	hnputl(h->tag, f->tag = newtag(d));
	f->dl = l;
	f->nl = l->nl;
	f->eaidx = i;
	f->ticksent = sys->ticks;

	return f->tag;
}

static int
resend(Aoedev *d, Frame *f)
{
	Proc *up = externup();
	uint32_t n;
	Aoeata *a;

	a = (Aoeata *)f->hdr;
	if(hset(d, f, a, a->cmd) == -1)
		return -1;
	n = f->bcnt;
	if(n > d->maxbcnt) {
		n = d->maxbcnt; /* mtu mismatch (jumbo fail?) */
		if(f->dlen > n)
			f->dlen = n;
	}
	a->scnt = n / Aoesectsz;
	f->dl->resent++;
	f->dl->npkt++;
	if(waserror())
		return -1;
	f->nl->dc->dev->bwrite(f->nl->dc, allocfb(f), 0);
	poperror();
	return 0;
}

static void
discover(int major, int minor)
{
	Proc *up = externup();
	Aoehdr *h;
	Block *b;
	Netlink *nl, *e;

	nl = netlinks.nl;
	e = nl + nelem(netlinks.nl);
	for(; nl < e; nl++) {
		if(nl->cc == nil)
			continue;
		b = allocb(ETHERMINTU);
		if(waserror()) {
			freeb(b);
			nexterror();
		}
		b->wp = b->rp + ETHERMINTU;
		memset(b->rp, 0, ETHERMINTU);
		h = (Aoehdr *)b->rp;
		memset(h->dst, 0xff, sizeof h->dst);
		memmove(h->src, nl->ea, sizeof h->src);
		hnputs(h->type, Aoetype);
		h->verflag = Aoever << 4;
		hnputs(h->major, major);
		h->minor = minor;
		h->cmd = ACconfig;
		poperror();
		/* send b down the queue */
		nl->dc->dev->bwrite(nl->dc, b, 0);
	}
}

/*
 * Check all frames on device and resend any frames that have been
 * outstanding for 200% of the device round trip time average.
 */
static void
aoesweepproc(void *v)
{
	Proc *up = externup();
	uint32_t i, tx, timeout, nbc;
	int64_t starttick;
	enum { Nms = 100,
	       Nbcms = 30 * 1000,
	};
	uint8_t *ea;
	Aoeata *a;
	Aoedev *d;
	Devlink *l;
	Frame *f, *e;

	nbc = Nbcms / Nms;
loop:
	if(nbc-- == 0) {
		if(rediscover && !waserror()) {
			discover(0xffff, 0xff);
			poperror();
		}
		nbc = Nbcms / Nms;
	}
	starttick = sys->ticks;
	rlock(&devs);
	for(d = devs.d; d; d = d->next) {
		if(!canqlock(d))
			continue;
		if(!UP(d)) {
			qunlock(d);
			continue;
		}
		tx = 0;
		f = d->frames;
		e = f + d->nframes;
		for(; f < e; f++) {
			if(f->tag == Tfree)
				continue;
			l = f->dl;
			timeout = l->rttavg << 1;
			i = tsince(f->tag);
			if(i < timeout)
				continue;
			if(d->nout == d->maxout) {
				if(d->maxout > 1)
					d->maxout--;
				d->lastwadj = sys->ticks;
			}
			a = (Aoeata *)f->hdr;
			if(a->scnt > Dbcnt / Aoesectsz &&
			   ++f->nl->lostjumbo > (d->nframes << 1)) {
				ea = f->dl->eatab[f->eaidx];
				eventlog("%æ: jumbo failure on %s:%E; lba%lld\n",
					 d, f->nl->path, ea, f->lba);
				d->maxbcnt = Dbcnt;
				d->flag &= ~Djumbo;
			}
			resend(d, f);
			if(tx++ == 0) {
				if((l->rttavg <<= 1) > Rtmax)
					l->rttavg = Rtmax;
				eventlog("%æ: rtt %ldms\n", d, Tk2ms(l->rttavg));
			}
		}
		if(d->nout == d->maxout && d->maxout < d->nframes &&
		   TK2MS(sys->ticks - d->lastwadj) > 10 * 1000) {
			d->maxout++;
			d->lastwadj = sys->ticks;
		}
		qunlock(d);
	}
	runlock(&devs);
	i = Nms - TK2MS(sys->ticks - starttick);
	if(i > 0)
		tsleep(&up->sleep, return0, 0, i);
	goto loop;
}

static int
fmta(Fmt *f)
{
	char buf[8];
	Aoedev *d;

	d = va_arg(f->args, Aoedev *);
	snprint(buf, sizeof buf, "aoe%d.%d", d->major, d->minor);
	return fmtstrcpy(f, buf);
}

static void netbind(char *path);

static void
aoecfg(void)
{
	Proc *up = externup();
	int n, i;
	char *p, *f[32], buf[24];

	// if((p = getconf("aoeif")) == nil || (n = tokenize(p, f, nelem(f))) < 1)
	return;
	/* goo! */
	for(i = 0; i < n; i++) {
		p = f[i];
		if(strncmp(p, "ether", 5) == 0)
			snprint(buf, sizeof buf, "#l%c/ether%c", p[5], p[5]);
		else if(strncmp(p, "#l", 2) == 0)
			snprint(buf, sizeof buf, "#l%c/ether%c", p[2], p[2]);
		else
			continue;
		if(!waserror()) {
			netbind(buf);
			poperror();
		}
	}
}

static void
aoeinit(void)
{
	static int init;
	static QLock l;

	if(!canqlock(&l))
		return;
	if(init == 0) {
		fmtinstall(L'a', fmta);
		events.rp = events.wp = events.buf;
		kproc("aoesweep", aoesweepproc, nil);
		aoecfg();
		init = 1;
	}
	qunlock(&l);
}

static Chan *
aoeattach(char *spec)
{
	Chan *c;

	if(*spec)
		error(Enonexist);
	aoeinit();
	c = devattach(L'æ', spec);
	mkqid(&c->qid, Qzero, 0, QTDIR);
	return c;
}

static Aoedev *
unit2dev(uint32_t unit)
{
	Proc *up = externup();
	int i;
	Aoedev *d;

	rlock(&devs);
	i = 0;
	for(d = devs.d; d; d = d->next)
		if(i++ == unit) {
			runlock(&devs);
			return d;
		}
	runlock(&devs);
	uprint("unit lookup failure: %lux pc %p", unit, getcallerpc(&unit));
	error(up->genbuf);
	return nil;
}

static int
unitgen(Chan *c, uint32_t type, Dir *dp)
{
	int perm, t;
	uint32_t vers;
	int64_t size;
	char *p;
	Aoedev *d;
	Qid q;

	d = unit2dev(UNIT(c->qid));
	perm = 0644;
	size = 0;
	vers = d->vers;
	t = QTFILE;

	switch(type) {
	default:
		return -1;
	case Qctl:
		p = "ctl";
		break;
	case Qdata:
		p = "data";
		perm = 0640;
		if(UP(d))
			size = d->bsize;
		break;
	case Qconfig:
		p = "config";
		if(UP(d))
			size = d->nconfig;
		break;
	case Qident:
		p = "ident";
		if(UP(d))
			size = sizeof d->ident;
		break;
	case Qdevlinkdir:
		p = "devlink";
		t = QTDIR;
		perm = 0555;
		break;
	}
	mkqid(&q, QID(UNIT(c->qid), type), vers, t);
	devdir(c, q, p, size, eve, perm, dp);
	return 1;
}

static int
topgen(Chan *c, uint32_t type, Dir *d)
{
	int perm;
	int64_t size;
	char *p;
	Qid q;

	perm = 0444;
	size = 0;
	switch(type) {
	default:
		return -1;
	case Qtopctl:
		p = "ctl";
		perm = 0644;
		break;
	case Qtoplog:
		p = "log";
		size = eventcount();
		break;
	}
	mkqid(&q, type, 0, QTFILE);
	devdir(c, q, p, size, eve, perm, d);
	return 1;
}

static int
aoegen(Chan *c, char *e, Dirtab *dir, int j, int s, Dir *dp)
{
	Proc *up = externup();
	int i;
	Aoedev *d;
	Qid q;

	if(c->qid.path == 0) {
		switch(s) {
		case DEVDOTDOT:
			q.path = 0;
			q.type = QTDIR;
			devdir(c, q, "#æ", 0, eve, 0555, dp);
			break;
		case 0:
			q.path = Qtopdir;
			q.type = QTDIR;
			devdir(c, q, "aoe", 0, eve, 0555, dp);
			break;
		default:
			return -1;
		}
		return 1;
	}

	switch(TYPE(c->qid)) {
	default:
		return -1;
	case Qtopdir:
		if(s == DEVDOTDOT) {
			mkqid(&q, Qzero, 0, QTDIR);
			devdir(c, q, "aoe", 0, eve, 0555, dp);
			return 1;
		}
		if(s < Qtopfiles)
			return topgen(c, Qtopbase + s, dp);
		s -= Qtopfiles;
		if(s >= units.ref)
			return -1;
		mkqid(&q, QID(s, Qunitdir), 0, QTDIR);
		d = unit2dev(s);
		devdir(c, q, unitname(d), 0, eve, 0555, dp);
		return 1;
	case Qtopctl:
	case Qtoplog:
		return topgen(c, TYPE(c->qid), dp);
	case Qunitdir:
		if(s == DEVDOTDOT) {
			mkqid(&q, QID(0, Qtopdir), 0, QTDIR);
			uprint("%uld", UNIT(c->qid));
			devdir(c, q, up->genbuf, 0, eve, 0555, dp);
			return 1;
		}
		return unitgen(c, Qunitbase + s, dp);
	case Qctl:
	case Qdata:
	case Qconfig:
	case Qident:
		return unitgen(c, TYPE(c->qid), dp);
	case Qdevlinkdir:
		i = UNIT(c->qid);
		if(s == DEVDOTDOT) {
			mkqid(&q, QID(i, Qunitdir), 0, QTDIR);
			devdir(c, q, "devlink", 0, eve, 0555, dp);
			return 1;
		}
		if(i >= units.ref)
			return -1;
		d = unit2dev(i);
		if(s >= d->ndl)
			return -1;
		uprint("%d", s);
		mkqid(&q, Q3(s, i, Qdevlink), 0, QTFILE);
		devdir(c, q, up->genbuf, 0, eve, 0755, dp);
		return 1;
	case Qdevlink:
		uprint("%d", s);
		mkqid(&q, Q3(s, UNIT(c->qid), Qdevlink), 0, QTFILE);
		devdir(c, q, up->genbuf, 0, eve, 0755, dp);
		return 1;
	}
}

static Walkqid *
aoewalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, nil, 0, aoegen);
}

static int32_t
aoestat(Chan *c, uint8_t *db, int32_t n)
{
	return devstat(c, db, n, nil, 0, aoegen);
}

static Chan *
aoeopen(Chan *c, int omode)
{
	Proc *up = externup();
	Aoedev *d;

	if(TYPE(c->qid) != Qdata)
		return devopen(c, omode, 0, 0, aoegen);

	d = unit2dev(UNIT(c->qid));
	qlock(d);
	if(waserror()) {
		qunlock(d);
		nexterror();
	}
	if(!UP(d))
		error(Enotup);
	c = devopen(c, omode, 0, 0, aoegen);
	d->nopen++;
	poperror();
	qunlock(d);
	return c;
}

static void
aoeclose(Chan *c)
{
	Proc *up = externup();
	Aoedev *d;

	if(TYPE(c->qid) != Qdata || (c->flag & COPEN) == 0)
		return;

	d = unit2dev(UNIT(c->qid));
	qlock(d);
	if(--d->nopen == 0 && !waserror()) {
		discover(d->major, d->minor);
		poperror();
	}
	qunlock(d);
}

static void
atarw(Aoedev *d, Frame *f)
{
	Proc *up = externup();
	uint32_t bcnt;
	char extbit, writebit;
	Aoeata *ah;
	Srb *srb;

	extbit = 0x4;
	writebit = 0x10;

	srb = d->inprocess;
	bcnt = d->maxbcnt;
	if(bcnt > srb->len)
		bcnt = srb->len;
	f->nhdr = AOEATASZ;
	memset(f->hdr, 0, f->nhdr);
	ah = (Aoeata *)f->hdr;
	if(hset(d, f, ah, ACata) == -1)
		return;
	f->dp = srb->dp;
	f->bcnt = bcnt;
	f->lba = srb->sector;
	f->srb = srb;

	ah->scnt = bcnt / Aoesectsz;
	putlba(ah, f->lba);
	if(d->flag & Dllba)
		ah->aflag |= AAFext;
	else {
		extbit = 0;
		ah->lba[3] &= 0x0f;
		ah->lba[3] |= 0xe0; /* LBA bit+obsolete 0xa0 */
	}
	if(srb->write) {
		ah->aflag |= AAFwrite;
		f->dlen = bcnt;
	} else {
		writebit = 0;
		f->dlen = 0;
	}
	ah->cmdstat = 0x20 | writebit | extbit;

	/* mark tracking fields and load out */
	srb->nout++;
	srb->dp = (uint8_t *)srb->dp + bcnt;
	srb->len -= bcnt;
	srb->sector += bcnt / Aoesectsz;
	if(srb->len == 0)
		d->inprocess = nil;
	d->nout++;
	f->dl->npkt++;
	if(waserror()) {
		f->tag = Tfree;
		d->inprocess = nil;
		nexterror();
	}
	f->nl->dc->dev->bwrite(f->nl->dc, allocfb(f), 0);
	poperror();
}

static char *
aoeerror(Aoehdr *h)
{
	int n;
	static char *errs[] = {
	    "aoe protocol error: unknown",
	    "aoe protocol error: bad command code",
	    "aoe protocol error: bad argument param",
	    "aoe protocol error: device unavailable",
	    "aoe protocol error: config string present",
	    "aoe protocol error: unsupported version",
	};

	if((h->verflag & AFerr) == 0)
		return 0;
	n = h->error;
	if(n > nelem(errs))
		n = 0;
	return errs[n];
}

static void
rtupdate(Devlink *l, int rtt)
{
	int n;

	n = rtt;
	if(rtt < 0) {
		n = -rtt;
		if(n < Rtmin)
			n = Rtmin;
		else if(n > Rtmax)
			n = Rtmax;
		l->mintimer += (n - l->mintimer) >> 1;
	} else if(n < l->mintimer)
		n = l->mintimer;
	else if(n > Rtmax)
		n = Rtmax;

	/* g == .25; cf. Congestion Avoidance and Control, Jacobson&Karels; 1988 */
	n -= l->rttavg;
	l->rttavg += n >> 2;
}

static int
srbready(void *v)
{
	Srb *s;

	s = v;
	return s->error || (!s->nout && !s->len);
}

static Frame *
getframe(Aoedev *d, int tag)
{
	Frame *f, *e;

	f = d->frames;
	e = f + d->nframes;
	for(; f < e; f++)
		if(f->tag == tag)
			return f;
	return nil;
}

static Frame *
freeframe(Aoedev *d)
{
	if(d->nout < d->maxout)
		return getframe(d, Tfree);
	return nil;
}

static void
work(Aoedev *d)
{
	Frame *f;

	while((f = freeframe(d)) != nil) {
		if(d->inprocess == nil) {
			if(d->head == nil)
				return;
			d->inprocess = d->_head;
			d->_head = d->_head->next;
			if(d->head == nil)
				d->tail = nil;
		}
		atarw(d, f);
	}
}

static void
strategy(Aoedev *d, Srb *srb)
{
	Proc *up = externup();
	qlock(d);
	if(waserror()) {
		qunlock(d);
		nexterror();
	}
	srb->next = nil;
	if(d->tail)
		d->_tail->next = srb;
	d->_tail = srb;
	if(d->_head == nil)
		d->_head = srb;
	work(d);
	poperror();
	qunlock(d);

	while(waserror())
		;
	sleep(srb, srbready, srb);
	poperror();
}

#define iskaddr(a) ((uintptr)(a) > KZERO)

static int32_t
rw(Aoedev *d, int write, uint8_t *db, int32_t len, uint64_t off)
{
	Proc *up = externup();
	int32_t n, nlen, copy;
	enum { Srbsz = 1 << 18,
	};
	Srb *srb;

	if((off | len) & (Aoesectsz - 1))
		error("offset and length must be sector multiple.\n");
	if(off > d->bsize || len == 0)
		return 0;
	if(off + len > d->bsize)
		len = d->bsize - off;
	copy = 0;
	if(iskaddr(db)) {
		srb = srbkalloc(db, len);
		copy = 1;
	} else
		srb = srballoc(Srbsz <= len ? Srbsz : len);
	if(waserror()) {
		srbfree(srb);
		nexterror();
	}
	nlen = len;
	srb->write = write;
	do {
		if(!UP(d))
			error(Eio);
		srb->sector = off / Aoesectsz;
		srb->dp = srb->data;
		n = nlen;
		if(n > Srbsz)
			n = Srbsz;
		srb->len = n;
		if(write && !copy)
			memmove(srb->data, db, n);
		strategy(d, srb);
		if(srb->error)
			error(srb->error);
		if(!write && !copy)
			memmove(db, srb->data, n);
		nlen -= n;
		db += n;
		off += n;
	} while(nlen > 0);
	poperror();
	srbfree(srb);
	return len;
}

static int32_t
readmem(uint32_t off, void *dst, int32_t n, void *src, int32_t size)
{
	if(off >= size)
		return 0;
	if(off + n > size)
		n = size - off;
	memmove(dst, (uint8_t *)src + off, n);
	return n;
}

static char *
pflag(char *s, char *e, uint8_t f)
{
	uint8_t i, m;

	for(i = 0; i < 8; i++) {
		m = 1 << i;
		if(f & m)
			s = seprint(s, e, "%s ", flagname[i] ? flagname[i] : "oops");
	}
	return seprint(s, e, "\n");
}

static int
pstat(Aoedev *d, char *db, int len, int off)
{
	int i;
	char *state, *s, *p, *e;

	s = p = malloc(READSTR);
	e = p + READSTR;

	state = "down";
	if(UP(d))
		state = "up";

	p = seprint(p, e,
		    "state: %s\n"
		    "nopen: %d\n"
		    "nout: %d\n"
		    "nmaxout: %d\n"
		    "nframes: %d\n"
		    "maxbcnt: %d\n"
		    "fw: %.4ux\n"
		    "model: %s\n"
		    "serial: %s\n"
		    "firmware: %s\n",
		    state, d->nopen, d->nout,
		    d->maxout, d->nframes, d->maxbcnt,
		    d->fwver,
		    d->model, d->serial, d->firmware);
	p = seprint(p, e, "flag: ");
	p = pflag(p, e, d->flag);

	if(p - s < len)
		len = p - s;
	i = readstr(off, db, len, s);
	free(s);
	return i;
}

static int32_t
unitread(Chan *c, void *db, int32_t len, int64_t off)
{
	Aoedev *d;

	d = unit2dev(UNIT(c->qid));
	if(d->vers != c->qid.vers)
		error(Echange);
	switch(TYPE(c->qid)) {
	default:
		error(Ebadarg);
	case Qctl:
		return pstat(d, db, len, off);
	case Qdata:
		return rw(d, Read, db, len, off);
	case Qconfig:
		if(!UP(d))
			error(Enotup);
		return readmem(off, db, len, d->config, d->nconfig);
	case Qident:
		if(!UP(d))
			error(Enotup);
		return readmem(off, db, len, d->ident, sizeof d->ident);
	}
}

static int
devlinkread(Chan *c, void *db, int len, int off)
{
	int i;
	char *s, *p, *e;
	Aoedev *d;
	Devlink *l;

	d = unit2dev(UNIT(c->qid));
	i = L(c->qid);
	if(i >= d->ndl)
		return 0;
	l = d->dl + i;

	s = p = malloc(READSTR);
	e = s + READSTR;

	p = seprint(p, e, "addr: ");
	for(i = 0; i < l->nea; i++)
		p = seprint(p, e, "%E ", l->eatab[i]);
	p = seprint(p, e, "\n");
	p = seprint(p, e, "npkt: %uld\n", l->npkt);
	p = seprint(p, e, "resent: %uld\n", l->resent);
	p = seprint(p, e, "flag: ");
	p = pflag(p, e, l->flag);
	p = seprint(p, e, "rttavg: %uld\n", Tk2ms(l->rttavg));
	p = seprint(p, e, "mintimer: %uld\n", Tk2ms(l->mintimer));

	p = seprint(p, e, "nl path: %s\n", l->nl->path);
	p = seprint(p, e, "nl ea: %E\n", l->nl->ea);
	p = seprint(p, e, "nl flag: ");
	p = pflag(p, e, l->flag);
	p = seprint(p, e, "nl lostjumbo: %d\n", l->nl->lostjumbo);
	p = seprint(p, e, "nl datamtu: %d\n", l->nl->datamtu);

	if(p - s < len)
		len = p - s;
	i = readstr(off, db, len, s);
	free(s);
	return i;
}

static int32_t
topctlread(Chan *c, void *db, int len, int off)
{
	int i;
	char *s, *p, *e;
	Netlink *n;

	s = p = malloc(READSTR);
	e = s + READSTR;

	p = seprint(p, e, "debug: %d\n", debug);
	p = seprint(p, e, "autodiscover: %d\n", autodiscover);
	p = seprint(p, e, "rediscover: %d\n", rediscover);

	for(i = 0; i < Nnetlink; i++) {
		n = netlinks.nl + i;
		if(n->cc == 0)
			continue;
		p = seprint(p, e, "if%d path: %s\n", i, n->path);
		p = seprint(p, e, "if%d ea: %E\n", i, n->ea);
		p = seprint(p, e, "if%d flag: ", i);
		p = pflag(p, e, n->flag);
		p = seprint(p, e, "if%d lostjumbo: %d\n", i, n->lostjumbo);
		p = seprint(p, e, "if%d datamtu: %d\n", i, n->datamtu);
	}

	if(p - s < len)
		len = p - s;
	i = readstr(off, db, len, s);
	free(s);
	return i;
}

static int32_t
aoeread(Chan *c, void *db, int32_t n, int64_t off)
{
	switch(TYPE(c->qid)) {
	default:
		error(Eperm);
	case Qzero:
	case Qtopdir:
	case Qunitdir:
	case Qdevlinkdir:
		return devdirread(c, db, n, 0, 0, aoegen);
	case Qtopctl:
		return topctlread(c, db, n, off);
	case Qtoplog:
		return eventlogread(db, n);
	case Qctl:
	case Qdata:
	case Qconfig:
	case Qident:
		return unitread(c, db, n, off);
	case Qdevlink:
		return devlinkread(c, db, n, off);
	}
}

static int32_t
configwrite(Aoedev *d, void *db, int32_t len)
{
	Proc *up = externup();
	char *s;
	Aoeqc *ch;
	Frame *f;
	Srb *srb;

	if(!UP(d))
		error(Enotup);
	if(len > ETHERMAXTU - AOEQCSZ)
		error(Etoobig);
	srb = srballoc(len);
	s = malloc(len);
	memmove(s, db, len);
	if(waserror()) {
		srbfree(srb);
		free(s);
		nexterror();
	}
	for(;;) {
		qlock(d);
		if(waserror()) {
			qunlock(d);
			nexterror();
		}
		f = freeframe(d);
		if(f != nil)
			break;
		poperror();
		qunlock(d);
		if(waserror())
			nexterror();
		tsleep(&up->sleep, return0, 0, 100);
		poperror();
	}
	f->nhdr = AOEQCSZ;
	memset(f->hdr, 0, f->nhdr);
	ch = (Aoeqc *)f->hdr;
	if(hset(d, f, ch, ACconfig) == -1)
		return 0;
	f->srb = srb;
	f->dp = s;
	ch->verccmd = AQCfset;
	hnputs(ch->cslen, len);
	d->nout++;
	srb->nout++;
	f->dl->npkt++;
	f->dlen = len;
	/*
	 * these refer to qlock & waserror in the above for loop.
	 * there's still the first waserror outstanding.
	 */
	poperror();
	qunlock(d);

	f->nl->dc->dev->bwrite(f->nl->dc, allocfb(f), 0);
	sleep(srb, srbready, srb);
	if(srb->error)
		error(srb->error);

	qlock(d);
	if(waserror()) {
		qunlock(d);
		nexterror();
	}
	memmove(d->config, s, len);
	d->nconfig = len;
	poperror();
	qunlock(d);

	poperror(); /* pop first waserror */

	srbfree(srb);
	memmove(db, s, len);
	free(s);
	return len;
}

static int getmtu(Chan *);

static int
devmaxdata(Aoedev *d)
{
	int i, mm, mtu;
	Devlink *l;
	Netlink *n;

	mtu = 100000;
	for(i = 0; i < d->ndl; i++) {
		l = d->dl + i;
		n = l->nl;
		if((l->flag & Dup) == 0 || (n->flag & Dup) == 0)
			continue;
		mm = getmtu(n->mtu);
		if(mm < mtu)
			mtu = mm;
	}
	if(mtu == 100000)
		mtu = 0;
	mtu -= AOEATASZ;
	return mtu;
}

static int
toggle(char *s, int init)
{
	if(s == nil)
		return init ^ 1;
	return strcmp(s, "on") == 0;
}

static void ataident(Aoedev *);

static int32_t
unitctlwrite(Aoedev *d, void *db, int32_t n)
{
	Proc *up = externup();
	uint maxbcnt, mm;
	uint64_t bsize;
	enum {
		Failio,
		Ident,
		Jumbo,
		Maxbno,
		Mtu,
		Setsize,
	};
	Cmdbuf *cb;
	Cmdtab *ct;
	static Cmdtab cmds[] = {
	    {Failio, "failio", 1},
	    {Ident, "identify", 1},
	    {Jumbo, "jumbo", 0},
	    {Maxbno, "maxbno", 0},
	    {Mtu, "mtu", 0},
	    {Setsize, "setsize", 0},
	};

	cb = parsecmd(db, n);
	qlock(d);
	if(waserror()) {
		qunlock(d);
		free(cb);
		nexterror();
	}
	ct = lookupcmd(cb, cmds, nelem(cmds));
	switch(ct->index) {
	case Failio:
		downdev(d, "i/o failure");
		break;
	case Ident:
		ataident(d);
		break;
	case Jumbo:
		mm = 0;
		if(d->flag & Djumbo)
			mm = 1;
		toggle(cb->f[1], mm);
		if(mm)
			d->flag |= Djumbo;
		else
			d->flag &= ~Djumbo;
		break;
	case Maxbno:
	case Mtu:
		maxbcnt = devmaxdata(d);
		if(cb->nf > 2)
			error(Ecmdargs);
		if(cb->nf == 2) {
			mm = strtoul(cb->f[1], 0, 0);
			if(ct->index == Maxbno)
				mm *= Aoesectsz;
			else {
				mm -= AOEATASZ;
				mm &= ~(Aoesectsz - 1);
			}
			if(mm > maxbcnt)
				cmderror(cb, "maxb greater than media mtu");
			maxbcnt = mm;
		}
		d->maxbcnt = maxbcnt;
		break;
	case Setsize:
		bsize = d->realbsize;
		if(cb->nf > 2)
			error(Ecmdargs);
		if(cb->nf == 2) {
			bsize = strtoull(cb->f[1], 0, 0);
			if(bsize % Aoesectsz)
				cmderror(cb, "disk size must be sector aligned");
		}
		d->bsize = bsize;
		break;
	default:
		cmderror(cb, "unknown aoe control message");
	}
	poperror();
	qunlock(d);
	free(cb);
	return n;
}

static int32_t
unitwrite(Chan *c, void *db, int32_t n, int64_t off)
{
	int32_t rv;
	char *buf;
	Aoedev *d;

	d = unit2dev(UNIT(c->qid));
	switch(TYPE(c->qid)) {
	default:
		error(Ebadarg);
	case Qctl:
		return unitctlwrite(d, db, n);
	case Qident:
		error(Eperm);
	case Qdata:
		return rw(d, Write, db, n, off);
	case Qconfig:
		if(off + n > sizeof d->config)
			error(Etoobig);
		buf = malloc(sizeof d->config);
		memmove(buf, d->config, d->nconfig);
		memmove(buf + off, db, n);
		rv = configwrite(d, buf, n + off);
		free(buf);
		return rv;
	}
}

static Netlink *
addnet(char *path, Chan *cc, Chan *dc, Chan *mtu, uint8_t *ea)
{
	Proc *up = externup();
	Netlink *nl, *e;

	lock(&netlinks);
	if(waserror()) {
		unlock(&netlinks);
		nexterror();
	}
	nl = netlinks.nl;
	e = nl + nelem(netlinks.nl);
	for(; nl < e && nl->cc; nl++)
		continue;
	if(nl >= e)
		error("out of netlink structures");
	nl->cc = cc;
	nl->dc = dc;
	nl->mtu = mtu;
	strncpy(nl->path, path, sizeof nl->path);
	memmove(nl->ea, ea, sizeof nl->ea);
	poperror();
	nl->flag |= Dup;
	unlock(&netlinks);
	return nl;
}

static int
newunit(void)
{
	int x;

	lock(&units);
	if(units.ref == Maxunits)
		x = -1;
	else
		x = units.ref++;
	unlock(&units);
	return x;
}

static int
dropunit(void)
{
	int x;

	lock(&units);
	x = --units.ref;
	unlock(&units);
	return x;
}

static Aoedev *
newdev(int32_t major, int32_t minor, int n)
{
	Aoedev *d;
	Frame *f, *e;

	d = mallocz(sizeof *d, 1);
	f = mallocz(sizeof *f * n, 1);
	if(!d || !f) {
		free(d);
		free(f);
		error("aoe device allocation failure");
	}
	d->nframes = n;
	d->frames = f;
	for(e = f + n; f < e; f++)
		f->tag = Tfree;
	d->maxout = n;
	d->major = major;
	d->minor = minor;
	d->maxbcnt = Dbcnt;
	d->flag = Djumbo;
	d->unit = newunit(); /* bzzt.  inaccurate if units removed */
	if(d->unit == -1) {
		free(d);
		free(d->frames);
		error("too many units");
	}
	d->dl = d->dltab;
	return d;
}

static Aoedev *
mm2dev(int major, int minor)
{
	Aoedev *d;

	rlock(&devs);
	for(d = devs.d; d; d = d->next)
		if(d->major == major && d->minor == minor) {
			runlock(&devs);
			return d;
		}
	runlock(&devs);
	eventlog("mm2dev: %d.%d not found\n", major, minor);
	return nil;
}

/* Find the device in our list.  If not known, add it */
static Aoedev *
getdev(int32_t major, int32_t minor, int n)
{
	Proc *up = externup();
	Aoedev *d;

	wlock(&devs);
	if(waserror()) {
		wunlock(&devs);
		nexterror();
	}
	for(d = devs.d; d; d = d->next)
		if(d->major == major && d->minor == minor)
			break;
	if(d == nil) {
		d = newdev(major, minor, n);
		d->next = devs.d;
		devs.d = d;
	}
	poperror();
	wunlock(&devs);
	return d;
}

static uint16_t
gbit16(void *a)
{
	uint8_t *i;

	i = a;
	return i[1] << 8 | i[0];
}

static uint32_t
gbit32(void *a)
{
	uint32_t j;
	uint8_t *i;

	i = a;
	j = i[3] << 24;
	j |= i[2] << 16;
	j |= i[1] << 8;
	j |= i[0];
	return j;
}

static uint64_t
gbit64(void *a)
{
	uint8_t *i;

	i = a;
	return (uint64_t)gbit32(i + 4) << 32 | gbit32(a);
}

static void
ataident(Aoedev *d)
{
	Aoeata *a;
	Block *b;
	Frame *f;

	f = freeframe(d);
	if(f == nil)
		return;
	f->nhdr = AOEATASZ;
	memset(f->hdr, 0, f->nhdr);
	a = (Aoeata *)f->hdr;
	if(hset(d, f, a, ACata) == -1)
		return;
	a->cmdstat = Cid; /* ata 6, page 110 */
	a->scnt = 1;
	a->lba[3] = 0xa0;
	d->nout++;
	f->dl->npkt++;
	f->bcnt = 512;
	f->dlen = 0;
	b = allocfb(f);
	f->nl->dc->dev->bwrite(f->nl->dc, b, 0);
}

static int
getmtu(Chan *mm)
{
	Proc *up = externup();
	int n, mtu;
	char buf[36];

	mtu = 1514;
	if(mm == nil || waserror())
		return mtu;
	n = mm->dev->read(mm, buf, sizeof buf - 1, 0);
	if(n > 12) {
		buf[n] = 0;
		mtu = strtoul(buf + 12, 0, 0);
	}
	poperror();
	return mtu;
}

static int
newdlea(Devlink *l, uint8_t *ea)
{
	int i;
	uint8_t *t;

	for(i = 0; i < Nea; i++) {
		t = l->eatab[i];
		if(i == l->nea) {
			memmove(t, ea, Eaddrlen);
			return l->nea++;
		}
		if(memcmp(t, ea, Eaddrlen) == 0)
			return i;
	}
	return -1;
}

static Devlink *
newdevlink(Aoedev *d, Netlink *n, Aoeqc *c)
{
	int i;
	Devlink *l;

	for(i = 0; i < Ndevlink; i++) {
		l = d->dl + i;
		if(i == d->ndl) {
			d->ndl++;
			newdlea(l, c->src);
			l->nl = n;
			l->flag |= Dup;
			l->mintimer = Rtmin;
			l->rttavg = Rtmax;
			return l;
		}
		if(l->nl == n)
			return l;
	}
	eventlog("%æ: out of links: %s:%E to %E\n", d, n->path, n->ea, c->src);
	return 0;
}

static void
errrsp(Block *b, char *s)
{
	int n;
	Aoedev *d;
	Aoehdr *h;
	Frame *f;

	h = (Aoehdr *)b->rp;
	n = nhgetl(h->tag);
	if(n == Tmgmt || n == Tfree)
		return;
	d = mm2dev(nhgets(h->major), h->minor);
	if(d == 0)
		return;
	if(f = getframe(d, n))
		frameerror(d, f, s);
}

static void
qcfgrsp(Block *b, Netlink *nl)
{
	Proc *up = externup();
	int major, cmd, cslen, blen;
	unsigned n;
	Aoedev *d;
	Aoeqc *ch;
	Devlink *l;
	Frame *f;

	ch = (Aoeqc *)b->rp;
	major = nhgets(ch->major);
	n = nhgetl(ch->tag);
	if(n != Tmgmt) {
		d = mm2dev(major, ch->minor);
		if(d == nil)
			return;
		qlock(d);
		f = getframe(d, n);
		if(f == nil) {
			qunlock(d);
			eventlog("%æ: unknown response tag %ux\n", d, n);
			return;
		}
		cslen = nhgets(ch->cslen);
		blen = BLEN(b) - AOEQCSZ;
		if(cslen < blen)
			eventlog("%æ: cfgrsp: tag %.8ux oversized %d %d\n",
				 d, n, cslen, blen);
		if(cslen > blen) {
			eventlog("%æ: cfgrsp: tag %.8ux runt %d %d\n",
				 d, n, cslen, blen);
			cslen = blen;
		}
		memmove(f->dp, ch + 1, cslen);
		f->srb->nout--;
		wakeup(f->srb);
		d->nout--;
		f->srb = nil;
		f->tag = Tfree;
		qunlock(d);
		return;
	}

	cmd = ch->verccmd & 0xf;
	if(cmd != 0) {
		eventlog("aoe%d.%d: cfgrsp: bad command %d\n", major, ch->minor, cmd);
		return;
	}
	n = nhgets(ch->bufcnt);
	if(n > Maxframes)
		n = Maxframes;

	if(waserror()) {
		eventlog("getdev: %d.%d ignored: %s\n", major, ch->minor, up->errstr);
		return;
	}
	d = getdev(major, ch->minor, n);
	poperror();

	qlock(d);
	if(waserror()) {
		qunlock(d);
		eventlog("%æ: %s\n", d, up->errstr);
		nexterror();
	}

	l = newdevlink(d, nl, ch); /* add this interface. */

	d->fwver = nhgets(ch->fwver);
	n = nhgets(ch->cslen);
	if(n > sizeof d->config)
		n = sizeof d->config;
	d->nconfig = n;
	memmove(d->config, ch + 1, n);
	if(l != 0 && d->flag & Djumbo) {
		n = getmtu(nl->mtu) - AOEATASZ;
		n /= Aoesectsz;
		if(n > ch->scnt)
			n = ch->scnt;
		n = n ? n * Aoesectsz : Dbcnt;
		if(n != d->maxbcnt) {
			eventlog("%æ: setting %d byte data frames on %s:%E\n",
				 d, n, nl->path, nl->ea);
			d->maxbcnt = n;
		}
	}
	if(d->nopen == 0)
		ataident(d);
	poperror();
	qunlock(d);
}

void
aoeidmove(char *p, uint16_t *u, unsigned n)
{
	int i;
	char *op, *e, *s;

	op = p;
	/*
	 * the ushort `a' is sometimes not aligned on a short boundary,
	 * so dereferencing a[i] would cause an alignment exception on
	 * some machines.
	 */
	s = (char *)u;
	for(i = 0; i < n; i += 2) {
		*p++ = s[i + 1];
		*p++ = s[i];
	}
	*p = 0;
	while(p > op && *--p == ' ')
		*p = 0;
	e = p;
	p = op;
	while(*p == ' ')
		p++;
	memmove(op, p, n - (e - p));
}

static int64_t
aoeidentify(Aoedev *d, uint16_t *id)
{
	int i;
	int64_t s;

	d->flag &= ~(Dllba | Dpower | Dsmart | Dnop | Dup);

	i = gbit16(id + 83) | gbit16(id + 86);
	if(i & (1 << 10)) {
		d->flag |= Dllba;
		s = gbit64(id + 100);
	} else
		s = gbit32(id + 60);

	i = gbit16(id + 83);
	if((i >> 14) == 1) {
		if(i & (1 << 3))
			d->flag |= Dpower;
		i = gbit16(id + 82);
		if(i & 1)
			d->flag |= Dsmart;
		if(i & (1 << 14))
			d->flag |= Dnop;
	}
	//	eventlog("%æ up\n", d);
	d->flag |= Dup;
	memmove(d->ident, id, sizeof d->ident);
	return s;
}

static void
newvers(Aoedev *d)
{
	lock(&drivevers);
	d->vers = drivevers.ref++;
	unlock(&drivevers);
}

static int
identify(Aoedev *d, uint16_t *id)
{
	int64_t osectors, s;
	uint8_t oserial[21];

	s = aoeidentify(d, id);
	if(s == -1)
		return -1;
	osectors = d->realbsize;
	memmove(oserial, d->serial, sizeof d->serial);

	aoeidmove(d->serial, id + 10, 20);
	aoeidmove(d->firmware, id + 23, 8);
	aoeidmove(d->model, id + 27, 40);

	s *= Aoesectsz;
	if((osectors == 0 || osectors != s) &&
	   memcmp(oserial, d->serial, sizeof oserial) != 0) {
		d->bsize = s;
		d->realbsize = s;
		//		d->mediachange = 1;
		newvers(d);
	}
	return 0;
}

static void
atarsp(Block *b)
{
	Proc *up = externup();
	unsigned n;
	int16_t major;
	Aoeata *ahin, *ahout;
	Aoedev *d;
	Frame *f;
	Srb *srb;

	ahin = (Aoeata *)b->rp;
	major = nhgets(ahin->major);
	d = mm2dev(major, ahin->minor);
	if(d == nil)
		return;
	qlock(d);
	if(waserror()) {
		qunlock(d);
		nexterror();
	}
	n = nhgetl(ahin->tag);
	f = getframe(d, n);
	if(f == nil) {
		dprint("%æ: unexpected response; tag %ux\n", d, n);
		goto bail;
	}
	rtupdate(f->dl, tsince(f->tag));
	ahout = (Aoeata *)f->hdr;
	srb = f->srb;

	if(ahin->cmdstat & 0xa9) {
		eventlog("%æ: ata error cmd %.2ux stat %.2ux\n",
			 d, ahout->cmdstat, ahin->cmdstat);
		if(srb)
			srb->error = Eio;
	} else {
		n = ahout->scnt * Aoesectsz;
		switch(ahout->cmdstat) {
		case Crd:
		case Crdext:
			if(BLEN(b) - AOEATASZ < n) {
				eventlog("%æ: runt read blen %ld expect %d\n",
					 d, BLEN(b), n);
				goto bail;
			}
			memmove(f->dp, (uint8_t *)ahin + AOEATASZ, n);
		case Cwr:
		case Cwrext:
			if(n > Dbcnt)
				f->nl->lostjumbo = 0;
			if(f->bcnt -= n) {
				f->lba += n / Aoesectsz;
				f->dp = (uint8_t *)f->dp + n;
				resend(d, f);
				goto bail;
			}
			break;
		case Cid:
			if(BLEN(b) - AOEATASZ < 512) {
				eventlog("%æ: runt identify blen %ld expect %d\n",
					 d, BLEN(b), n);
				goto bail;
			}
			identify(d, (uint16_t *)((uint8_t *)ahin + AOEATASZ));
			break;
		default:
			eventlog("%æ: unknown ata command %.2ux \n",
				 d, ahout->cmdstat);
		}
	}

	if(srb && --srb->nout == 0 && srb->len == 0)
		wakeup(srb);
	f->srb = nil;
	f->tag = Tfree;
	d->nout--;

	work(d);
bail:
	poperror();
	qunlock(d);
}

static void
netrdaoeproc(void *v)
{
	Proc *up = externup();
	int idx;
	char name[Maxpath + 1], *s;
	Aoehdr *h;
	Block *b;
	Netlink *nl;

	nl = (Netlink *)v;
	idx = nl - netlinks.nl;
	netlinks.reader[idx] = 1;
	kstrcpy(name, nl->path, Maxpath);

	if(waserror()) {
		eventlog("netrdaoe exiting: %s\n", up->errstr);
		netlinks.reader[idx] = 0;
		wakeup(netlinks.rendez + idx);
		pexit(up->errstr, 1);
	}
	if(autodiscover)
		discover(0xffff, 0xff);
	for(;;) {
		if(!(nl->flag & Dup)) {
			uprint("%s: netlink is down", name);
			error(up->genbuf);
		}
		if(nl->dc == nil)
			panic("netrdaoe: nl->dc == nil");
		b = nl->dc->dev->bread(nl->dc, 1 << 16, 0);
		if(b == nil) {
			uprint("%s: nil read from network", name);
			error(up->genbuf);
		}
		h = (Aoehdr *)b->rp;
		if(h->verflag & AFrsp)
			if(s = aoeerror(h)) {
				eventlog("%s: %s\n", nl->path, up->errstr);
				errrsp(b, s);
			} else
				switch(h->cmd) {
				case ACata:
					atarsp(b);
					break;
				case ACconfig:
					qcfgrsp(b, nl);
					break;
				default:
					eventlog("%s: unknown cmd %d\n",
						 nl->path, h->cmd);
					errrsp(b, "unknown command");
				}
		freeb(b);
	}
}

static void
getaddr(char *path, uint8_t *ea)
{
	Proc *up = externup();
	int n;
	char buf[2 * Eaddrlen + 1];
	Chan *c;
	extern int parseether(uint8_t *, char *);

	uprint("%s/addr", path);
	c = namec(up->genbuf, Aopen, OREAD, 0);
	if(waserror()) {
		cclose(c);
		nexterror();
	}
	if(c == nil)
		panic("æ: getaddr: c == nil");
	n = c->dev->read(c, buf, sizeof buf - 1, 0);
	poperror();
	cclose(c);
	buf[n] = 0;
	if(parseether(ea, buf) < 0)
		error("parseether failure");
}

static void
netbind(char *path)
{
	Proc *up = externup();
	char addr[Maxpath];
	uint8_t ea[2 * Eaddrlen + 1];
	Chan *dc, *cc, *mtu;
	Netlink *nl;

	snprint(addr, sizeof addr, "%s!0x%x", path, Aoetype);
	dc = chandial(addr, nil, nil, &cc);
	snprint(addr, sizeof addr, "%s/mtu", path);
	if(waserror())
		mtu = nil;
	else {
		mtu = namec(addr, Aopen, OREAD, 0);
		poperror();
	}

	if(waserror()) {
		cclose(dc);
		cclose(cc);
		if(mtu)
			cclose(mtu);
		nexterror();
	}
	if(dc == nil || cc == nil)
		error(Enonexist);
	getaddr(path, ea);
	nl = addnet(path, cc, dc, mtu, ea);
	snprint(addr, sizeof addr, "netrdaoe@%s", path);
	kproc(addr, netrdaoeproc, nl);
	poperror();
}

static int
unbound(void *v)
{
	return *(int *)v != 0;
}

static void
netunbind(char *path)
{
	Proc *up = externup();
	int i, idx;
	Aoedev *d, *p, *next;
	Chan *dc, *cc;
	Devlink *l;
	Frame *f;
	Netlink *n, *e;

	n = netlinks.nl;
	e = n + nelem(netlinks.nl);

	lock(&netlinks);
	for(; n < e; n++)
		if(n->dc && strcmp(n->path, path) == 0)
			break;
	unlock(&netlinks);
	if(n >= e)
		error("device not bound");

	/*
	 * hunt down devices using this interface; disable
	 * this also terminates the reader.
	 */
	idx = n - netlinks.nl;
	wlock(&devs);
	for(d = devs.d; d; d = d->next) {
		qlock(d);
		for(i = 0; i < d->ndl; i++) {
			l = d->dl + i;
			if(l->nl == n)
				l->flag &= ~Dup;
		}
		qunlock(d);
	}
	n->flag &= ~Dup;
	wunlock(&devs);

	/* confirm reader is down. */
	while(waserror())
		;
	sleep(netlinks.rendez + idx, unbound, netlinks.reader + idx);
	poperror();

	/* reschedule packets. */
	wlock(&devs);
	for(d = devs.d; d; d = d->next) {
		qlock(d);
		for(i = 0; i < d->nframes; i++) {
			f = d->frames + i;
			if(f->tag != Tfree && f->nl == n)
				resend(d, f);
		}
		qunlock(d);
	}
	wunlock(&devs);

	/* squeeze devlink pool.  (we assert nobody is using them now) */
	wlock(&devs);
	for(d = devs.d; d; d = d->next) {
		qlock(d);
		for(i = 0; i < d->ndl; i++) {
			l = d->dl + i;
			if(l->nl == n)
				memmove(l, l + 1, sizeof *l * (--d->ndl - i));
		}
		qunlock(d);
	}
	wunlock(&devs);

	/* close device link. */
	lock(&netlinks);
	dc = n->dc;
	cc = n->cc;
	if(n->mtu)
		cclose(n->mtu);
	memset(n, 0, sizeof *n);
	unlock(&netlinks);

	cclose(dc);
	cclose(cc);

	/* squeeze orphan devices */
	wlock(&devs);
	for(p = d = devs.d; d; p = d, d = next) {
		next = d->next;
		if(d->ndl > 0)
			continue;
		if(p != devs.d)
			p->next = next;
		else
			devs.d = next;
		free(d->frames);
		free(d);
		dropunit();
	}
	wunlock(&devs);
}

static void
removedev(char *name)
{
	int i;
	Aoedev *d, *p;

	wlock(&devs);
	for(p = d = devs.d; d; p = d, d = d->next)
		if(strcmp(name, unitname(d)) == 0)
			goto found;
	wunlock(&devs);
	error("device not bound");
found:
	d->flag &= ~Dup;
	newvers(d);
	d->ndl = 0;

	for(i = 0; i < d->nframes; i++)
		frameerror(d, d->frames + i, Enotup);

	if(p != devs.d)
		p->next = d->next;
	else
		devs.d = d->next;
	free(d->frames);
	free(d);
	dropunit();
	wunlock(&devs);
}

static void
discoverstr(char *f)
{
	uint16_t shelf, slot;
	uint32_t sh;
	char *s;

	if(f == 0) {
		discover(0xffff, 0xff);
		return;
	}

	shelf = sh = strtol(f, &s, 0);
	if(s == f || sh > 0xffff)
		error("bad shelf");
	f = s;
	if(*f++ == '.') {
		slot = strtol(f, &s, 0);
		if(s == f || slot > 0xff)
			error("bad shelf");
	} else
		slot = 0xff;
	discover(shelf, slot);
}

static int32_t
topctlwrite(void *db, int32_t n)
{
	enum {
		Autodiscover,
		Bind,
		Debug,
		Discover,
		Closewait,
		Rediscover,
		Remove,
		Unbind,
	};
	Proc *up = externup();
	char *f;
	Cmdbuf *cb;
	Cmdtab *ct;
	static Cmdtab cmds[] = {
	    {Autodiscover, "autodiscover", 0},
	    {Bind, "bind", 2},
	    {Debug, "debug", 0},
	    {Discover, "discover", 0},
	    {Rediscover, "rediscover", 0},
	    {Remove, "remove", 2},
	    {Unbind, "unbind", 2},
	};

	cb = parsecmd(db, n);
	if(waserror()) {
		free(cb);
		nexterror();
	}
	ct = lookupcmd(cb, cmds, nelem(cmds));
	f = cb->f[1];
	switch(ct->index) {
	case Autodiscover:
		autodiscover = toggle(f, autodiscover);
		break;
	case Bind:
		netbind(f);
		break;
	case Debug:
		debug = toggle(f, debug);
		break;
	case Discover:
		discoverstr(f);
		break;
	case Rediscover:
		rediscover = toggle(f, rediscover);
		break;
	case Remove:
		removedev(f);
		break;
	case Unbind:
		netunbind(f);
		break;
	default:
		cmderror(cb, "unknown aoe control message");
	}
	poperror();
	free(cb);
	return n;
}

static int32_t
aoewrite(Chan *c, void *db, int32_t n, int64_t off)
{
	switch(TYPE(c->qid)) {
	default:
	case Qzero:
	case Qtopdir:
	case Qunitdir:
	case Qtoplog:
		error(Eperm);
	case Qtopctl:
		return topctlwrite(db, n);
	case Qctl:
	case Qdata:
	case Qconfig:
	case Qident:
		return unitwrite(c, db, n, off);
	}
}

Dev aoedevtab = {
    L'æ',
    "aoe",

    devreset,
    devinit,
    devshutdown,
    aoeattach,
    aoewalk,
    aoestat,
    aoeopen,
    devcreate,
    aoeclose,
    aoeread,
    devbread,
    aoewrite,
    devbwrite,
    devremove,
    devwstat,
    devpower,
    devconfig,
};
