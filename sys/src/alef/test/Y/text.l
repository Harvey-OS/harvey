#include <alef.h>
#include <fcall.h>
#include <libg.h>
#include <frame.h>
#include "y.h"

#define runemove(a, b, c)	memmove(a, b, (c)*sizeof(Rune))

Buf*
newbuf(int size)
{
	Buf *b;

	b = malloc(sizeof(Buf)+sizeof(Rune));
	b->t = malloc(size*sizeof(Rune));
	b->size = size;

	return b;
}

void
wwrite(Window *w, byte *p, int count)
{
	Buf *b;
	Frame *f;
	Rune *r, *c;
	int pend, s;

	count = cvtbufcr(w->screen, &r, p, count);

	b = w->screen;
	if(w->input+count > b->size) {
		s = count-(b->size-w->input);
		runemove(b->t, b->t+s, b->size-s);

		w->origin -= s;
		w->input -= s;
		w->wrt -= s;
		w->sp0 -= s;
		if(w->sp0 < 0)
			w->sp0 = 0;
		w->sp1 -= s;
		if(w->sp1 < 0)
			w->sp1 = 0;
	}

	pend = w->input - w->wrt;
	c = b->t+w->wrt;
	runemove(c+count, c, pend);
	runemove(c, r, count);
	free(r);

	if(w->sp0 >= w->wrt) {
		w->sp0 += count;
		w->sp1 += count;
	}

	f = w->frame;
	if(w->wrt >= w->origin)
		frinsert(f, c, c+count, w->wrt-w->origin);

	w->wrt += count;
	w->input += count;
	b->t[w->input] = 0;

	if(w->scrollon && w->origin+f->nchars < w->input) {
		s = scan(w, w->input, f->maxlines, '\n');
		clnto(w, w->input-s);
	}
	clntsel(w, w->sp0, w->sp1);
}

void
winsert(Window *w, int posn, Rune *r, int count)
{
	Buf *b;
	int s, end;

	b = w->screen;
	if(w->input+count > b->size) {
		s = count-(b->size-w->input);
		runemove(b->t, b->t+s, b->size-s);

		w->origin -= s;
		w->input -= s;
		w->wrt -= s;
		w->sp0 -= s;
		if(w->sp0 < 0)
			w->sp0 = 0;
		w->sp1 -= s;
		if(w->sp1 < 0)
			w->sp1 = 0;
	}
	if(posn > w->input)
		posn = w->input;
	end = posn+count;

	runemove(b->t+end, b->t+posn, w->input-posn);
	runemove(b->t+posn, r, count);
	w->input += count;
	if(posn < w->wrt)
		w->wrt += count;
	if(posn < w->origin)
		w->origin += count;

	b->t[w->input] = 0;
}

void
wdelete(Window *w, int posn, int count)
{
	Rune *b;
	int len, end;

	b = w->screen->t;
	len = w->input-(posn+count);
	end = posn+count;
	runemove(b+posn, b+end, len*sizeof(Rune));
	w->input -= count;
	if(end <= w->wrt)
		w->wrt -= count;
	else
	if(posn < w->wrt && end > w->wrt)
		w->wrt -= count+(end-w->wrt);

	b[w->input] = 0;
}

void
cutbuf(Rune *r, int count)
{
	if(count > Cbsize-1) {
		cut->nr = 0;
		return;
	}
	cut->nr = count;
	runemove(cut->t, r, count);
}

int
cvtbufcr(Buf *b, Rune **r, byte *buf, int n)
{
	int l;
	Rune *p;

	p = malloc((n+1)*sizeof(Rune));
	*r = p;

	if(b->np != 0) {
		memmove(buf+b->np, buf, n);
		memmove(buf, b->part, b->np);
		n += b->np;
		b->np = 0;
	}
	while(n > UTFmax) {
		l = chartorune(p, buf);
		buf += l;
		n -= l;
		if(*p != 0)
			p++;
	}
	while(fullrune(buf, n)) {
		l = chartorune(p, buf);
		buf += l;
		n -= l;
		if(*p != 0)
			p++;
	}
	if(n != 0) {
		b->np = n;
		memmove(b->part, buf, n);
	}	
	return p - *r;
}

int
iseot(Rune *r, int cnt)
{
	int c;

	while(cnt) {
		c = *r++;
		if(c == '\n' || c == Eot)
			return 1;
		cnt--;
	}
	return 0;
}

void
select(Window *w)
{
	Frame *f;
	uint p0, p1, q;
	Point pt0, pt1, qt;

	f = w->frame;

	frselectp(f, frones, F&~D);
	p0 = frcharofpt(f, w->mse.xy);
	p1 = p0;
	pt0 = frptofchar(f, p0);
	pt1 = frptofchar(f, p1);
	frselectf(f, pt0, pt1, frones, F&~D);

	for(;;){
		q = frcharofpt(f, w->mse.xy);
		if(p1 != q) {
			if(p0 == p1)
				frselectf(f, pt0, pt1, frones, F&~D);
			qt = frptofchar(f, q);
			if(p1 < q)
				frselectf(f, pt1, qt, frones, F&~D);
			else
				frselectf(f, qt, pt1, frones, F&~D);
			p1 = q;
			pt1 = qt;
			if(p0 == p1)
				frselectf(f, pt0, pt1, frones, F&~D);
		}
		if(p0 < p1) {
			f->p0 = p0;
			f->p1 = p1;
		}
		else {
			f->p0 = p1;
			f->p1 = p0;
		}
		windref(w);
		while(event(w, 1) == 0)
			;
		if((w->mse.buttons&Mouse_l) == 0)
			break;
	}
	w->sp0 = w->origin+f->p0;
	w->sp1 = w->origin+f->p1;
}

void
docut(Window *w, int save)
{
	Rune *r;
	Frame *f;
	int nchar;

	nchar = w->sp1 - w->sp0;
	if(nchar <= 0)
		return;

	f = w->frame;
	r = w->screen->t;

	if(save)
		cutbuf(r+w->sp0, nchar);

	wdelete(w, w->sp0, nchar);
	if(w->sp0 >= w->origin) {
		frdelete(f, w->sp0-w->origin, w->sp1-w->origin);
		frinsert(f, r+w->origin+f->nchars, r+w->input, f->nchars);
	}
	clntsel(w, w->sp0, w->sp0);
}

void
textsel(Window *w, int op)
{
	Frame *f;
	int nchar, wop;
	Rune eol, *r, *t;

	f = w->frame;
	t = w->screen->t;
	switch(op) {
	case Cut:
		docut(w, 1);
		break;

	case Paste:
		docut(w, 0);
		if(cut->nr == 0)
			break;
		winsert(w, w->sp0, cut->t, cut->nr);
		wop = w->sp0 - w->origin;
		if(wop >= 0)
			frinsert(f, cut->t, cut->t+cut->nr, wop);
		clntsel(w, w->sp0, w->sp0+cut->nr);
		break;

	case Snarf:
		nchar = w->sp1 - w->sp0;
		if(nchar <= 0)
			return;

		if(nchar > Cbsize-1)
			return;

		cutbuf(t+w->sp0, nchar);
		break;

	case Send:
		nchar = w->sp1 - w->sp0;
		wop = w->input-w->origin;
		check wop >= 0;

		clntactive(w);

		if(nchar > 0) {
			frinsert(f, t+w->sp0, t+w->sp1, wop);
			r = t+w->sp0;
			cutbuf(r, nchar);
			winsert(w, w->input, r, nchar);
			eol = r[nchar-1];
		}
		else {
			if(cut->nr == 0)
				return;

			frinsert(f, cut->t, cut->t+cut->nr, wop);
			r = cut->t;
			winsert(w, w->input, r, cut->nr);
			eol = r[cut->nr-1];
		}
		if(eol != '\n') {
			eol = '\n';
			frinsert(f, &eol, &eol+1, w->input-w->origin);
			winsert(w, w->input, &eol, 1);
		}
		clntsel(w, w->input, w->input);
		break;

	case Swap:
		r = malloc(cut2->nr*sizeof(Rune));
		runemove(r, cut2->t, cut2->nr);
		nchar = cut2->nr;
		memmove(cut2->t, cut->t, cut->nr);
		cut2->nr = cut->nr;
		runemove(cut->t, r, nchar);
		cut->nr = nchar;
		free(r);
	}
	windref(w);
}
