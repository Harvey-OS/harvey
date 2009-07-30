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
rowinit(Row *row, Rectangle r)
{
	Rectangle r1;
	Text *t;

	draw(screen, r, display->white, nil, ZP);
	row->r = r;
	row->col = nil;
	row->ncol = 0;
	r1 = r;
	r1.max.y = r1.min.y + font->height;
	t = &row->tag;
	textinit(t, screen, r1, font, tagcols);
	t->what = Rowtag;
	t->row = row;
	t->w = nil;
	t->col = nil;
	r1.min.y = r1.max.y;
	r1.max.y += Border;
	draw(screen, r1, display->black, nil, ZP);
	textinsert(t, 0, L"Newcol Google Exit ", 19);
	textsetselect(t, t->rs.nr, t->rs.nr);
}

Column*
rowadd(Row *row, Column *c, int x)
{
	Rectangle r, r1;
	Column *d;
	int i;

	d = nil;
	r = row->r;
	r.min.y = row->tag.r.max.y+Border;
	if(x<r.min.x && row->ncol>0){	/*steal 40% of last column by default */
		d = row->col[row->ncol-1];
		x = d->r.min.x + 3*Dx(d->r)/5;
	}
	/* look for column we'll land on */
	for(i=0; i<row->ncol; i++){
		d = row->col[i];
		if(x < d->r.max.x)
			break;
	}
	if(row->ncol > 0){
		if(i < row->ncol)
			i++;	/* new column will go after d */
		r = d->r;
		if(Dx(r) < 100)
			return nil;
		draw(screen, r, display->white, nil, ZP);
		r1 = r;
		r1.max.x = min(x, r.max.x-50);
		if(Dx(r1) < 50)
			r1.max.x = r1.min.x+50;
		colresize(d, r1);
		r1.min.x = r1.max.x;
		r1.max.x = r1.min.x+Border;
		draw(screen, r1, display->black, nil, ZP);
		r.min.x = r1.max.x;
	}
	if(c == nil){
		c = emalloc(sizeof(Column));
		colinit(c, r);
	}else
		colresize(c, r);
	c->row = row;
	c->tag.row = row;
	row->col = realloc(row->col, (row->ncol+1)*sizeof(Column*));
	memmove(row->col+i+1, row->col+i, (row->ncol-i)*sizeof(Column*));
	row->col[i] = c;
	row->ncol++;
	clearmouse();
	return c;
}

void
rowresize(Row *row, Rectangle r)
{
	int i, dx, odx;
	Rectangle r1, r2;
	Column *c;

	dx = Dx(r);
	odx = Dx(row->r);
	row->r = r;
	r1 = r;
	r1.max.y = r1.min.y + font->height;
	textresize(&row->tag, screen, r1);
	r1.min.y = r1.max.y;
	r1.max.y += Border;
	draw(screen, r1, display->black, nil, ZP);
	r.min.y = r1.max.y;
	r1 = r;
	r1.max.x = r1.min.x;
	for(i=0; i<row->ncol; i++){
		c = row->col[i];
		r1.min.x = r1.max.x;
		if(i == row->ncol-1)
			r1.max.x = r.max.x;
		else
			r1.max.x = r1.min.x+Dx(c->r)*dx/odx;
		if(i > 0){
			r2 = r1;
			r2.max.x = r2.min.x+Border;
			draw(screen, r2, display->black, nil, ZP);
			r1.min.x = r2.max.x;
		}
		colresize(c, r1);
	}
}

void
rowdragcol(Row *row, Column *c, int)
{
	Rectangle r;
	int i, b, x;
	Point p, op;
	Column *d;

	clearmouse();
	setcursor(mousectl, &boxcursor);
	b = mouse->buttons;
	op = mouse->xy;
	while(mouse->buttons == b)
		readmouse(mousectl);
	setcursor(mousectl, nil);
	if(mouse->buttons){
		while(mouse->buttons)
			readmouse(mousectl);
		return;
	}

	for(i=0; i<row->ncol; i++)
		if(row->col[i] == c)
			goto Found;
	error("can't find column");

  Found:
	if(i == 0)
		return;
	p = mouse->xy;
	if((abs(p.x-op.x)<5 && abs(p.y-op.y)<5))
		return;
	if((i>0 && p.x<row->col[i-1]->r.min.x) || (i<row->ncol-1 && p.x>c->r.max.x)){
		/* shuffle */
		x = c->r.min.x;
		rowclose(row, c, FALSE);
		if(rowadd(row, c, p.x) == nil)	/* whoops! */
		if(rowadd(row, c, x) == nil)		/* WHOOPS! */
		if(rowadd(row, c, -1)==nil){		/* shit! */
			rowclose(row, c, TRUE);
			return;
		}
		colmousebut(c);
		return;
	}
	d = row->col[i-1];
	if(p.x < d->r.min.x+80+Scrollsize)
		p.x = d->r.min.x+80+Scrollsize;
	if(p.x > c->r.max.x-80-Scrollsize)
		p.x = c->r.max.x-80-Scrollsize;
	r = d->r;
	r.max.x = c->r.max.x;
	draw(screen, r, display->white, nil, ZP);
	r.max.x = p.x;
	colresize(d, r);
	r = c->r;
	r.min.x = p.x;
	r.max.x = r.min.x;
	r.max.x += Border;
	draw(screen, r, display->black, nil, ZP);
	r.min.x = r.max.x;
	r.max.x = c->r.max.x;
	colresize(c, r);
	colmousebut(c);
}

void
rowclose(Row *row, Column *c, int dofree)
{
	Rectangle r;
	int i;

	for(i=0; i<row->ncol; i++)
		if(row->col[i] == c)
			goto Found;
	error("can't find column");
  Found:
	r = c->r;
	if(dofree)
		colcloseall(c);
	memmove(row->col+i, row->col+i+1, (row->ncol-i)*sizeof(Column*));
	row->ncol--;
	row->col = realloc(row->col, row->ncol*sizeof(Column*));
	if(row->ncol == 0){
		draw(screen, r, display->white, nil, ZP);
		return;
	}
	if(i == row->ncol){		/* extend last column right */
		c = row->col[i-1];
		r.min.x = c->r.min.x;
		r.max.x = row->r.max.x;
	}else{			/* extend next window left */
		c = row->col[i];
		r.max.x = c->r.max.x;
	}
	draw(screen, r, display->white, nil, ZP);
	colresize(c, r);
}

Column*
rowwhichcol(Row *row, Point p)
{
	int i;
	Column *c;

	for(i=0; i<row->ncol; i++){
		c = row->col[i];
		if(ptinrect(p, c->r))
			return c;
	}
	return nil;
}

Text*
rowwhich(Row *row, Point p, Rune r, int key)
{
	Column *c;

	if(ptinrect(p, row->tag.all))
		return &row->tag;
	c = rowwhichcol(row, p);
	if(c)
		return colwhich(c, p, r, key);
	return nil;
}
