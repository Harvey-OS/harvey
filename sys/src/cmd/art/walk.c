#include "art.h"
/*
 * walk a display list, calling f with a transformed version of each
 * primitive node.  tx is the transformation currently in effect.
 */
void walk(Item *p, Dpoint tx, void (*f)(Item *, Item *)){
	if(p==0) return;
	if(p->type==HEAD){
		for(;;){
			p=p->next;
			if(p->type==HEAD) break;
			walk1(p, tx, f, p);
		}
	}
	else walk1(p, tx, f, p);
}
void walk1(Item *p, Dpoint tx, void (*f)(Item *, Item *), Item *top){
	Item i;
	Dpoint pt[200];	/* wrong!!! secret unchecked limit */
	int n;
	if(p->type==HEAD){
		for(;;){
			p=p->next;
			if(p->type==HEAD) break;
			walk1(p, tx, f, top);
		}
	}
	else if(p->type==GROUP){
		if(!(p->flags&INVIS)		/* could be wrong, but makes draw() work */
		&& goodgroup(p->group))
			walk1(group[p->group], dadd(tx, p->p[0]), f, top);
	}
	else{
		i=*p;
		i.p=pt;
		for(n=0;n!=i.np;n++) i.p[n]=dadd(p->p[n], tx);
		(*f)(&i, top);
	}
}
void mwalk(Item *p, Dpoint tx, void (*f)(Item *, Item *)){
	if(p==0) return;
	if(p->type==HEAD){
		for(;;){
			p=p->next;
			if(p->type==HEAD) break;
			if(!(p->flags&MOVING)) walk1(p, tx, f, p);
		}
	}
	else if(!(p->flags&MOVING)) walk1(p, tx, f, p);
}
