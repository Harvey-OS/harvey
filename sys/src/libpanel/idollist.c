/*
 * grouped pictures/text lists.
 */
#include <u.h>
#include <libc.h>
#include <libg.h>
#include <panel.h>
#include "pldefs.h"

typedef struct	Idollist	Idollist;

struct Idollist
{
	Idol	*top;		/* of display */
	Idol	*sel;		/* current selection */
	Font	*font;
	Idol	*idols;
	Idol	*bot;		/* of display */
	int	wid;		/* current width of display area */
	int	hgt;
	int	buttons;
	void	(*hit)(Panel*, int, void*);
	Point	minsize;		/* smallest acceptible window size */
};

struct Idol
{
	void	*rock;		/* for user to identfy the idol */
	Bitmap	*p;
	Bitmap	*psel;
	char	*text;
	Idol	*next;
	Idol	*last;
	Point	pp;
	Point	tp;
	int	wid;		/* of text */
	Rectangle	r;
};

enum
{
	PICOFF	= 4,		/* distance from left to picture */
	TXTOFF	= 8,		/* distance from picture to text */
	PICSEL	= 2,		/* width of box around picture */
};

Idol*	pl_idolscroll(Idol*, Idol*, int, int, int);
Idol*	pl_idolhit(Idol*, Idol*, Point, Point);
Idol*	pl_idoldraw(Bitmap*, Rectangle, Font*, Idol*, Idol*);
int	pl_idolfmt(Idol*, Font*, int);
Idol*	pl_idolbot(Rectangle, Idol*);

Idol*
plmkidol(Idol **it, Bitmap *p, Bitmap *psel, char *text, void *rock)
{
	Idol *new;

	new = malloc(sizeof(Idol));
	if(new == nil)
		return nil;
	new->rock = rock;
	new->p = p;
	new->psel = psel;
	if(text == nil)
		text = "";
	new->text = text;
	new->r = Rect(0,0,0,0);
	new->pp = Pt(0, 0);
	new->tp = new->pp;
	new->next = nil;
	if(*it != nil)
		(*it)->last->next = new;
	else
		*it = new;
	(*it)->last = new;
	return new;
}

void
plfreeidol(Idol *i)
{
	Idol *next;

	while(i){
		next = i->next;
		free(i);
		i = next;
	}
}

Point
plidolsize(Idol *idols, Font *f, int wid)
{
	Point p;
	int n;

	pl_idolfmt(idols, f, wid);
	p = Pt(0, 0);
	for(; idols != nil; idols = idols->next){
		n = Dx(idols->r);
		if(n > p.x)
			p.x = n;
		n = Dy(idols->r);
		if(n > p.y)
			p.y = n;
	}
	return p;
}

void*
plidollistgetsel(Panel *p)
{
	Idollist *ip;

	ip = p->data;
	if(ip->sel != nil)
		return ip->sel->rock;
	return nil;
}

void
pl_hiliteidol(Panel *p, Idol *i, Bitmap *b, int hilite){
	Rectangle dr, save;
	Bitmap *on;
	Point offs;
	Idollist *ip;
	int code;

	if(i == nil)
		return;
	ip = p->data;
	if(ip->top == nil)
		return;

	offs = sub(p->r.min, ip->top->r.min);
	dr = raddp(i->r, offs);
	if(rectinrect(dr, p->r)){
		save = b->clipr;
		clipr(b, dr);
		if(i->psel != nil){
			on = i->p;
			if(hilite)
				on = i->psel;
			bitblt(b, add(i->pp, offs), on, on->r, S);
		}else if(i->p != nil){
			code = Zero;
			if(hilite)
				code = F;
			border(b, raddp(i->p->r, add(i->pp, offs)), -PICSEL, code);
		}
		code = S;
		if(hilite)
			code = ~S;
		string(b, add(i->tp, offs), ip->font, i->text, code);
		clipr(b, save);
	}
}

void
pl_drawidollist(Panel *p)
{
	int lo, hi, wid;
	Rectangle r;
	Panel *sb;
	Bitmap *back;
	Idollist *ip;

	ip = p->data;
	back = balloc(p->r, p->b->ldepth);
	if(back == nil)
		back = p->b;
	r = p->r;
	wid = r.max.x - r.min.x;
	if(wid != ip->wid){
		ip->wid = wid;
		ip->hgt = pl_idolfmt(ip->idols, ip->font, ip->wid);
		p->scr.size.y = ip->hgt;
	}
	if(ip->top)
		p->scr.pos.y = ip->top->r.min.y;
	else
		p->scr.pos.y = p->scr.size.y;
	ip->bot = pl_idoldraw(back, r, ip->font, ip->top, nil);
	pl_hiliteidol(p, ip->sel, back, 1);
	lo = hi = ip->hgt;
	if(ip->top != nil)
		lo = ip->top->r.min.y;
	if(ip->bot != nil)
		hi = ip->bot->r.max.y;
	if(back != p->b){
		bitblt(p->b, back->r.min, back, back->r, S);
		bfree(back);
	}
	sb = p->yscroller;
	if(sb!=nil && sb->setscrollbar!=nil)
		sb->setscrollbar(sb, lo, hi, ip->hgt);
}

int
pl_hitidollist(Panel *p, Mouse *m)
{
	Idol *oldsel;
	Idollist *ip;
	int hitme;

	ip = p->data;
	oldsel = ip->sel;
	hitme = 0;
	if(m->buttons & OUT){
		p->state = UP;
	}else if(m->buttons & 7){
		ip->buttons = m->buttons;
		ip->sel = pl_idolhit(ip->top, ip->bot, m->xy, p->r.min);
		p->state = DOWN;
	}else{
		if(p->state == DOWN)
			hitme = 1;
		p->state = UP;
	}
	if(ip->sel != oldsel){
		pl_hiliteidol(p, oldsel, p->b, 0);
		pl_hiliteidol(p, ip->sel, p->b, 1);
	}
	if(hitme && ip->hit != nil && ip->sel != nil)
		(*ip->hit)(p, ip->buttons, ip->sel->rock);
	return p->state == DOWN;
}

void
pl_scrollidollist(Panel *p, int dir, int buttons, int num, int den)
{
	Bitmap *back;
	Panel *sb;
	Rectangle r;
	Point size;
	Idollist *ip;
	Idol *top;
	int lo, hi, h, nh;

	if(dir != VERT)
		return;
	ip = p->data;
	size = sub(p->r.max, p->r.min);
	top = pl_idolscroll(ip->idols, ip->top, buttons, num*size.y/den, size.y);
	if(top != ip->top){
		if(ip->wid != size.x || ip->top == nil || top == nil){
			ip->top = top;
			pldraw(p, p->b);
			return;
		}
		r = p->r;
		r.max.x = r.min.x + top->r.max.x;
		back = balloc(r, p->b->ldepth);
		if(back == nil)
			back = p->b;

		h = ip->hgt;
		if(ip->bot != nil)
			h = ip->bot->r.min.y;
		/*
		 * scroll forward and back using existing image if possible
		 */
		if(top->r.min.y >= ip->top->r.min.y && top->r.max.y <= h){
			r.max.y = r.min.y + h - ip->top->r.min.y;
			r.min.y += top->r.min.y - ip->top->r.min.y;
			bitblt(back, p->r.min, p->b, r, S);
			r.min.y = p->r.min.y + h - top->r.min.y;
			if(ip->bot == nil){
				bitblt(back, r.min, p->b, r, Zero);
			}else{
				r.max = p->r.max;
				ip->bot = pl_idoldraw(back, r, ip->font, ip->bot, nil);
			}
			r.min.y = p->r.min.y;
		}else if(top->r.min.y < ip->top->r.min.y && top->r.min.y+size.y > ip->top->r.max.y){
			ip->bot = pl_idolbot(r, top);
			nh = ip->hgt;
			if(ip->bot != nil)
				nh = ip->bot->r.min.y;
			r.max.y = r.min.y-ip->top->r.min.y+nh;
			bitblt(back, Pt(r.min.x, r.min.y-top->r.min.y+ip->top->r.min.y), p->b, r, S);
			r.max.y = r.min.y-top->r.min.y+ip->top->r.min.y;
			pl_idoldraw(back, r, ip->font, top, ip->top);
			h -= ip->top->r.min.y;
			nh  -= top->r.min.y;
			if(h > nh){
				r.min.y = r.min.y + nh - top->r.min.y;
				r.max.y = r.min.y + h - nh;
				bitblt(back, r.min, back, r, Zero);
				r = p->r;
				nh = h;
			}
			r.max.y = r.min.y + nh;
		}else
			ip->bot = pl_idoldraw(back, r, ip->font, top, nil);

		ip->top = top;
		if(ip->top)
			p->scr.pos.y = ip->top->r.min.y;
		else
			p->scr.pos.y = p->scr.size.y;
		pl_hiliteidol(p, ip->sel, back, 1);
		lo = hi = ip->hgt;
		if(ip->top != nil)
			lo = ip->top->r.min.y;
		if(ip->bot != nil)
			hi = ip->bot->r.max.y;

		if(back != p->b){
			bitblt(p->b, back->r.min, back, r, S);
			bfree(back);
		}
		sb = p->yscroller;
		if(sb!=nil && sb->setscrollbar!=nil)
			sb->setscrollbar(sb, lo, hi, ip->hgt);
	}
}

void
pl_typeidollist(Panel *g, Rune c)
{
	USED(g, c);
}

Point
pl_getsizeidollist(Panel *p, Point children)
{
	USED(children);
	return ((Idollist *)p->data)->minsize;
}

void
pl_childspaceidollist(Panel *g, Point *ul, Point *size)
{
	USED(g, ul, size);
}

void
plinitidollist(Panel *v, int flags, Point minsize, Font *f, Idol *i, void (*hit)(Panel *, int, void *))
{
	Idollist *ip;

	ip = v->data;
	v->flags =flags|LEAF;
	v->state = UP;
	v->draw = pl_drawidollist;
	v->hit = pl_hitidollist;
	v->type = pl_typeidollist;
	v->getsize = pl_getsizeidollist;
	v->childspace = pl_childspaceidollist;
	ip->hit = hit;
	ip->minsize = minsize;
	ip->font = f;
	ip->idols = i;
	ip->top = i;
	ip->bot= nil;
	ip->sel = nil;
	v->scroll = pl_scrollidollist;
	ip->wid = -1;
	v->scr.pos = Pt(0,0);
	v->scr.size = Pt(0,1);
}

Panel*
plidollist(Panel *parent, int flags, Point minsize, Font *f, Idol *i, void (*hit)(Panel *, int, void *))
{
	Panel *v;

	v = pl_newpanel(parent, sizeof(Idollist));
	plinitidollist(v, flags, minsize, f, i, hit);
	return v;
}

/*
 * initialize rectangles & of text starting at t,
 * galley width is wid.  Returns the total length of the idols
 */
int
pl_idolfmt(Idol *idols, Font *font, int wid)
{
	Idol *i;
	Point p;
	int pwid, twid, w, h, ph, fh, fm;

	/*
	 * find out the width of each piece of text
	 * and the max width pictures and text
	 */
	pwid = 0;
	twid = 0;
	for(i = idols; i != nil; i = i->next){
		i->wid = strwidth(font, i->text);
		if(i->wid > twid)
			twid = i->wid;
		if(i->p != nil){
			w = i->p->r.max.x - i->p->r.min.x;
			if(w > pwid)
				pwid = w;
		}
	}

	if(PICOFF + pwid + TXTOFF + twid < wid)
		wid = PICOFF + pwid + TXTOFF + twid;

	/*
	 * set the rectangle and origin points of each idol
	 * try to make the middle of the text coincide with
	 * the middle of the picture.
	 */
	fm = (font->ascent+1) / 2;
	fh = fm * 2 + (font->height-font->ascent) * 2;
	p = Pt(0, 0);
	for(i = idols; i != nil; i = i->next){
		ph = 0;
		if(i->p != nil)
			ph = Dy(i->p->r);
		if(i->psel == nil)
			ph += 2*PICSEL;
		h = ph;
		if(h < fh)
			h = fh;
		i->r.min = p;
		i->r.max.x = i->r.min.x + wid;
		i->r.max.y = i->r.min.y + h;

		i->pp.x = i->r.min.x + PICOFF;
		i->pp.y = i->r.min.y + h/2 - Dy(i->p->r)/2;

		i->tp.x = i->pp.x + pwid + TXTOFF;
		i->tp.y = i->r.min.y + h/2 - fm;

		/*
		 * check to see what fits; wrap text?
		 */
		if(i->tp.x > wid)
			i->r = Rect(0, 0, 0, 0);

		p.y += Dy(i->r);
	}
	return p.y;
}

Idol*
pl_idolbot(Rectangle r, Idol *i)
{
	Point offs;
	Rectangle dr;

	offs = sub(r.min, i->r.min);
	for(; i != nil; i = i->next){
		if(!eqrect(i->r, Rect(0,0,0,0))){
			dr = raddp(i->r, offs);
			if(dr.max.y > r.max.y)
				break;
		}
	}
	return i;
}

Idol*
pl_idoldraw(Bitmap *b, Rectangle r, Font *f, Idol *i, Idol *stop)
{
	Point offs;
	Rectangle save, dr;

	bitblt(b, r.min, b, r, Zero);
	if(i == nil)
		return nil;
	save = b->clipr;
	clipr(b, r);
	offs = sub(r.min, i->r.min);
	for(; i != stop; i = i->next){
		if(!eqrect(i->r, Rect(0,0,0,0))){
			dr = raddp(i->r, offs);
			if(dr.max.y > r.max.y)
				break;
			if(i->p != nil)
				bitblt(b, add(i->pp, offs), i->p, i->p->r, S|D);
			string(b, add(i->tp, offs), f, i->text, S|D);
		}
	}
	clipr(b, save);
	return i;
}

Idol*
pl_idolhit(Idol *i, Idol *bot, Point p, Point ul)
{
	if(i == nil)
		return nil;
	p.x -= ul.x;
	p.y += i->r.min.y - ul.y;
	while(i != bot && i->next != nil && i->next->r.min.y <= p.y)
		i = i->next;
	if(i != bot && ptinrect(p, i->r))
		return i;
	return nil;
}

Idol*
pl_idolscroll(Idol *i, Idol *top, int buttons, int pos, int len)
{
	Idol *ip;
	int y;

	if(i == nil)
		return nil;
	if(top == nil)
		return i;
	switch(buttons){
	default:
		SET(y);
		break;
	case 1:		/* left -- top moves to pointer */
		if(top->next != nil)
			y = top->next->r.min.y - pos;
		else
			y = top->r.max.y - pos;
		break;
	case 2:		/* middle -- absolute index of file */
		for(ip = i; ip->next != nil; ip = ip->next)
			;
		y = ip->r.max.y * pos/len;
		break;
	case 4:		/* right -- line pointed at moves to top */
		y = top->r.min.y + pos;
		break;
	}
	for(ip = i; ip->next != nil; ip = ip->next)
		if(ip->r.max.y > y)
			break;
	return ip;
}
