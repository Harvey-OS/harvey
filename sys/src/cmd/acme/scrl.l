#include <alef.h>
#include <libg.h>
#include <frame.h>
#include <fcall.h>
#include "dat.h"
#include "fns.h"

intern
Rectangle
scrpos(Rectangle r, uint p0, uint p1, uint tot)
{
	uint h;
	Rectangle q;

	q = inset(r, 1);
	h = q.max.y-q.min.y;
	if(tot == 0)
		return q;
	if(tot > 1024*1024){
		tot >>= 10;
		p0 >>= 10;
		p1 >>= 10;
	}
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

intern Bitmap *scrx;

void
Text.scrdraw(Text *t)
{
	Rectangle r, r1, r2;
	int y, h, fd;
	byte buf[5*12];

	if(t->w==nil || t->what!=Body)
		return;
	r = t->scrollr;
	r.min.x += 1;	/* border between margin and bar */
	r1 = r;
	if(scrx == nil){
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
		scrx = balloc(Rect(0, 0, 32, h), t->b->ldepth);
		if(scrx == nil)
			error("scroll balloc");
	}
	r1.min.x = 0;
	r1.max.x = Dx(r);
	r2 = scrpos(r1, t->org, t->org+t->nchars, t->file->nc);
	if(!eqrect(r2, t->lastsr)){
		t->lastsr = r2;
		bitblt(scrx, r1.min, scrx, r1, F);
		texture(scrx, inset(r1, 1), lightgrey, S);
		bitblt(scrx, r2.min, scrx, r2, 0);
		r2.max.y = r2.min.y;
		r2.min.y--;
		bitblt(scrx, r2.min, scrx, r2, 0xF);
		r2 = t->lastsr;
		r2.min.y = r2.max.y;
		r2.max.y++;
		bitblt(scrx, r2.min, scrx, r2, 0xF);
		bitblt(t->b, r.min, scrx, r1, S);
	}
}

void
scrsleep(uint dt)
{
	Timer	*timer;

	timer = timerstart(dt);
	bflush();
	/* only run from mouse task, so safe to use cmouse */
	alt{
	case <-(timer->c):
		timerstop(timer);
		break;
	case mouse = <-cmouse:
		task timerwaittask(timer);
		break;
	}
}

void
Text.scroll(Text *t, int but)
{
	uint p0, oldp0;
	Rectangle s;
	int x, y, my, h, first;

	s = inset(t->scrollr, 1);
	h = s.max.y-s.min.y;
	x = (s.min.x+s.max.x)/2;
	oldp0 = ~0;
	first = TRUE;
	do{
		if(mouse.xy.x<s.min.x || s.max.x<=mouse.xy.x){
			frgetmouse();
		}else{
			my = mouse.xy.y;
			if(my < s.min.y)
				my = s.min.y;
			if(my >= s.max.y)
				my = s.max.y;
			if(!eqpt(mouse.xy, Pt(x, my)))
				cursorset(Pt(x, my));
			if(but == 2){
				y = my;
				if(y > s.max.y-2)
					y = s.max.y-2;
				if(t->file->nc > 1024*1024)
					p0 = ((t->file->nc>>10)*(y-s.min.y)/h)<<10;
				else
					p0 = t->file->nc*(y-s.min.y)/h;
				if(oldp0 != p0)
					t->setorigin(p0, FALSE);
				oldp0 = p0;
				frgetmouse();
				continue;
			}
			if(but == 1)
				p0 = t->backnl(t->org, (my-s.min.y)/t->font->height);
			else
				p0 = t->org+frcharofpt(t, Pt(s.max.x, my));
			if(oldp0 != p0)
				t->setorigin(p0, TRUE);
			oldp0 = p0;
			/* debounce */
			if(first){
				bflush();
				sleep(200);
				if(?cmouse)
					mouse = <-cmouse;
				first = FALSE;
			}
			scrsleep(80);
		}
	}while(mouse.buttons & (1<<(but-1)));
	while(mouse.buttons)
		frgetmouse();
}
