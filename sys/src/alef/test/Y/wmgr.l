#include <alef.h>
#include <fcall.h>
#include <graphics.h>
#include <frame.h>
#include "y.h"

intern Bitmap *rbox;
intern Bitmap *ybox;
intern Bitmap *sbox;
intern Bitmap *hbox;
intern Rectangle zeror;
intern int id;
intern Point lastm;

Cursor sweep0 = {
	{-7, -7},
	{0x03, 0xC0, 0x03, 0xC0, 0x03, 0xC0, 0x03, 0xC0,
	 0x03, 0xC0, 0x03, 0xC0, 0xFF, 0xFF, 0xFF, 0xFF,
	 0xFF, 0xFF, 0xFF, 0xFF, 0x03, 0xC0, 0x03, 0xC0,
	 0x03, 0xC0, 0x03, 0xC0, 0x03, 0xC0, 0x03, 0xC0},
	{0x00, 0x00, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80,
	 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x7F, 0xFE,
	 0x7F, 0xFE, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80,
	 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x00, 0x00}
};

Cursor boxcurs = {
	{-7, -7},
	{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	 0xFF, 0xFF, 0xF8, 0x1F, 0xF8, 0x1F, 0xF8, 0x1F,
	 0xF8, 0x1F, 0xF8, 0x1F, 0xF8, 0x1F, 0xFF, 0xFF,
	 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
	{0x00, 0x00, 0x7F, 0xFE, 0x7F, 0xFE, 0x7F, 0xFE,
	 0x70, 0x0E, 0x70, 0x0E, 0x70, 0x0E, 0x70, 0x0E,
	 0x70, 0x0E, 0x70, 0x0E, 0x70, 0x0E, 0x70, 0x0E,
	 0x7F, 0xFE, 0x7F, 0xFE, 0x7F, 0xFE, 0x00, 0x00}
};

Cursor sight = {
	{-7, -7},
	{0x1F, 0xF8, 0x3F, 0xFC, 0x7F, 0xFE, 0xFB, 0xDF,
	 0xF3, 0xCF, 0xE3, 0xC7, 0xFF, 0xFF, 0xFF, 0xFF,
	 0xFF, 0xFF, 0xFF, 0xFF, 0xE3, 0xC7, 0xF3, 0xCF,
	 0x7B, 0xDF, 0x7F, 0xFE, 0x3F, 0xFC, 0x1F, 0xF8,},
	{0x00, 0x00, 0x0F, 0xF0, 0x31, 0x8C, 0x21, 0x84,
	 0x41, 0x82, 0x41, 0x82, 0x41, 0x82, 0x7F, 0xFE,
	 0x7F, 0xFE, 0x41, 0x82, 0x41, 0x82, 0x41, 0x82,
	 0x21, 0x84, 0x31, 0x8C, 0x0F, 0xF0, 0x00, 0x00,}
};

char rboxbits[] =
{
	0xff, 0xff, 0x81, 0x11, 0x80, 0x01, 0x81, 0x11, 
	0x80, 0x01, 0x81, 0x11, 0x80, 0x01, 0xd5, 0x11, 
	0x80, 0x01, 0x80, 0x11, 0x80, 0x01, 0xd5, 0x51, 
	0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0xff, 0xff, 
};

char yboxbits[] =
{
	0xff, 0xff, 0x80, 0x01, 0xbc, 0x3d, 0x98, 0x19, 
	0x8c, 0x31, 0x86, 0x61, 0x83, 0xc1, 0x81, 0x81, 
	0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 
	0x81, 0x81, 0x83, 0xc1, 0x80, 0x01, 0xff, 0xff, 
};

char hboxbits[] =
{
	0xff, 0xff, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 
	0x81, 0x81, 0x83, 0xc1, 0x87, 0xe1, 0x87, 0xe1, 
	0x87, 0xe1, 0x83, 0xc1, 0x81, 0x81, 0x80, 0x01, 
	0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0xff, 0xff, 
};

char sboxbits[] =
{
	0xff, 0xff, 0x80, 0x01, 0x80, 0x01, 0x9d, 0xb9, 
	0x80, 0x01, 0x9b, 0x79, 0x80, 0x01, 0x9d, 0xd9, 
	0x80, 0x01, 0x81, 0x01, 0x81, 0x01, 0x87, 0xc1, 
	0x83, 0x81, 0x81, 0x01, 0x80, 0x01, 0xff, 0xff, 
};

void	init(Window*);

char *menu1[] =
{
	"New",
	"Move",
	"Reshape",
	"Delete",
	"Raise",
	"Lower",
	nil
};

char *hidelist[Hidemax];

enum
{
	New = 1,
	Move,
	Reshape,
	Delete,
	Raise,
	Lower,
};

void
winmgr()
{
	Mesg m;
	int br;
	Fcall thdr;
	Point deltam;
	Rlist *rl, *next;

	rbox = balloc(Rect(0, 0, 16, 16), 0);
	if(rbox == nil)
		error("no bitmap");
	wrbitmap(rbox, 0, 16, rboxbits);
	ybox = balloc(Rect(0, 0, 16, 16), 0);
	if(ybox == nil)
		error("no bitmap");
	wrbitmap(ybox, 0, 16, yboxbits);
	sbox = balloc(Rect(0, 0, 16, 16), 0);
	if(sbox == nil)
		error("no bitmap");
	wrbitmap(sbox, 0, 16, sboxbits);
	hbox = balloc(Rect(0, 0, 16, 16), 0);
	if(hbox == nil)
		error("no bitmap");
	wrbitmap(hbox, 0, 16, hboxbits);

	alloc menuserv;
	task menuserver(menuserv);

	mgrstate = 0;

	for(;;) {
		m = <-wmgr;
		switch(m.type) {
		case MsgMouse:
			br = m.mb&Mouse_r;
			if(sweep.doit) {
				if(br == 0) {
					execute(mgrstate);
					break;
				}
				if(mgrstate == Move) {
					deltam = sub(m.m, lastm);
					sweep.on = raddp(sweep.on, deltam);	
				}
				else {			/* Reshape */
					sweep.on.min = sweep.start;
					sweep.on.max = m.m;
					sweep.on = rcanon(sweep.on);
				}
			}
			if(br)
				bleft(&m);
			else
			if(m.mb&Mouse_m)
				dohides(&m);
			else
			if(m.mb&Mouse_l)
				wraise(&m);
			lastm = m.m;
			break;

		case MsgMenu1:
			mgrstate = m.sel;
			if(mgrstate != New)
				mgrcurs = &sight;
			else
				mgrcurs = &sweep0;
			break;

		case MsgRefresh:
			for(rl = m.refresh; rl; rl = next) {
				texture(&screen, *rl, bgrnd, S);
				next = rl->next;
				free(rl);
			}
			bflush();
			break;

		case MsgFlush:
			reply(&m.io->fcall, &thdr, nil);
			free(m.io);
			break;
		}
		cuset(nil);
	}
}

void
wraise(Mesg *m)
{
	Window *w;

	for(w = whead; w; w = w->next) {
		if(ptinrect(m->m, w->cache->r)) {
			wtrls = 1;
			setcur(w);
			tofront(w);
			refresh(w->cache->r);
			break;
		}
	}
}

void
setcur(Window *w)
{
	if(wcurr == w)
		return;

	if(wcurr != nil && wcurr->top == 0)
		texture(&screen, wcurr->cache->r, ncur, S|D);

	wcurr = w;
}

void
dohides(Mesg *m)
{
	int i;
	Window *w;

	i = 0;
	for(w = whead; w != nil && i < Hidemax; w = w->next)
		if(w->hidden)
			hidelist[i++] = w->label;
	if(i == 0)
		return;

	hidelist[i] = nil;
	m->type = MsgUnhide;
	m->menu = hidelist;
	m->rchan = whead->in;
	menuserv <-= *m;
}

void
bleft(Mesg *m)
{
	Window *w;

	if(sweep.doit)
		return;

	if(mgrstate == 0) {
		sweep.w = nil;
		m->type = MsgMenu1;
		m->menu = menu1;
		m->rchan = wmgr;
		menuserv <-= *m;
		return;
	}

	sweep.start = m->m;
	switch(mgrstate) {
	case Move:
		sweep.w = wsearch();
		if(sweep.w == nil) {
			mgrcurs = nil;
			mgrstate = 0;
			break;
		}
		sweep.doit = 1;
		sweep.on = sweep.w->cache->r;
		sweep.off = zeror;
		mgrcurs = &boxcurs;
		break;

	case Delete:
		mgrstate = 0;
		mgrcurs = nil;
		w = wsearch();
		if(w == nil)
			break;
		m->type = MsgClose;
		w->in <-= *m;
		break;

	case Reshape:
		if(sweep.w == nil) {
			sweep.w = wsearch();
			if(sweep.w == nil) {
				mgrcurs = nil;
				mgrstate = 0;
				break;
			}
			mgrcurs = &sweep0;
			break;
		}
		/* Fall through */
	case New:
		sweep.doit = 1;
		sweep.off = zeror;
		sweep.on = sweep.off;
		mgrcurs = &boxcurs;
		break;

	default:
		execute(mgrstate);
		mgrstate = 0;
		break;
	}
}

void
clntmgr(Window *w, Mesg *m)
{
	int x, mx;

	if(mgrstate != 0 || (m->mb&(Mouse_r|Mouse_m|Mouse_l)) == 0)
		return;

	x = w->titler.max.x - (3*20);
	mx = m->m.x;
	if(mx > x && mx < x+20) {
		wtrls = 1;
		w->hidden = 1;
		if(wcurr == w)
			wcurr = nil;
		windref(w);
		return;
	}
	x += 20;
	if(mx > x && mx < x+20) {
		wtrls = 1;
		w->scrollon = !w->scrollon;
		if(w->scrollon)
			clntstart(w);
		if(w->titleb)
			titlebar(w);	
		windref(w);
		return;
	}
	x += 20;
	if(mx > x) {
		mgrstate = Reshape;
		sweep.w = w;
		sweep.doit = 1;
		sweep.on = w->cache->r;
		sweep.start = sweep.on.min;
		sweep.off = zeror;
		cursorset(sweep.on.max);
		mgrcurs = &boxcurs;
		return;
	}
	if(mx < w->titler.min.x+20) {
		wtrls = 1;
		clntclose(w);
		return;
	}

	mgrstate = Move;
	sweep.w = w;
	sweep.doit = 1;
	sweep.on = w->cache->r;
	sweep.off = zeror;
	mgrcurs = &boxcurs;
	lastm = m->m;
}

Window*
wsearch()
{
	Window *w;

	for(w = whead; w; w = w->next)
		if(ptinrect(sweep.start, w->cache->r))
			return w;

	return nil;
}

int
sweepsmall()
{
	if(Dx(sweep.on) < 60)
		return 1;
	if(Dy(sweep.on) < 60)
		return 1;
	return 0;
}

void
execute(int func)
{
	Mesg m;
	Rlist *rl;
	Bitmap *c;
	Window *w;

	sweeper(Off);
	sweep.doit = 0;
	mgrcurs = nil;

	switch(func) {
	case New:
		if(sweepsmall())
			break;
		w = malloc(sizeof(Window));
		rescue {
			free(w);
			break;
		}
		strcpy(w->label, "/bin/rc");
		w->id = id++;
		w->list = wlist;
		wlist = w;

		w->screen = newbuf(Dispbuf);
		w->keybuf = newbuf(Keybbuf);

		w->cache = balloc(sweep.on, screen.ldepth);
		if(w->cache == nil)
			raise;
		w->scrollb = 1;
		w->scrollon = 1;
		w->titleb = 1;
		w->dcursor = 1;
		init(w);
		w->frame = malloc(sizeof(Frame));
		frinit(w->frame, w->win, font, w->cache);

		bflush();
		pushw(w);
		task client(w);
		setcur(w);
		break;

	case Raise:
		for(w = whead; w; w = w->next) {
			if(ptinrect(sweep.start, w->cache->r)) {
				tofront(w);
				refresh(w->cache->r);
				break;
			}
		}
		break;

	case Reshape:
		if(sweepsmall())
			break;
		w = sweep.w;
		sweep.w = nil;
		c = balloc(sweep.on, screen.ldepth);
		if(c == nil)
			break;

		rl = malloc(sizeof(Rlist));
		rl->Rectangle = sweep.on;
		rl->next = intersect(sweep.on, w->cache->r);
		if(rl->next == nil) {
			rl->next = malloc(sizeof(Rlist));
			rl->next->Rectangle = w->cache->r;
		}
		bfree(w->cache);
		w->cache = c;
		init(w);

		setcur(w);
		tofront(w);
		wcurr = w;

		m.type = MsgReshape;
		m.refresh = rl;
		w->in <-= m;
		break;

	case Move:
		w = sweep.w;
		sweep.w = nil;
		c = balloc(sweep.on, screen.ldepth);
		if(c == nil)
			break;

		rl = malloc(sizeof(Rlist));
		rl->Rectangle = sweep.on;
		if(rl->next == nil) {
			rl->next = malloc(sizeof(Rlist));
			rl->next->Rectangle = w->cache->r;
		}

		bitblt(c, c->r.min, w->cache, w->cache->r, S);
		bfree(w->cache);
		w->cache = c;
		init(w);
		frsetrects(w->frame, w->win, c);

		setcur(w);
		tofront(w);
		wcurr = w;

		w->reshape = 1;
		m.type = MsgRefresh;
		m.refresh = rl;
		w->in <-= m;
		break;
	}
	mgrstate = 0;
}

void
tofront(Window *w)
{
	Window *s, *p;

	if(w == whead)
		return;

	for(s = whead; s; s = s->next) {
		if(s == w) {
			p->out = w->out;
			p->next = w->next;
			w->out = whead->in;
			w->next = whead;
			whead = w;
			return;
		}
		p = s;
	}
}

void
todel(Window *w)
{
	Window *f, *p;

	if(w == wcurr)
		wcurr = nil;
	if(w == whead) {
		whead = w->next;
		return;
	}

	for(f = whead; f; f = f->next) {
		if(f == w)
			break;
		p = f;
	}
	p->out = w->out;
	p->next = w->next;
}

void
init(Window *w)
{
	int h;
	Point p1;
	Rectangle r;

	r = w->cache->r;
	w->win = inset(r, 1);
	border(w->cache, r, 1, F);

	if(w->titleb) {
		h = titlebar(w);
		w->win.min.y += h;
	}
	if(w->scrollb) {
		p1 = w->win.min;
		w->scroll.min = Pt(p1.x+3, p1.y+3);
		w->scroll.max = Pt(p1.x+15, r.max.y-3);
		w->win.min.x += 18;
		scrollb(w, 0, 10, 10);
	}
	w->clip = w->win;
}

int
titlebar(Window *w)
{
	int i, n, wid, fc;
	Bitmap *cache;
	Point p1, p2, pf;

	cache = w->cache;
	pf = strsize(font, w->label);
	n = 8+pf.y;
	if(n < 16)
		n = 16;
	p1 = cache->r.min;
	p2 = Pt(cache->r.max.x, cache->r.min.y);
	for(i = 0; i < n; i += 2) {
		segment(cache, p1, p2, ~0, F);
		p1.y += 2;
		p2.y += 2;
	}
	w->titler = (Rectangle){ {cache->r.min.x, cache->r.min.y}, { p2.x, p2.y}};

	wid = cache->r.min.x+((cache->r.max.x-cache->r.min.x)/2)-(pf.x/2);
	p1 = Pt(wid, cache->r.min.y+4);
	string(cache, p1, font, w->label, S);
	i = (n-16)/2;
	p1 = add(cache->r.min, Pt(4, i));
	bitblt(cache, p1, ybox, Rect(0, 0, 16, 16), S);
	p1 = Pt(cache->r.max.x - 20, cache->r.min.y+i);
	bitblt(cache, p1, rbox, Rect(0, 0, 16, 16), S);
	p1.x -= 20;

	fc = ~S;
	if(w->scrollon)
		fc = S;
	bitblt(cache, p1, sbox, Rect(0, 0, 16, 16), fc);
		
	p1.x -= 20;
	bitblt(cache, p1, hbox, Rect(0, 0, 16, 16), S);
	return n;
}

void
scrollb(Window *w, int min, int max, int size)
{
	int height;
	Bitmap *cache;
	Rectangle *s, active;

	cache = w->cache;
	s = &w->scroll;

	height = s->max.y-s->min.y-2;
	bitblt(cache, s->min, &screen, *s, F);

	active.min.x = s->min.x;
	active.max.x = s->max.x;
	if(size == 0)
		size = 1;
	active.min.y = s->min.y + (min*height)/size;
	active.max.y = s->min.y + (max*height)/size;
	if(active.max.y > s->max.y)
		active.max.y = s->max.y;

	w->active = active;

	bitblt(cache, active.min, cache, active, Zero);
	border(cache, w->scroll, 1, F);
}

void
refresh(Rectangle r)
{
	Mesg m;
	Rlist *rl;

	rl = malloc(sizeof(Rlist));
	rl->Rectangle = r;

	m.type = MsgRefresh;
	m.refresh = rl;

	/*
	 * If the dispatcher is busy queue the
	 * refresh otherwise send the message
	 */
	if(dispatch?)
		dispatch <-= m;
	else {
		rl->next = rpend;
		rpend = rl;
	}
}

void
popw(Window *w)
{
	todel(w);
	refresh(w->cache->r);
	bfree(w->cache);
	free(w);
}

void
pushw(Window *w)
{
	chan(Mesg) new;

	alloc new;
	if(whead == nil) {
		whead = w;
		w->in = new;
		w->out = wmgr;
		return;
	}
	w->out = whead->in;
	w->in = new;
	w->next = whead;
	whead = w;
}
