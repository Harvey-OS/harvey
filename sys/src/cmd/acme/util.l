#include <alef.h>
#include <libg.h>
#include <frame.h>
#include <fcall.h>
#include "dat.h"
#include "fns.h"

aggr Fbuf{
	QLock;
	void *free;
};

intern	Fbuf		fbuf;
intern	Point		prevmouse;
intern	Window	*mousew;

void
cvttorunes(byte *p, int n, Rune *r, int *nb, int *nr, int *nulls)
{
	byte *q;
	Rune *s;
	int j, w;

	/*
	 * Always guaranteed that n bytes may be interpreted
	 * without worrying about partial runes.  This may mean
	 * reading up to UTFmax-1 more bytes than n; the caller
	 * knows this.  If n is a firm limit, the caller should
	 * set p[n] = 0.
	 */
	q = p;
	s = r;
	for(j=0; j<n; j+=w){
		if(*q < Runeself){
			w = 1;
			*s = *q++;
		}else{
			w = chartorune(s, q);
			q += w;
		}
		if(*s)
			s++;
		else if(nulls)
				*nulls = TRUE;
	}
	*nb = q-p;
	*nr = s-r;
}

void*
fbufalloc()
{
	int i;
	void *f;
	byte *g;

	fbuf.lock();
	if(fbuf.free == nil){
		/* malloc rounds up to 2^n; avoid breakage here */
		g = malloc(7 * (BUFSIZE+sizeof(uint)));
		for(i=0; i<7; i++){
			((void**)g)[0] = fbuf.free;
			fbuf.free = g;
			g += BUFSIZE+sizeof(uint);
		}
	}
	f = fbuf.free;
	fbuf.free = ((void**)f)[0];
	fbuf.unlock();
	return f;
}

void
fbuffree(void *f)
{
	fbuf.lock();
	((void**)f)[0] = fbuf.free;
	fbuf.free = f;
	fbuf.unlock();
}

void
error(byte *s)
{
	fprint(2, "acme: %s: %r\n", s);
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
	runemove(r+n, $"+Errors", 7);
	n += 7;
	w = lookfile(r, n);
	if(w == nil){
		w = row.col[row.ncol-1]->add(nil, nil, -1);
		w->filemenu = FALSE;
		w->setname(r, n);
	}
	free(r);
	for(i=nincl; --i>=0; ){
		n = runestrlen(incl[i]);
		r = runemalloc(n);
		runemove(r, incl[i], n);
		w->addincl(r, n);
	}
	return w;
}

void
warning(Mntdir *md, byte *s, ...)
{
	byte *buf;
	Rune *r;
	int n, nb, nr, q0, owner;
	Window *w;
	Text *t;

	if(row.ncol == 0){	/* really early error */
		row.init(screen.clipr);
		row.add(nil, -1);
		row.add(nil, -1);
		if(row.ncol == 0)
			error("initializing columns in warning()");
	}
	buf = fbufalloc();
	n = doprint(buf, buf+BUFSIZE+1, s, ...)-buf;
	r = runemalloc(n);
	cvttorunes(buf, n, r, &nb, &nr, nil);
	fbuffree(buf);
	if(md){
		w = errorwin(md->dir, md->ndir, md->incl, md->nincl);
		w->lock('E');
	}else
		w = errorwin(nil, 0, nil, 0);
	t = &w->body;
	owner = w->owner;
	if(owner == 0)
		w->owner = 'E';
	w->commit(t);
	(q0, nr) = t->bsinsert(t->file->nc, r, nr, TRUE);
	t->show(q0, q0+nr);
	t->w->settag();
	t->scrdraw();
	w->owner = owner;
	w->dirty = FALSE;
	if(md)
		w->unlock();
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

byte*
runetobyte(Rune *r, int n)
{
	byte *s;

	s = malloc(n*UTFmax+1);
	snprint(s, n*UTFmax+1, "%.*S", n, r);
	return s;
}

(Rune*, int)
bytetorune(byte *s)
{
	Rune *r;
	int nb, nr;

	nb = strlen(s);
	r = runemalloc(nb);
	cvttorunes(s, nb, r, &nb, &nr, nil);
	return (r, nr);
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
rgetc(Rune *r, uint n)
{
	return r[n];
}

int
tgetc(Text *t, uint n)
{
	if(n >= t->file->nc)
		return 0;
	return t->readc(n);
}

(Rune*, int)
skipbl(Rune *r, int n)
{
	while(n>0 && *r==' ' || *r=='\t' || *r=='\n'){
		--n;
		r++;
	}
	return (r, n);
}

(Rune*, int)
findbl(Rune *r, int n)
{
	while(n>0 && *r!=' ' && *r!='\t' && *r!='\n'){
		--n;
		r++;
	}
	return (r, n);
}

void
savemouse(Window *w)
{
	prevmouse = mouse.xy;
	mousew = w;
}

void
restoremouse(Window *w)
{
	if(mousew!=nil && mousew==w)
		cursorset(prevmouse);
	mousew = nil;
}

void
clearmouse()
{
	mousew = nil;
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
		if(row.ncol==0 && row.add(nil, -1)==nil)
			error("can't make column");
		c = row.col[row.ncol-1];
	}
	activecol = c;
	if(t==nil || t->w==nil || c->nw==0)
		return c->add(nil, nil, -1);

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
	w = c->add(nil, nil, y);
	if(w->body.maxlines < 2)
		w->col->grow(w, 1);
	return w;
}

void
setenv(byte *name, byte *val)
{
	byte buf[128];
	int fd;

	sprint(buf, "/env/%s", name);
	fd = create(buf, OWRITE, 0666);
	if(fd >= 0){
		write(fd, val, strlen(val));
		close(fd);
	}
}
