#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include "flashfs.h"

static	int	nextfnum;
static	Intmap*	map;
static	Dir	dirproto;

static	char	user[]	= "flash";

	Entry	*root;
	ulong	used;
	ulong	limit;
	ulong	maxwrite;

enum
{
	debug	= 0,
};

static int
fnum(void)
{
	return ++nextfnum;
}

static void
maxfnum(int n)
{
	if(n > nextfnum)
		nextfnum = n;
}

static int
hash(char *s)
{
	int c, d, h;

	h = 0;
	while((c = *s++) != '\0') {
		d = c;
		c ^= c << 6;
		h += (c << 11) ^ (c >> 1);
		h ^= (d << 14) + (d << 7) + (d << 4) + d;
	}

	if(h < 0)
		return ~h;
	return h;
}

static void
dirinit(Entry *e)
{
	Entry **t;

	e->size = 0;
	t = emalloc9p(HSIZE * sizeof(Entry*));
	memset(t, 0, HSIZE * sizeof(Entry*));
	e->htab = t;
	e->files = nil;
	e->readers = nil;
}

static void
fileinit(Entry *e)
{
	e->size = 0;
	e->gen[0].head = nil;
	e->gen[0].tail = nil;
	e->gen[1].head = nil;
	e->gen[1].tail = nil;
}

static void
elfree(Extent *x)
{
	Extent *t;

	while(x != nil) {
		t = x->next;
		used -= x->size;
		free(x);
		x = t;
	}
}

static void
extfree(Entry *e)
{
	elfree(e->gen[0].head);
	elfree(e->gen[1].head);
}

static void
efree(Entry *e)
{
	if(debug)
		fprint(2, "free %s\n", e->name);

	if(e->mode & DMDIR)
		free(e->htab);
	else
		extfree(e);

	free(e->name);
	free(e);
}

void
einit(void)
{
	Entry *e;

	e = emalloc9p(sizeof(Entry));
	e->ref = 1;
	e->parent = nil;
	dirinit(e);
	e->name = estrdup9p("");
	e->fnum = 0;
	e->mode = DMDIR | 0775;
	e->mnum = 0;
	e->mtime = 0;
	root = e;
	map = allocmap(nil);
}

static void
dumptree(Entry *e, int n)
{
	Entry *l;

	if(debug)
		fprint(2, "%d %s %d\n", n, e->name, e->ref);

	if(e->mode & DMDIR) {
		n++;
		for(l = e->files; l != nil; l = l->fnext)
			dumptree(l, n);
	}
}

void
edump(void)
{
	if(debug)
		dumptree(root, 0);
}

Entry *
elookup(ulong key)
{
	if(key == 0)
		return root;
	maxfnum(key);
	return lookupkey(map, key);
}

Extent *
esum(Entry *e, int sect, ulong addr, int *more)
{
	Exts *x;
	Extent *t, *u;

	x = &e->gen[eparity];
	t = x->tail;
	if(t == nil || t->sect != sect || t->addr != addr)
		return nil;
	u = t->prev;
	if(u != nil) {
		u->next = nil;
		*more = 1;
	}
	else {
		x->head = nil;
		*more = 0;
	}
	x->tail = u;
	x = &e->gen[eparity^1];
	u = x->head;
	t->next = u;
	x->head = t;
	if(u == nil)
		x->tail = t;
	else
		u->prev = t;
	return t;
}

void
edestroy(Entry *e)
{
	e->ref--;
	if(e->ref == 0)
		efree(e);
}

Entry *
ecreate(Entry *d, char *name, ulong n, ulong mode, ulong mtime, char **err)
{
	int h;
	Entry *e, *f;

	h = hash(name) & HMASK;
	for(e = d->htab[h]; e != nil; e = e->hnext) {
		if(strcmp(e->name, name) == 0) {
			*err = Eexists;
			return nil;
		}
	}

	e = emalloc9p(sizeof(Entry));
	e->ref = 1;
	e->parent = d;
	d->ref++;
	f = d->htab[h];
	e->hnext = f;
	e->hprev = nil;
	if(f != nil)
		f->hprev = e;
	d->htab[h] = e;
	f = d->files;
	e->fnext = f;
	e->fprev = nil;
	if(f != nil)
		f->fprev = e;
	d->files = e;

	d->ref--;
	e->ref++;
	e->name = estrdup9p(name);
	if(n == 0)
		n = fnum();
	else
		maxfnum(n);
	insertkey(map, n, e);
	e->fnum = n;
	e->mode = mode & d->mode;
	e->mnum = 0;
	e->mtime = mtime;

	if(e->mode & DMDIR)
		dirinit(e);
	else
		fileinit(e);

	d->mtime = mtime;
	return e;
}

void
etrunc(Entry *e, ulong n, ulong mtime)
{
	extfree(e);
	deletekey(map, e->fnum);
	if(n == 0)
		n = fnum();
	else
		maxfnum(n);
	e->fnum = n;
	e->mnum = 0;
	e->mtime = mtime;
	insertkey(map, n, e);
	fileinit(e);
	e->parent->mtime = mtime;
}

char *
eremove(Entry *e)
{
	Dirr *r;
	Entry *d, *n, *p;

	d = e->parent;
	if(d == nil)
		return Eperm;

	if((e->mode & DMDIR) != 0 && e->files != nil)
		return Edirnotempty;

	p = e->hprev;
	n = e->hnext;
	if(n != nil)
		n->hprev = p;
	if(p != nil)
		p->hnext = n;
	else
		d->htab[hash(e->name) & HMASK] = n;

	for(r = d->readers; r != nil; r = r->next) {
		if(r->cur == e)
			r->cur = e->fnext;
	}

	p = e->fprev;
	n = e->fnext;
	if(n != nil)
		n->fprev = p;
	if(p != nil)
		p->fnext = n;
	else
		d->files = n;

	e->parent = nil;
	d->ref--;
	deletekey(map, e->fnum);
	edestroy(e);
	return nil;
}

Entry *
ewalk(Entry *d, char *name, char **err)
{
	Entry *e;

	if((d->mode & DMDIR) == 0) {
		*err = Enotdir;
		return nil;
	}

	if(strcmp(name, "..") == 0) {
		e = d->parent;
		if(e == nil)
			return d;
		edestroy(d);
		e->ref++;
		return e;
	}

	for(e = d->htab[hash(name) & HMASK]; e != nil; e = e->hnext) {
		if(strcmp(e->name, name) == 0) {
			d->ref--;
			e->ref++;
			return e;
		}
	}

	*err = Enonexist;
	return nil;
}

static void
eread0(Extent *e, Extent *x, uchar *a, ulong n, ulong off)
{
	uchar *a0, *a1;
	ulong n0, n1, o0, o1, d, z;

	for(;;) {
		while(e != nil) {
			if(off < e->off + e->size && off + n > e->off) {
				if(off >= e->off) {
					d = off - e->off;
					z = e->size - d;
					if(n <= z) {
						readdata(e->sect, a, n, e->addr + d);
						return;
					}
					readdata(e->sect, a, z, e->addr + d);
					a += z;
					n -= z;
					off += z;
				}
				else {
					a0 = a;
					n0 = e->off - off;
					o0 = off;
					a += n0;
					n -= n0;
					off += n0;
					z = e->size;
					if(n <= z) {
						readdata(e->sect, a, n, e->addr);
						a = a0;
						n = n0;
						off = o0;
					}
					else {
						readdata(e->sect, a, z, e->addr);
						a1 = a + z;
						n1 = n - z;
						o1 = off + z;
						if(n0 < n1) {
							eread0(e->next, x, a0, n0, o0);
							a = a1;
							n = n1;
							off = o1;
						}
						else {
							eread0(e->next, x, a1, n1, o1);
							a = a0;
							n = n0;
							off = o0;
						}
					}
				}
			}
			e = e->next;
		}

		if(x == nil)
			break;

		e = x;
		x = nil;
	}

	memset(a, 0, n);
}

ulong
eread(Entry *e, int parity, void *a, ulong n, ulong off)
{
	if(n + off >= e->size)
		n = e->size - off;
	if(n <= 0)
		return 0;
	eread0(e->gen[parity].head, e->gen[parity^1].head, a, n, off);
	return n;
}

void
ewrite(Entry *e, Extent *x, int parity, ulong mtime)
{
	ulong z;
	Extent *t;

	t = e->gen[parity].head;
	x->next = t;
	x->prev = nil;
	e->gen[parity].head = x;
	if(t == nil)
		e->gen[parity].tail = x;
	else
		t->prev = x;
	if(mtime != 0)
		e->mtime = mtime;
	used += x->size;
	z = x->off + x->size;
	if(z > e->size)
		e->size = z;
}

ulong
echmod(Entry *e, ulong mode, ulong mnum)
{
	if(mnum != 0)
		e->mnum = mnum;
	else
		e->mnum++;
	e->mode &= ~0777;
	e->mode |= mode;
	return e->mnum;
}

Qid
eqid(Entry *e)
{
	Qid qid;

	if(e->mode & DMDIR)
		qid.type = QTDIR;
	else
		qid.type = 0;
	qid.path = e->fnum;
	return qid;
}

void
estat(Entry *e, Dir *d, int alloc)
{
	d->type = 'z';
	d->dev = 0;
	if(alloc) {
		d->uid = estrdup9p(user);
		d->gid = estrdup9p(user);
		d->muid = estrdup9p(user);
		d->name = estrdup9p(e->name);
	}
	else {
		d->uid = user;
		d->gid = user;
		d->muid = user;
		d->name = e->name;
	}
	d->mode = e->mode;
	d->length = e->size;
	d->atime = e->mtime;
	d->mtime = e->mtime;
	d->qid = eqid(e);
}

Dirr *
ediropen(Entry *e)
{
	Dirr *d, *t;

	d = emalloc9p(sizeof(Dirr));
	d->dir = e;
	d->cur = e->files;
	t = e->readers;
	d->next = t;
	d->prev = nil;
	if(t != nil)
		t->prev = d;
	e->readers = d;
	e->ref++;
	return d;
}

int
edirread(Dirr *r, char *a, long n)
{
	Dir d;
	Entry *e;
	int m, x;

	m = 0;
	for(e = r->cur; e != nil; e = e->fnext) {
		estat(e, &d, 0);
		x = convD2M(&d, (uchar *)a, n);
		if(x <= BIT16SZ)
			break;
		a += x;
		n -= x;
		m += x;
	}

	r->cur = e;
	return m;
}

void
edirclose(Dirr *d)
{
	Entry *e;
	Dirr *p, *n;

	e = d->dir;
	p = d->prev;
	n = d->next;
	if(n != nil)
		n->prev = p;
	if(p != nil)
		p->next = n;
	else
		e->readers = n;

	edestroy(e);
	free(d);
}

static	Renum	R;

static void
xrenum(Extent *x)
{
	while(x != nil) {
		if(x->sect == R.old)
			x->sect = R.new;
		x = x->next;
	}
}

static void
renum(Entry *e)
{
	if(e->mode & DMDIR) {
		for(e = e->files; e != nil; e = e->fnext)
			renum(e);
	}
	else {
		xrenum(e->gen[0].head);
		xrenum(e->gen[1].head);
	}
}

void
erenum(Renum *r)
{
	R = *r;
	renum(root);
}
