#include <alef.h>
#include <libg.h>
#include <frame.h>
#include <fcall.h>
#include "dat.h"
#include "fns.h"

void
Column.init(Column *c, Rectangle r)
{
	Rectangle r1;
	Text *t;

	bitblt(&screen, r.min, &screen, r, 0);
	c->r = r;
	c->w = nil;
	c->nw = 0;
	t = &c->tag;
	t->w = nil;
	t->col = c;
	r1 = r;
	r1.max.y = r1.min.y + font->height;
	t->init(.File.addtext(t), r1, &reffont);
	t->what = Columntag;
	r1.min.y = r1.max.y;
	r1.max.y += Border;
	bitblt(&screen, r1.min, &screen, r1, 0xF);
	t->insert(0, $"New Cut Paste Snarf Sort Zerox Delcol ", 38, TRUE);
	t->setselect(t->file->nc, t->file->nc);
	bitblt(&screen, t->scrollr.min, colbutton, colbutton->r, S);
	c->safe = TRUE;
}

Window*
Column.add(Column *c, Window *w, Window *clone, int y)
{
	Rectangle r, r1;
	Window *v;
	int i, t;

	v = nil;
	r = c->r;
	r.min.y = c->tag.r.max.y+Border;
	if(y<r.min.y && c->nw>0){	/* steal half of last window by default */
		v = c->w[c->nw-1];
		y = v->body.r.min.y+Dy(v->body.r)/2;
	}
	/* look for window we'll land on */
	for(i=0; i<c->nw; i++){
		v = c->w[i];
		if(y < v->r.max.y)
			break;
	}
	if(c->nw > 0){
		if(i < c->nw)
			i++;	/* new window will go after v */
		/*
		 * if v's too small, grow it first.
		 */
		if(!c->safe || v->body.maxlines<=3){
			c->grow(v, 1);
			y = v->body.r.min.y+Dy(v->body.r)/2;
		}
		r = v->r;
		if(i == c->nw)
			t = c->r.max.y;
		else
			t = c->w[i]->r.min.y-Border;
		r.max.y = t;
		bitblt(&screen, r.min, &screen, r, 0);
		r1 = r;
		y = min(y, t-(v->tag.font->height+v->body.font->height+Border+1));
		r1.max.y = min(y, v->body.r.min.y+v->body.nlines*v->body.font->height);
		r1.min.y = v->reshape(r1, FALSE);
		r1.max.y = r1.min.y+Border;
		bitblt(&screen, r1.min, &screen, r1, 0xF);
		r.min.y = r1.max.y;
	}
	if(w == nil){
		w = malloc(sizeof(Window));
		w->init(clone, r);
	}else
		w->reshape(r, FALSE);
	w->tag.col = c;
	w->tag.row = c->row;
	w->body.col = c;
	w->body.row = c->row;
	w->col = c;
	c->w = realloc(c->w, (c->nw+1)*sizeof(Window*));
	memmove(c->w+i+1, c->w+i, (c->nw-i)*sizeof(Window*));
	c->nw++;
	c->w[i] = w;
	savemouse(w);
	/* near but not on the button */
	cursorset(add(w->tag.scrollr.max, Pt(3, 3)));
	barttext = &w->body;
	c->safe = TRUE;
	return w;
}

void
Column.close(Column *c, Window *w, int dofree)
{
	Rectangle r;
	int i;

	/* w is locked */
	if(!c->safe)
		c->grow(w, 1);
	for(i=0; i<c->nw; i++)
		if(c->w[i] == w)
			goto Found;
	error("can't find window");
  Found:
	r = w->r;
	w->tag.col = nil;
	w->body.col = nil;
	w->col = nil;
	restoremouse(w);
	if(dofree){
		w->delete();
		w->close();
	}
	memmove(c->w+i, c->w+i+1, (c->nw-i)*sizeof(Window*));
	c->nw--;
	c->w = realloc(c->w, c->nw*sizeof(Window*));
	if(c->nw == 0){
		bitblt(&screen, r.min, &screen, r, 0);
		return;
	}
	if(i == c->nw){		/* extend last window down */
		w = c->w[i-1];
		r.min.y = w->r.min.y;
		r.max.y = c->r.max.y;
	}else{			/* extend next window up */
		w = c->w[i];
		r.max.y = w->r.max.y;
	}
	bitblt(&screen, r.min, &screen, r, 0);
	if(c->safe)
		w->reshape(r, FALSE);
}

void
Column.closeall(Column *c)
{
	int i;
	Window *w;

	if(c == activecol)
		activecol = nil;
	c->tag.close();
	for(i=0; i<c->nw; i++){
		w = c->w[i];
		w->close();
	}
	c->nw = 0;
	free(c->w);
	free(c);
	clearmouse();
}

void
Column.mousebut(Column *c)
{
	cursorset(div(add(c->tag.scrollr.min, c->tag.scrollr.max), 2));
}

void
Column.reshape(Column *c, Rectangle r)
{
	int i;
	Rectangle r1, r2;
	Window *w;

	clearmouse();
	r1 = r;
	r1.max.y = r1.min.y + c->tag.font->height;
	c->tag.reshape(r1);
	bitblt(&screen, c->tag.scrollr.min, colbutton, colbutton->r, S);
	r1.min.y = r1.max.y;
	r1.max.y += Border;
	bitblt(&screen, r1.min, &screen, r1, 0xF);
	r1.max.y = r.max.y;
	for(i=0; i<c->nw; i++){
		w = c->w[i];
		w->maxlines = 0;
		if(i == c->nw-1)
			r1.max.y = r.max.y;
		else
			r1.max.y = r1.min.y+(Dy(w->r)+Border)*Dy(r)/Dy(c->r);
		r2 = r1;
		r2.max.y = r2.min.y+Border;
		bitblt(&screen, r2.min, &screen, r2, 0xF);
		r1.min.y = r2.max.y;
		r1.min.y = w->reshape(r1, FALSE);
	}
	c->r = r;
}

intern
int
colcmp(void *a, void *b)
{
	Rune *r1, *r2;
	int i, nr1, nr2;

	r1 = (*(Window**)a)->body.file->name;
	nr1 = (*(Window**)a)->body.file->nname;
	r2 = (*(Window**)b)->body.file->name;
	nr2 = (*(Window**)b)->body.file->nname;
	for(i=0; i<nr1 && i<nr2; i++){
		if(*r1 != *r2)
			return *r1-*r2;
		r1++;
		r2++;
	}
	return nr1-nr2;
}

void
Column.sort(Column *c)
{
	int i, y;
	Rectangle r, r1, *rp;
	Window **wp, *w;

	if(c->nw == 0)
		return;
	clearmouse();
	rp = malloc(c->nw*sizeof(Rectangle));
	wp = malloc(c->nw*sizeof(Window*));
	memmove(wp, c->w, c->nw*sizeof(Window*));
	qsort(wp, c->nw, sizeof(Window*), colcmp);
	for(i=0; i<c->nw; i++)
		rp[i] = wp[i]->r;
	r = c->r;
	r.min.y = c->tag.r.max.y;
	bitblt(&screen, r.min, &screen, r, 0);
	y = r.min.y;
	for(i=0; i<c->nw; i++){
		w = wp[i];
		r.min.y = y;
		if(i == c->nw-1)
			r.max.y = c->r.max.y;
		else
			r.max.y = r.min.y+Dy(w->r)+Border;
		r1 = r;
		r1.max.y = r1.min.y+Border;
		bitblt(&screen, r1.min, &screen, r1, 0xF);
		r.min.y = r1.max.y;
		y = w->reshape(r, FALSE);
	}
	free(rp);
	free(c->w);
	c->w = wp;
}

void
Column.grow(Column *c, Window *w, int but)
{
	Rectangle r, cr;
	int i, j, k, l, y1, y2, *nl, *ny, tot, nnl, onl, dnl, h;
	Window *v;

	for(i=0; i<c->nw; i++)
		if(c->w[i] == w)
			goto Found;
	error("can't find window");

  Found:
	cr = c->r;
	if(but < 0){	/* make sure window fills its own space properly */
		r = w->r;
		if(i == c->nw-1)
			r.max.y = cr.max.y;
		else
			r.max.y = c->w[i+1]->r.min.y;
		w->reshape(r, FALSE);
		return;
	}
	cr.min.y = c->w[0]->r.min.y;
	if(but == 3){	/* full size */
		if(i != 0){
			v = c->w[0];
			c->w[0] = w;
			c->w[i] = v;
		}
		bitblt(&screen, cr.min, &screen, cr, 0);
		w->reshape(cr, FALSE);
		for(i=1; i<c->nw; i++)
			c->w[i]->body.maxlines = 0;
		c->safe = FALSE;
		return;
	}
	/* store old #lines for each window */
	onl = w->body.maxlines;
	nl = malloc(c->nw * sizeof(int));
	ny = malloc(c->nw * sizeof(int));
	tot = 0;
	for(j=0; j<c->nw; j++){
		l = c->w[j]->body.maxlines;
		nl[j] = l;
		tot += l;
	}
	/* approximate new #lines for this window */
	if(but == 2){	/* as big as can be */
		memset(nl, 0, c->nw * sizeof(int));
		nl[j] = tot;
		goto Pack;
	}
	nnl = min(onl + max(min(5, w->maxlines), onl/2), tot);
	if(nnl < w->maxlines)
		nnl = (w->maxlines+nnl)/2;
	if(nnl == 0)
		nnl = 2;
	dnl = nnl - onl;
	/* compute new #lines for each window */
	for(k=1; k<c->nw; k++){
		/* prune from later window */
		j = i+k;
		if(j<c->nw && nl[j]){
			l = min(dnl, max(1, nl[j]/2));
			nl[j] -= l;
			nl[i] += l;
			dnl -= l;
		}
		/* prune from earlier window */
		j = i-k;
		if(j>=0 && nl[j]){
			l = min(dnl, max(1, nl[j]/2));
			nl[j] -= l;
			nl[i] += l;
			dnl -= l;
		}
	}
    Pack:
	/* pack everyone above */
	y1 = cr.min.y;
	for(j=0; j<i; j++){
		v = c->w[j];
		r = v->r;
		r.min.y = y1;
		r.max.y = y1+Dy(v->tag.all);
		if(nl[j])
			r.max.y += 1 + nl[j]*v->body.font->height;
		if(!c->safe || !eqrect(v->r, r)){
			bitblt(&screen, r.min, &screen, r, 0);
			v->reshape(r, c->safe);
		}
		r.min.y = v->r.max.y;
		r.max.y += Border;
		bitblt(&screen, r.min, &screen, r, 0xF);
		y1 = r.max.y;
	}
	/* scan to see new size of everyone below */
	y2 = c->r.max.y;
	for(j=c->nw-1; j>i; j--){
		v = c->w[j];
		r = v->r;
		r.min.y = y2-Dy(v->tag.all);
		if(nl[j])
			r.min.y -= 1 + nl[j]*v->body.font->height;
		r.min.y -= Border;
		ny[j] = r.min.y;
		y2 = r.min.y;
	}
	/* compute new size of window */
	r = w->r;
	r.min.y = y1;
	r.max.y = r.min.y+Dy(w->tag.all);
	h = w->body.font->height;
	if(y2-r.max.y >= 1+h+Border){
		r.max.y += 1;
		r.max.y += h*((y2-r.max.y)/h);
	}
	/* draw window */
	if(!c->safe || !eqrect(w->r, r)){
		bitblt(&screen, r.min, &screen, r, 0);
		w->reshape(r, c->safe);
	}
	if(i < c->nw-1){
		r.min.y = r.max.y;
		r.max.y += Border;
		bitblt(&screen, r.min, &screen, r, 0xF);
		for(j=i+1; j<c->nw; j++)
			ny[j] -= (y2-r.max.y);
	}
	/* pack everyone below */
	y1 = r.max.y;
	for(j=i+1; j<c->nw; j++){
		v = c->w[j];
		r = v->r;
		r.min.y = y1;
		r.max.y = y1+Dy(v->tag.all);
		if(nl[j])
			r.max.y += 1 + nl[j]*v->body.font->height;
		if(!c->safe || !eqrect(v->r, r)){
			bitblt(&screen, r.min, &screen, r, 0);
			v->reshape(r, c->safe);
		}
		if(j < c->nw-1){	/* no border on last window */
			r.min.y = v->r.max.y;
			r.max.y += Border;
			bitblt(&screen, r.min, &screen, r, 0xF);
		}
		y1 = r.max.y;
	}
	r = w->r;
	r.min.y = y1;
	r.max.y = c->r.max.y;
	bitblt(&screen, r.min, &screen, r, 0);
	free(nl);
	free(ny);
	c->safe = TRUE;
	w->mousebut();
}

void
Column.dragwin(Column *c, Window *w, int but)
{
	Rectangle r;
	int i, b;
	Point p, op;
	Window *v;
	Column *nc;

	clearmouse();
	cursorswitch(&boxcursor);
	b = mouse.buttons;
	op = mouse.xy;
	while(mouse.buttons == b)
		frgetmouse();
	cursorswitch(nil);
	if(mouse.buttons){
		while(mouse.buttons)
			frgetmouse();
		return;
	}

	for(i=0; i<c->nw; i++)
		if(c->w[i] == w)
			goto Found;
	error("can't find window");

  Found:
	p = mouse.xy;
	if(abs(p.x-op.x)<5 && abs(p.y-op.y)<5){
		c->grow(w, but);
		w->mousebut();
		return;
	}
	/* is it a flick to the right? */
	if(abs(p.y-op.y)<10 && p.x>op.x+30)
		p.x += Dx(w->r);	/* yes: toss to next column */
	nc = c->row->whichcol(p);
	if(nc!=nil && nc!=c){
		c->close(w, FALSE);
		nc->add(w, nil, p.y);
		w->mousebut();
		return;
	}
	if(i==0 && c->nw==1)
		return;			/* can't do it */
	if((i>0 && p.y<c->w[i-1]->r.min.y) || (i<c->nw-1 && p.y>w->r.max.y)
	|| (i==0 && p.y>w->r.max.y)){
		/* shuffle */
		c->close(w, FALSE);
		c->add(w, nil, p.y);
		w->mousebut();
		return;
	}
	if(i == 0)
		return;
	v = c->w[i-1];
	if(p.y < v->tag.all.max.y)
		p.y = v->tag.all.max.y;
	if(p.y > w->r.max.y-Dy(w->tag.all)-Border)
		p.y = w->r.max.y-Dy(w->tag.all)-Border;
	r = v->r;
	r.max.y = p.y;
	if(r.max.y > v->body.r.min.y){
		r.max.y -= (r.max.y-v->body.r.min.y)%v->body.font->height;
		if(v->body.r.min.y == v->body.r.max.y)
			r.max.y++;
	}
	if(!eqrect(r, v->r)){
		bitblt(&screen, r.min, &screen, r, 0);
		v->reshape(r, c->safe);
	}
	r.min.y = v->r.max.y;
	r.max.y = r.min.y+Border;
	bitblt(&screen, r.min, &screen, r, 0xF);
	r.min.y = r.max.y;
	r.max.y = w->r.max.y;
	if(!eqrect(r, w->r)){
		bitblt(&screen, r.min, &screen, r, 0);
		w->reshape(r, c->safe);
	}
	c->safe = TRUE;
    	w->mousebut();
}

Text*
Column.which(Column *c, Point p)
{
	int i;
	Window *w;

	if(!ptinrect(p, c->r))
		return nil;
	if(ptinrect(p, c->tag.all))
		return &c->tag;
	for(i=0; i<c->nw; i++){
		w = c->w[i];
		if(ptinrect(p, w->r)){
			if(ptinrect(p, w->tag.all))
				return &w->tag;
			return &w->body;
		}
	}
	return nil;
}

int
Column.clean(Column *c)
{
	int clean;
	int i;

	clean = TRUE;
	for(i=0; i<c->nw; i++)
		clean &= c->w[i]->clean(TRUE);
	return clean;
}
