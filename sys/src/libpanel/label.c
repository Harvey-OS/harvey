#include <u.h>
#include <libc.h>
#include <libg.h>
#include <panel.h>
#include "pldefs.h"
typedef struct Label Label;
struct Label{
	int placement;
	Icon *icon;
};
void pl_drawlabel(Panel *p){
	Label *l;
	l=p->data;
	pl_drawicon(p->b, pl_box(p->b, p->r, PASSIVE), l->placement, p->flags, l->icon);
}
int pl_hitlabel(Panel *p, Mouse *m){
	USED(p, m);
	return 0;
}
void pl_typelabel(Panel *p, Rune c){
	USED(p, c);
}
Point pl_getsizelabel(Panel *p, Point children){
	USED(children);		/* shouldn't have any children */
	return pl_boxsize(pl_iconsize(p->flags, ((Label *)p->data)->icon), PASSIVE);
}
void pl_childspacelabel(Panel *g, Point *ul, Point *size){
	USED(g, ul, size);
}
void plinitlabel(Panel *v, int flags, Icon *icon){
	v->flags=flags|LEAF;
	((Label *)(v->data))->icon=icon;
	v->draw=pl_drawlabel;
	v->hit=pl_hitlabel;
	v->type=pl_typelabel;
	v->getsize=pl_getsizelabel;
	v->childspace=pl_childspacelabel;
	v->kind="label";
}
Panel *pllabel(Panel *parent, int flags, Icon *icon){
	Panel *p;
	p=pl_newpanel(parent, sizeof(Label));
	plinitlabel(p, flags, icon);
	plplacelabel(p, PLACECEN);
	return p;
}
void plplacelabel(Panel *p, int placement){
	((Label *)(p->data))->placement=placement;
}
