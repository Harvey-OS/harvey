#include <u.h>
#include <libc.h>
#include <libg.h>
#include "menu.h"
#include "term.h"
#include "proto.h"

#define		MSIZE		100

#define	xmtob(c)	(scrn.bmax.origin.x+muldiv(scrn.bd.x, c-scrn.map.origin.x, MSIZE))
#define	xbtom(c)	(scrn.map.origin.x+muldiv(MSIZE, c-scrn.bmax.origin.x, scrn.bd.x))
#define	ymtob(c)	(scrn.bmax.origin.y+muldiv(scrn.bd.y, scrn.map.corner.y-c, MSIZE))
#define	ybtom(c)	(scrn.map.corner.y-muldiv(MSIZE, c-scrn.bmax.origin.y, scrn.bd.y))

extern Bitmap screen;
extern Bitmap *grey;;

Point
pmtob(Point p)
{
	p.x = xmtob(p.x);
	p.y = ymtob(p.y);
	return(p);
}

Point pbtom(Point p)
{
	p.x = xbtom(p.x);
	p.y = ybtom(p.y);
	return(p);
}

Rectangle
rmtob(Rectangle r)
{
	int y;

	r.origin.x = xmtob(r.origin.x);
	y = ymtob(r.corner.y);
	r.corner.x = xmtob(r.corner.x);
	r.corner.y = ymtob(r.origin.y);
	r.origin.y = y;
	return(r);
}

Rectangle
rbtom(Rectangle r)
{
	int y;

	r.origin.x = xbtom(r.origin.x);
	y = ybtom(r.corner.y);
	r.corner.x = xbtom(r.corner.x);
	r.corner.y = ybtom(r.origin.y);
	r.origin.y = y;
	return(r);
}
extern int errfd;
void
drawmap(void)
{
	register Chip *c;
	Rectangle r;

	rectf(&screen, scrn.map, F_CLR);
	if(b.chips)
		for(c = b.chips; !(c->flags&EOLIST); c++){
			r = rbtom(c->br);
/*			border(&screen, r, 1, F_XOR); */
			rectf(&screen, r, /*F_XOR*/ F);
		}
	rectf(&screen, scrn.bmr, F_XOR);
}

void
panto(Point p)
{
	Rectangle r;

	p = snap(p);
	if(rinr(scrn.bmax, scrn.br))
		scrn.br.origin = scrn.bmax.origin;
	else {
		r = scrn.bmax;
		r.corner.x -= scrn.br.corner.x - scrn.br.origin.x;
		r.corner.y -= scrn.br.corner.y - scrn.br.origin.y;
		r.corner.x = Max(r.origin.x, r.corner.x);
		r.corner.y = Max(r.origin.y, r.corner.y);
		scrn.br.origin = confine(p, r);
	}
	scrn.br = rstob(scrn.sr);
	newbmax();
	rectf(&screen, scrn.bmr, F_XOR);
	scrn.bmr = rbtom(scrn.br);
	rectf(&screen, scrn.bmr, F_XOR);
	draw();
}

void
rectblk(Rectangle r)
{
	rectf(&screen, r, F_XOR);
}

void
pan(void)
{
	Rectangle r;
	int but = 1;

	cursorswitch(&blank);
	rectf(&screen, scrn.bmr, F_XOR);
	r = pan_it(but, scrn.bmr, scrn.map, ident, ident, rectblk);
	cursorswitch((Cursor *) 0);
	rectf(&screen, scrn.bmr, F_XOR);
	panto(rmtob(r).origin);
}

void
newbmax(void)
{
	scrn.bmax = rmax(scrn.br, scrn.bmaxx);
	scrn.bd = sub(scrn.bmax.corner, scrn.bmax.origin);
}
