#include	"lib9.h"
#include	"sys.h"
#include	"error.h"

Pthash	syspt;		/* Local system path cache */

static void	ptgc(Pthash*);

Path*
ptenter(Pthash *pt, Path *parent, char *name)
{
	char *p;
	int h, i;
	Path *f, **l;

	if(name[0] == '.' && name[1] == '.' && name[2] == '\0') {
		f = parent->parent;
		if(f == 0) {
			refinc(&parent->r);
			return parent;
		}
		refinc(&f->r);
		return f;
	}

	p = name;
	h = 0;
	for(i = 0; i < 4 && *p; i++)
		h += *p++;
	h &= (NSCACHE-1);

	qlock(&pt->ql);

	l = &pt->hash[h];
	for(f = *l; f; f = f->hash) {
		if(f->parent == parent && strcmp(f->elem, name) == 0) {
			refinc(&f->r);
			qunlock(&pt->ql);
			return f;
		}
	}

	if(pt->npt > NSMAX)
		ptgc(pt);

	pt->npt++;

	f = mallocz(sizeof(Path));
	f->r.ref = 1;
	f->parent = parent;
	f->pthash = pt;
	if(parent != 0)	
		refinc(&parent->r);
	strcpy(f->elem, name);
	f->hash = *l;
	*l = f;
	qunlock(&pt->ql);

	return f;
}

void
ptclose(Pthash *pt)
{
	int i;
	Path *f, *next;

	for(i = 0; i < NSCACHE; i++) {
		for(f = pt->hash[i]; f; f = next) {
			next = f->hash;
			free(f);
		}
	}

	memset(pt->hash, 0, sizeof pt->hash);
}

void
ptclone(Chan *c, int l, int id)
{
	Path *np;
	char buf[32];

	np = c->path->parent;
	if(l)
		np = np->parent;

	sprint(buf, "%d", id);
	np = ptenter(np->pthash, np, buf);
	np = ptenter(np->pthash, np, "ctl");

	refdec(&c->path->r);
	c->path = np;
}

int
ptpath(Path *path, char *buf, int size)
{
	int n, l, i;
	char *elem, *c;
	Path *p, **pav;

	n = 0;
	qlock(&path->pthash->ql);
	for(p = path; p && p->parent != p; p = p->parent)
		n++;

	pav = mallocz(sizeof(Path*)*n);
	if(pav == 0) {
		qunlock(&path->pthash->ql);
		error(Enomem);
	}

	l = 0;
	i = n;
	for(p = path; p && p->parent != p; p = p->parent) {
		pav[--i] = p;
		l += strlen(p->elem)+1;
	}

	if(l >= size) {
		qunlock(&path->pthash->ql);
		free(pav);
		error(Etoosmall);
	}

	c = buf;
	for(i = 0; i < n; i++) {
		elem = pav[i]->elem;
		if(!(elem[0] == '#' && i == 0))
			*buf++ = '/';
		strcpy(buf, elem);
		buf += strlen(elem);
	}

	qunlock(&path->pthash->ql);
	free(pav);
	return strlen(c);
}

static
void
clean(Pthash *pt, Path *p)
{
	char *c;
	int h, i;
	Path *f, **l;

	while(p) {
		lock(&p->r.l);
		if(--p->r.ref != 0) {
			unlock(&p->r.l);
			return;
		}

		h = 0;
		c = p->elem;
		for(i = 0; i < 4 && *c; i++)
			h += *c++;
		h &= (NSCACHE-1);

		l = &pt->hash[h];
		for(f = *l; f; f = f->hash) {
			if(f == p) {
				*l = f->hash;
				break;
			}
			l = &f->hash;
		}
		pt->npt--;
		f = p->parent;
		free(p);
		p = f;
	}
}

static
void
ptgc(Pthash *pt)
{
	int i;
	Path *p, **l;

	for(i = 0; i < NSCACHE; i++) {
		chain:
		l = &pt->hash[i];
		for(p = *l; p; p = p->hash) {
			if(p->r.ref == 0) {
				lock(&p->r.l);
				if(p->r.ref == 0) {
					*l = p->hash;
					clean(pt, p->parent);
					free(p);
					pt->npt--;
					goto chain;
				}
				unlock(&p->r.l);	
			}
			l = &p->hash;
		}			
	}
}
