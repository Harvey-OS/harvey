/*
 * code to allocate and free items and itemlists
 */
#include "art.h"
#include <stdarg.h>
Item *freelist;
/*
 * common code to delete items, called from item-specific code
 */
delete(Item *p){
	(*p->fn->delete)(p);
	if(p->p){
		free(p->p);
		p->p=0;
	}
	p->next->prev=p->prev;
	p->prev->next=p->next;
	p->next=freelist;
	freelist=p;
}
Item *additem(Item *head, int type, Flt r, Typeface *face, char *text, int group, Itemfns *fn, int np, ...){
	va_list l;
	Item *ip;
	int i;
	if(freelist){
		ip=freelist;
		freelist=freelist->next;
	}
	else{
		ip=(Item *)malloc(sizeof(Item));
		if(ip==0) fatal("out of space");
	}
	if(head==selection) drawsel();
	if(head==0)
		ip->next=ip->prev=ip;
	else{
		ip->next=head;
		ip->prev=head->prev;
		ip->next->prev=ip;
		ip->prev->next=ip;
	}
	ip->np=np;
	if(np){
		ip->p=(Dpoint *)malloc(np*sizeof(Dpoint));
		if(ip->p==0) fatal("Out of space");
		va_start(l, np);
		for(i=0;i!=np;i++) ip->p[i]=va_arg(l, Dpoint);
		va_end(l);
	}
	else ip->p=0;
	ip->r=r;
	ip->face=face;
	ip->text=text?strdup(text):0;
	ip->group=group;
	ip->type=type;
	ip->flags=0;
	ip->style=0;		/* should be a parameter */
	ip->fn=fn;
	if(head==selection) drawsel();
	return ip;
}
Item *additemv(Item *head, int type, Flt r, Typeface *face, char *text, int group, Itemfns *fn, int np, Dpoint *p, Dpoint offs){
	Item *ip=additem(head, type, r, face, text, group, fn, 0);
	int i;
	if(np){
		ip->np=np;
		ip->p=(Dpoint *)malloc(np*sizeof(Dpoint));
		if(ip->p==0) fatal("Out of space");
		for(i=0;i!=np;i++) ip->p[i]=dadd(p[i], offs);
	}
	return ip;
}
