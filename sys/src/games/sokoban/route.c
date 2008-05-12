#include <u.h>
#include <libc.h>
#include <draw.h>

#include "sokoban.h"

static int dirlist[] = { Up, Down, Left, Right, Up, Down, Left, Right, };
static int ndir = 4;

static Point
dir2point(int dir)
{
	switch(dir) {
	case Up:
		return Pt(0, -1);
	case Down:
		return Pt(0, 1);
	case Left:
		return Pt(-1, 0);
	case Right:
		return Pt(1, 0);
	}
	return Pt(0,0);
}

int
validpush(Point g, Step *s, Point *t)
{
	int i;
	Point m;

	if (s == nil)
		return 0;

	m = dir2point(s->dir);

	// first test for  Cargo next to us (in direction dir)
	if (s->count > 0) {
		g = addpt(g, m);
		if (!ptinrect(g, Rpt(Pt(0,0), level.max)))
			return 0;
		switch (level.board[g.x][g.y]) {
		case Wall:
		case Empty:
		case Goal:
			return 0;
		}
	}
	// then test for  enough free space for us _and_ Cargo
	for (i=0; i < s->count; i++) {
		g = addpt(g, m);
		if (!ptinrect(g, Rpt(Pt(0,0), level.max)))
			return 0;
		switch (level.board[g.x][g.y]) {
		case Wall:
		case Cargo:
		case GoalCargo:
			return 0;
		}
	}
	if (t != nil)
		*t = g;
	return 1;
}

int
isvalid(Point s, Route* r, int (*isallowed)(Point, Step*, Point*))
{
	Point m;
	Step *p;

	if (r == nil)
		return 0;

	m = s;
	for (p = r->step; p < r->step + r->nstep; p++)
		if (!isallowed(m, p, &m))
			return 0;
	return 1;
}

static Walk*
newwalk(void)
{
	Walk *w;

	w = mallocz(sizeof *w, 1);
	if (w == nil)
		sysfatal("cannot allocate walk");
	return w;
}

static void
resetwalk(Walk *w)
{
	Route **p;

	if (w == nil || w->route == nil)
		return;

	for (p=w->route; p < w->route + w->nroute; p++)
		freeroute(*p);
	w->nroute = 0;
}

static void
freewalk(Walk *w)
{
	if (w == nil)
		return;

	resetwalk(w);
	if(w->route)
		free(w->route);
	free(w);
}

static void
addroute(Walk *w, Route *r)
{
	if (w == nil || r == nil)
		return;

	if (w->beyond < w->nroute+1) {
		w->beyond = w->nroute+1;
		w->route = realloc(w->route, sizeof(Route*)*w->beyond);
	}
	if (w->route == nil)
		sysfatal("cannot allocate route in addroute");
	w->route[w->nroute] = r;
	w->nroute++;
}

void
freeroute(Route *r)
{
	if (r == nil)
		return;
	free(r->step);
	free(r);
}

Route*
extend(Route *rr, int dir, int count, Point dest)
{
	Route *r;

	r = mallocz(sizeof *r, 1);
	if (r == nil)
		sysfatal("cannot allocate route in extend");
	r->dest = dest;

	if (count > 0) {
		r->nstep = 1;
		if (rr != nil)
			r->nstep += rr->nstep;

		r->step = malloc(sizeof(Step)*r->nstep);
		if (r->step == nil)
			sysfatal("cannot allocate step in extend");

		if (rr != nil)
			memmove(r->step, rr->step, sizeof(Step)*rr->nstep);

		r->step[r->nstep-1].dir = dir;
		r->step[r->nstep-1].count = count;
	}
	return r;
}

static Step*
laststep(Route*r)
{
	if (r != nil && r->nstep > 0)
		return &r->step[r->nstep-1];
	return nil;
}

static int*
startwithdirfromroute(Route *r, int* dl, int n)
{
	Step *s;
	int *p;
	
	if (r == nil || dl == nil)
		return dl;

	s =  laststep(r);
	if (s == nil || s->count == 0)
		return dl;

	for (p=dl; p < dl + n; p++)
		if (*p == s->dir)
			break;
	return p;
}

static Route*
bfstrydir(Route *r, int dir, Visited *v)
{
	Point m, p, n;

	if (r == nil)
		return nil;

	m = r->dest;
	p = dir2point(dir);
	n = addpt(m, p);

	if (ptinrect(n, Rpt(Pt(0,0), level.max)) && v->board[n.x][n.y] == 0) {
		v->board[n.x][n.y] = 1;
		switch (level.board[n.x][n.y]) {
		case Empty:
		case Goal:
			return extend(r, dir, 1, n);
		}
	}
	return nil;
}

static Route*
bfs(Point src, Point dst, Visited *v)
{
	Walk *cur, *new, *tmp;
	Route *r, **p;
	int progress, *dirs, *dirp;
	Point n;

	if (v == nil)
		return nil;

	cur = newwalk();
	new = newwalk();
	v->board[src.x][src.y] = 1;
	r = extend(nil, 0, 0, src);
	if (eqpt(src, dst)) {
		freewalk(cur);
		freewalk(new);
		return r;
	}
	addroute(cur, r);
	progress = 1;
	while (progress) {
		progress = 0;
		for (p=cur->route; p < cur->route + cur->nroute; p++) {
			dirs = startwithdirfromroute(*p, dirlist, ndir);
			for (dirp=dirs; dirp < dirs + ndir; dirp++) {
				r = bfstrydir(*p, *dirp, v);
				if (r != nil) {
					progress = 1;
					n = r->dest;
					if (eqpt(n, dst)) {
						freewalk(cur);
						freewalk(new);
						return(r);
					}
					addroute(new, r);
				}
			}
		}
		resetwalk(cur);
		tmp = cur;
		cur = new;
		new = tmp;
	}
	freewalk(cur);
	freewalk(new);
	return nil;
}

Route*
findroute(Point src, Point dst)
{
	Visited v;
	Route* r;

	memset(&v, 0, sizeof(Visited));
	r = bfs(src, dst, &v);
	return r;
}
