#include <stdio.h>
#include "geom.h"
#include "thing.h"
#include "text.h"
#include "wire.h"
#include "conn.h"
#include "box.h"

Box::Box()
{
	part = 0;
	suffix = 0;
	pins = new ConnList(8);
}

Box::Box(Rectangle r)
{
	part = 0;
	R = r;
	suffix = 0;
	pins = new ConnList(8);
}

Macro::Macro(Rectangle r)
{
	part = 0;
	R = r;
	suffix = 0;
	pins = new ConnList(8);
	b = new BoxList(8);
}

inline Box::~Box()			{delete pins;}

inline Macro::~Macro()			{delete pins; delete b;}

int Box::namepart(Text *t)
{
	register char *p;
	if (t->p < R) {
		if (part)		/* hokay, we have some magic to do */
			part->merge(t);
		else
			part = t;
		suffix = 0;
		for (p = part->s->s; *p; p++)
			if (*p == '.')
				suffix = p+1;
		return 1;
	}
	return 0;
}

int Box::namekahrs(Text *t)
{
	register char *p;
	if (t->p <= R.inset(Point(-16,-16))) {
		if (part)		/* hokay, we have some magic to do */
			part->merge(t);
		else
			part = t;
		suffix = 0;
		for (p = part->s->s; *p; p++)
			if (*p == '.')
				suffix = p+1;
		return 1;
	}
	return 0;
}

int Box::namepin(Text *t)
{
	if (t->p <= R && !(t->p < R)) {
		pins->append(new Conn(t,(t->p.x == R.c.x) ? OUT : IN));
		return 1;
	}
	return 0;
}

void Box::put(FILE *ouf)
{
	if (suffix) {
		*(suffix-1) = 0;	/* get rid of period */
		pins->apply((int *) suffix,(int(*)(...))Conn::fixup);
	}
	if (part == 0)
		fprintf(stderr,"unnamed part at %d,%d\n",X,Y);
	else
		fprintf(ouf,".c	%s	%s\n",part->s->s,part->t->s);
	pins->put(ouf);
}

void Box::getname(WireList *wl)
{
	if (part == 0)
		wl->apply((int *) this,(int(*)(...))Wire::namebox);
}

int BoxList::parts(Text *t)
{
	register i,j;
	for (i = 0, j = 0; i < n; i++)
		j |= box(i)->namepart(t);
	return j;
}

int BoxList::pins(Text *t)
{
	register i,j;
	for (i = 0, j = 0; i < n; i++)
		j |= box(i)->namepin(t);
	return j;
}

int BoxList::kahrs(Text *t)
{
	register i,j;
	for (i = 0, j = 0; i < n; i++)
		j |= box(i)->namekahrs(t);
	return j;
}

void BoxList::nets(WireList *w)
{
	register i;
	for (i = 0; i < n; i++)
		(box(i)->pins)->nets(w);
}

void BoxList::expand(Vector *v)
{
	register i;
	for (i = 0; i < n; i++)
		a[i]->expand(v);
}

extern "C" {
	void qsort (void*, unsigned, unsigned, int(*)(const void*, const void*));
	int strcmp(const char*, const char*);
}

blcomp(Macro **aa,Macro **bb)
{
	int i;
	Macro *a=*aa,*b=*bb;
	for (i = 0; i < b->b->n; i++)
		if (strcmp(a->part->s->s,b->box(i)->part->t->s) == 0)
			return -1;
	for (i = 0; i < a->b->n; i++)
		if (strcmp(b->part->s->s,a->box(i)->part->t->s) == 0)
			return 1;
	return 0;
}

void BoxList::absorb(BoxList *b)
{
	int i;
	for (i = 0; i < n; i++)
		b->inside((Macro *)a[i]);
	qsort((char *) a,n,(int) sizeof(Macro *),(int(*)(const void*,const void*))blcomp);
}

void Macro::put(FILE *ouf)
{
	extern mflag;
	extern char *stem;
	if (part)
		fprintf(ouf,".f %s.w\n",part->s->s);
	if (mflag && !(part && strcmp(part->s->s,stem) != 0)) {
		if (part == 0)
			part = new Text(stem,stem);
		Box::put(ouf);
	}
	else {
		pins->apply((int *) ouf,(int(*)(...))Conn::putm);
	}
	b->put(ouf);
}

void BoxList::inside(Macro *m)
{
	int i;
	for (i = 0; i < n;)
		if (((Box *)a[i])->R < m->R)
			m->b->append(remove(i));
		else
			i++;
}
