#include <u.h>
#include <libc.h>
#include <libg.h>
#include <frame.h>
#include "dat.h"
#include "fns.h"


Rectangle	scrpos(Rectangle, ulong, ulong, ulong);

void
scrdraw(Text *t)
{
	Rectangle r, r1, r2;
	Bitmap *b;
	long tot;
	static Bitmap *x;

	tot = t->n;
	r = t->scrollr;
	r.min.x += 1;	/* border between margin and bar */
	r1 = r;
	if(x==0){
		x = balloc(Rect(0, 0, 32, 1024), screen.ldepth);
		if(x == 0)
			berror("scrdraw balloc");
	}
	b = x;
	r1.min.x = 0;
	r1.max.x = Dx(r);
	bitblt(b, r1.min, b, r1, F);
	texture(b, inset(r1, 1), darkgrey, S);
	r2 = scrpos(r1, t->org, t->org+t->nchars, tot);
	bitblt(b, r2.min, b, r2, 0);
	bitblt(&screen, r.min, b, r1, S);
}

Rectangle
scrpos(Rectangle r, ulong p0, ulong p1, ulong tot)
{
	long h;
	Rectangle q;

	q = inset(r, 1);
	h = q.max.y-q.min.y;
	if(tot == 0)
		return q;
	if(tot > 1024L*1024L)
		tot >>= 10, p0 >>= 10, p1 >>= 10;
	if(p0 > 0)
		q.min.y += h*p0/tot;
	if(p1 < tot)
		q.max.y -= h*(tot-p1)/tot;
	if(q.max.y < q.min.y+2){
		if(q.min.y+2 <= r.max.y)
			q.max.y = q.min.y+2;
		else
			q.min.y = q.max.y-2;
	}
	return q;
}

void
scrflip(Text *t, Rectangle r)
{
	if(rectclip(&r, t->scrollr))
		bitblt(&screen, r.min, &screen, r, F&~D);
}

void
scroll(Text *t, int but)
{
	int in=0, oin;
	long tot;
	Rectangle scr, r, s;
	int x, y, my, oy, h;
	long p0;

	tot = t->n;
	s = inset(t->scrollr, 1);
	x = s.min.x+SCROLLWID/2;
	scr = scrpos(t->scrollr, t->org, t->org+t->nchars, tot);
	r = scr;
	y = scr.min.y;
	my = mouse.xy.y;
	do{
		oin = in;
		in = abs(x-mouse.xy.x)<=SCROLLWID/2;
		if(oin != in)
			scrflip(t, r);
		if(in){
			oy = y;
			my = mouse.xy.y;
			if(my < s.min.y)
				my = s.min.y;
			if(my >= s.max.y)
				my = s.max.y;
			if(!eqpt(mouse.xy, Pt(x, my)))
				cursorset(Pt(x, my));
			if(but == 1){
				p0 = t->org-frcharofpt(t, Pt(s.max.x, my));
				y = scrpos(t->scrollr, p0, p0+t->nchars, tot).min.y;
			}else if(but == 3){
				p0 = t->org+frcharofpt(t, Pt(s.max.x, my));
				y = scrpos(t->scrollr, p0, p0+t->nchars, tot).min.y;
			}else{
				y = my;
				if(y > s.max.y-2)
					y = s.max.y-2;
			}
			if(y != oy){
				scrflip(t, r);
				r = raddp(scr, Pt(0, y-scr.min.y));
				scrflip(t, r);
			}
		}
		frgetmouse();
	}while(mouse.buttons & (1<<(but-1)));
	if(in){
		h = s.max.y-s.min.y;
		scrflip(t, r);
		if(but == 1)
			p0 = (long)(my-s.min.y)/t->font->height;
		else if(but == 3){
			p0 = t->org+frcharofpt(t, Pt(s.max.x, my));
			if(p0 > tot)
				p0 = tot;
		}else{
			if(tot > 1024L*1024L)
				p0 = ((tot>>10)*(y-s.min.y)/h)<<10;
			else
				p0 = tot*(y-s.min.y)/h;
		}
		scrorigin(t, but, p0);
	}
}
