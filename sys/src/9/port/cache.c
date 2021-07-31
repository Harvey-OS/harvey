#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

enum
{
	NHASH		= 128,
	MAXCACHE	= 1024*1024,
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
	int	dev;
	int	type;
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
static int maxcache = MAXCACHE;

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
		e = xalloc(NEXTENT*sizeof(Extent));
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
	memset(e, 0, sizeof(Extent));
	ecache.free--;
	unlock(&ecache);

	return e;
}

void
cinit(void)
{
	int i;
	Mntcache *m;

	cache.head = xalloc(sizeof(Mntcache)*NFILE);
	m = cache.head;
	if (m == nil)
		panic("cinit: no memory");

	/* a better algorithm would be nice */
	if(conf.npage*BY2PG > 400*MB)
		maxcache = 50*MAXCACHE;
	else if(conf.npage*BY2PG > 200*MB)
		maxcache = 10*MAXCACHE;

	for(i = 0; i < NFILE-1; i++) {
		m->next = m+1;
		m->prev = m-1;
		m++;
	}

	cache.tail = m;
	cache.tail->next = 0;
	cache.head->prev = 0;

	fscache.notext = 1;
}

void
cprint(Chan *c, Mntcache *m, char *s)
{
	ulong o;
	int nb, ct;
	Extent *e;

	nb = 0;
	ct = 1;
	o = 0;
	if(m->list)
		o = m->list->start;
	for(e = m->list; e; e = e->next) {
		nb += e->len;
		if(o != e->start)
			ct = 0;
		o = e->start+e->len;
	}
	pprint("%s: %#llux.%#lux %d %d %s (%d %c)\n",
	s, m->qid.path, m->qid.vers, m->type, m->dev, c->path->s, nb, ct ? 'C' : 'N');

	for(e = m->list; e; e = e->next) {
		pprint("\t%4d %5lud %4d %#p\n",
			e->bid, e->start, e->len, e->cache);
	}
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
cnodata(Mntcache *m)
{
	Extent *e, *n;

	/*
	 * Invalidate all extent data
	 * Image lru will waste the pages
	 */
	for(e = m->list; e; e = n) {
		n = e->next;
		extentfree(e);
	}
	m->list = 0;
}

void
ctail(Mntcache *m)
{
	/* Unlink and send to the tail */
	if(m->prev)
		m->prev->next = m->next;
	else
		cache.head = m->next;
	if(m->next)
		m->next->prev = m->prev;
	else
		cache.tail = m->prev;

	if(cache.tail) {
		m->prev = cache.tail;
		cache.tail->next = m;
		m->next = 0;
		cache.tail = m;
	}
	else {
		cache.head = m;
		cache.tail = m;
		m->prev = 0;
		m->next = 0;
	}
}

void
copen(Chan *c)
{
	int h;
	Extent *e, *next;
	Mntcache *m, *f, **l;

	/* directories aren't cacheable and append-only files confuse us */
	if(c->qid.type&(QTDIR|QTAPPEND))
		return;

	h = c->qid.path%NHASH;
	qlock(&cache);
	for(m = cache.hash[h]; m; m = m->hash) {
		if(m->qid.path == c->qid.path)
		if(m->qid.type == c->qid.type)
		if(m->dev == c->dev && m->type == c->type) {
			c->mcp = m;
			ctail(m);
			qunlock(&cache);

			/* File was updated, invalidate cache */
			if(m->qid.vers != c->qid.vers) {
				m->qid.vers = c->qid.vers;
				qlock(m);
				cnodata(m);
				qunlock(m);
			}
			return;
		}
	}

	/* LRU the cache headers */
	m = cache.head;
	l = &cache.hash[m->qid.path%NHASH];
	for(f = *l; f; f = f->hash) {
		if(f == m) {
			*l = m->hash;
			break;
		}
		l = &f->hash;
	}

	m->qid = c->qid;
	m->dev = c->dev;
	m->type = c->type;

	l = &cache.hash[h];
	m->hash = *l;
	*l = m;
	ctail(m);

	qlock(m);
	c->mcp = m;
	e = m->list;
	m->list = 0;
	qunlock(&cache);

	while(e) {
		next = e->next;
		extentfree(e);
		e = next;
	}
	qunlock(m);
}

static int
cdev(Mntcache *m, Chan *c)
{
	if(m->qid.path != c->qid.path)
		return 0;
	if(m->qid.type != c->qid.type)
		return 0;
	if(m->dev != c->dev)
		return 0;
	if(m->type != c->type)
		return 0;
	if(m->qid.vers != c->qid.vers)
		return 0;
	return 1;
}

int
cread(Chan *c, uchar *buf, int len, vlong off)
{
	KMap *k;
	Page *p;
	Mntcache *m;
	Extent *e, **t;
	int o, l, total;
	ulong offset;

	if(off+len > maxcache)
		return 0;

	m = c->mcp;
	if(m == 0)
		return 0;

	qlock(m);
	if(cdev(m, c) == 0) {
		qunlock(m);
		return 0;
	}

	offset = off;
	t = &m->list;
	for(e = *t; e; e = e->next) {
		if(offset >= e->start && offset < e->start+e->len)
			break;
		t = &e->next;
	}

	if(e == 0) {
		qunlock(m);
		return 0;
	}

	total = 0;
	while(len) {
		p = cpage(e);
		if(p == 0) {
			*t = e->next;
			extentfree(e);
			qunlock(m);
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
			qunlock(m);
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

	qunlock(m);
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
		if(l > BY2PG)
			l = BY2PG;

		e->cache = p;
		e->start = offset;
		e->len = l;

		qlock(&cache);
		e->bid = cache.pgno;
		cache.pgno += BY2PG;
		/* wrap the counter; low bits are unused by pghash but checked by lookpage */
		if((cache.pgno & ~(BY2PG-1)) == 0){
			if(cache.pgno == BY2PG-1){
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
	Mntcache *m;
	Extent *tail;
	Extent *e, *f, *p;
	int o, ee, eblock;
	ulong offset;

	if(off > maxcache || len == 0)
		return;

	m = c->mcp;
	if(m == 0)
		return;
	qlock(m);
	if(cdev(m, c) == 0) {
		qunlock(m);
		return;
	}

	/*
	 * Find the insertion point
	 */
	offset = off;
	p = 0;
	for(f = m->list; f; f = f->next) {
		if(f->start > offset)
			break;
		p = f;
	}

	/* trim if there is a successor */
	eblock = offset+len;
	if(f != 0 && eblock > f->start) {
		len -= (eblock - f->start);
		if(len <= 0) {
			qunlock(m);
			return;
		}
	}

	if(p == 0) {		/* at the head */
		e = cchain(buf, offset, len, &tail);
		if(e != 0) {
			m->list = e;
			tail->next = f;
		}
		qunlock(m);
		return;
	}

	/* trim to the predecessor */
	ee = p->start+p->len;
	if(offset < ee) {
		o = ee - offset;
		len -= o;
		if(len <= 0) {
			qunlock(m);
			return;
		}
		buf += o;
		offset += o;
	}

	/* try and pack data into the predecessor */
	if(offset == ee && p->len < BY2PG) {
		o = len;
		if(o > BY2PG - p->len)
			o = BY2PG - p->len;
		if(cpgmove(p, buf, p->len, o)) {
			p->len += o;
			buf += o;
			len -= o;
			offset += o;
			if(len <= 0) {
if(f && p->start + p->len > f->start) print("CACHE: p->start=%uld p->len=%d f->start=%uld\n", p->start, p->len, f->start);
				qunlock(m);
				return;
			}
		}
	}

	e = cchain(buf, offset, len, &tail);
	if(e != 0) {
		p->next = e;
		tail->next = f;
	}
	qunlock(m);
}

void
cwrite(Chan* c, uchar *buf, int len, vlong off)
{
	int o, eo;
	Mntcache *m;
	ulong eblock, ee;
	Extent *p, *f, *e, *tail;
	ulong offset;

	if(off > maxcache || len == 0)
		return;

	m = c->mcp;
	if(m == 0)
		return;

	qlock(m);
	if(cdev(m, c) == 0) {
		qunlock(m);
		return;
	}

	offset = off;
	m->qid.vers++;
	c->qid.vers++;

	p = 0;
	for(f = m->list; f; f = f->next) {
		if(f->start >= offset)
			break;
		p = f;
	}

	if(p != 0) {
		ee = p->start+p->len;
		eo = offset - p->start;
		/* pack in predecessor if there is space */
		if(offset <= ee && eo < BY2PG) {
			o = len;
			if(o > BY2PG - eo)
				o = BY2PG - eo;
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
		m->list = f;
	else
		p->next = f;
	qunlock(m);
}
