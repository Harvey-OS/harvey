#include <u.h>
#include <libc.h>
#include <libg.h>
#include <panel.h>
#include "pldefs.h"
typedef struct List List;
struct List{
	void (*hit)(Panel *, int, int);	/* call user back on hit */
	char *(*gen)(Panel *, int);	/* return text given index or 0 if out of range */
	int lo;				/* indices of first, last items displayed */
	int sel;			/* index of hilited item */
	int len;			/* # of items in list */
	Rectangle listr;
	Point minsize;
	int buttons;
};
#define	MAXHGT	12
void pl_listsel(Panel *p, int sel){
	List *lp;
	int hi;
	Rectangle r;
	lp=p->data;
	hi=lp->lo+(lp->listr.max.y-lp->listr.min.y)/font->height;
	if(lp->lo<=sel && sel<hi && sel<lp->len){
		r=lp->listr;
		r.min.y+=(sel-lp->lo)*font->height;
		r.max.y=r.min.y+font->height;
		bitblt(p->b, r.min, p->b, r, F&~D);
	}
}
void pl_liststrings(Panel *p, int lo, int hi, Rectangle r){
	Panel *sb;
	List *lp;
	char *s;
	int i;
	lp=p->data;
	for(i=lo;i!=hi && (s=lp->gen(p, i));i++){
		r.max.y=r.min.y+font->height;
		pl_drawicon(p->b, r, PLACEW, 0, s);
		r.min.y+=font->height;
	}
	if(lo<=lp->sel && lp->sel<hi) pl_listsel(p, lp->sel);
	sb=p->yscroller;
	if(sb && sb->setscrollbar)
		sb->setscrollbar(sb, lp->lo,
			lp->lo+(lp->listr.max.y-lp->listr.min.y)/font->height, lp->len);
}
void pl_drawlist(Panel *p){
	List *lp;
	lp=p->data;
	lp->listr=pl_box(p->b, p->r, UP);
	pl_liststrings(p, lp->lo, lp->lo+(lp->listr.max.y-lp->listr.min.y)/font->height,
		lp->listr);
}
int pl_hitlist(Panel *p, Mouse *m){
	int oldsel, hitme;
	Point ul, size;
	List *lp;
	lp=p->data;
	hitme=0;
	ul=p->r.min;
	size=sub(p->r.max, p->r.min);
	pl_interior(p->state, &ul, &size);
	oldsel=lp->sel;
	if(m->buttons&OUT){
		p->state=UP;
		if(m->buttons&~OUT) lp->sel=-1;
	}
	else if(p->state==DOWN || m->buttons&7){
		lp->sel=(m->xy.y-ul.y)/font->height+lp->lo;
		if(m->buttons&7){
			lp->buttons=m->buttons;
			p->state=DOWN;
		}
		else{
			hitme=1;
			p->state=UP;
		}
	}
	if(oldsel!=lp->sel){
		pl_listsel(p, oldsel);
		pl_listsel(p, lp->sel);
	}
	if(hitme && 0<=lp->sel && lp->sel<lp->len && lp->hit)
		lp->hit(p, lp->buttons, lp->sel);
	return 0;
}
void pl_scrolllist(Panel *p, int dir, int buttons, int val, int len){
	Point ul, size;
	int nlist, oldlo, hi, nline, y;
	List *lp;
	Rectangle r;
	lp=p->data;
	ul=p->r.min;
	size=sub(p->r.max, p->r.min);
	pl_interior(p->state, &ul, &size);
	nlist=size.y/font->height;
	oldlo=lp->lo;
	if(dir==VERT) switch(buttons){
	case 1: lp->lo-=nlist*val/len; break;
	case 2: lp->lo=lp->len*val/len; break;
	case 4:	lp->lo+=nlist*val/len; break;
	}
	if(lp->lo<0) lp->lo=0;
	if(lp->lo>=lp->len) lp->lo=lp->len-1;
	if(lp->lo==oldlo) return;
	p->scr.pos.y=lp->lo;
	r=lp->listr;
	nline=(r.max.y-r.min.y)/font->height;
	hi=lp->lo+nline;
	if(hi<=oldlo || lp->lo>=oldlo+nline){
		pl_box(p->b, r, PASSIVE);
		pl_liststrings(p, lp->lo, hi, r);
	}
	else if(lp->lo<oldlo){
		y=r.min.y+(oldlo-lp->lo)*font->height;
		bitblt(p->b, Pt(r.min.x, y), p->b,
			Rect(r.min.x, r.min.y, r.max.x, r.min.y+(hi-oldlo)*font->height),
			S);
		r.max.y=y;
		pl_box(p->b, r, PASSIVE);
		pl_liststrings(p, lp->lo, oldlo, r);
	}
	else{
		bitblt(p->b, r.min, p->b, Rect(r.min.x, r.min.y+(lp->lo-oldlo)*font->height,
			r.max.x, r.max.y), S);
		r.min.y=r.min.y+(oldlo+nline-lp->lo)*font->height;
		pl_box(p->b, r, PASSIVE);
		pl_liststrings(p, oldlo+nline, hi, r);
	}
}
void pl_typelist(Panel *g, Rune c){
	USED(g, c);
}
Point pl_getsizelist(Panel *p, Point children){
	USED(children);
	return pl_boxsize(((List *)p->data)->minsize, p->state);
}
void pl_childspacelist(Panel *g, Point *ul, Point *size){
	USED(g, ul, size);
}
void plinitlist(Panel *v, int flags, char *(*gen)(Panel *, int), int nlist, void (*hit)(Panel *, int, int)){
	List *lp;
	int wid, max;
	char *str;
	lp=v->data;
	v->flags=flags|LEAF;
	v->state=UP;
	v->draw=pl_drawlist;
	v->hit=pl_hitlist;
	v->type=pl_typelist;
	v->getsize=pl_getsizelist;
	v->childspace=pl_childspacelist;
	lp->gen=gen;
	lp->hit=hit;
	max=0;
	for(lp->len=0;str=gen(v, lp->len);lp->len++){
		wid=strwidth(font, str);
		if(wid>max) max=wid;
	}
	if(flags&EXPAND){
		for(lp->len=0;gen(v, lp->len);lp->len++);
		lp->minsize=Pt(0, nlist*font->height);
	}
	else{
		max=0;
		for(lp->len=0;str=gen(v, lp->len);lp->len++){
			wid=strwidth(font, str);
			if(wid>max) max=wid;
		}
		lp->minsize=Pt(max, nlist*font->height);
	}
	lp->sel=-1;
	lp->lo=0;
	v->scroll=pl_scrolllist;
	v->scr.pos=Pt(0,0);
	v->scr.size=Pt(0,lp->len);
	v->kind="list";
}
Panel *pllist(Panel *parent, int flags, char *(*gen)(Panel *, int), int nlist, void (*hit)(Panel *, int, int)){
	Panel *v;
	v=pl_newpanel(parent, sizeof(List));
	plinitlist(v, flags, gen, nlist, hit);
	return v;
}
