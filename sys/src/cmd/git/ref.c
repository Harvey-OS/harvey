#include <u.h>
#include <libc.h>
#include <ctype.h>

#include "git.h"

typedef struct Eval	Eval;
typedef struct XObject	XObject;
typedef struct Objq	Objq;

enum {
	Blank,
	Keep,
	Drop,
};

struct Eval {
	char	*str;
	char	*p;
	Object	**stk;
	int	nstk;
	int	stksz;
};

struct XObject {
	Object	*obj;
	Object	*mark;
	XObject	*queue;
	XObject	*next;
};

struct Objq {
	Objq	*next;
	Object	*o;
	int	color;
};

static Object zcommit = {
	.type=GCommit
};

void
eatspace(Eval *ev)
{
	while(isspace(ev->p[0]))
		ev->p++;
}

int
objdatecmp(void *pa, void *pb)
{
	Object *a, *b;
	int r;

	a = readobject((*(Object**)pa)->hash);
	b = readobject((*(Object**)pb)->hash);
	assert(a->type == GCommit && b->type == GCommit);
	if(a->commit->mtime == b->commit->mtime)
		r = 0;
	else if(a->commit->mtime < b->commit->mtime)
		r = -1;
	else
		r = 1;
	unref(a);
	unref(b);
	return r;
}

void
push(Eval *ev, Object *o)
{
	if(ev->nstk == ev->stksz){
		ev->stksz = 2*ev->stksz + 1;
		ev->stk = erealloc(ev->stk, ev->stksz*sizeof(Object*));
	}
	ev->stk[ev->nstk++] = o;
}

Object*
pop(Eval *ev)
{
	if(ev->nstk == 0)
		sysfatal("stack underflow");
	return ev->stk[--ev->nstk];
}

Object*
peek(Eval *ev)
{
	if(ev->nstk == 0)
		sysfatal("stack underflow");
	return ev->stk[ev->nstk - 1];
}

int
isword(char e)
{
	return isalnum(e) || e == '/' || e == '-' || e == '_' || e == '.';
}

int
word(Eval *ev, char *b, int nb)
{
	char *p, *e;
	int n;

	p = ev->p;
	for(e = p; isword(*e) && strncmp(e, "..", 2) != 0; e++)
		/* nothing */;
	/* 1 for nul terminator */
	n = e - p + 1;
	if(n >= nb)
		n = nb;
	snprint(b, n, "%s", p);
	ev->p = e;
	return n > 0;
}

int
take(Eval *ev, char *m)
{
	int l;

	l = strlen(m);
	if(strncmp(ev->p, m, l) != 0)
		return 0;
	ev->p += l;
	return 1;
}

XObject*
hnode(XObject *ht[], Object *o)
{
	XObject *h;
	int	hh;

	hh = o->hash.h[0] & 0xff;
	for(h = ht[hh]; h; h = h->next)
		if(hasheq(&o->hash, &h->obj->hash))
			return h;

	h = emalloc(sizeof(*h));
	h->obj = o;
	h->mark = nil;
	h->queue = nil;
	h->next = ht[hh];
	ht[hh] = h;
	return h;
}

Object*
ancestor(Object *a, Object *b)
{
	Object *o, *p, *r;
	XObject *ht[256];
	XObject *h, *q, *q1, *q2;
	int i;

	if(a == b)
		return a;
	if(a == nil || b == nil)
		return nil;
	r = nil;
	memset(ht, 0, sizeof(ht));
	q1 = nil;

	h = hnode(ht, a);
	h->mark = a;
	h->queue = q1;
	q1 = h;

	h = hnode(ht, b);
	h->mark = b;
	h->queue = q1;
	q1 = h;

	while(1){
		q2 = nil;
		while(q = q1){
			q1 = q->queue;
			q->queue = nil;
			o = q->obj;
			for(i = 0; i < o->commit->nparent; i++){
				p = readobject(o->commit->parent[i]);
				if(p == nil)
					goto err;
				h = hnode(ht, p);
				if(h->mark != nil){
					if(h->mark != q->mark){
						r = h->obj;
						goto done;
					}
				} else {
					h->mark = q->mark;
					h->queue = q2;
					q2 = h;
				}
			}
		}
		if(q2 == nil){
err:
			werrstr("no common ancestor");
			break;
		}
		q1 = q2;
	}
done:
	for(i=0; i<nelem(ht); i++){
		while(h = ht[i]){
			ht[i] = h->next;
			free(h);
		}
	}
	return r;
}

int
lca(Eval *ev)
{
	Object *a, *b, *o;

	if(ev->nstk < 2){
		werrstr("ancestor needs 2 objects");
		return -1;
	}
	a = pop(ev);
	b = pop(ev);
	o = ancestor(a, b);
	if(o == nil)
		return -1;
	push(ev, o);
	return 0;
}

static int
repaint(Objset *keep, Objset *drop, Object *o)
{
	Object *p;
	int i;

	if(!oshas(keep, o->hash) && !oshas(drop, o->hash)){
		dprint(2, "repaint: blank => drop %H\n", o->hash);
		osadd(drop, o);
		return 0;
	}
	if(oshas(keep, o->hash))
		dprint(2, "repaint: keep => drop %H\n", o->hash);
	osadd(drop, o);
	for(i = 0; i < o->commit->nparent; i++){
		if((p = readobject(o->commit->parent[i])) == nil)
			return -1;
		if(repaint(keep, drop, p) == -1)
			return -1;
		unref(p);
	}
	return 0;
}

int
findtwixt(Hash *head, int nhead, Hash *tail, int ntail, Object ***res, int *nres)
{
	Objq *q, *e, *n, **p;
	Objset keep, drop;
	Object *o, *c;
	int i, ncolor;

	e = nil;
	q = nil;
	p = &q;
	osinit(&keep);
	osinit(&drop);
	for(i = 0; i < nhead; i++){
		if(hasheq(&head[i], &Zhash))
			continue;
		if((o = readobject(head[i])) == nil){
			fprint(2, "warning: %H does not point at commit\n", o->hash);
			werrstr("read head %H: %r", head[i]);
			return -1;
		}
		if(o->type != GCommit){
			fprint(2, "warning: %H does not point at commit\n", o->hash);
			unref(o);
			continue;
		}
		dprint(1, "twixt init: keep %H\n", o->hash);
		e = emalloc(sizeof(Objq));
		e->o = o;
		e->color = Keep;
		*p = e;
		p = &e->next;
		unref(o);
	}		
	for(i = 0; i < ntail; i++){
		if(hasheq(&tail[i], &Zhash))
			continue;
		if((o = readobject(tail[i])) == nil){
			werrstr("read tail %H: %r", tail[i]);
			return -1;
		}
		if(o->type != GCommit){
			fprint(2, "warning: %H does not point at commit\n", o->hash);
			unref(o);
			continue;
		}
		dprint(1, "init: drop %H\n", o->hash);
		e = emalloc(sizeof(Objq));
		e->o = o;
		e->color = Drop;
		*p = e;
		p = &e->next;
		unref(o);
	}

	dprint(1, "finding twixt commits\n");
	while(q != nil){
		if(oshas(&drop, q->o->hash))
			ncolor = Drop;
		else if(oshas(&keep, q->o->hash))
			ncolor = Keep;
		else
			ncolor = Blank;
		if(ncolor == Drop || ncolor == Keep && q->color == Keep)
			goto next;
		if(ncolor == Keep && q->color == Drop){
			if(repaint(&keep, &drop, q->o) == -1)
				goto error;
		}else if (ncolor == Blank) {
			dprint(2, "visit: %s %H\n", q->color == Keep ? "keep" : "drop", q->o->hash);
			if(q->color == Keep)
				osadd(&keep, q->o);
			else
				osadd(&drop, q->o);
			for(i = 0; i < q->o->commit->nparent; i++){
				if((c = readobject(q->o->commit->parent[i])) == nil)
					goto error;
				if(c->type != GCommit){
					fprint(2, "warning: %H does not point at commit\n", c->hash);
					unref(c);
					continue;
				}
				dprint(2, "enqueue: %s %H\n", q->color == Keep ? "keep" : "drop", c->hash);
				n = emalloc(sizeof(Objq));
				n->color = q->color;
				n->next = nil;
				n->o = c;
				e->next = n;
				e = n;
				unref(c);
			}
		}else{
			sysfatal("oops");
		}
next:
		n = q->next;
		free(q);
		q = n;
	}
	*res = eamalloc(keep.nobj, sizeof(Object*));
	*nres = 0;
	for(i = 0; i < keep.sz; i++){
		if(keep.obj[i] != nil && !oshas(&drop, keep.obj[i]->hash)){
			(*res)[*nres] = keep.obj[i];
			(*nres)++;
		}
	}
	osclear(&keep);
	osclear(&drop);
	return 0;
error:
	for(; q != nil; q = n) {
		n = q->next;
		free(q);
	}
	return -1;
}

static int
parent(Eval *ev)
{
	Object *o, *p;

	o = pop(ev);
	/* Special case: first commit has no parent. */
	if(o->commit->nparent == 0)
		p = emptydir();
	else if ((p = readobject(o->commit->parent[0])) == nil){
		werrstr("no parent for %H", o->hash);
		return -1;
	}
		
	push(ev, p);
	return 0;
}

static int
unwind(Eval *ev, Object **obj, int *idx, int nobj, Object **p, Objset *set, int keep)
{
	int i;

	for(i = nobj; i >= 0; i--){
		idx[i]++;
		if(keep && !oshas(set, obj[i]->hash)){
			push(ev, obj[i]);
			osadd(set, obj[i]);
		}else{
			osadd(set, obj[i]);
		}
		if(idx[i] < obj[i]->commit->nparent){
			*p = obj[i];
			return i;
		}
		unref(obj[i]);
	}
	return -1;
}

static int
range(Eval *ev)
{
	Object *a, *b, *p, *q, **all;
	int nall, *idx, mark;
	Objset keep, skip;

	b = pop(ev);
	a = pop(ev);
	if(hasheq(&b->hash, &Zhash))
		b = &zcommit;
	if(hasheq(&a->hash, &Zhash))
		a = &zcommit;
	if(a->type != GCommit || b->type != GCommit){
		werrstr("non-commit object in range");
		return -1;
	}

	p = b;
	all = nil;
	idx = nil;
	nall = 0;
	mark = ev->nstk;
	osinit(&keep);
	osinit(&skip);
	osadd(&keep, a);
	while(1){
		all = earealloc(all, (nall + 1), sizeof(Object*));
		idx = earealloc(idx, (nall + 1), sizeof(int));
		all[nall] = p;
		idx[nall] = 0;
		if(p == a || p->commit->nparent == 0 && a == &zcommit){
			if((nall = unwind(ev, all, idx, nall, &p, &keep, 1)) == -1)
				break;
		}else if(p->commit->nparent == 0){
			if((nall = unwind(ev, all, idx, nall, &p, &skip, 0)) == -1)
				break;
		}else if(oshas(&keep, p->hash)){
			if((nall = unwind(ev, all, idx, nall, &p, &keep, 1)) == -1)
				break;
		}else if(oshas(&skip, p->hash))
			if((nall = unwind(ev, all, idx, nall, &p, &skip, 0)) == -1)
				break;
		if(p->commit->nparent == 0)
			break;
		if((q = readobject(p->commit->parent[idx[nall]])) == nil){
			werrstr("bad commit %H", p->commit->parent[idx[nall]]);
			goto error;
		}
		if(q->type != GCommit){
			werrstr("not commit: %H", q->hash);
			goto error;
		}
		p = q;
		nall++;
	}
	free(all);
	qsort(ev->stk + mark, ev->nstk - mark, sizeof(Object*), objdatecmp);
	return 0;
error:
	free(all);
	return -1;
}

int
readref(Hash *h, char *ref)
{
	static char *try[] = {"", "refs/", "refs/heads/", "refs/remotes/", "refs/tags/", nil};
	char buf[256], s[256], **pfx;
	int r, f, n;

	/* TODO: support hash prefixes */
	if((r = hparse(h, ref)) != -1)
		return r;
	if(strcmp(ref, "HEAD") == 0){
		snprint(buf, sizeof(buf), ".git/HEAD");
		if((f = open(buf, OREAD)) == -1)
			return -1;
		if((n = readn(f, s, sizeof(s) - 1))== -1)
			return -1;
		s[n] = 0;
		strip(s);
		r = hparse(h, s);
		goto found;
	}
	for(pfx = try; *pfx; pfx++){
		snprint(buf, sizeof(buf), ".git/%s%s", *pfx, ref);
		if((f = open(buf, OREAD)) == -1)
			continue;
		if((n = readn(f, s, sizeof(s) - 1)) == -1)
			continue;
		s[n] = 0;
		strip(s);
		r = hparse(h, s);
		close(f);
		goto found;
	}
	return -1;

found:
	if(r == -1 && strstr(s, "ref: ") == s)
		r = readref(h, s + strlen("ref: "));
	return r;
}

int
evalpostfix(Eval *ev)
{
	char name[256];
	Object *o;
	Hash h;

	eatspace(ev);
	if(!word(ev, name, sizeof(name))){
		werrstr("expected name in expression");
		return -1;
	}
	if(readref(&h, name) == -1){
		werrstr("invalid ref %s", name);
		return -1;
	}
	if(hasheq(&h, &Zhash))
		o = &zcommit;
	else if((o = readobject(h)) == nil){
		werrstr("invalid ref %s (hash %H)", name, h);
		return -1;
	}
	push(ev, o);

	while(1){
		eatspace(ev);
		switch(ev->p[0]){
		case '^':
		case '~':
			ev->p++;
			if(parent(ev) == -1)
				return -1;
			break;
		case '@':
			ev->p++;
			if(lca(ev) == -1)
				return -1;
			break;
		default:
			goto done;
			break;
		}	
	}
done:
	return 0;
}

int
evalexpr(Eval *ev, char *ref)
{
	memset(ev, 0, sizeof(*ev));
	ev->str = ref;
	ev->p = ref;

	while(1){
		if(evalpostfix(ev) == -1)
			return -1;
		if(ev->p[0] == '\0')
			return 0;
		else if(take(ev, ":") || take(ev, "..")){
			if(evalpostfix(ev) == -1)
				return -1;
			if(ev->p[0] != '\0'){
				werrstr("junk at end of expression");
				return -1;
			}
			return range(ev);
		}
	}
}

int
resolverefs(Hash **r, char *ref)
{
	Eval ev;
	Hash *h;
	int i;

	if(evalexpr(&ev, ref) == -1){
		free(ev.stk);
		return -1;
	}
	h = eamalloc(ev.nstk, sizeof(Hash));
	for(i = 0; i < ev.nstk; i++)
		h[i] = ev.stk[i]->hash;
	*r = h;
	free(ev.stk);
	return ev.nstk;
}

int
resolveref(Hash *r, char *ref)
{
	Eval ev;

	if(evalexpr(&ev, ref) == -1){
		free(ev.stk);
		return -1;
	}
	if(ev.nstk != 1){
		werrstr("ambiguous ref expr");
		free(ev.stk);
		return -1;
	}
	*r = ev.stk[0]->hash;
	free(ev.stk);
	return 0;
}

int
readrefdir(Hash **refs, char ***names, int *nrefs, char *dpath, char *dname)
{
	Dir *d, *e, *dir;
	char *path, *name, *sep;
	int ndir;

	if((ndir = slurpdir(dpath, &dir)) == -1)
		return -1;
	sep = (*dname == '\0') ? "" : "/";
	e = dir + ndir;
	for(d = dir; d != e; d++){
		path = smprint("%s/%s", dpath, d->name);
		name = smprint("%s%s%s", dname, sep, d->name);
		if(d->mode & DMDIR) {
			if(readrefdir(refs, names, nrefs, path, name) == -1)
				goto noref;
		}else{
			*refs = erealloc(*refs, (*nrefs + 1)*sizeof(Hash));
			*names = erealloc(*names, (*nrefs + 1)*sizeof(char*));
			if(resolveref(&(*refs)[*nrefs], name) == -1)
				goto noref;
			(*names)[*nrefs] = name;
			*nrefs += 1;
			goto next;
		}
noref:		free(name);
next:		free(path);
	}
	free(dir);
	return 0;
}

int
listrefs(Hash **refs, char ***names)
{
	int nrefs;

	*refs = nil;
	*names = nil;
	nrefs = 0;
	if(readrefdir(refs, names, &nrefs, ".git/refs", "") == -1){
		free(*refs);
		return -1;
	}
	return nrefs;
}
