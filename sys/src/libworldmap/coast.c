#include	<u.h>
#include	<libc.h>
#include	<thread.h>
#include	<ctype.h>
#include	<bio.h>
#include	<draw.h>
#include	<keyboard.h>
#include	<mouse.h>
#include	<worldmap.h>
#include	"mapimpl.h"

enum {
	North	= 0x1,
	East	= 0x2,
	South	= 0x4,
	West	= 0x8,
	Clockw	= 0x10,
};

enum {	/* flags in Edgepoint */
	Start	= 0x01,
	CWdone	= 0x02,
	CCWdone	= 0x04,
	Corner	= 0x08,
	Funny	= 0x10,
};

int sideorder[16] = {
	[North|West]	= 1,
	[North]			= 2,
	[North|East]	= 3,
	[East]			= 4,
	[South|East]	= 5,
	[South]			= 6,
	[South|West]	= 7,
	[West]			= 8,
};

char *sidename[16] = {
	[North|West]	= "nw",
	[North]			= "n",
	[North|East]	= "ne",
	[East]			= "e",
	[South|East]	= "se",
	[South]			= "s",
	[South|West]	= "sw",
	[West]			= "w",
};

int			polynr;

static void	background(Maptile *m, int flags, void *arg);

void
freeedge(Edgept *edgepts) {
	Edgept *e, *x;

	e = edgepts;

	if (e) {
		do {
			x = e;
			e = e->next;
			if (x->flags & (Start|Corner))	/* do it only once */
				if (x->pol) {
					if (x->pol->p)
						free(x->pol->p);
					free(x->pol);
				}
			free(x);
		} while (e != edgepts);
	}
}

void
edgedump(Edgept *edgepts) {
	Edgept *e;

	e = edgepts;
	do {
		fprint(2, "%8p: %3d %2s %8d %x %8p %8p %8p\n",
			e, e->polynr, sidename[e->side], e->dist, e->flags, e->peer, e->next, e->prev);
		e = e->next;
	} while (e != edgepts);
}

int
edgeless(Edgept *p1, Edgept *p2) {
	if (p1->side == p2->side)
		return p1->dist < p2->dist;
	else
		return sideorder[p1->side] < sideorder[p2->side];
}

Edgept *
corner(Point pt, int side) {
	Edgept *e;

	e = mallocz(sizeof(Edgept), 1);
	e->flags = Corner;
	e->pol = mallocz(sizeof(Poly), 1);
	e->pol->p = malloc(sizeof(Point));
	e->pol->p[0] = pt;
	e->pol->np = 1;
	e->side = side;
	e->dist = 0;
	return e;
}

void
edgeptins(Maptile *m, Edgept *p) {
	Edgept *e;

	if (debug & 0x10)
		fprint(2, "add poly %d (%d): %p\n", p->polynr, p->flags&Start, p);
	if (m->edgepts == nil) {
		Edgept *nw, *ne, *se, *sw;

		nw = corner(Pt(m->fr.min.x, m->fr.max.y), North|West);
		ne = corner(Pt(m->fr.max.x, m->fr.max.y), North|East);
		se = corner(Pt(m->fr.max.x, m->fr.min.y), South|East);
		sw = corner(Pt(m->fr.min.x, m->fr.min.y), South|West);
		m->edgepts = nw;
		nw->next = ne;
		nw->prev = sw;
		ne->next = se;
		ne->prev = nw;
		se->next = sw;
		se->prev = ne;
		sw->next = nw;
		sw->prev = se;
	}
	e = m->edgepts;
	do {
		if (edgeless(p, e)) {
			p->next = e;
			p->prev = e->prev;
			p->prev->next = p;
			e->prev = p;
			return;
		}
		e = e->next;
	} while (e != m->edgepts);
	p->next = e;
	p->prev = e->prev;
	p->prev->next = p;
	e->prev = p;
}

Edgept *
edgept(Point pt, Rectangle r) {
	Edgept *e = nil;

	if (pt.x == r.min.x) {
		if (e == nil) e = mallocz(sizeof *e, 1);
		e->side |= West;
		e->dist = pt.y - r.min.y;
	}
	if (pt.y == r.min.y) {
		if (e == nil) e = mallocz(sizeof *e, 1);
		e->side |= South;
		e->dist = r.max.x - pt.x;
	}
	if (pt.x == r.max.x) {
		if (e == nil) e = mallocz(sizeof *e, 1);
		e->side |= East;
		e->dist = r.max.y - pt.y;
	}
	if (pt.y == r.max.y) {
		if (e == nil) e = mallocz(sizeof *e, 1);
		e->side |= North;
		e->dist = pt.x - r.min.x;
	}
	return e;
}

void
finishpoly(Maptile *m, Poly *pol) {
	Point p0, pn;
	Edgept *ep0, *epn;

	if (pol == nil)
		return;
	if (pol->np < 2) {
		if (debug)
			fprint(2, "Ignoring polygon of %d points\n", pol->np);
		free(pol->p);
		free(pol);
		return;
	}
	p0 = pol->p[0];
	pn = pol->p[pol->np-1];
	if (!eqpt(p0, pn)) {
		if (debug & 0x10)
			fprint(2, "not closed\n");
		ep0 = edgept(p0, m->fr);
		epn = edgept(pn, m->fr);
		if (ep0 == nil || epn == nil) {
			if (debug)
				fprint(2, "Ignoring polygon of %d points: %P, %P\n",
					pol->np, p0, pn);
			if (ep0) free(ep0);
			if (epn) free(epn);
			free(pol->p);
			pol->p = nil;
			pol->np = 0;
			return;
		}
		if (debug & 0x10) {
			fprint(2, "Open poly nr %d has %d points\n", polynr, pol->np);
		}
		ep0->polynr = polynr;
		epn->polynr = polynr;
		polynr++;
		ep0->flags |= Start;
		ep0->peer = epn;
		epn->peer = ep0;
		ep0->pol = pol;
		epn->pol = pol;
		edgeptins(m, ep0);
		edgeptins(m, epn);
	} else {
		m->cp.pol = realloc(m->cp.pol, (m->cp.npol+1)*sizeof *pol);
		m->cp.pol[m->cp.npol] = pol;
		m->cp.npol++;
	}
}

void
edgepoly(Edgept *e, int cw, void *arg) {
	Edgept *es, *ef;
	Poly pol;

	pol.np = 0;
	pol.p = nil;
	es = e;
	if (debug & 0x10)
		fprint(2, "New edgepoly %p\n", e);
	do {
		if (es->flags & Corner) {
			if (debug & 0x10)
				fprint(2, "Corner is %p\n", es);
			assert(es->pol->np == 1);
			pol.p = realloc(pol.p, sizeof(Point)*(pol.np + es->pol->np));
			memmove(&pol.p[pol.np], es->pol->p,
				sizeof(Point)*es->pol->np);
			pol.np += es->pol->np;
			es = cw?es->next:es->prev;
		} else if (es->flags & Funny) {
			/* Skip funnies */
			if (debug & 0x10)
				fprint(2, "Skip funny es: %x %p\n", es->flags, es);
			es = cw?es->next:es->prev;
			continue;
		} else if ((es->flags & (Start|(cw?CWdone:CCWdone))) != Start) {
			if (debug & 0x10) {
				fprint(2, "Funny es: %x %p\n", es->flags, es);
				edgedump(e);
			}
			es->flags |= Funny;
			es->peer->flags |= Funny;
			es = cw?es->next:es->prev;
		} else {
			/* add poly starting at es to pol */
			if (debug & 0x10)
				fprint(2, "add poly es: %x %p\n", es->flags, es);
			pol.p = realloc(pol.p, sizeof(Point)*(pol.np + es->pol->np));
			memmove(&pol.p[pol.np], es->pol->p,
				sizeof(Point)*es->pol->np);
			pol.np += es->pol->np;
			es->flags |= cw?CWdone:CCWdone;
	
			/* find the endpoint */
			ef = es->peer;
			if (debug & 0x10)
				fprint(2, "Peer is %p\n", ef);
			if ((ef->flags & (Start|(cw?CWdone:CCWdone))) != 0) {
				fprint(2, "peer: %x %p\n", ef->flags, ef);
				edgedump(e);
			}
	
			/* find next point on edge in the correct direction */
			es = cw?ef->next:ef->prev;
		}
	} while (es != e);
	if (debug & 0x10)
		fprint(2, "Done %p\n", es);
	drawpoly(&pol, cw?Sea:Land, arg);
	free(pol.p);
}

int
edgepolys(Maptile *m, int flags, void *arg) {
	Edgept *e;
	int n = 0;

	e = m->edgepts;
	if (e == nil)
		return 0;
	do {
		e->flags &= ~(CWdone|CCWdone);
		e = e->next;
	} while (e != m->edgepts);
	do {
		if ((flags & (1<<Sea)) && (e->flags & (Start|CWdone|Funny)) == Start) {
			/* Clockwise, not yet drawn */
			n++;
			edgepoly(e, 1, arg);
		}
		if ((flags & (1<<Land)) && (e->flags & (Start|CCWdone|Funny)) == Start) {
			/* Counterclockwise, not yet drawn */
			n++;
			edgepoly(e, 0, arg);
		}
		e = e->next;
	} while (e != m->edgepts);
	return n;
}

int
drawfunnies(Maptile *m, void *arg) {
	Edgept *e;
	int n = 0;
	Poly pol;

	pol.np = 0;
	pol.p = nil;

	e = m->edgepts;
	if (e == nil)
		return 0;
	do {
		if ((e->flags & (Start|Funny)) == (Start|Funny)) {
			/* Funny, not yet drawn */
			if (debug & 0x10)
				fprint(2, "add funny e: %x %p\n", e->flags, e);
			pol.p = realloc(pol.p, sizeof(Point)*(pol.np + e->pol->np));
			memmove(&pol.p[pol.np], e->pol->p,
				sizeof(Point)*e->pol->np);
			pol.np += e->pol->np;
			drawpoly(&pol, Unknown, arg);
			free(pol.p);
		}
		e = e->next;
	} while (e != m->edgepts);
	return n;
}

void
closedpolys(Maptile *m, int flags, void *arg) {
	int n;
	int cw;

	for (n = 0; n < m->cp.npol; n++) {
		if ((cw = clockwise(m->cp.pol[n], nil)) && (flags & (1<<Sea)))
			drawpoly(m->cp.pol[n], Sea, arg);
		if (cw == 0 && (flags & (1<<Land)))
			drawpoly(m->cp.pol[n], Land, arg);
	}
}

void
drawcoastline(Maptile *m, void *arg) {
	int n;
	Edgept *e;

	e = m->edgepts;
	if (e) {
		do {
			if (e->flags & Start)
				drawpoly(e->pol, Coastline, arg);
			e = e->next;
		} while (e != m->edgepts);
	}
	for (n = 0; n < m->cp.npol; n++)
			drawpoly(m->cp.pol[n], Coastline, arg);
}

void
drawcoast(Maptile *m, int flags, void *arg) {

	if (edgepolys(m, flags, arg) == 0) {
		/* do background */
		background(m, flags, arg);
	}
	closedpolys(m, flags, arg);

	if (debug & 0xf0)
		drawfunnies(m, arg);
}

static void
background(Maptile *m, int flags, void *arg) {
	int type;
	Poly pol;

	switch (m->background) {
	case Sea:
		if (debug & 0x20)
			fprint(2, "%d %d: background is Sea\n", m->lat, m->lon);
		type = Sea;
		break;
	case Land:
		if (debug & 0x20)
			fprint(2, "%d %d: background is Land\n", m->lat, m->lon);
		type = Land;
		break;
	default:
		if (debug & 0x20)
			fprint(2, "%d %d: background is Unknown\n", m->lat, m->lon);
		type = Unknown;
	}
	if (m->cp.npol > 0) {
		type = clockwise(m->cp.pol[0], nil)?Land:Sea;
		if (debug & 0x20)
			fprint(2, "%d %d: poly is %sclockwise\n",
				m->lat, m->lon, (type == Land)?"counter":"");
	}
	if (flags & (1 << type)) {
		pol.np = 5;
		pol.p = malloc(sizeof(Point)*5);
		pol.p[0] = m->fr.min;
		pol.p[1] = Pt(m->fr.min.x, m->fr.max.y);
		pol.p[2] = m->fr.max;
		pol.p[3] = Pt(m->fr.max.x, m->fr.min.y);
		pol.p[4] = m->fr.min;

		drawpoly(&pol, type, arg);

		free(pol.p);
	}
}

void
drawriver(Maptile *m, int feat, void *arg) {
	int i;

	for (i = 0; i < m->features[feat].npol; i++)
		drawpoly(m->features[feat].pol[i], feat, arg);
}
