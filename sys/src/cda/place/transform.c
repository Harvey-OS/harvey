#include <u.h>
#include <libc.h>
#include <libg.h>
#include "menu.h"
#include "term.h"
#include "proto.h"


#define	xstob(c)	(scrn.br.origin.x+muldiv(UNITMAG*10, c-scrn.sr.origin.x, scrn.mag))
#define	xbtos(c)	(scrn.sr.origin.x+muldiv(scrn.mag, c-scrn.br.origin.x, UNITMAG*10))
#define	ystob(c)	(scrn.br.origin.y+muldiv(UNITMAG*10, scrn.sr.corner.y-c, scrn.mag))
#define	ybtos(c)	(scrn.sr.corner.y-muldiv(scrn.mag, c-scrn.br.origin.y, UNITMAG*10))

Point
pstob(Point p)
{
	p.x = xstob(p.x);
	p.y = ystob(p.y);
	return(p);
}

Point
pbtos(Point p)
{
	p.x = xbtos(p.x);
	p.y = ybtos(p.y);
	return(p);
}

Rectangle
rstob(Rectangle r)
{
	int y;

	r.origin.x = xstob(r.origin.x);
	y = ystob(r.corner.y);
	r.corner.x = xstob(r.corner.x);
	r.corner.y = ystob(r.origin.y);
	r.origin.y = y;
	return(r);
}

Rectangle
rbtos(Rectangle r)
{
	int y;

	r.origin.x = xbtos(r.origin.x);
	y = ybtos(r.corner.y);
	r.corner.x = xbtos(r.corner.x);
	r.corner.y = ybtos(r.origin.y);
	r.origin.y = y;
	return(r);
}

Point
confine(Point p, Rectangle r)
{
	if(p.x < r.origin.x) p.x = r.origin.x;
	if(p.y < r.origin.y) p.y = r.origin.y;
	if(p.x > r.corner.x) p.x = r.corner.x;
	if(p.y > r.corner.y) p.y = r.corner.y;
	return(p);
}

int
rinr(Rectangle a, Rectangle b)
{
	return(ptinrect(a.origin, b) && ptinrect(sub(a.corner, Pt(1, 1)), b));
}

Rectangle
rmax(Rectangle a, Rectangle b)
{
	Rectangle r;

	r.origin.x = Min(a.origin.x, b.origin.x);
	r.origin.y = Min(a.origin.y, b.origin.y);
	r.corner.x = Max(a.corner.x, b.corner.x);
	r.corner.y = Max(a.corner.y, b.corner.y);
	return(r);
}

Point
snap(Point p)
{
	Point q;

	q.x = (p.x+scrn.grid-1)/scrn.grid;
	q.x *= scrn.grid;
	if(q.x-p.x > scrn.grid/2)
		q.x -= scrn.grid;
	q.y = (p.y+scrn.grid-1)/scrn.grid;
	q.y *= scrn.grid;
	if(q.y-p.y > scrn.grid/2)
		q.y -= scrn.grid;
	return(q);
}

Point
ident(Point p)
{
	return(p);
}

/*
	pan_it(but, tr, br, stoc, ctos, draw)

	pan rectangle tr inside br. on every new mouse pos,
	(*draw) a rect congruent to tr. stoc converts scrn coords
	to coords in tr/br; ctos converts the other way.
	it returns the last tr
*/

Rectangle
pan_it(int but, Rectangle tr, Rectangle br,
  Point (*stoc)(Point), Point (*ctos)(Point), void (*draw)(Rectangle))
{
	Rectangle rr;
	Point cent, dif, p;
	extern int moved;

	USED(ctos);
	but = (but == 3) ? 4 : but;
	cent = (*stoc)(mouse.xy);	/* was midpt(tr.origin, tr.corner);*/
	dif = sub(cent, tr.origin);
	rr.origin = add(br.origin, dif);
	rr.corner = add(br.corner, sub(cent, tr.corner));
	(*draw)(tr);			/* rip */
	moved = 0;
	while(but ? mouse.buttons & but : !mouse.buttons) {
		p = confine(dif = (*stoc)(mouse.xy), rr);
		if(!eqpt(cent, p)) {
			moved = 1;
			(*draw)(tr);
			tr = raddp(tr, sub(p, cent));
			cent = p;
			(*draw)(tr);
		}
		mouse = emouse();
	}
	(*draw)(tr);			/* and tear */
	return(tr);
}

Point
midpt(Point a, Point b)
{
	Point c;

	c.x = ((long)a.x+(long)b.x)/2;
	c.y = ((long)a.y+(long)b.y)/2;
	return(c);
}
