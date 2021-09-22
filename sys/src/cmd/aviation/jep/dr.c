#include "jep.h"
#include <draw.h>

int
pox(int x)
{
	return 10 + x * .25;
}

int
poy(int y)
{
	return y * .25;
}

int
por(int r)
{
	return r * .25;
}

Point
coo(int x, int y)
{
	Point p;

	p.x = screen->r.min.x + pox(x);
	p.y = screen->r.min.y + poy(y);
	return p;
}

void
ereshaped(int new)
{
	Node *n;
	int i, x, y;
	char ord[10];

	if(new && getwindow(display, Refmesg) < 0) {
		print("can't reattach to window\n");
		exits("reshap");
	}

	for(i=0; i<nelem(tname); i++)
		if(type && tname[i])
			if(strcmp(type, tname[i]) == 0)
				break;
	for(n=root; n; n = n->link) {
		if(n->type == i)
			n->here = 1;
		if(n->ord >= herel && n->ord <= hereh)
			n->here = 1;
		if(pxflag && (n->p.paramf&(1<<Px)) && n->p.param[Px] == pxflag)
			n->here = 1;
	}

	for(n=root; n; n=n->link) {
		if(!n->here)
			continue;
		x = 0;
		y = 0;
		switch(n->type) {
		default:
			print("draw type %d\n", n->type);
			break;
		case Tgroup1:
		case Tgroup2:
		case Tgroup3:
			break;

		case Tstring:
			if(!sflag)
				break;
			string(screen,
				coo(n->s.x, n->s.y),
				display->black,
				ZP, font, n->s.str);
			break;
		case Tvect1:
		case Tvect2:
		case Tvect3:
			x = n->v.vec[0].x;
			y = n->v.vec[0].y;
			for(i=0; i<n->v.count-1; i++)
				line(screen,
					coo(n->v.vec[i].x, n->v.vec[i].y),
					coo(n->v.vec[i+1].x, n->v.vec[i+1].y),
					Enddisc, Enddisc, 0,
					display->black, ZP);
			if(n->type == Tvect2)
				line(screen,
					coo(n->v.vec[i].x, n->v.vec[i].y),
					coo(n->v.vec[0].x, n->v.vec[0].y),
					Enddisc, Enddisc, 0,
					display->black, ZP);
			break;
		case Tpoint:
			x = n->l.vec[0].x;
			y = n->l.vec[0].y;
			line(screen,
				coo(n->l.vec[0].x, n->l.vec[0].y),
				coo(n->l.vec[0].x, n->l.vec[0].y),
				Enddisc, Enddisc, 0,
				display->black, ZP);
			break;
		case Tline1:
		case Tline2:
		case Tline3:
		case Tline4:
			x = n->l.vec[0].x;
			y = n->l.vec[0].y;
			line(screen,
				coo(n->l.vec[0].x, n->l.vec[0].y),
				coo(n->l.vec[1].x, n->l.vec[1].y),
				Enddisc, Enddisc, 0,
				display->black, ZP);
			break;
		case Tarc:
			x = n->c.x;
			y = n->c.y;
if(0)
			arc(screen,
				coo(n->c.x, n->c.y),
				por(n->c.r),
				por(n->c.r), 0,
				display->black, ZP,
				n->c.a1, n->c.a2-n->c.a1);
			break;
		case Tcirc1:
		case Tcirc2:
		case Tcirc3:
			x = n->c.x;
			y = n->c.y;
			ellipse(screen,
				coo(n->c.x, n->c.y),
				por(n->c.r),
				por(n->c.r), 0,
				display->black, ZP);
			break;
		}
		if(herel && (x || y)) {
			sprint(ord, "%d", n->ord);
			string(screen, coo(x, y), display->black, ZP, font, ord);
		}
	}
	flushimage(display, 1);
}

void
drnode(uchar*)
{
	if(initdraw(0, 0, "jepview") < 0) {
		print("initdraw %r\n");
		exits("initdraw");
	}
	ereshaped(0);
	
	for(;;) sleep(1);
}
