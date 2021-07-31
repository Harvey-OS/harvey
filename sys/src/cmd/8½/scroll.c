#include <u.h>
#include <libc.h>
#include <libg.h>
#include <frame.h>
#include <layer.h>
#include <auth.h>
#include <fcall.h>
#include "dat.h"
#include "fns.h"


Rectangle	scrpos(Rectangle, ulong, ulong, ulong);

void
scrdraw(Window *w)
{
	Rectangle r, r1, r2;
	Bitmap *b;
	long tot;
	static Bitmap *x;
	int y, h, fd;
	char buf[5*12];

	tot = w->text.n;
	r = w->scrollr;
	r.min.x += 1;	/* border between margin and bar */
	r1 = r;
	if(x==0){
		h = 1024;
		fd = open("/dev/screen", OREAD);
		if(fd > 0){
			if(read(fd, buf, sizeof buf) == sizeof buf){
				y = atoi(buf+4*12)-atoi(buf+2*12);
				if(y > 0)
					h = y;
			}
			close(fd);
		}
		x = balloc(Rect(0, 0, 32, h), w->l->ldepth);
		if(x == 0)
			berror("scroll balloc");
	}
	b = x;
	r1.min.x = 0;
	r1.max.x = Dx(r);
	r2 = scrpos(r1, w->org, w->org+w->f.nchars, tot);
	if(!eqrect(r2, w->lastbar)){
		bitblt(b, r1.min, b, r1, F);
		texture(b, inset(r1, 1), darkgrey, S);
		bitblt(b, r2.min, b, r2, 0);
		bitblt(w->l, r.min, b, r1, S);
		w->lastbar = r2;
	}
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
scrflip(Window *w, Rectangle r)
{
	if(rectclip(&r, w->scrollr))
		bitblt(w->l, r.min, w->l, r, F&~D);
}

void
scroll(Window *w, int but, Mouse *m)
{
	int in=0, oin;
	long tot;
	Rectangle scr, r, s;
	int x, y, my, oy, h;
	long p0;

	tot = w->text.n;
	s = inset(w->scrollr, 1);
	x = s.min.x+SCROLLWID/2;
	scr = scrpos(w->scrollr, w->org, w->org+w->f.nchars, tot);
	r = scr;
	y = scr.min.y;
	my = m->xy.y;
	do{
		oin = in;
		in = abs(x-m->xy.x)<=SCROLLWID/2;
		if(oin != in)
			scrflip(w, r);
		if(in){
			oy = y;
			my = m->xy.y;
			if(my < s.min.y)
				my = s.min.y;
			if(my >= s.max.y)
				my = s.max.y;
			if(!eqpt(m->xy, Pt(x, my)))
				cursorset(Pt(x, my));
			if(but == 1){
				p0 = w->org-frcharofpt(&w->f, Pt(s.max.x, my));
				y = scrpos(w->scrollr, p0, p0+w->f.nchars, tot).min.y;
			}else if(but == 3){
				p0 = w->org+frcharofpt(&w->f, Pt(s.max.x, my));
				y = scrpos(w->scrollr, p0, p0+w->f.nchars, tot).min.y;
			}else{
				y = my;
				if(y > s.max.y-2)
					y = s.max.y-2;
			}
			if(y != oy){
				scrflip(w, r);
				r = raddp(scr, Pt(0, y-scr.min.y));
				scrflip(w, r);
			}
		}
		frgetmouse();
		/* window can go away while we're asleep! */
		if(w->l == 0)
			return;
	}while(m->buttons & (1<<(but-1)));
	if(in){
		h = s.max.y-s.min.y;
		scrflip(w, r);
		if(but == 1)
			p0 = (long)(my-s.min.y)/w->f.font->height;
		else if(but == 3){
			p0 = w->org+frcharofpt(&w->f, Pt(s.max.x, my));
			if(p0 > tot)
				p0 = tot;
		}else{
			if(tot > 1024L*1024L)
				p0 = ((tot>>10)*(y-s.min.y)/h)<<10;
			else
				p0 = tot*(y-s.min.y)/h;
		}
		scrorigin(w, but, p0);
	}
}
