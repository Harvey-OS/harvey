#include <u.h>
#include <libc.h>
#include <libg.h>
#include <panel.h>
#include "pldefs.h"
typedef struct Button Button;
struct Button{
	int btype;			/* button type */
	Icon *icon;			/* what to write on the button */
	int check;			/* for check/radio buttons */
	void (*hit)(Panel *, int, int);	/* call back user code on check/radio hit */
	void (*menuhit)(int, int);	/* call back user code on menu item hit */
	void (*pl_buttonhit)(Panel *, int);	/* call back user code on button hit */
	int index;			/* arg to menuhit */
	int buttons;
};
/*
 * Button types
 */
#define	BUTTON	1
#define	CHECK	2
#define	RADIO	3
void pl_drawbutton(Panel *p){
	Rectangle r;
	Button *bp;
	bp=p->data;
	r=pl_box(p->b, p->r, p->state);
	switch(bp->btype){
	case CHECK:
		r=pl_check(p->b, r, bp->check);
		break;
	case RADIO:
		r=pl_radio(p->b, r, bp->check);
		break;
	}
	pl_drawicon(p->b, r, PLACECEN, p->flags, bp->icon);
}
int pl_hitbutton(Panel *p, Mouse *m){
	int oldstate, hitme;
	Panel *sib;
	Button *bp;
	bp=p->data;
	oldstate=p->state;
	if(m->buttons&OUT){
		hitme=0;
		p->state=UP;
	}
	else if(m->buttons&7){
		hitme=0;
		p->state=DOWN;
		bp->buttons=m->buttons;
	}
	else{	/* mouse inside, but no buttons down */
		hitme=p->state==DOWN;
		p->state=UP;
	}
	if(hitme) switch(bp->btype){
	case CHECK:
		if(hitme) bp->check=!bp->check;
		break;
	case RADIO:
		if(bp->check) bp->check=0;
		else{
			if(p->parent){
				for(sib=p->parent->child;sib;sib=sib->next){
					if(((Button *)sib->data)->btype==RADIO
					&& ((Button *)sib->data)->check){
						((Button *)sib->data)->check=0;
						pldraw(sib, p->b);
					}
				}
			}
			bp->check=1;
		}
		break;
	}
	if(hitme || oldstate!=p->state) pldraw(p, p->b);
	if(hitme && bp->hit){
		bp->hit(p, bp->buttons, bp->check);
		p->state=UP;
	}
	return 0;
}
void pl_typebutton(Panel *g, Rune c){
	USED(g, c);
}
Point pl_getsizebutton(Panel *p, Point children){
	Point s;
	int ckw;
	Button *bp;
	USED(children);		/* shouldn't have any children */
	bp=p->data;
	s=pl_iconsize(p->flags, bp->icon);
	if(bp->btype!=BUTTON){
		ckw=pl_ckwid();
		if(s.y<ckw){
			s.x+=ckw;
			s.y=ckw;
		}
		else s.x+=s.y;
	}
	return pl_boxsize(s, p->state);
}
void pl_childspacebutton(Panel *g, Point *ul, Point *size){
	USED(g, ul, size);
}
void pl_initbtype(Panel *v, int flags, Icon *icon, void (*hit)(Panel *, int, int), int btype){
	Button *bp;
	bp=v->data;
	v->flags=flags|LEAF;
	v->state=UP;
	v->draw=pl_drawbutton;
	v->hit=pl_hitbutton;
	v->type=pl_typebutton;
	v->getsize=pl_getsizebutton;
	v->childspace=pl_childspacebutton;
	bp->btype=btype;
	bp->check=0;
	bp->hit=hit;
	bp->icon=icon;
	switch(btype){
	case BUTTON: v->kind="button"; break;
	case CHECK:  v->kind="checkbutton"; break;
	case RADIO:  v->kind="radiobutton"; break;
	}
}
void pl_buttonhit(Panel *p, int buttons, int check){
	USED(check);
	if(((Button *)p->data)->pl_buttonhit) ((Button *)p->data)->pl_buttonhit(p, buttons);
}
void plinitbutton(Panel *p, int flags, Icon *icon, void (*hit)(Panel *, int)){
	((Button *)p->data)->pl_buttonhit=hit;
	pl_initbtype(p, flags, icon, pl_buttonhit, BUTTON);
}
void plinitcheckbutton(Panel *p, int flags, Icon *icon, void (*hit)(Panel *, int, int)){
	pl_initbtype(p, flags, icon, hit, CHECK);
}
void plinitradiobutton(Panel *p, int flags, Icon *icon, void (*hit)(Panel *, int, int)){
	pl_initbtype(p, flags, icon, hit, RADIO);
}
Panel *plbutton(Panel *parent, int flags, Icon *icon, void (*hit)(Panel *, int)){
	Panel *p;
	p=pl_newpanel(parent, sizeof(Button));
	plinitbutton(p, flags, icon, hit);
	return p;
}
Panel *plcheckbutton(Panel *parent, int flags, Icon *icon, void (*hit)(Panel *, int, int)){
	Panel *p;
	p=pl_newpanel(parent, sizeof(Button));
	plinitcheckbutton(p, flags, icon, hit);
	return p;
}
Panel *plradiobutton(Panel *parent, int flags, Icon *icon, void (*hit)(Panel *, int, int)){
	Panel *p;
	p=pl_newpanel(parent, sizeof(Button));
	plinitradiobutton(p, flags, icon, hit);
	return p;
}
void pl_hitmenu(Panel *p, int buttons){
	((Button *)p->data)->menuhit(buttons, ((Button *)p->data)->index);
}
void plinitmenu(Panel *v, int flags, Icon **item, int cflags, void (*hit)(int, int)){
	Panel *b;
	int i;
	v->flags=flags;
	v->kind="menu";
	if(v->child){
		plfree(v->child);
		v->child=0;
	}
	for(i=0;item[i];i++){
		b=plbutton(v, cflags, item[i], pl_hitmenu);
		((Button *)b->data)->menuhit=hit;
		((Button *)b->data)->index=i;
	}
}
Panel *plmenu(Panel *parent, int flags, Icon **item, int cflags, void (*hit)(int, int)){
	Panel *v;
	v=plgroup(parent, flags);
	plinitmenu(v, flags, item, cflags, hit);
	return v;
}
void plsetbutton(Panel *p, int val){
	((Button *)p->data)->check=val;
}
