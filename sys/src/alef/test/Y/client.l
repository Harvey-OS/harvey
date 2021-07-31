#include <alef.h>
#include <fcall.h>
#include <libg.h>
#include <frame.h>
#include "y.h"

byte *menu2[] =
{
	"cut",
	"paste",
	"snarf",
	"send",
	"swap",
	nil
};

Cursor whitearrow =
{
	{0, 0},
	{0xFF, 0xE0, 0xFF, 0xE0, 0xFF, 0xC0, 0xFF, 0x00,
	 0xFF, 0x00, 0xFF, 0x80, 0xFF, 0xC0, 0xFF, 0xE0,
	 0xE7, 0xF0, 0xE3, 0xF8, 0xC1, 0xFC, 0x00, 0xFE,
	 0x00, 0x7F, 0x00, 0x3E, 0x00, 0x1C, 0x00, 0x08,},
	{0xFF, 0xE0, 0xFF, 0xE0, 0xC1, 0xC0, 0xC3, 0x00,
	 0xC3, 0x00, 0xC1, 0x80, 0xD8, 0xC0, 0xFC, 0x60,
	 0xE6, 0x30, 0xE3, 0x18, 0xC1, 0x8C, 0x00, 0xC6,
	 0x00, 0x63, 0x00, 0x36, 0x00, 0x1C, 0x00, 0x08,}
};

void
client(Window *w)
{
	proc mkclient(w);

	for(;;)
		event(w, 0);
}

int
event(Window *w, int sel)
{
	Mesg m;

	m = <-w->in;

	switch(m.type) {
	case MsgMouse:
		if(sel) {
			w->mse = m.Mouse;
			return 1;
		}
		clntmouse(w, &m);
		break;
	case MsgRefresh:
		clntref(w, &m);
		break;
	case MsgReshape:
		clntreshape(w);
		m.type = MsgRefresh;
		clntref(w, &m);
		break;
	case MsgKeybd:
		clntkey(w, &m);
		windref(w);
		break;
	case MsgIO:
		qio(w, m.io);
		break;
	case MsgClose:
		clntclose(w);
		break;
	case MsgFlush:
		if(clntflush(w, m.io) == 0)
			w->out <-= m;
		break;
	case MsgMenu1:
		textsel(w, m.sel);
		break;
	case MsgUnhide:
		if(w->hidden) {
			m.sel--;
			if(m.sel == 0) {
				w->hidden = 0;
				setcur(w);
				windref(w);
				break;
			}
		}
		w->out <-= m;
	}
	doio(w);
	return 0;
}

void
clntmouse(Window *w, Mesg *m)
{
	if(w->hidden) {
		w->out <-= *m;
		return;
	}

	if(m->buttons == 0 && w->didscroll) {
		w->didscroll = 0;
		return;
	}

	if(w->titleb && ptinrect(m->xy, w->titler)) {
		if(m->buttons != 0)
			clntmgr(w, m);
		return;
	}

	if(w != wcurr) {
		w->out <-= *m;
		return;
	}

	if(w == grab || ptinrect(m->xy, w->win)) {
		w->mse = m->Mouse;
		if(w->mouseopen) {
			w->msedelta = 1;
			if(m->buttons)
				grab = w;
		}
		else
		if(m->buttons & Mouse_l)
			select(w);
		else
		if(m->buttons & Mouse_r) {
			m->type = MsgMenu1;
			m->menu = menu2;
			m->rchan = w->in;
			menuserv <-= *m;
		}
		return;
	}

	if(m->buttons && !w->mouseopen &&
	   w->scrollb && ptinrect(m->xy, w->scroll)) {
		clntscroll(w, m);
		return;
	}
	w->out <-= *m;
}

void
qio(Window *w, Ioreq *f)
{
	switch(f->fcall.type) {
	case Tread:
		f->link = w->rdioqhd;
		w->rdioqhd = f;
		break;
	case Twrite:
		f->link = w->wrioqhd;
		w->wrioqhd = f;
		break;
	}
}

int
clntread(Window *w, Ioreq *m)
{
	switch(m->file) {
	case Qbitblt:
		bitread(w, m);
		return 1;
	case Qcons:
		return consread(w, m);
	case Qmouse:
		if(w->msedelta == 0 && w->reshape == 0)
			break;

		w->msedelta = 0;
		/* Fall through */
	case Qnbmouse:
		mouseread(w, m);
		return 1;
	case Qsnarf:
		break;
	case Qwindow:
		winread(w, m);
		return 1;
	case Qlabel:
		labio(w, m, Read);
		return 1;
	}
	return 0;
}

int
clntwrite(Window *w, Ioreq *m)
{
	switch(m->file) {
	case Qbitblt:
		bitwrite(w, m);
		windref(w);
		return 1;
	case Qcons:
		return conswrite(w, m);
	case Qctl:
		clntcons(w, m);
		return 1;
	case Qmouse:
	case Qnbmouse:
	case Qsnarf:
	case Qwindow:
		break;
	case Qlabel:
		labio(w, m, Write);
		return 1;
	}
	return 0;
}

void
doio(Window *w)
{
	Ioreq *m, **l, *next;

	l = &w->rdioqhd;
	for(m = *l; m; m = next) {
		if(clntread(w, m)) {
			next = m->link;
			*l = next;
			free(m);
		}
		else {
			l = &m->link;
			next = m->link;
		}
	}

	l = &w->wrioqhd;
	for(m = *l; m; m = next) {
		if(clntwrite(w, m)) {
			next = m->link;
			*l = m->link;
			free(m);
		}
		else {
			l = &m->link;
			next = m->link;
		}
	}
}

int
conswrite(Window *w, Ioreq *i)
{
	Fcall thdr;

	if(w->scrollon == 0 && w->frame->lastlinefull)
		return 0;
	
	wwrite(w, i->fcall.data, i->fcall.count);
	windref(w);
	thdr.count = i->fcall.count;
	reply(&i->fcall, &thdr, nil);
	return 1;
}

void
clntstart(Window *w)
{
	int s;

	s = scan(w, w->input, w->frame->maxlines, '\n');
	clnto(w, w->input-s);
	clntactive(w);
	clntsel(w, w->input, w->input);
}

int
consread(Window *w, Ioreq *i)
{
	Buf *kb;
	Rune *r;
	byte *p;
	Fcall thdr;
	int l, cnt, n;

	if(w->raw) {
		kb = w->keybuf;
		if(kb->nr == 0)
			return 0;

		n = 0;
		r = kb->t;
		p = i->buf;
		cnt = i->fcall.count;
		while(cnt) {
			if(n >= kb->nr)
				break;
			l = runetochar(p, r);
			if(cnt-l < 0)
				break;
			r++;
			n++;
			p += l;
			cnt -= l;
		}
		if(p == i->buf)
			return 0;
		memmove(kb->t, kb->t+n, kb->nr-n);
		kb->nr -= n;
	}
	else {
		if(w->hold)
			return 0;

		r = w->screen->t+w->wrt;
		if(iseot(r, w->input-w->wrt) == 0)
			return 0;

		cnt = i->fcall.count;
		p = i->buf;
		while(cnt) {
			l = runetochar(p, r);
			if(*p == '\n' || *p == Eot) {
				w->wrt++;
				if(*p != Eot)
					p += l;
				break;
			}
			if(cnt-l < 0)
				break;
			r++;
			p += l;
			cnt -= l;
			w->wrt++;
		}
	}

	thdr.data = i->buf;
	thdr.count = p - i->buf;
	reply(&i->fcall, &thdr, nil);

	return 1;
}

int
clntflush(Window *w, Ioreq *i)
{
	Fcall thdr;
	Ioreq **l, *m;

	l = &w->rdioqhd;
	for(m = *l; m; m = m->link) {
		if(m->fcall.tag == i->fcall.oldtag) {
			*l = m->link;
			if(m->file == Qcons)
				w->wrt = w->input;

			free(m);
			reply(&i->fcall, &thdr, nil);
			free(i);
			return 1;
		}
		l = &m->link;
	}

	l = &w->wrioqhd;
	for(m = *l; m; m = m->link) {
		if(m->fcall.tag == i->fcall.oldtag) {
			*l = m->link;
			free(m);
			reply(&i->fcall, &thdr, nil);
			free(i);
			return 1;
		}
		l = &m->link;
	}
	return 0;
}

void
clntclunk(Window *w, int file)
{
	switch(file) {
	case Qwindow:
		if(w->window) {
			free(w->window);
			w->window = nil;
		}
		break;
	case Qbitblt:
		w->bitopen = 0;
		w->dcursor = 1;
		break;
	case Qmouse:
	case Qnbmouse:
		w->mouseopen = 0;
		w->dcursor = 1;
		break;
	case Qctl:
		w->ctlref--;
		if(w->ctlref == 0 && w->raw)
			clntrawoff(w);
		break;
	}
}

void
clntscroll(Window *w, Mesg *m)
{
	Rune *r;
	Frame *f;
	int h, o, l;

	if(m->buttons & Mouse_m) {
		h = w->scroll.max.y - w->scroll.min.y;
		o = m->xy.y - w->scroll.min.y;
		l = (w->input*o)/h;
		l -= scan(w, l, 1, '\n');
		clnto(w, l);
		clntsel(w, w->sp0, w->sp1);
		h = w->scroll.min.x + Dx(w->scroll)/2;
		o = abs(h-m->xy.x);
		if(o > 2 && o < 50)
			cursorset(Pt(h, m->xy.y));
		windref(w);

		return;
	}

	if(w->didscroll)
		return;

	f = w->frame;
	h = w->scroll.max.y - w->scroll.min.y;
	o = m->xy.y - w->scroll.min.y;
	l = (o*f->maxlines)/h;
	if(l == 0)
		l = 1;

	if(m->buttons & Mouse_l) {
		l = scan(w, w->origin, l, '\n');
		w->origin -= l;
		r = w->screen->t+w->origin;
		frinsert(f, r, r+l, 0);

	}
	else
	if(m->buttons & Mouse_r)
		clntshow(w, l);

	w->didscroll = 1;
	clntsel(w, w->sp0, w->sp1);
	windref(w);
}

void
clntsel(Window *w, int sp0, int sp1)
{
	Frame *f;

	w->sp0 = sp0;
	w->sp1 = sp1;

	f = w->frame;
	if(w->origin+f->p0 == sp0 && w->origin+f->p1 == sp1)
		return;

	frselectp(f, frones, F&~D);
	f->p0 = sp0-w->origin;
	if(f->p0 < 0)
		f->p0 = 0;
	f->p1 = sp1-w->origin;
	if(f->p1 < 0)
		f->p1 = 0;
	frselectp(f, frones, F&~D);
}

void
clntshow(Window *w, int l)
{
	Frame *f;

	f = w->frame;

	l *= font->height;
	l = frcharofpt(f, Pt(f->r.min.x, f->r.min.y+l));
	if(w->origin+l > w->input)
		l = w->input-w->origin;

	frdelete(f, 0, l);
	w->origin += l;
	clntfill(w);
	clntsel(w, w->sp0, w->sp1);
}

void
clntfill(Window *w)
{
	Frame *f;
	Rune *s, *e;

	f = w->frame;

	s = w->screen->t;
	e = s+w->input;
	s = s+w->origin+f->nchars;
	if(s < e)
		frinsert(f, s, e, f->nchars);
}

void
clnto(Window *w, int o)
{
	Rune *r;
	Frame *f;
	int delta;

	f = w->frame;
	delta = o - w->origin;
	r = w->screen->t;

	if(delta >= 0 && delta < f->nchars)
		frdelete(f, 0, delta);
	else
	if(delta < 0 && -delta < f->nchars)
		frinsert(f, r+o, r+w->origin, 0);
	else
		frdelete(f, 0, f->nchars);
	w->origin = o;
	clntfill(w);
}

void
clntreshape(Window *w)
{
	w->reshape = 1;
	frclear(w->frame);
	frinit(w->frame, w->win, font, w->cache);

	clntfill(w);
	if(w->scrollb)
		scrollb(w, w->origin, w->origin+w->frame->nchars, w->input);
}

void
clntnote(Window *w, byte *s)
{
	write(w->notefd, s, strlen(s));
}

void
clntactive(Window *w)
{
	if(w->sp0 < w->wrt)
		return;

	while(w->origin+w->frame->nchars < w->sp0)
		scroll(w);	
}

void
clntkey(Window *w, Mesg *m) 
{
	Buf *b;
	Rune c;
	Frame *f;
	int n, posn, s;

	c = m->keyb;
	check c != 0;
	if(w->raw) {
		b = w->keybuf;
		if(b->nr < b->size)
			b->t[b->nr++] = c;
		return;
	}
	f = w->frame;

	if(w->scrollon)
		clntactive(w);

	if(w->sp0 != w->sp1) {
		posn = w->sp0;
		n = w->sp1 - w->sp0;
		cutbuf(w->screen->t+posn, n);
		wdelete(w, posn, n);
		if(w->sp0 >= w->origin)
			frdelete(f, w->sp0-w->origin, w->sp1-w->origin);
		clntsel(w, w->sp0, w->sp0);
	}

	switch(c) {
	case View:
		scroll(w);
		break;
	case BS:
		posn = w->sp0;
		if(posn == 0)
			break;
		if(w->wrt == posn)
			break;
		wdelete(w, posn-1, 1);
		if(w->sp0 >= w->origin) {
			posn -= w->origin;
			frdelete(f, posn-1, posn);
		}
		w->sp0--;
		clntsel(w, w->sp0, w->sp0);
		break;
	case CtrlU:
		posn = w->sp0;
		s = 0;
		for(n = 0; n < 2 && s == 0; n++)
			s = scan(w, posn, n+1, '\n');
		if(s == 0)
			break;

		if(posn > w->wrt && posn-s < w->wrt)
			s = posn - w->wrt;

		wdelete(w, posn-s, s);
		if(w->sp0 >= w->origin) {
			posn = w->sp0 - w->origin;
			frdelete(f, posn-s, posn);
		}
		w->sp0 -= s;
		clntsel(w, w->sp0, w->sp0);
		break;
	case CtrlW:
		posn = w->sp0;
		s = 0;
		for(n = 0; n < 2 && s == 0; n++)
			s = scan(w, posn, n+1, ' ');
		if(s == 0)
			break;

		if(posn > w->wrt && posn-s < w->wrt)
			s = posn - w->wrt;

		wdelete(w, posn-s, s);
		if(w->sp0 >= w->origin) {
			posn = w->sp0 - w->origin;
			frdelete(f, posn-s, posn);
		}
		w->sp0 -= s;
		clntsel(w, w->sp0, w->sp0);
		break;
	case DEL:
		clntnote(w, "interrupt");
		break;
	case ESC:
		w->hold = !w->hold;
		hold(w);
		break;
	default:
		winsert(w, w->sp0, &c, 1);
		if(w->sp0 >= w->origin)
			frinsert(f, &c, &c+1, w->sp0-w->origin);
		w->sp0++;
		clntsel(w, w->sp0, w->sp0);
		if(f->lastlinefull && w->scrollon)
			scroll(w);
	}
}

void
clntclose(Window *w)
{
	Rectangle r;

	if(w->closed) {
		close(w->notefd);
		free(w);
		exits(nil);
	}
	w->closed = 1;

	r = w->cache->r;

	clntnote(w, "hangup");
	todel(w);
	frclear(w->frame);
	bfree(w->cache);
	free(w->screen->t);
	free(w->screen);
	free(w->keybuf->t);
	free(w->keybuf);
	cleanio(&w->rdioqhd);
	cleanio(&w->wrioqhd);
	unalloc w->in;
	refresh(r);
}

void
mouseread(Window *w, Ioreq *i)
{
	byte *p;
	Fcall thdr;

	p = i->buf;
	p[0] = 'm';
	p[1] = w->mse.buttons;
	if(w->reshape) {
		p[1] |= 0x80;
		w->reshape = 0;
	}
	BPLONG(p+2, w->mse.xy.x);
	BPLONG(p+6, w->mse.xy.y);
	BPLONG(p+14,w->mse.msec);
	thdr.data = i->buf;
	thdr.count = 14;
	reply(&i->fcall, &thdr, nil);
}

void
scroll(Window *w)
{
	int l;
	Frame *f;

	f = w->frame;
	l = f->nlines/4;
	if(l == 0)
		l = 1;

	l *= font->height;
	l = frcharofpt(f, Pt(f->r.min.x, f->r.min.y+l));
	if(l > 0) {
		frdelete(f, 0, l);
		w->origin += l;
		clntfill(w);
	}
}

void
windref(Window *w)
{
	if(w->scrollb)
		scrollb(w, w->origin, w->origin+w->frame->nchars, w->wrt);
	refresh(w->cache->r);
}

void
clntref(Window *w, Mesg *m)
{
	int stiple;

	if(w->hidden == 0) {
		stiple = 1;
		if(wcurr == w || w->top)
			stiple = 0;
		m->refresh = update(m->refresh, w->cache, stiple);
	}

	if(m->refresh != nil)
		w->out <-= *m;
	else
		bflush();
}

int
scan(Window *w, int posn, int l, Rune delim)
{
	int count;
	Rune *r, *rb;

	count = 0;
	rb = w->screen->t;
	r = rb + posn;
	while(r > rb) {
		r--;
		if(*r == delim) {
			l--;
			if(l == 0)
				break;
		}
		count++;
	}
	check count >= 0;
	return count;
}

void
labio(Window *w, Ioreq *i, int op)
{
	int l, n;
	byte *p, *e;
	Fcall thdr;

	l = i->fcall.count;
	p = w->label+i->fcall.offset;
	e = w->label+NAMELEN-1;
	if(p > e)
		p = e;
	if(p+l > e)
		l = e - p;

	if(op == Read) {
		n = utflen(p);
		if(l > n)
			l = n;
		thdr.data = i->buf;
		memmove(i->buf, p, l);
	}
	else {
		memmove(p, i->fcall.data, l);
		w->label[l] = 0;
		if(w->titleb) {
			bitblt(w->cache, w->titler.min, w->cache, w->titler, Zero);
			titlebar(w);
			windref(w);
		}	
	}

	thdr.count = l;
	reply(&i->fcall, &thdr, nil);
}


void
clntcons(Window *w, Ioreq *i)
{
	int n;
	byte *p;
	Fcall thdr;

	p = i->fcall.data;

	if(strncmp(p, "rawon", 5) == 0 && w->raw == 0) {
		n = w->input-w->wrt;
		w->keybuf->nr = n;
		if(n != 0) {
			memmove(w->keybuf->t, w->screen->t+w->wrt, n*sizeof(Rune));
			w->wrt = w->input;
		}
		w->raw = 1;
	}
	else
	if(strncmp(p, "rawoff", 6) == 0 && w->raw)
		clntrawoff(w);
	else
	if(strncmp(p, "scrollbaroff", 12) == 0 && w->scrollb) {
		w->scrollb = 0;
		init(w);
		clntreshape(w);
	}
	else
	if(strncmp(p, "scrollbaron", 11) == 0 && w->scrollb == 0) {
		w->scrollb = 1;
		init(w);
		clntreshape(w);
	}
	else
	if(strncmp(p, "titleoff", 8) == 0 && w->titleb) {
		w->titleb = 0;
		init(w);
		clntreshape(w);
	}
	else
	if(strncmp(p, "titleon", 7) == 0 && w->titleb == 0) {
		w->titleb = 1;
		init(w);
		clntreshape(w);
	}
	else
	if(strncmp(p, "selecton", 8) == 0 && w->top == 1) {
		w->top = 0;
		init(w);
		clntreshape(w);
	}
	else
	if(strncmp(p, "selectoff", 9) == 0 && w->top == 0) {
		w->top = 1;
		init(w);
		clntreshape(w);
	}
	else
	if(strncmp(p, "holdon", 9) == 0 && w->hold == 0) {
		w->hold = 1;
		hold(w);
	}
	else
	if(strncmp(p, "holdoff", 9) == 0 && w->hold) {
		w->hold = 0;
		hold(w);
	}
	thdr.count = i->fcall.count;
	reply(&i->fcall, &thdr, nil);
}

void
hold(Window *w)
{
	if(w->hold) {
		w->dcursor = 0;
		w->Cursor = whitearrow;
		cuset(nil);
		return;
	}
	w->dcursor = 1;
	cuset(nil);
	doio(w);
}

void
clntrawoff(Window *w)
{
	winsert(w, w->wrt, w->keybuf->t, w->keybuf->nr);
	clntsel(w, w->input, w->input);
	clntfill(w);
	w->raw = 0;
}

void
mkclient(Window *w)			/* This routine leaks a stack for now */
{
	int cfd;
	byte buf[2*NAMELEN];

	rfork(RFNAMEG|RFENVG|RFNOTEG);

	sprint(buf, "/proc/%d/notepg", getpid());
	w->notefd = open(buf, OWRITE);
	if(w->notefd < 0)
		error("notefd: %r");

	rfork(RFFDG);
	close(w->notefd);
	close(mtp[1]);
	cfd = open(srv, ORDWR);	
	if(cfd < 0)
		error("srv %s: %r", srv);

	sprint(buf, "Y%d", w->id);
	if(mount(cfd, "/mnt/8½", MREPL, buf) < 0)
		error("client mount");

	if(bind("/mnt/8½", "/dev", MBEFORE) < 0)
		error("client bind");

	close(0);
	close(1);
	if(open("/dev/cons", OREAD) != 0)
		error("/dev/cons stdin: %r");

	if(open("/dev/cons", OWRITE) != 1)
		error("/dev/cons stdout: %r");

	dup(1, 2);
	execl(w->label, w->label, 0);
	error("exec /bin/rc");
}

Rlist *
update(Rlist *r, Bitmap *b, int nc)
{
	Rectangle t;
	Rlist *f, *n, **l;

	l = &r;
	for(f = r; f; f = *l) {
		t = f->Rectangle;
		if(rectclip(&t, b->r)) {
			bitblt(&screen, t.min, b, t, S);
			if(nc)
				texture(&screen, t, ncur, S|D);
			n = intersect(b->r, *f);
			if(n == nil) {
				*l = f->next;
				free(f);
				continue;
			}
			*l = n;
			while(n->next)
				n = n->next;
			n->next = f->next;
			free(f);
			l = &n->next;
			continue;
		}
		l = &f->next;
	}
	return r;
}

Rlist *
intersect(Rectangle f, Rectangle b)
{
	Rlist *new, *r;

	if(rectclip(&f, b) == 0)
		return nil;

	r = nil;
	if(f.min.y > b.min.y) {
		alloc new;
		new->min = b.min;
		new->max = Pt(b.max.x, f.min.y);
		b.min.y = f.min.y;
		new->next = r;
		r = new;
	}
	if(f.max.y < b.max.y) {
		alloc new;
		new->min = Pt(b.min.x, f.max.y);
		new->max = b.max;
		b.max.y = f.max.y;
		new->next = r;
		r = new;
	}
	if(f.min.x > b.min.x) {
		alloc new;
		new->min = b.min;
		new->max = Pt(f.min.x, b.max.y);
		b.min.x = f.min.x;
		new->next = r;
		r = new;
	}
	if(f.max.x < b.max.x) {
		alloc new;
		new->min = Pt(f.max.x, b.min.y);
		new->max = b.max;
		b.max.x = f.max.x;
		new->next = r;
		r = new;
	}
	return r;
}
