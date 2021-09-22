#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"

enum
{
	NHASH		= 128,
	MAXCACHE	= 1024*1024, /* cache <= this much of a file's start */
	NFILE		= 4096,
	NEXTENT		= 200,		/* extent allocation size */
};

typedef struct Extent Extent;
struct Extent
{
	int	bid;
	ulong	start;
	int	len;
	Page	*cache;
	Extent	*next;
};

typedef struct Mntcache Mntcache;
struct Mntcache
{
	Qid	qid;
	uint	devno;
	Dev*	dev;
	QLock;
	Extent	 *list;
	Mntcache *hash;
	Mntcache *prev;
	Mntcache *next;
};

typedef struct Cache Cache;
struct Cache
{
	QLock;
	int		pgno;
	Mntcache	*head;
	Mntcache	*tail;
	Mntcache	*hash[NHASH];
};

typedef struct Ecache Ecache;
struct Ecache
{
	Lock;
	int	total;
	int	free;
	Extent*	head;
};

static Image fscache;
static Cache cache;
static Ecache ecache;

static void
extentfree(Extent* e)
{
	lock(&ecache);
	e->next = ecache.head;
	ecache.head = e;
	ecache.free++;
	unlock(&ecache);
}

static Extent*
extentalloc(void)
{
	Extent *e;
	int i;

	lock(&ecache);
	if(ecache.head == nil){
		e = malloc(NEXTENT*sizeof(Extent));
		if(e == nil){
			unlock(&ecache);
			return nil;
		}
		for(i = 0; i < NEXTENT; i++){
			e->next = ecache.head;
			ecache.head = e;
			e++;
		}
		ecache.free += NEXTENT;
		ecache.total += NEXTENT;
	}

	e = ecache.head;
	ecache.head = e->next;
	ecache.free--;
	unlock(&ecache);

	memset(e, 0, sizeof(Extent));
	return e;
}

void
cinit(void)
{
	int i;
	Mntcache *mc;

	if((cache.head = malloc(sizeof(Mntcache)*NFILE)) == nil)
		panic("cinit: no memory");
	mc = cache.head;

	for(i = 0; i < NFILE-1; i++) {
		mc->next = mc+1;
		mc->prev = mc-1;
		mc++;
	}

	cache.tail = mc;
	cache.tail->next = cache.head->prev = 0;

	fscache.notext = 1;
}

Page*
cpage(Extent *e)
{
	/* Easy consistency check */
	if(e->cache->daddr != e->bid)
		return 0;

	return lookpage(&fscache, e->bid);
}

void
cnodata(Mntcache *mc)
{
	Extent *e, *n;

	/*
	 * Invalidate all extent data
	 * Image lru will waste the pages
	 */
	for(e = mc->list; e; e = n) {
		n = e->next;
		extentfree(e);
	}
	mc->list = 0;
}

void
ctail(Mntcache *mc)
{
	/* Unlink and send to the tail */
	if(mc->prev)
		mc->prev->next = mc->next;
	else
		cache.head = mc->next;
	if(mc->next)
		mc->next->prev = mc->prev;
	else
		cache.tail = mc->prev;

	if(cache.tail) {
		mc->prev = cache.tail;
		cache.tail->next = mc;
	} else {
		cache.head = mc;
		mc->prev = 0;
	}
	mc->next = 0;
	cache.tail = mc;
}

void
copen(Chan *c)
{
	int h;
	Extent *e, *next;
	Mntcache *mc, *f, **l;

	/* directories aren't cacheable and append-only files confuse us */
	if(c->qid.type&(QTDIR|QTAPPEND))
		return;

	h = c->qid.path%NHASH;
	qlock(&cache);
	for(mc = cache.hash[h]; mc != nil; mc = mc->hash) {
		if(mc->qid.path == c->qid.path)
		if(mc->qid.type == c->qid.type)
		if(mc->devno == c->devno && mc->dev == c->dev) {
			c->mc = mc;
			ctail(mc);
			qunlock(&cache);

			/* File was updated, invalidate cache */
			if(mc->qid.vers != c->qid.vers) {
				mc->qid.vers = c->qid.vers;
				qlock(mc);
				cnodata(mc);
				qunlock(mc);
			}
			return;
		}
	}

	/* LRU the cache headers */
	mc = cache.head;
	l = &cache.hash[mc->qid.path%NHASH];
	for(f = *l; f; f = f->hash) {
		if(f == mc) {
			*l = mc->hash;
			break;
		}
		l = &f->hash;
	}

	mc->qid = c->qid;
	mc->devno = c->devno;
	mc->dev = c->dev;

	l = &cache.hash[h];
	mc->hash = *l;
	*l = mc;
	ctail(mc);

	qlock(mc);
	c->mc = mc;
	e = mc->list;
	mc->list = 0;
	qunlock(&cache);

	while(e) {
		next = e->next;
		extentfree(e);
		e = next;
	}
	qunlock(mc);
}

static int
cdev(Mntcache *mc, Chan *c)
{
	if(mc->qid.path != c->qid.path)
		return 0;
	if(mc->qid.type != c->qid.type)
		return 0;
	if(mc->devno != c->devno)
		return 0;
	if(mc->dev != c->dev)
		return 0;
	if(mc->qid.vers != c->qid.vers)
		return 0;
	return 1;
}

int
cread(Chan *c, uchar *buf, int len, vlong off)
{
	KMap *k;
	Page *p;
	Mntcache *mc;
	Extent *e, **t;
	int o, l, total;
	ulong offset;

	if(off+len > MAXCACHE)
		return 0;

	mc = c->mc;
	if(mc == nil)
		return 0;

	qlock(mc);
	if(cdev(mc, c) == 0) {
		qunlock(mc);
		return 0;
	}

	offset = off;
	t = &mc->list;
	for(e = *t; e; e = e->next) {
		if(offset >= e->start && offset < e->start+e->len)
			break;
		t = &e->next;
	}

	if(e == 0) {
		qunlock(mc);
		return 0;
	}

	total = 0;
	while(len) {
		p = cpage(e);
		if(p == 0) {
			*t = e->next;
			extentfree(e);
			qunlock(mc);
			return total;
		}

		o = offset - e->start;
		l = len;
		if(l > e->len-o)
			l = e->len-o;

		k = kmap(p);
		if(waserror()) {
			kunmap(k);
			putpage(p);
			qunlock(mc);
			nexterror();
		}

		memmove(buf, (uchar*)VA(k) + o, l);

		poperror();
		kunmap(k);

		putpage(p);

		buf += l;
		len -= l;
		offset += l;
		total += l;
		t = &e->next;
		e = e->next;
		if(e == 0 || e->start != offset)
			break;
	}

	qunlock(mc);
	return total;
}

Extent*
cchain(uchar *buf, ulong offset, int len, Extent **tail)
{
	int l;
	Page *p;
	KMap *k;
	Extent *e, *start, **t;

	start = 0;
	*tail = 0;
	t = &start;
	while(len) {
		e = extentalloc();
		if(e == 0)
			break;

		p = auxpage();
		if(p == 0) {
			extentfree(e);
			break;
		}
		l = len;
		if(l > PGSZ)
			l = PGSZ;

		e->cache = p;
		e->start = offset;
		e->len = l;

		qlock(&cache);
		e->bid = cache.pgno;
		cache.pgno += PGSZ;
		/* wrap the counter; low bits are unused by pghash but checked by lookpage */
		if((cache.pgno & ~(PGSZ-1)) == 0){
			if(cache.pgno == PGSZ-1){
				print("cache wrapped\n");
				cache.pgno = 0;
			}else
				cache.pgno++;
		}
		qunlock(&cache);

		p->daddr = e->bid;
		k = kmap(p);
		if(waserror()) {		/* buf may be virtual */
			kunmap(k);
			nexterror();
		}
		memmove((void*)VA(k), buf, l);
		poperror();
		kunmap(k);

		cachepage(p, &fscache);
		putpage(p);

		buf += l;
		offset += l;
		len -= l;

		*t = e;
		*tail = e;
		t = &e->next;
	}

	return start;
}

int
cpgmove(Extent *e, uchar *buf, int boff, int len)
{
	Page *p;
	KMap *k;

	p = cpage(e);
	if(p == 0)
		return 0;

	k = kmap(p);
	if(waserror()) {		/* Since buf may be virtual */
		kunmap(k);
		nexterror();
	}

	memmove((uchar*)VA(k)+boff, buf, len);

	poperror();
	kunmap(k);
	putpage(p);

	return 1;
}

void
cupdate(Chan *c, uchar *buf, int len, vlong off)
{
	Mntcache *mc;
	Extent *tail;
	Extent *e, *f, *p;
	int o, ee, eblock;
	ulong offset;

	if(off > MAXCACHE || len == 0)
		return;

	mc = c->mc;
	if(mc == nil)
		return;
	qlock(mc);
	if(cdev(mc, c) == 0) {
		qunlock(mc);
		return;
	}

	/*
	 * Find the insertion point
	 */
	offset = off;
	p = 0;
	for(f = mc->list; f; f = f->next) {
		if(f->start > offset)
			break;
		p = f;
	}

	/* trim if there is a successor */
	eblock = offset+len;
	if(f != 0 && eblock > f->start) {
		len -= (eblock - f->start);
		if(len <= 0) {
			qunlock(mc);
			return;
		}
	}

	if(p == 0) {		/* at the head */
		e = cchain(buf, offset, len, &tail);
		if(e != 0) {
			mc->list = e;
			tail->next = f;
		}
		qunlock(mc);
		return;
	}

	/* trim to the predecessor */
	ee = p->start+p->len;
	if(offset < ee) {
		o = ee - offset;
		len -= o;
		if(len <= 0) {
			qunlock(mc);
			return;
		}
		buf += o;
		offset += o;
	}

	/* try and pack data into the predecessor */
	if(offset == ee && p->len < PGSZ) {
		o = len;
		if(o > PGSZ - p->len)
			o = PGSZ - p->len;
		if(cpgmove(p, buf, p->len, o)) {
			p->len += o;
			buf += o;
			len -= o;
			offset += o;
			if(len <= 0) {
if(f && p->start + p->len > f->start) print("CACHE: p->start=%uld p->len=%d f->start=%uld\n", p->start, p->len, f->start);
				qunlock(mc);
				return;
			}
		}
	}

	e = cchain(buf, offset, len, &tail);
	if(e != 0) {
		p->next = e;
		tail->next = f;
	}
	qunlock(mc);
}

void
cwrite(Chan* c, uchar *buf, int len, vlong off)
{
	int o, eo;
	Mntcache *mc;
	ulong eblock, ee;
	Extent *p, *f, *e, *tail;
	ulong offset;

	if(off > MAXCACHE || len == 0)
		return;

	mc = c->mc;
	if(mc == nil)
		return;

	qlock(mc);
	if(cdev(mc, c) == 0) {
		qunlock(mc);
		return;
	}

	offset = off;
	mc->qid.vers++;
	c->qid.vers++;

	p = 0;
	for(f = mc->list; f; f = f->next) {
		if(f->start >= offset)
			break;
		p = f;
	}

	if(p != 0) {
		ee = p->start+p->len;
		eo = offset - p->start;
		/* pack in predecessor if there is space */
		if(offset <= ee && eo < PGSZ) {
			o = len;
			if(o > PGSZ - eo)
				o = PGSZ - eo;
			if(cpgmove(p, buf, eo, o)) {
				if(eo+o > p->len)
					p->len = eo+o;
				buf += o;
				len -= o;
				offset += o;
			}
		}
	}

	/* free the overlap -- it's a rare case */
	eblock = offset+len;
	while(f && f->start < eblock) {
		e = f->next;
		extentfree(f);
		f = e;
	}

	/* link the block (if any) into the middle */
	e = cchain(buf, offset, len, &tail);
	if(e != 0) {
		tail->next = f;
		f = e;
	}

	if(p == 0)
		mc->list = f;
	else
		p->next = f;
	qunlock(mc);
}
