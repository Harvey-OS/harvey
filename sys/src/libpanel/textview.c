/*
 * Fonted text viewer, calls out to code in rtext.c
 */
#include <u.h>
#include <libc.h>
#include <libg.h>
#include "panel.h"
#include "pldefs.h"
typedef struct Textview Textview;
struct Textview{
	void (*hit)(Panel *, int, Rtext *); /* call back to user on hit */
	Rtext *text;			/* text */
	Rtext *top;			/* text at top of screen */
	Rtext *bot;			/* text after bottom of screen */
	Rtext *hitword;			/* text to hilite */
	int twid;			/* text width */
	int thgt;			/* text height */
	Point minsize;			/* smallest acceptible window size */
	int buttons;
};
void pl_hiliteword(Panel *p, Rtext *w, Bitmap *b){
	Point ul, size;
	Rectangle r;
	Textview *tp;
	if(w==0 || (w->b==0 && w->p!=0)) return;
	tp=p->data;
	ul=p->r.min;
	size=sub(p->r.max, p->r.min);
	pl_interior(UP, &ul, &size);
	if(tp->top)			/* is this right? */
		ul.y-=tp->top->topy;
	r=raddp(w->r, ul);
	bitblt(b, r.min, b, r, F&~D);
}
void pl_stuffbitmap(Panel *p, Bitmap *b){
	p->b=b;
	for(p=p->child;p;p=p->next)
		pl_stuffbitmap(p, b);
}
/*
 * We draw the text in a backup bitmap and copy it onto the screen.
 * This leaves the bitmap pointers in all the subpanels pointing
 * to the wrong bitmap.  This code fixes them.
 */
void pl_drawnon(Rtext *rp, Rtext *last, Bitmap *b){
	for(;rp!=last;rp=rp->next)
		if(rp->b==0 && rp->p!=0)
			pl_stuffbitmap(rp->p, b);
}
void pl_drawtextview(Panel *p){
	int lo, hi, twid;
	Rectangle r;
	Panel *sb;
	Bitmap *back;
	Textview *tp;
	tp=p->data;
	back=balloc(p->r, p->b->ldepth);
	if(back==0) back=p->b;
	r=pl_box(back, p->r, p->state);
	twid=r.max.x-r.min.x;
	if(twid!=tp->twid){
		tp->twid=twid;
		tp->thgt=pl_rtfmt(tp->text, tp->twid);
		p->scr.size.y=tp->thgt;
	}
	if(tp->top) p->scr.pos.y=tp->top->topy;
	else p->scr.pos.y=p->scr.size.y;
	tp->bot=pl_rtdraw(back, r, tp->top);
	pl_drawnon(tp->top, tp->bot, p->b);
	pl_hiliteword(p, tp->hitword, back);
	lo=tp->top?tp->top->topy:tp->thgt;
	hi=tp->bot?tp->bot->topy:tp->thgt;	/* wrong! */
	sb=p->yscroller;
	if(back!=p->b){
		bitblt(p->b, back->r.min, back, back->r, S);
		bfree(back);
	}
	if(sb && sb->setscrollbar) sb->setscrollbar(sb, lo, hi, tp->thgt);
}
/*
 * If t is a panel word, pass the mouse event on to it
 */
void pl_passon(Rtext *t, Mouse *m){
	if(t && t->b==0 && t->p!=0) plmouse(t->p, *m);
}
int pl_hittextview(Panel *p, Mouse *m){
	Rtext *oldhitword;
	int hitme;
	Point ul, size;
	Textview *tp;
	tp=p->data;
	oldhitword=tp->hitword;
	hitme=0;
	pl_passon(oldhitword, m);
	if(m->buttons&OUT)
		p->state=UP;
	else if(m->buttons&7){
		tp->buttons=m->buttons;
		p->state=DOWN;
		ul=p->r.min;
		size=sub(p->r.max, p->r.min);
		pl_interior(p->state, &ul, &size);
		tp->hitword=pl_rthit(tp->top, m->xy, ul);
		if(tp->hitword!=0 && tp->hitword->hot==0) tp->hitword=0;
	}
	else{
		if(p->state==DOWN) hitme=1;
		p->state=UP;
	}
	if(tp->hitword!=oldhitword){
		pl_hiliteword(p, oldhitword, p->b);
		pl_hiliteword(p, tp->hitword, p->b);
		pl_passon(tp->hitword, m);
	}
	if(hitme && tp->hit && tp->hitword){
		pl_hiliteword(p, tp->hitword, p->b);
		if(tp->hitword->b!=0 || tp->hitword->p==0)
			tp->hit(p, tp->buttons, tp->hitword);
		tp->hitword=0;
	}
	return 0;
}
void pl_scrolltextview(Panel *p, int dir, int buttons, int num, int den){
	Rtext *top;
	Point ul, size;
	Textview *tp;
	if(dir!=VERT) return;
	tp=p->data;
	ul=p->r.min;
	size=sub(p->r.max, p->r.min);
	pl_interior(p->state, &ul, &size);
	top=pl_rtscroll(tp->text, tp->top, buttons, num*size.y/den, size.y);
	if(top!=tp->top){
		tp->top=top;
		pldraw(p, p->b);
	}
}
void pl_typetextview(Panel *g, Rune c){
	USED(g, c);
}
Point pl_getsizetextview(Panel *p, Point children){
	USED(children);
	return pl_boxsize(((Textview *)p->data)->minsize, p->state);
}
void pl_childspacetextview(Panel *g, Point *ul, Point *size){
	USED(g, ul, size);
}
void plinittextview(Panel *v, int flags, Point minsize, Rtext *t, void (*hit)(Panel *, int, Rtext *)){
	Textview *tp;
	tp=v->data;
	v->flags=flags|LEAF;
	v->state=UP;
	v->draw=pl_drawtextview;
	v->hit=pl_hittextview;
	v->type=pl_typetextview;
	v->getsize=pl_getsizetextview;
	v->childspace=pl_childspacetextview;
	tp->hit=hit;
	tp->minsize=minsize;
	tp->text=t;
	tp->top=t;
	tp->bot=0;
	tp->hitword=0;
	v->scroll=pl_scrolltextview;
	tp->twid=-1;
	v->scr.pos=Pt(0,0);
	v->scr.size=Pt(0,1);
}
Panel *pltextview(Panel *parent, int flags, Point minsize, Rtext *t, void (*hit)(Panel *, int, Rtext *)){
	Panel *v;
	v=pl_newpanel(parent, sizeof(Textview));
	plinittextview(v, flags, minsize, t, hit);
	return v;
}
Rtext *plgetpostextview(Panel *p){
	return ((Textview *)p->data)->top;
}
void plsetpostextview(Panel *p, Rtext *top){
	((Textview *)p->data)->top=top;
	pldraw(p, p->b);
}
