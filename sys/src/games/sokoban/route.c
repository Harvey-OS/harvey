#include <u.h>
#include <libc.h>
#include <draw.h>

#include "sokoban.h"

static int trydir(int, Point, Point, Route*, Visited*);
static int dofind(Point, Point, Route*, Visited*);

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

Route*
newroute(void)
{
	Route *r = malloc(sizeof(Route));
	memset(r, 0, sizeof(Route));
	return r;
}

void
freeroute(Route *r)
{
	if (r->step != nil) {
		free(r->step);
		memset(r, 0, sizeof(Route));
	}
	free(r);
}

void
reverseroute(Route *r)
{
	int i;
	Step tmp;

	for (i=1; i< r->nstep; i++) {
		tmp = r->step[i];
		r->step[i] = r->step[i-1];
		r->step[i-1] = tmp;
	}
}

void
pushstep(Route *r, int dir, int count)
{
	if (r->beyond < r->nstep+1) {
		r->beyond = r->nstep+1;
		r->step = realloc(r->step, sizeof(Step)*r->beyond);
	}
	if (r->step == nil)
		exits("pushstep out of memory");
	r->step[r->nstep].dir = dir;
	r->step[r->nstep].count = count;
	r->nstep++;
}

void
popstep(Route*r)
{
	if (r->nstep > 0) {
		r->nstep--;
		r->step[r->nstep].dir = 0;
		r->step[r->nstep].count = 0;
	}
}

int
validpush(Point g, Step s, Point *t)
{
	int i;
	Point m = dir2point(s.dir);

	// first test for  Cargo next to us (in direction dir)
	if (s.count > 0) {
		g = addpt(g, m);
		if (!ptinrect(g, Rpt(Pt(0,0), level.max)))
			return 0;
		switch (level.board[g.x ][g.y]) {
		case Wall:
		case Empty:
		case Goal:
			return 0;
		}
	}
	// then test for  enough free space for us _and_ Cargo
	for (i=0; i < s.count; i++) {
		g = addpt(g, m);
		if (!ptinrect(g, Rpt(Pt(0,0), level.max)))
			return 0;
		switch (level.board[g.x ][g.y]) {
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
validwalk(Point g, Step s, Point *t)
{
	int i;
	Point m = dir2point(s.dir);

	for (i=0; i < s.count; i++) {
		g = addpt(g, m);
		if (!ptinrect(g, Rpt(Pt(0,0), level.max)))
			return 0;
		switch (level.board[g.x ][g.y]) {
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
isvalid(Point s, Route* r, int (*isallowed)(Point, Step, Point*))
{
	int i;
	Point m = s;

	for (i=0; i< r->nstep; i++)
		if (! isallowed(m, r->step[i], &m))
			return 0;
	return 1;
}

static int
trydir(int dir, Point m, Point d, Route *r, Visited *v)
{
	Point p = dir2point(dir);
	Point n = addpt(m, p);

	if (ptinrect(n, Rpt(Pt(0,0), level.max)) &&
	    v->board[n.x][n.y] == 0) {
		v->board[n.x][n.y] = 1;
		switch (level.board[n.x ][n.y]) {
		case Empty:
		case Goal:
			pushstep(r, dir, 1);
			if (dofind(n, d, r, v))
				return 1;
			else
				popstep(r);
		}
	}
	return 0;
}

static int
dofind(Point m, Point d, Route *r, Visited *v)
{
	if (eqpt(m, d))
		return 1;

	v->board[m.x][m.y] = 1;

	return trydir(Left, m, d, r, v) ||
	            trydir(Up, m, d, r, v) ||
	            trydir(Right, m, d, r, v) ||
	            trydir(Down, m, d, r, v);
}

static int
dobfs(Point m, Point d, Route *r, Visited *v)
{
	if (eqpt(m, d))
		return 1;

	v->board[m.x][m.y] = 1;

	return trydir(Left, m, d, r, v) ||
	            trydir(Up, m, d, r, v) ||
	            trydir(Right, m, d, r, v) ||
	            trydir(Down, m, d, r, v);
}

int
findwalk(Point src, Point dst, Route *r)
{
	Visited* v;
	int found;

	v = malloc(sizeof(Visited));
	memset(v, 0, sizeof(Visited));
	found = dofind(src, dst, r, v);
	free(v);
	
	return found;
}

void
applyroute(Route *r)
{
	int j, i;
	
	for (i=0; i< r->nstep; i++) {
		j = r->step[i].count;
		while (j > 0) {
			move(r->step[i].dir);
			if (animate) {
				drawscreen();
				sleep(200);
			}
			j--;
		}
	}
}
