#include "a.h"

List*
mklist(void *hd, List *tl)
{
	List *l;

	l = malloc(sizeof(*l));
	l->hd = hd;
	l->tl = tl;
	return l;
}

void
listadd(Alist *xx, void *v)
{
	List *l;

	l = mklist(v, nil);
	if(xx->first == nil)
		xx->first = l;
	else
		xx->last->tl = l;
	xx->last = l;
}

int
listlen(List *l)
{
	int n;

	n = 0;
	for(; l; l=l->tl)
		n++;
	return n;
}

void**
listflatten(List *l)
{
	void **v;
	int n;

	n = listlen(l);
	v = malloc(n*sizeof(v[0]));
	n = 0;
	for(; l; l=l->tl)
		v[n++] = l->hd;
	return v;
}

void
listaconcat(Alist *al, Alist *l)
{
	if(l->first == nil)
		return;

	if(al->first == nil)
		al->first = l->first;
	else
		al->last->tl = l->first;
	al->last = l->last;
}

void
listconcat(Alist *al, List *l)
{
	if(l == nil)
		return;

	if(al->first == nil)
		al->first = l;
	else
		al->last->tl = l;
	for(; l && l->tl; l=l->tl)
		;
	al->last = l;
}

int
listeq(List *l1, List *l2)
{
	while(l1 && l2){
		if(l1->hd != l2->hd)
			return 0;
		l1 = l1->tl;
		l2 = l2->tl;
	}
	if(l1 != l2)
		return 0;
	return 1;
}

List*
listcopy(List *l)
{
	Alist al;

	memset(&al, 0, sizeof al);
	for(; l; l=l->tl)
		listadd(&al, l->hd);
	return al.first;
}

void
listreset(Alist *al)
{
	al->first = al->last = nil;
}

List*
listreverse(List *l)
{
	List *last, *next;

	last = nil;
	for(; l; last=l, l=next){
		next = l->tl;
		l->tl = last;
	}
	return last;
}

void
listtoalist(List *l, Alist *al)
{
	List *last;

	al->first = l;
	for(last=l; l; last=l, l=l->tl)
		;
	al->last = last;
}

static List*
merge(List *l1, List *l2, int (*cmp)(void*,void*))
{
	int i;
	List lhead, *l;

	l = &lhead;
	while(l1 || l2){
		if(l1 == nil)
			i = 1;
		else if(l2 == nil)
			i = -1;
		else
			i = (*cmp)(l1->hd, l2->hd);
		if(i <= 0){
			l->tl = l1;
			l1 = l1->tl;
		}else{
			l->tl = l2;
			l2 = l2->tl;
		}
		l = l->tl;
	}
	l->tl = nil;
	return lhead.tl;
}

List*
listsort(List *l, int (*cmp)(void*,void*))
{
	List *l1, *l2;
	List l1head, l2head;

	if(l == nil)
		return nil;

	if(l->tl == nil)
		return l;

	l1 = &l1head;
	l2 = &l2head;
	while(l){
		l1->tl = l;
		l1 = l;
		if(l = l->tl){
			l2->tl = l;
			l2 = l;
			l = l->tl;
		}
	}
	l1->tl = nil;
	l2->tl = nil;

	l1 = listsort(l1head.tl, cmp);
	l2 = listsort(l2head.tl, cmp);

	return merge(l1, l2, cmp);
}

int
listiterstart(List **state, List *l)
{
	*state = l;
	return 1;
}

int
listiternext(List **state, void **v)
{
	if(*state == nil)
		return 0;
	*v = (*state)->hd;
	*state = (*state)->tl;
	return 1;
}

void
listiterend(List **state)
{
	USED(state);
}

