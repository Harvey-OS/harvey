#include <u.h>
#include <libc.h>
#include <libg.h>
#include <panel.h>
#include "pldefs.h"
void pl_drawgroup(Panel *p){
	USED(p);
}
int pl_hitgroup(Panel *p, Mouse *m){
	USED(p, m);
	return 0;
}
void pl_typegroup(Panel *p, Rune c){
	USED(p, c);
}
Point pl_getsizegroup(Panel *p, Point children){
	USED(p);
	return children;
}
void pl_childspacegroup(Panel *p, Point *ul, Point *size){
	USED(p, ul, size);
}
void plinitgroup(Panel *v, int flags){
	v->flags=flags;
	v->draw=pl_drawgroup;
	v->hit=pl_hitgroup;
	v->type=pl_typegroup;
	v->getsize=pl_getsizegroup;
	v->childspace=pl_childspacegroup;
	v->kind="group";
}
Panel *plgroup(Panel *parent, int flags){
	Panel *p;
	p=pl_newpanel(parent, 0);
	plinitgroup(p, flags);
	return p;
}
