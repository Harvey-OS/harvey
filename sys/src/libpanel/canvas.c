#include <u.h>
#include <libc.h>
#include <libg.h>
#include <panel.h>
#include "pldefs.h"
typedef struct Canvas Canvas;
struct Canvas{
	void (*draw)(Panel *);
	void (*hit)(Panel *, Mouse *);
};
void pl_drawcanvas(Panel *p){
	Canvas *c;
	c=p->data;
	if(c->draw) c->draw(p);
}
int pl_hitcanvas(Panel *p, Mouse *m){
	Canvas *c;
	c=p->data;
	if(c->hit) c->hit(p, m);
	return 0;
}
void pl_typecanvas(Panel *p, Rune c){
	USED(p, c);
}
Point pl_getsizecanvas(Panel *p, Point children){
	USED(p, children);
	return Pt(0,0);
}
void pl_childspacecanvas(Panel *p, Point *ul, Point *size){
	USED(p, ul, size);
}
void plinitcanvas(Panel *v, int flags, void (*draw)(Panel *), void (*hit)(Panel *, Mouse *)){
	Canvas *c;
	v->flags=flags|LEAF;
	v->draw=pl_drawcanvas;
	v->hit=pl_hitcanvas;
	v->type=pl_typecanvas;
	v->getsize=pl_getsizecanvas;
	v->childspace=pl_childspacecanvas;
	v->kind="canvas";
	c=v->data;
	c->draw=draw;
	c->hit=hit;
}
Panel *plcanvas(Panel *parent, int flags, void (*draw)(Panel *), void (*hit)(Panel *, Mouse *)){
	Panel *p;
	p=pl_newpanel(parent, sizeof(Canvas));
	plinitcanvas(p, flags, draw, hit);
	return p;
}
