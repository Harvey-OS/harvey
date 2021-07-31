#include <u.h>
#include <libc.h>
#include <draw.h>
#include <thread.h>
#include <mouse.h>
#include <keyboard.h>
#include <frame.h>
#include <auth.h>
#include <fcall.h>
#include <plumb.h>
#include "dat.h"
#include "fns.h"

static	Point		prevmouse;
static	Window	*mousew;

void
cvttorunes(char *p, int n, Rune *r, int *nb, int *nr, int *nulls)
{
	uchar *q;
	Rune *s;
	int j, w;

	/*
	 * Always guaranteed that n bytes may be interpreted
	 * without worrying about partial runes.  This may mean
	 * reading up to UTFmax-1 more bytes than n; the caller
	 * knows this.  If n is a firm limit, the caller should
	 * set p[n] = 0.
	 */
	q = (uchar*)p;
	s = r;
	for(j=0; j<n; j+=w){
		if(*q < Runeself){
			w = 1;
			*s = *q++;
		}else{
			w = chartorune(s, (char*)q);
			q += w;
		}
		if(*s)
			s++;
		else if(nulls)
			*nulls = TRUE;
	}
	*nb = (char*)q-p;
	*nr = s-r;
}

void*
fbufalloc(void)
{
	return emalloc(BUFSIZE);
}

void
fbuffree(void *f)
{
	free(f);
}

void
error(char *s)
{
	fprint(2, "acme: %s: %r\n", s);
	remove(acmeerrorfile);
	notify(nil);
	abort();
}

Window*
errorwin(Rune *dir, int ndir, Rune **incl, int nincl)
{
	Window *w;
	Rune *r;
	int i, n;

	r = runemalloc(ndir+7);
	if(n = ndir)	/* assign = */
		runemove(r, dir, ndir);
	runemove(r+n, L"+Errors", 7);
	n += 7;
	w = lookfile(r, n);
	if(w == nil){
		w = coladd(row.col[row.ncol-1], nil, nil, -1);
		w->filemenu = FALSE;
		winsetname(w, r, n);
	}
	free(r);
	for(i=nincl; --i>=0; ){
		n = runestrlen(incl[i]);
		r = runemalloc(n);
		runemove(r, incl[i], n);
		winaddincl(w, r, n);
	}
	return w;
}

void
warning(Mntdir *md, char *s, ...)
{
	char *buf;
	Rune *r;
	int n, nb, nr, q0, owner;
	Window *w;
	Text *t;
	va_list arg;

	if(row.ncol == 0){	/* really early error */
		rowinit(&row, screen->clipr);
		rowadd(&row, nil, -1);
		rowadd(&row, nil, -1);
		if(row.ncol == 0)
			error("initializing columns in warning()");
	}
	buf = fbufalloc();
	va_start(arg, s);
	n = doprint(buf, buf+BUFSIZE, s, arg)-buf;
	va_end(arg);
	r = runemalloc(n);
	cvttorunes(buf, n, r, &nb, &nr, nil);
	fbuffree(buf);
	if(md)
		for(;;){
			w = errorwin(md->dir, md->ndir, md->incl, md->nincl);
			winlock(w, 'E');
			if(w->col != nil)
				break;
			/* window was deleted too fast */
			winunlock(w);
		}
	else
		w = errorwin(nil, 0, nil, 0);
	t = &w->body;
	owner = w->owner;
	if(owner == 0)
		w->owner = 'E';
	wincommit(w, t);
	q0 = textbsinsert(t, t->file->nc, r, nr, TRUE, &nr);
	textshow(t, q0, q0+nr);
	winsettag(t->w);
	textscrdraw(t);
	w->owner = owner;
	w->dirty = FALSE;
	if(md)
		winunlock(w);
	free(r);
}

int
runeeq(Rune *s1, uint n1, Rune *s2, uint n2)
{
	if(n1 != n2)
		return FALSE;
	return memcmp(s1, s2, n1*sizeof(Rune)) == 0;
}

int
runestrlen(Rune *s)
{
	int i;

	i = 0;
	while(*s++)
		i++;
	return i;
}

Rune*
strrune(Rune *s, Rune c)
{
	Rune c1;

	if(c == 0) {
		while(*s++)
			;
		return s-1;
	}

	while(c1 = *s++)
		if(c1 == c)
			return s-1;
	return nil;
}

uint
min(uint a, uint b)
{
	if(a < b)
		return a;
	return b;
}

uint
max(uint a, uint b)
{
	if(a > b)
		return a;
	return b;
}

char*
runetobyte(Rune *r, int n)
{
	char *s;

	if(n == 0)
		return nil;
	s = emalloc(n*UTFmax+1);
	snprint(s, n*UTFmax+1, "%.*S", n, r);
	return s;
}

Rune*
bytetorune(char *s, int *ip)
{
	Rune *r;
	int nb, nr;

	nb = strlen(s);
	r = runemalloc(nb+1);
	cvttorunes(s, nb, r, &nb, &nr, nil);
	r[nr] = '\0';
	*ip = nr;
	return r;
}

int
isalnum(Rune c)
{
	/*
	 * Hard to get absolutely right.  Use what we know about ASCII
	 * and assume anything above the Latin control characters is
	 * potentially an alphanumeric.
	 */
	if(c <= ' ')
		return FALSE;
	if(0x7F<=c && c<=0xA0)
		return FALSE;
	if(utfrune("!\"#$%&'()*+,-./:;<=>?@[\\]^`{|}~", c))
		return FALSE;
	return TRUE;
}

int
rgetc(void *v, uint n)
{
	return ((Rune*)v)[n];
}

int
tgetc(void *a, uint n)
{
	Text *t;

	t = a;
	if(n >= t->file->nc)
		return 0;
	return textreadc(t, n);
}

Rune*
skipbl(Rune *r, int n, int *np)
{
	while(n>0 && *r==' ' || *r=='\t' || *r=='\n'){
		--n;
		r++;
	}
	*np = n;
	return r;
}

Rune*
findbl(Rune *r, int n, int *np)
{
	while(n>0 && *r!=' ' && *r!='\t' && *r!='\n'){
		--n;
		r++;
	}
	*np = n;
	return r;
}

void
savemouse(Window *w)
{
	prevmouse = mouse->xy;
	mousew = w;
}

void
restoremouse(Window *w)
{
	if(mousew!=nil && mousew==w)
		moveto(mousectl, prevmouse);
	mousew = nil;
}

void
clearmouse()
{
	mousew = nil;
}

void*
emalloc(uint n)
{
	void *p;

	p = malloc(n);
	if(p == nil)
		error("malloc failed");
	memset(p, 0, n);
	return p;
}

/*
 * Heuristic city.
 */
Window*
newwindow(Text *t)
{
	Column *c;
	Window *w, *bigw, *emptyw;
	Text *emptyb;
	int i, y, el;

	if(activecol)
		c = activecol;
	else if(seltext && seltext->col)
		c = seltext->col;
	else if(t && t->col)
		c = t->col;
	else{
		if(row.ncol==0 && rowadd(&row, nil, -1)==nil)
			error("can't make column");
		c = row.col[row.ncol-1];
	}
	activecol = c;
	if(t==nil || t->w==nil || c->nw==0)
		return coladd(c, nil, nil, -1);

	/* find biggest window and biggest blank spot */
	emptyw = c->w[0];
	bigw = emptyw;
	for(i=1; i<c->nw; i++){
		w = c->w[i];
		/* use >= to choose one near bottom of screen */
		if(w->body.maxlines >= bigw->body.maxlines)
			bigw = w;
		if(w->body.maxlines-w->body.nlines >= emptyw->body.maxlines-emptyw->body.nlines)
			emptyw = w;
	}
	emptyb = &emptyw->body;
	el = emptyb->maxlines-emptyb->nlines;
	/* if empty space is big, use it */
	if(el>15 || (el>3 && el>(bigw->body.maxlines-1)/2))
		y = emptyb->r.min.y+emptyb->nlines*font->height;
	else{
		/* if this window is in column and isn't much smaller, split it */
		if(t->col==c && Dy(t->w->r)>2*Dy(bigw->r)/3)
			bigw = t->w;
		y = (bigw->r.min.y + bigw->r.max.y)/2;
	}
	w = coladd(c, nil, nil, y);
	if(w->body.maxlines < 2)
		colgrow(w->col, w, 1);
	return w;
}
