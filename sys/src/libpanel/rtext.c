/*
 * Rich text with images.
 * Should there be an offset field, to do subscripts & kerning?
 */
#include <u.h>
#include <libc.h>
#include <libg.h>
#include <panel.h>
#include "pldefs.h"
#define	LEAD	4		/* extra space between lines */
Rtext *pl_rtnew(Rtext **t, int space, int indent, Bitmap *b, Panel *p, Font *f, char *s, int hot, void *user){
	Rtext *new;
	new=malloc(sizeof(Rtext));
	if(new==0) return 0;
	new->hot=hot;
	new->user=user;
	new->space=space;
	new->indent=indent;
	new->b=b;
	new->p=p;
	new->font=f;
	new->text=s;
	new->next=0;
	new->r=Rect(0,0,0,0);
	if(*t)
		(*t)->last->next=new;
	else
		*t=new;
	(*t)->last=new;
	return new;
}
Rtext *plrtpanel(Rtext **t, int space, int indent, Panel *p, void *user){
	return pl_rtnew(t, space, indent, 0, p, 0, 0, 1, user);
}
Rtext *plrtstr(Rtext **t, int space, int indent, Font *f, char *s, int hot, void *user){
	return pl_rtnew(t, space, indent, 0, 0, f, s, hot, user);
}
Rtext *plrtbitmap(Rtext **t, int space, int indent, Bitmap *b, int hot, void *user){
	return pl_rtnew(t, space, indent, b, 0, 0, 0, hot, user);
}
void plrtfree(Rtext *t){
	Rtext *next;
	while(t){
		next=t->next;
		free(t);
		t=next;
	}
}
/*
 * initialize rectangles & nextlines of text starting at t,
 * galley width is wid.  Returns the total length of the text
 */
int pl_rtfmt(Rtext *t, int wid){
	Rtext *tp, *eline;
	int ascent, descent, x, space, a, d, w, topy;
	Point p;
	p=Pt(0,0);
	eline=t;
	while(t){
		ascent=0;
		descent=0;
		space=t->indent;
		x=0;
		tp=t;
		for(;;){
			if(tp->b){
				a=tp->b->r.max.y-tp->b->r.min.y+2;
				d=0;
				w=tp->b->r.max.x-tp->b->r.min.x+4;
			}
			else if(tp->p){
				/* what if plpack fails? */
				plpack(tp->p, Rect(0,0,wid,wid));
				plmove(tp->p, sub(Pt(0,0), tp->p->r.min));
				a=tp->p->r.max.y-tp->p->r.min.y;
				d=0;
				w=tp->p->r.max.x-tp->p->r.min.x;
			}
			else{
				a=tp->font->ascent;
				d=tp->font->height-a;
				w=tp->wid=strwidth(tp->font, tp->text);
			}
			if(x+w+space>wid) break;
			if(a>ascent) ascent=a;
			if(d>descent) descent=d;
			x+=w+space;
			tp=tp->next;
			if(tp==0){
				eline=0;
				break;
			}
			space=tp->space;
			if(space) eline=tp;
		}
		if(eline==t){	/* No progress!  Force fit the first block! */
			ascent=a;
			descent=d;
			eline=t->next;
		}
		topy=p.y;
		p.y+=ascent;
		p.x=t->indent;
		for(;;){
			t->topy=topy;
			t->r.min.x=p.x;
			if(t->b){
				t->r.max.y=p.y;
				t->r.min.y=p.y-(t->b->r.max.y-t->b->r.min.y);
				p.x+=t->b->r.max.x-t->b->r.min.x+2;
				t->r=raddp(t->r, Pt(2, 2));
			}
			else if(t->p){
				t->r.max.y=p.y;
				t->r.min.y=p.y-t->p->r.max.y;
				p.x+=t->p->r.max.x;
			}
			else{
				t->r.min.y=p.y-t->font->ascent;
				t->r.max.y=t->r.min.y+t->font->height;
				p.x+=t->wid;
			}
			t->r.max.x=p.x;
			t->nextline=eline;
			t=t->next;
			if(t==eline) break;
			p.x+=t->space;
		}
		p.y+=descent+LEAD;
	}
	return p.y;
}
Rtext *pl_rtdraw(Bitmap *b, Rectangle r, Rtext *t){
	Point offs;
	Rectangle dr;
	bitblt(b, r.min, b, b->r, Zero);
	if(t==0) return t;
	offs=sub(r.min, Pt(0, t->topy));
	while(t){
		if(!eqrect(t->r, Rect(0,0,0,0))){
			dr=raddp(t->r, offs);
			if(dr.max.y>r.max.y) break;
			if(t->b){
				bitblt(b, dr.min, t->b, t->b->r, S|D);
				if(t->hot) border(b, inset(dr, -2), 1, F);
			}
			else if(t->p){
				plmove(t->p, sub(dr.min, t->p->r.min));
				pldraw(t->p, b);
			}
			else{
				string(b, dr.min, t->font, t->text, S|D);
				if(t->hot)
					segment(b, Pt(dr.min.x, dr.max.y-1),
						Pt(dr.max.x, dr.max.y-1), ~0, S|D);
			}
		}
		t=t->next;
	}
	return t;
}
Rtext *pl_rthit(Rtext *t, Point p, Point ul){
	if(t==0) return 0;
	p.x-=ul.x;
	p.y+=t->topy-ul.y;
	while(t->nextline && t->nextline->topy<=p.y) t=t->nextline;
	for(;t!=0;t=t->next){
		if(t->topy>p.y) return 0;
		if(ptinrect(p, t->r)) return t;
	}
	return 0;
}
/*
 * Returns a pointer to text that should appear at the top of the screen
 * after scrolling.
 *	t points to the start of the text.
 *	top points to the text appearing at the top of the screen.
 *	buttons is mouse buttons.
 *	pos is the vertical position of the scroll bar, between 0 and len.
 *	len is the length of the text window.
 *	pos and len must be in pixel units!
 */
Rtext *pl_rtscroll(Rtext *t, Rtext *top, int buttons, int pos, int len){
	Rtext *tp;
	int y, bot;
	if(t==0) return 0;
	switch(buttons){
	default:
		SET(y);
		break;
	case 1:		/* left -- top moves to pointer */
		if(top==0) return t;
		if(top->nextline) y=top->nextline->topy-pos;
		else{
			y=top->r.max.y;
			for(tp=top;tp;tp=tp->next) if(tp->r.max.y>y) y=tp->r.max.y;
			y-=pos;
		}
		break;
	case 2:		/* middle -- absolute index of file */
		bot=0;
		for(tp=t;tp;tp=tp->next) if(tp->r.max.y>bot) bot=tp->r.max.y;
		y=bot*pos/len;
		break;
	case 4:		/* right -- line pointed at moves to top */
		if(top==0) return t;
		y=top->topy+pos;
		break;
	}
	for(tp=t;tp->nextline!=0;tp=tp->nextline) if(tp->nextline->topy>y) break;
	return tp;
}
