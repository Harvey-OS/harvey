#include <u.h>
#include <libc.h>
#include <libg.h>
#include <panel.h>
#include "pldefs.h"
void *pl_emalloc(int n){
	void *v;
	v=malloc(n);
	if(v==0){
		fprint(2, "Can't malloc!\n");
		exits("no mem");
	}
	return v;
}
void pl_unexpected(Panel *g, char *rou){
	fprint(2, "%s called unexpectedly (%s %8x)\n", rou, g->kind, g);
	abort();
}
void pl_drawerror(Panel *g){
	pl_unexpected(g, "draw");
}
int pl_hiterror(Panel *g, Mouse *m){
	USED(m);
	pl_unexpected(g, "hit");
	return 0;
}
void pl_typeerror(Panel *g, Rune c){
	USED(c);
	pl_unexpected(g, "type");
}
Point pl_getsizeerror(Panel *g, Point childsize){
	pl_unexpected(g, "getsize");
	return childsize;
}
void pl_childspaceerror(Panel *g, Point *ul, Point *size){
	USED(ul, size);
	pl_unexpected(g, "childspace");
}
void pl_scrollerror(Panel *g, int dir, int button, int num, int den){
	USED(dir, button, num, den);
	pl_unexpected(g, "scroll");
}
void pl_setscrollbarerror(Panel *g, int top, int bot, int den){
	USED(top, bot, den);
	pl_unexpected(g, "setscrollbar");
}
Panel *pl_newpanel(Panel *parent, int ndata){
	Panel *v;
	if(parent && parent->flags&LEAF){
		fprint(2, "newpanel: can't create child of %s %x\n", parent->kind, parent);
		exits("bad newpanel");
	}
	v=pl_emalloc(sizeof(Panel));
	v->r=Rect(0,0,0,0);
	v->flags=0;
	v->ipad=Pt(0,0);
	v->pad=Pt(0,0);
	v->size=Pt(0,0);
	v->sizereq=Pt(0,0);
	v->lastmouse=0;
	v->next=0;
	v->child=0;
	v->echild=0;
	v->b=0;
	v->scrollee=0;
	v->xscroller=0;
	v->yscroller=0;
	v->parent=parent;
	v->scr.pos=Pt(0,0);
	v->scr.size=Pt(0,0);
	if(parent){
		if(parent->child==0)
			parent->child=v;
		else
			parent->echild->next=v;
		parent->echild=v;
	}
	v->draw=pl_drawerror;
	v->hit=pl_hiterror;
	v->type=pl_typeerror;
	v->getsize=pl_getsizeerror;
	v->childspace=pl_childspaceerror;
	v->scroll=pl_scrollerror;
	v->setscrollbar=pl_setscrollbarerror;
	v->free=0;
	if(ndata)
		v->data=pl_emalloc(ndata);
	else
		v->data=0;
	return v;
}
void plfree(Panel *p){
	Panel *cp, *ncp;
	for(cp=p->child;cp;cp=ncp){
		ncp=cp->next;
		plfree(cp);
	}
	if(p->free) p->free(p);
	if(p->data) free(p->data);
	free(p);
}
