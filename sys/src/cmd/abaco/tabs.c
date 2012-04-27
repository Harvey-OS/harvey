#include <u.h>
#include <libc.h>
#include <draw.h>
#include <memdraw.h>
#include <thread.h>
#include <cursor.h>
#include <mouse.h>
#include <keyboard.h>
#include <frame.h>
#include <plumb.h>
#include <html.h>
#include "dat.h"
#include "fns.h"

void
drawtable(Box *b, Page *p, Image *im)
{
	Rectangle r, cr;
	Tablecell *c;
	Table *t;

	t = ((Itable *)b->i)->table;
	r = rectsubpt(b->r, p->pos);
	draw(im, r, getcolor(t->background.color), nil, ZP);
	if(t->border)
		border(im, r, t->border, display->black, ZP);
	for(c=t->cells; c!=nil; c=c->next){
		cr = rectsubpt(c->lay->r, p->pos);
		if(c->background.color != t->background.color)
			draw(im, cr, getcolor(c->background.color), nil, ZP);
		if(t->border)
			border(im, cr, t->border, display->black, ZP);
		laydraw(p, im,  c->lay);
	}
}

enum
{
	Tablemax = 2000,
	Ttoplevel = 1<<0,

};

static
void
settable(Table *t)
{
	Tablecell *c;
	Lay *lay;

	for(c=t->cells; c!=nil; c=c->next){
		lay = layitems(c->content, Rect(0,0,0,0), FALSE);
		c->minw = Dx(lay->r);
		layfree(lay);
		if(dimenkind(c->wspec) == Dnone){
			lay = layitems(c->content, Rect(0,0,Tablemax,0), FALSE);
			c->maxw = Dx(lay->r);
			layfree(lay);
		}
	}
}

void
settables(Page *p)
{
	Table *t;
	Item *i;

	if(p->doc==nil)
		return;
	for(i=p->items; i!=nil; i=i->next)
		if(i->tag == Itabletag)
			((Itable *)i)->table->flags |= Ttoplevel;

	for(t=p->doc->tables; t!=nil; t=t->next)
		settable(t);
}

static
int
cellwidth(Table *t, Tablecell *c, int sep)
{
	int w, i, n;

	n = c->colspan;
	if(n == 1)
		return t->cols[c->col].width;
	if(n == 0)
		n = t->ncol - c->col;

	w = t->cellspacing*(n-1) + n*sep;
	for(i=c->col; i<c->col+n; i++)
		w += t->cols[i].width;

	return w;
}

static
int
cellheight(Table *t, Tablecell *c, int sep)
{
	int h, i, n;

	n = c->rowspan;
	if(n == 1)
		return t->rows[c->row].height;
	if(n == 0)
		n = t->nrow - c->row;

	h = t->cellspacing*(n-1) + n*sep;
	for(i=c->row; i<c->row+n; i++)
		h += t->rows[i].height;

	return h;
}

static
int
getwidth(int *w, int n)
{
	int i, tot;

	tot = 0;
	for(i=0; i<n; i++)
		tot += w[i];

	return tot;
}

static
void
fixcols(Table *t, int *width, int sep, int domax)
{
	Tablecell *c;
	int w, aw, i, d, n, rem;


	for(c=t->cells; c!=nil; c=c->next){
		if(c->colspan == 1)
			continue;

		n = c->colspan;
		if(n == 0)
			n = t->ncol - c->col;

		w = domax ? c->maxw : c->minw;
		w -= t->cellspacing*(n-1) + n*sep;

		aw = 0;
		for(i=c->col; i<c->col+n; i++)
			aw += width[i];

		rem = w-aw;
		if(rem <= 0)
			continue;

		for(i=c->col; i<c->col+n; i++){
			if(aw > 0){
				d = width[i]*100/aw;
				d = d*rem/100;
			}else
				d = rem/n;
			width[i] += d;
		}
	}
}

static
int
tablewidth(Table *t, int tw, int sep)
{
	Tablecell *c;
	int i, w, tmin, tmax, d;
	int *maxw, *minw;
	int totw;

	maxw = emalloc(sizeof(int)*t->ncol);
	minw = emalloc(sizeof(int)*t->ncol);
	for(c=t->cells; c!=nil; c=c->next){
		if(dimenkind(c->wspec) != Dnone){
			d = c->minw;
			c->minw = c->maxw = max(dimwidth(c->wspec, tw), c->minw);
			c->minw = d;
		}
		if(c->colspan != 1)
			continue;
		maxw[c->col] = max(maxw[c->col], c->maxw);
		minw[c->col] = max(minw[c->col], c->minw);
	}
	totw = 0;
	fixcols(t, maxw, sep, TRUE);
	tmax = getwidth(maxw, t->ncol);
	if(tmax <= tw){
		d = 0;
		if(tw>tmax && dimenkind(t->width)!=Dnone && t->availw!=Tablemax)
			d = (tw-tmax)/t->ncol;
		for(i=0; i<t->ncol; i++){
			t->cols[i].width = maxw[i] + d;
			totw += t->cols[i].width;
		}
	}else{
		fixcols(t, minw, sep, FALSE);
		tmin = getwidth(minw, t->ncol);
		w = tw - tmin;
		d = tmax - tmin;
		for(i=0; i<t->ncol; i++){
			if(w<=0 || d<=0)
				t->cols[i].width = minw[i];
			else
				t->cols[i].width = minw[i] + (maxw[i] - minw[i])*w/d;
			totw += t->cols[i].width;
		}
	}
	free(minw);
	free(maxw);

	return totw;
}

static
void
fixrows(Table *t, int sep)
{
	Tablecell *c;
	Lay *lay;
	int h, ah, i, d, n, rem;

	for(c=t->cells; c!=nil; c=c->next){
		if(c->rowspan == 1)
			continue;
		n = c->rowspan;
		if(n==0 || c->row+n>t->nrow)
			n = t->nrow - c->row;

		lay = layitems(c->content, Rect(0,0,cellwidth(t, c, sep),0), FALSE);
		h = max(Dy(lay->r), c->hspec);
		layfree(lay);
		h -= t->cellspacing*(n-1) + n*sep;
		ah = 0;
		for(i=c->row; i<c->row+n; i++)
			ah += t->rows[i].height;

		rem = h-ah;
		if(rem <= 0)
			continue;

		for(i=c->row; i<c->row+n; i++){
			if(ah > 0){
				d = t->rows[i].height*100/ah;
				d = d*rem/100;
			}else
				d = rem/n;

			t->rows[i].height += d;
		}
	}
}

static
int
tableheight(Table *t, int sep)
{
	Tablecell *c;
	Lay *lay;
	int i, h, toth;

	for(i=0; i<t->nrow; i++){
		h = 0;
		for(c=t->rows[i].cells; c!=nil; c=c->nextinrow){
			if(c->rowspan != 1)
				continue;
			lay = layitems(c->content, Rect(0, 0, cellwidth(t, c, sep), 0), FALSE);
			h = max(h, max(Dy(lay->r), c->hspec));
			layfree(lay);
		}
		t->rows[i].height = h;
	}
	fixrows(t, sep);
	toth = 0;
	for(i=0; i<t->nrow; i++)
		toth += t->rows[i].height;

	return toth;
}

void
tablesize(Table *t, int availw)
{
	int w, sep, hsep, vsep;

	t->availw = availw;
	sep = 2*(t->border+t->cellpadding);
	hsep = t->cellspacing*(t->ncol+1) + 2*t->border + t->ncol*sep;
	vsep = t->cellspacing*(t->nrow+1) + 2*t->border + t->nrow*sep;
	w = dimwidth(t->width, availw);
	w -= hsep;
	w = w>0 ? w : 0;
	t->totw = tablewidth(t, w, sep);
	t->totw += hsep;
	t->toth = tableheight(t, sep);
	t->toth += vsep;
}

void
laytable(Itable *it, Rectangle r)
{
	Rectangle cr;
	Tablecell *c;
	Table *t;
	int x, y, h, w;
	int sep, i;

	t = it->table;

	sep = (t->cellpadding+t->border) * 2;
	r = insetrect(r, t->cellspacing+t->border);
	for(c=t->cells; c!=nil; c=c->next){
		w = cellwidth(t, c, sep);
		h = cellheight(t, c, sep);
		x = r.min.x;
		if(c->col > 0)
			for(i=0; i<c->col; i++)
				x += t->cols[i].width + sep + t->cellspacing;
		y = r.min.y;
		if(c->row > 0)
			for(i=0;i <c->row; i++)
				y += t->rows[i].height + sep + t->cellspacing;
		cr = Rect(x, y, x+w+sep, y+h+sep);
		c->lay = layitems(c->content, insetrect(cr, sep/2), TRUE);
		c->lay->r = cr;
	}
}
