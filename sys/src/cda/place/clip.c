#include <u.h>
#include <libc.h>
#include <libg.h>
#include "menu.h"
#include "term.h"

extern Bitmap *grey;;
static Rectangle cliprec;

int clipline(Rectangle, Point *, Point *);
void
Clip(Rectangle r)
{
	cliprec = r;
}

void
Cbitblt(Bitmap *db, Point dp, Bitmap *sb, Rectangle sr, Fcode fc)
{
	Rectangle r;

	r = raddp(sr, sub(dp, sr.origin));
	if(rectclip(&r, cliprec))
	{
		r = rsubp(r, sub(dp, sr.origin));
		dp = add(dp, sub(r.origin, sr.origin));
		bitblt(db, dp, sb, r,  fc);
	}
}

void
Crectf(Bitmap *bp, Rectangle r, Fcode fc)
{
	if(rectclip(&r, cliprec))
		rectf(bp, r, fc);
}

Point
Cstring(Bitmap *bp, Point p, Font *f, char *s, Fcode fc)
{
/*	register c;
	int full = (fc == F_STORE);
	Rectangle r;
	register Fontchar *i;

	if (full) {
		r.origin.y = 0;
		r.corner.y = f->height;
	}
	for (; c = *s++; p.x += i->width) {
		i = f->info + c;
		if (!full) {
			r.origin.y = i->top;
			r.corner.y = i->bottom;
		}
		r.origin.x = i->x;
		r.corner.x = (i+1)->x;
		Cbitblt(bp,Pt(p.x+
			((i->left & 0x80) ? i->left | 0xffffff00 : i->left),
			p.y+r.origin.y), f->bits, r, fc);
	}
	return(p); */
return(string(bp, p, f, s, fc));
}

void
Ctexture(Bitmap *bp, Rectangle r, Bitmap *t, Fcode fc)
{
	if(rectclip(&r, cliprec))
		texture(bp, r, t, fc);
}

void
Csegment(Bitmap *bp, Point p1, Point p2, Fcode fc)
{
	if(clipline(cliprec, &p1, &p2)) {
		segment(bp, p1, p2, 0xf, fc);
	}
}


#define	code(p)	((p->x<xl?1:p->x>xr?2:0)|(p->y<yt?4:p->y>yb?8:0))
#define	xl r.min.x
#define yt r.min.y
#define	xr r.max.x
#define yb r.max.y
#define	x0 p0->x
#define	x1 p1->x
#define	y0 p0->y
#define	y1 p1->y

int
clipline(Rectangle r, Point *p0, Point *p1)
{
	int c0, c1, c;
	Point t;

	c0 = code(p0);
	c1 = code(p1);
	while(c0|c1){
		if(c0&c1)
			return 0;
		c = c0? c0 : c1;
		if(c&1)
			t.y=y0+(y1-y0)*(xl-x0)/(x1-x0), t.x=xl;
		else if(c&2)
			t.y=y0+(y1-y0)*(xr-x0)/(x1-x0), t.x=xr;
		else if(c&4)
			t.x=x0+(x1-x0)*(yt-y0)/(y1-y0), t.y=yt;
		else
			t.x=x0+(x1-x0)*(yb-y0)/(y1-y0), t.y=yb;
		if(c==c0)
			*p0=t, c0=code(p0);
		else
			*p1=t, c1=code(p1);
	}
	return 1;
}
