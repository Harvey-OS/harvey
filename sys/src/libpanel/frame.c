#include <u.h>
#include <libc.h>
#include <libg.h>
#include <panel.h>
#include "pldefs.h"
void pl_drawframe(Panel *p){
	pl_box(p->b, p->r, FRAME);
}
int pl_hitframe(Panel *p, Mouse *m){
	USED(p, m);
	return 0;
}
void pl_typeframe(Panel *p, Rune c){
	USED(p, c);
}
Point pl_getsizeframe(Panel *p, Point children){
	USED(p);
	return pl_boxsize(children, FRAME);
}
void pl_childspaceframe(Panel *p, Point *ul, Point *size){
	USED(p);
	pl_interior(FRAME, ul, size);
}
void plinitframe(Panel *v, int flags){
	v->flags=flags;
	v->draw=pl_drawframe;
	v->hit=pl_hitframe;
	v->type=pl_typeframe;
	v->getsize=pl_getsizeframe;
	v->childspace=pl_childspaceframe;
	v->kind="frame";
}
Panel *plframe(Panel *parent, int flags){
	Panel *p;
	p=pl_newpanel(parent, 0);
	plinitframe(p, flags);
	return p;
}
