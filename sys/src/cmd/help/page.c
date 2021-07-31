#include <u.h>
#include <libc.h>
#include <libg.h>
#include <frame.h>
#include "dat.h"
#include "fns.h"

Page	*rootpage;
Page	*page;
int	pageid;

Page*
newpage(Page *parent, Page *after)
{
	Page *p;

	p = emalloc(sizeof(Page));
	memset(p, 0, sizeof(Page));
	p->id = ++pageid;
	/*
	 * Insert in list
	 */
	if(parent){	/* false for rootpage */
		if(after == 0){
			p->next = parent->down;
			parent->down = p;
		}else{
			p->next = after->next;
			after->next = p;
		}
		p->n = parent->ntab++;
	}
	p->parent = parent;
	p->tag.page = p;
	p->body.page = p;
	Strinsert(&p->tag, 0, 0, 0);
	Strinsert(&p->body, 0, 0, 0);
	return p;
}

Page*
pagelookup1(Page *p, int id)
{
	Page *q;

    Again:
	if(p==0 || p->id==id)
		return p;
	q = pagelookup1(p->down, id);
	if(q)
		return q;
	p = p->next;
	goto Again;
}

Page*
pagelookup(int id)
{
	return pagelookup1(rootpage, id);
}

void
pagetab(Page *p, int pvis)
{
	Rectangle r;

	if(p->parent){
		r = p->parent->tabsr;
		if(p->parent->ver){
			r.min.y += p->n*TABSZ;
			r.max.y = r.min.y+TABSZ;
		}else{
			r.min.x += p->n*TABSZ;
			r.max.x = r.min.x+TABSZ;
		}
		if(pvis){
			r = inset(r, 1);
			bitblt(&screen, r.min, &screen, r, F);
		}
	}
}

void
pagedraw(Page *p, Rectangle r, int pvis)	/* pvis: parent is visible */
{
	int bdry, v;
	Rectangle r1;
	Rectangle or, osr;
	Page *q;

	or = p->r;
	osr = p->subr;
	p->r = r;
	v = pvis && p->visible;
	/*
	 * Clear
	 */
	if(v){
		bitblt(&screen, r.min, &screen, r, 0);
		border(&screen, r, 1, 0xF);
	}
	bdry = r.min.y+2+font->height+2;
	p->bdry = bdry;
	if(r.max.y < bdry)
		r.max.y = bdry;
	/*
	 * Tabs and rects for subpages
	 */
	if(p->ver){
		p->tabsr = Rect(p->r.min.x, bdry,
				p->r.min.x+TABSZ, p->r.max.y);
		p->subr = Rect(p->tabsr.max.x, bdry,
				p->r.max.x, p->r.max.y);
	}else{
		p->tabsr = Rect(p->r.min.x+TABSZ, bdry,
				p->r.max.x, p->bdry+TABSZ);
		p->subr = Rect(p->tabsr.min.x, p->tabsr.max.y,
				p->r.max.x, p->r.max.y);
	}
	p->tabsr = inset(p->tabsr, 1);
	/*
	 * My tab
	 */
	pagetab(p, pvis);
	/*
	 * Tag
	 */
	r1 = Rect(p->subr.min.x, r.min.y, r.max.x, bdry);
	r1 = inset(r1, 2);
	frinit(&p->tag, r1, font, &screen);
	p->body.scrollr = Rect(0, 0, 0, 0);
	/*
	 * Body
	 */
	r1.min.x = r.min.x;
	r1.min.y = bdry;
	r1.max = p->subr.max;
	r1 = inset(r1, 2);
	if(r1.max.y < r1.min.y)
		r1.max.y = r1.min.y;
	p->body.scrollr = r1;
	p->body.scrollr.max.x = r1.min.x+SCROLLWID;
	r1.min.x = p->body.scrollr.max.x+SCROLLGAP;
	frinit(&p->body, r1, font, &screen);
	p->line1 = p->body.r.min.y+font->height;
	/*
	 * Draw
	 */
	r1 = inset(p->subr, 1);
	r1.min.y = bdry;
	r1.max.y = bdry+1;
	if(v)
		bitblt(&screen, r1.min, &screen, r1, 0xF);
	if(p->tag.n){
		frclear(&p->tag);
		if(v)
			textdraw(&p->tag);
	}
	if(p->body.n){
		frclear(&p->body);
		if(v)
			textdraw(&p->body);
	}
	/*
	 * Do children
	 */
	for(q=p->down; q; q=q->next){
		if(eqrect(p->r, or))
			r = q->r;
		else if(p->ver){
			r.min.x = p->subr.min.x;
			r.max.x = p->subr.max.x;
			r.min.y = p->subr.min.y +
			    (q->r.min.y-osr.min.y)*Dy(p->r)/Dy(or);
			r.max.y = p->subr.min.y +
			    (q->r.max.y-osr.min.y)*Dy(p->r)/Dy(or);
		}else{
			r.min.x = p->subr.min.x +
			    (q->r.min.x-osr.min.x)*Dx(p->r)/Dx(or);
			r.max.x = p->subr.min.x +
			    (q->r.max.x-osr.min.x)*Dx(p->r)/Dx(or);
			r.min.y = p->subr.min.y;
			r.max.y = p->subr.max.y;
		}
		pagedraw(q, r, v);
	}
}

/*
 * Construct new list below p, of opposite orientation
 */
Page*
pagesplit(Page *parent, int ver)
{
	Page *new;

	if(parent->down || parent->body.n)
		return 0;
	parent->ver = ver;
	new = newpage(parent, 0);
	new->visible = 1;
	pagedraw(parent, parent->r, 1);
	pagedraw(new, parent->subr, 1);
	return new;
}

int
pagecompare(Page **ap, Page **bp)
{
	Page *a, *b;

	a = *ap;
	b = *bp;
	/* sort must be stable; never return 0 */
	if(a->parent->ver){
		if(a->r.min.y < b->r.min.y)
			return -1;
		if(a->r.min.y==b->r.min.y && a<b)
			return -1;
		return 1;
	}
	if(a->r.min.x < b->r.min.x)
		return -1;
	if(a->r.min.x==b->r.min.x && a < b)
		return -1;
	return 1;
}

Page**
sortlist(Page *p, int n)
{
	Page *q, **l;
	int i;

	l = emalloc((n+1)*sizeof(Page*));
	l[n] = 0;
	i = 0;
	for(q=p; q; q=q->next)
		l[i++] = q;
	qsort(l, n, sizeof(Page*), pagecompare);
	return l;
}

/*
 * Add new page at end of parent->down.
 */
Page*
pageadd(Page *parent, int adj)
{
	Page *new;
	Point pt;
	Frame *f;

	if(parent == 0)
		return 0;
	new = newpage(parent, 0);
	if(parent->ver)
		pt.y = parent->subr.min.y + 3*Dy(parent->subr)/5;
	else
		pt.x = parent->subr.min.x + 3*Dx(parent->subr)/5;
	if(adj){
		Page *q, **l;
		int i, n, xy, set;

		set = 0;
		for(n=0,q=parent->down; q; q=q->next,n++)
			;
		l = sortlist(parent->down, n);
		if(n==0 && parent->ver)
			pt.y = parent->subr.min.y;
		for(i=0; i<n; i++){
			q = l[i];
			if(parent->ver){
				/* see if can abut to the text */
				if(q->visible){
					f = &q->body;
					xy = f->r.min.y + f->nlines*font->height + 2;
					if(xy < parent->r.max.y-250){
						pt.y = xy;
						set++;
						continue;
					}
					xy = f->r.min.y + (f->nlines/3)*font->height + 2;
					if(xy < parent->r.max.y-100){
						pt.y = xy;
						set++;
						continue;
					}
					xy = f->r.min.y + 5*font->height + 2;
					if(xy < parent->r.max.y-50){
						pt.y = xy;
						set++;
						continue;
					}
				}
				if(!set){
					/* just abut the tags */
					xy = q->bdry;
					if(pt.y==q->r.min.y && xy<parent->r.max.y-10)
						pt.y = xy;
				}
			}else{
				xy = q->r.min.x+3*TABSZ;	/* leave a little body */
				if(pt.x==q->r.min.x && xy<parent->r.max.x-5*TABSZ)
					pt.x = xy;
			}
		}
		free(l);
	}
	pagetop(new, pt, 0);
	return new;
}

void
pagefree(Page *p)
{
	Page *q, *o;

	for(q=p->down; q; q=o){
		o = q->next;
		pagefree(q);
	}
	frclear(&p->tag);
	if(p->tag.s)
		free(p->tag.s);
	frclear(&p->body);
	if(p->body.s)
		free(p->body.s);
	if(page == p)
		page = 0;
	if(curt==&p->tag || curt==&p->body)
		curt = 0;
	free(p);
}

/*
 * Put page on top of its pile, extending size to corner of parent page.
 * Derive uninteresting coordinate of pt from parent geometry.
 * Adjust upper left pt if necessary to display no partial tags.
 * Fix visibilities in list.
 */
void
pagetop(Page *p, Point pt, int close)
{
	Rectangle r, npr, opr, or;
	int i, j, n, done, ov;
	Page *o, *q, *qq, *parent;
	Page **l;

	parent = p->parent;
	if(parent == 0)
		return;

	/*
	 * Unlink page from list; relink afterwards - painter's algorithm
	 */
	o = 0;
	for(q=parent->down; q!=p; q=q->next)
		o = q;
	if(o == 0)
		parent->down = p->next;
	else
		o->next = p->next;
	for(n=0,q=parent->down; q; q=q->next,n++)
		;
	if(close){
		bitblt(&screen, parent->tabsr.min, &screen, parent->tabsr, 0);
		parent->ntab = n;
		if(p->visible)
			bitblt(&screen, p->r.min, &screen, p->r, 0);
		if(page == p)
			page = 0;
		if(curt==&p->tag || curt==&p->body)
			curt = 0;
		goto Redraw;
	}

	/*
	 * Clip
	 */
	if(parent->ver){
		if(pt.y < parent->subr.min.y)
			pt.y = parent->subr.min.y;
		i = parent->subr.max.y - (2+font->height+2);
		if(pt.y > i)
			pt.y = i;
	}else{
		if(pt.x < parent->subr.min.x)
			pt.x = parent->subr.min.x;
		i = parent->subr.max.x - (SCROLLWID+SCROLLGAP+10);
		if(pt.x > i)
			pt.x = i;
	}
	/*
	 * Adjust pt until no partial tag is visible underneath (vertical)
	 * or at least one character is (somewhat) visible (horizontal)
	 * and exactly zero or at least one text line is in the body (vertical)
	 */
	do
		for(q=parent->down; q; q=q->next){
			if(parent->ver){
				if(q->r.min.y<pt.y && pt.y<q->bdry){
					pt.y = q->r.min.y;
					break;
				}else if(q->bdry<pt.y && pt.y<q->line1){
					pt.y = q->bdry;
					break;
				}
			}else{
				if(q->r.min.x<pt.x && pt.x<q->body.r.min.x+10){
					pt.x = q->r.min.x;
					break;
				}
			}
		}
	while(q);
	if(parent->ver)
		pt.x = parent->subr.min.x;
	else
		pt.y = parent->subr.min.y;
	opr = p->r;
	npr = Rpt(pt, parent->subr.max);
	p->r = npr;

	/*
	 * Relink
	 */
	for(q=parent->down; q; q=q->next)
		;
	if(q == 0){
		p->next = parent->down;
		parent->down = p;
	}else{
		p->next = 0;
		q->next = p;
	}
	p->visible = 1;
	n++;	/* one more in the list now */

    Redraw:
	/*
	 * Redraw other affected windows
	 */
	l = sortlist(parent->down, n);
	done = 0;
	for(i=0; i<n; i++){
		q = l[i];
		j = i+1;
		q->n = i;
		if(q == p){
			q->r = opr;	/* get reshapes right: restore r */
			pagedraw(q, npr, 1);
			done = 1;
			continue;
		}
		if(done){
			q->visible = 0;
			pagedraw(q, q->r, 1);
			continue;
		}
		ov = q->visible;
		r = q->r;
		or = r;
		qq = l[j];
		/*
		 * If we're closing, pop up all pages covered by page
		 * being closed; leave the rest alone.
		 */
		if(close && (pagecompare(&q, &p)>0 || eqpt(q->r.min, p->r.min)))
			q->visible = 1;
		else{
			/*
			 * First if: make sure all pieces of
			 * parent->subr are covered if possible
			 */
			if(i==0 && (qq==0 || !eqpt(q->r.min, qq->r.min)))
				q->visible = 1;
			if(!close && eqpt(q->r.min, p->r.min))	/* p wins */
				q->visible = 0;
			/*
			 * Find next visible or soon-visible page and abut q to it.
			 */
			for(; qq=l[j]; j++)	/* assign = */
				if(qq->visible
				|| (close && (pagecompare(&qq, &p)>0 || eqpt(qq->r.min, p->r.min))))
					break;
		}
		if(parent->ver){
			if(qq == 0)
				r.max.y = parent->subr.max.y;
			else
				r.max.y = qq->r.min.y;
		}else{
			if(qq == 0)
				r.max.x = parent->subr.max.x;
			else
				r.max.x = qq->r.min.x;
		}
		if(ov && eqrect(r, or))
			pagetab(q, 1);	/* save time; draw tab only */
		else
			pagedraw(q, r, 1);
	}
	r = parent->subr;
	if(l[0]){
		if(parent->ver){
			r.min.y++;
			r.max.y = l[0]->r.min.y;
		}else{
			r.min.x++;
			r.max.x = l[0]->r.min.x;
		}
		bitblt(&screen, r.min, &screen, r, 0);
	}
	free(l);
	if(close)
		pagefree(p);
}

/*
 * Set global page to that pointed to by mouse
 */
Page*
curpage(void)
{
	Page *p;

	p = rootpage;
    Again:
	for(;;){
		if(p == 0){
			page = 0;
			return 0;
		}
		if(p->visible && ptinrect(mouse.xy, p->r))
			break;
		p = p->next;
	}
	if(p->down && ptinrect(mouse.xy, p->tabsr))
		p->text = 0;
	else if(mouse.xy.y <= p->bdry)
		p->text = &p->tag;
	else{
		if(p->down){
			p = p->down;
			goto Again;
		}
		p->text = &p->body;
	}
	return p;
}

/*
 * Return nth page in p->down
 */
Page*
whichpage(Page *p, int n)
{
	for(p=p->down; p; p=p->next)
		if(p->n == n)
			return p;
	return 0;
}
