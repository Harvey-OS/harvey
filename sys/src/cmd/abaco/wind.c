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
wininit(Window *w, Window *, Rectangle r)
{
	Rectangle r1, br;

	incref(w);
	w->r = r;
	w->tag.w = w;
	w->url.w = w;
	w->page.w = w;
	w->status.w = w;
	r1 = r;
	r1.max.y = r1.min.y + font->height;
	textinit(&w->tag, screen, r1, font, tagcols);
	w->tag.what = Tag;
	r1.min.y = r1.max.y++;
	draw(screen, r1, tagcols[BORD], nil, ZP);
	br.min = w->tag.scrollr.min;
	br.max.x = br.min.x + Dx(button->r);
	br.max.y  = br.min.y + Dy(button->r);
	draw(screen, br, button, nil, button->r.min);
	r1.min.y = r1.max.y;
	r1.max.y += font->height;
	textinit(&w->url, screen, r1, font, tagcols);
	w->url.
	w->url.what = Urltag;
	r1.min.y = r1.max.y++;
	draw(screen, r1, tagcols[BORD], nil, ZP);
	r1.min.y = r1.max.y;
	r1.max.y = r.max.y - font->height - 1;
	w->page.all = r1;
	w->page.b = screen;
	draw(screen, r1, display->white, nil, ZP);
	r1.min.y = r1.max.y++;
	draw(screen, r1, tagcols[BORD], nil, ZP);
	r1.min.y = r1.max.y;
	r1.max.y += font->height;
	textinit(&w->status, screen, r1, font, tagcols);
	w->status.what = Statustag;
}

int
winresize(Window *w, Rectangle r, int safe)
{
	Rectangle r1, br;

	w->r = r;
	r1 = r;
	r1.max.y = r1.min.y + font->height;
	if(!safe || !eqrect(w->tag.r, r1)){
		textresize(&w->tag, screen, r1);
		br.min = w->tag.scrollr.min;
		br.max.x = r1.min.x + Dx(button->r);
		br.max.y  = r1.min.y + Dy(button->r);
		draw(screen, br, button, nil, button->r.min);
		r1.min.y = r1.max.y++;
		draw(screen, r1, tagcols[BORD], nil, ZP);
		r1.min.y = r1.max.y;
		r1.max.y += font->height;
		textresize(&w->url, screen, r1);
		r1.min.y = r1.max.y++;
		draw(screen, r1, tagcols[BORD], nil, ZP);
	}
	r1.min.y = r1.max.y;
	r1.max.y = r.max.y - font->height - 1;
	w->page.b = screen;
	if(!safe || !eqrect(w->page.all, r1)){
		if(Dy(r1) <= 0){
			w->page.all = ZR;
			pagerender(&w->page);
			w->r = r;
			w->r.max.y = r1.min.y;
			return w->r.max.y;
		}
		draw(screen, r1, display->white, nil, ZP);
		w->page.all = r1;
		pagerender(&w->page);
		r1.min.y = r1.max.y++;
		draw(screen, r1, tagcols[BORD], nil, ZP);
		r1.min.y = r1.max.y;
		r1.max.y = r.max.y;
		textresize(&w->status, screen, r1);
	}
	return w->r.max.y;
}

void
winclose1(Window *w)
{
	int i;

	if(decref(w) == 0){
		textclose(&w->tag);
		textclose(&w->url);
		textclose(&w->status);
		if(w->history.url){
			for(i=0; i<w->history.nurl; i++)
				urlfree(w->history.url[i]);
			free(w->history.url);
		}
		free(w);
	}
}

void
winclose(Window *w)
{
	pageclose(&w->page);
	winclose1(w);
}

void
winlock(Window *w, int owner)
{
	incref(w);
	qlock(w);
	w->owner = owner;
}

void
winunlock(Window *w)
{
	w->owner = 0;
	qunlock(w);
	winclose1(w);
}

void
winsettag1(Window *w)
{
	int i, j, k, n, bar;
	Rune *new, *r;
	Image *b;
	uint q0, q1;
	Rectangle br;
	Runestr old;

	memset(&old, 0, sizeof(Runestr));
	copyrunestr(&old, &w->tag.rs);
	for(i=0; i<w->tag.rs.nr; i++)
		if(old.r[i]==' ' || old.r[i]=='\t')
			break;

	if(runestreq(old, w->page.title) == FALSE){
		textdelete(&w->tag, 0, i);
		textinsert(&w->tag, 0, w->page.title.r, w->page.title.nr);
		closerunestr(&old);
		copyrunestr(&old, &w->tag.rs);
	}
	new = runemalloc(w->page.title.nr+100);
	i = 0;
	runemove(new+i, L" Del Snarf", 10);
	i += 10;
	if(w->history.nurl){
		if(w->history.cid > 0){
			runemove(new+i, L" Back", 5);
			i += 5;
		}
		if(w->history.cid < w->history.nurl-1){
			runemove(new+i, L" Next", 5);
			i += 5;
		}
		if(w->page.loading){
			runemove(new+i, L" Stop", 5);
			i += 5;
		}
	}
	runemove(new+i, L" Get", 4);
	i += 4;
	runemove(new+i, L" | ", 3);
	i += 3;
	runemove(new+i, w->page.title.r, w->page.title.nr);
	i += w->page.title.nr;
/*
	r = runestrchr(old.r, '|');
	r = nil;
	if(r)
		k = r-old.r+1;
	else{
		k = w->tag.rs.nr;
		if(w->page.url){
			runemove(new+i, L" Look ", 6);
			i += 6;
		}
	}
*/
	k = w->tag.rs.nr;
	if(runeeq(new, i, old.r, k) == FALSE){
		n = k;
		if(n > i)
			n = i;
		for(j=0; j<n; j++)
			if(old.r[j] != new[j])
				break;
		q0 = w->tag.q0;
		q1 = w->tag.q1;
		textdelete(&w->tag, j, k);
		textinsert(&w->tag, j, new+j, i-j);
		/* try to preserve user selection */
		r = runestrchr(old.r, '|');
		if(r){
			bar = r-old.r;
			if(q0 > bar){
				bar = (runestrchr(new, '|')-new)-bar;
				w->tag.q0 = q0+bar;
				w->tag.q1 = q1+bar;
			}
		}
	}
	closerunestr(&old);
	free(new);
	n = w->tag.rs.nr;
	if(w->tag.q0 > n)
		w->tag.q0 = n;
	if(w->tag.q1 > n)
		w->tag.q1 = n;
	textsetselect(&w->tag, w->tag.q0, w->tag.q1);
	b = button;
	br.min = w->tag.scrollr.min;
	br.max.x = br.min.x + Dx(b->r);
	br.max.y = br.min.y + Dy(b->r);
	draw(screen, br, b, nil, b->r.min);
}


void
winsettag(Window *w)
{
	if(w->col && w->col->safe)
		winsettag1(w);
}

void
winseturl(Window *w)
{
	if(w->page.url && runestreq(w->url.rs, w->page.url->act)==FALSE)
		textset(&w->url, w->page.url->act.r, w->page.url->act.nr);
}

void
winsetstatus(Window *w, Rune *r)
{
	if(w->col && w->col->safe)
		textset(&w->status, r, runestrlen(r));
}

void
winaddhist(Window *w, Url *u)
{
	Url **url;
	int cid, n, i;

	url = w->history.url;
	n = w->history.nurl;
	cid = w->history.cid;
	if(cid < n-1){
		for(i=cid+1; i<n; i++)
			urlfree(url[i]);
		n = cid+1;
	}
	w->history.url = erealloc(w->history.url, ++n*sizeof(Url *));
	w->history.url[n-1] = u;
	w->history.cid = u->id = n-1;
	w->history.nurl = n;
	incref(u);
}

void
wingohist(Window *w, int isnext)
{
	Page *p;
	int n, id;

	n = w->history.nurl;
	p = &w->page;
	if(!p->url)
		return;

	id = p->url->id;

	if(isnext)
		id++;
	else
		id--;

	if(n==0 || id<0 || id==n)
		return;

	incref(w->history.url[id]);
	pageload(p, w->history.url[id], FALSE);
	w->history.cid = id;
}

Text *
wintext(Window *w, Point xy)
{
	w->inpage = FALSE;
	if(ptinrect(xy, w->tag.all))
		return &w->tag;
	if(ptinrect(xy, w->url.all))
		return &w->url;
	if(ptinrect(xy, w->status.all))
		return &w->status;
	if(ptinrect(xy, w->page.all))
		w->inpage = TRUE;

	return nil;
}

Text *
wintype(Window *w, Point xy, Rune r)
{
	Text *t;

	t = wintext(w, xy);
	if(t && !ptinrect(xy, t->scrollr))
		return t;
	if(w->inpage)
		pagetype(&w->page, r, xy);

	return nil;
}

Text *
winmouse(Window *w, Point xy, int but)
{
	Text *t;

	t = wintext(w, xy);
	if(t)
		return t;
	if(w->inpage)
		pagemouse(&w->page, xy, but);

	return nil;
}

void
winmousebut(Window *w)
{
	moveto(mousectl, divpt(addpt(w->tag.scrollr.min, w->tag.scrollr.max), 2));
}

int
winclean(Window *, int)
{
	return TRUE;
}

void
windebug(Window *w)
{
	Page *p;
	int i;

	p = &w->page;
	fprint(2, "title:\t%S\n", p->title.r);
	fprint(2, "url:\t%.*S\n",w->url.rs.nr, w->url.rs.r);
	fprint(2, "aborting:\t%s\n", istrue(p->aborting));
	fprint(2, "changed:\t%s\n", istrue(p->changed));
	fprint(2, "loading:\t%s\n", istrue(p->loading));
	fprint(2, "status:\t%S\n", p->status);
	fprint(2, "HISTORY:\n");
	for(i=0; i<w->history.nurl; i++)
		fprint(2, "url[%d]: %S\n", i, w->history.url[i]->act.r);

	if(p->kidinfo)
		fprint(2, "name: %S\n", p->kidinfo->name);
}
