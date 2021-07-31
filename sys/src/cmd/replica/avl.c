#include "all.h"

/*
 * In-memory database stored as self-balancing AVL tree.
 * See Lewis & Denenberg, Data Structures and Their Algorithms.
 */
static void
singleleft(Avl **tp, Avl *p)
{
	Avl *a, *c;
	int l, r2;

	a = *tp;
	c = a->n[1];

	r2 = c->bal;
	l = (r2 > 0 ? r2 : 0)+1 - a->bal;

	if((a->n[1] = c->n[0]) != nil)
		a->n[1]->p = a;

	if((c->n[0] = a) != nil)
		c->n[0]->p = c;

	if((*tp = c) != nil)
		(*tp)->p = p;

	a->bal = -l;
	c->bal = r2 - ((l > 0 ? l : 0)+1);

}

static void
singleright(Avl **tp, Avl *p)
{
	Avl *a, *c;
	int l2, r;

	a = *tp;
	c = a->n[0];
	l2 = - c->bal;
	r = a->bal + ((l2 > 0 ? l2 : 0)+1);

	if((a->n[0] = c->n[1]) != nil)
		a->n[0]->p = a;

	if((c->n[1] = a) != nil)
		c->n[1]->p = c;

	if((*tp = c) != nil)
		(*tp)->p = p;

	a->bal = r;
	c->bal = ((r > 0 ? r : 0)+1) - l2;
}

static void
doublerightleft(Avl **tp, Avl *p)
{
	singleright(&(*tp)->n[1], *tp);
	singleleft(tp, p);
}

static void
doubleleftright(Avl **tp, Avl *p)
{
	singleleft(&(*tp)->n[0], *tp);
	singleright(tp, p);
}

static void
balance(Avl **tp, Avl *p)
{
	switch((*tp)->bal){
	case -2:
		if((*tp)->n[0]->bal <= 0)
			singleright(tp, p);
		else if((*tp)->n[0]->bal == 1)
			doubleleftright(tp, p);
		else
			assert(0);
		break;

	case 2:
		if((*tp)->n[1]->bal >= 0)
			singleleft(tp, p);
		else if((*tp)->n[1]->bal == -1)
			doublerightleft(tp, p);
		else
			assert(0);
		break;
	}
}

static int
_insertavl(Avl **tp, Avl *p, Avl *r, int (*cmp)(Avl*,Avl*), Avl **rfree)
{
	int i, ob;

	if(*tp == nil){
		r->bal = 0;
		r->n[0] = nil;
		r->n[1] = nil;
		r->p = p;
		*tp = r;
		return 1;
	}
	ob = (*tp)->bal;
	if((i=cmp(r, *tp)) != 0){
		(*tp)->bal += i*_insertavl(&(*tp)->n[(i+1)/2], *tp, r, cmp, rfree);
		balance(tp, p);
		return ob==0 && (*tp)->bal != 0;
	}

	/* install new entry */
	*rfree = *tp;	/* save old node for freeing */
	*tp = r;		/* insert new node */
	**tp = **rfree;	/* copy old node's Avl contents */
	if(r->n[0])		/* fix node's children's parent pointers */
		r->n[0]->p = r;
	if(r->n[1])
		r->n[1]->p = r;

	return 0;
}

static Avl*
_lookupavl(Avl *t, Avl *r, int (*cmp)(Avl*,Avl*))
{
	int i;
	Avl *p;

	p = nil;
	while(t != nil){
		assert(t->p == p);
		if((i=cmp(r, t))==0)
			return t;
		p = t;
		t = t->n[(i+1)/2];
	}
	return nil;
}

static int
successor(Avl **tp, Avl *p, Avl **r)
{
	int ob;

	if((*tp)->n[0] == nil){
		*r = *tp;
		*tp = (*r)->n[1];
		if(*tp)
			(*tp)->p = p;
		return -1;
	}
	ob = (*tp)->bal;
	(*tp)->bal -= successor(&(*tp)->n[0], *tp, r);
	balance(tp, p);
	return -(ob!=0 && (*tp)->bal==0);
}

static int
_deleteavl(Avl **tp, Avl *p, Avl *rx, int(*cmp)(Avl*,Avl*), Avl **del, void (*predel)(Avl*, void*), void *arg)
{
	int i, ob;
	Avl *r, *or;

	if(*tp == nil)
		return 0;

	ob = (*tp)->bal;
	if((i=cmp(rx, *tp)) != 0){
		(*tp)->bal += i*_deleteavl(&(*tp)->n[(i+1)/2], *tp, rx, cmp, del, predel, arg);
		balance(tp, p);
		return -(ob!=0 && (*tp)->bal==0);
	}

	if(predel)
		(*predel)(*tp, arg);

	or = *tp;
	if(or->n[i=0]==nil || or->n[i=1]==nil){
		*tp = or->n[1-i];
		if(*tp)
			(*tp)->p = p;
		*del = or;
		return -1;
	}

	/* deleting node with two kids, find successor */
	or->bal += successor(&or->n[1], or, &r);
	r->bal = or->bal;
	r->n[0] = or->n[0];
	r->n[1] = or->n[1];
	*tp = r;
	(*tp)->p = p;
	/* node has changed; fix children's parent pointers */
	if(r->n[0])
		r->n[0]->p = r;
	if(r->n[1])
		r->n[1]->p = r;
	*del = or;
	balance(tp, p);
	return -(ob!=0 && (*tp)->bal==0);
}

static void
checkparents(Avl *a, Avl *p)
{
	if(a==nil)
		return;
	if(a->p != p)
		print("bad parent\n");
	checkparents(a->n[0], a);
	checkparents(a->n[1], a);
}

struct Avltree
{
	Avl *root;
	int (*cmp)(Avl*, Avl*);
	Avlwalk *walks;
};
struct Avlwalk
{
	int started;
	Avlwalk *next;
	Avltree *tree;
	Avl *node;
};


Avltree*
mkavltree(int (*cmp)(Avl*, Avl*))
{
	Avltree *t;

	t = emalloc(sizeof(*t));
	t->cmp = cmp;
	return t;
}

void
insertavl(Avltree *t, Avl *new, Avl **oldp)
{
	*oldp = nil;
	_insertavl(&t->root, nil, new, t->cmp, oldp);
}

Avl*
lookupavl(Avltree *t, Avl *key)
{
	return _lookupavl(t->root, key, t->cmp);
}

static Avl*
findpredecessor(Avl *a)
{
	Avl *last;

	last = nil;
	while(a && a->n[0]==last){
		last = a;
		a = a->p;
	}
	if(a == nil)
		return a;
	if(a->n[1]==last)
		return a;
	a = a->n[0];
	while(a && a->n[1])
		a = a->n[1];
	return a;
}		

static Avl*
findsuccessor(Avl *a, int debug)
{
	Avl *last;

	last = nil;
	if(debug)
		fprint(2, "succ %p(%p,%p,%p)", a, !a?0:a->p, !a?0:a->n[0], !a?0:a->n[1]);
	while(a && a->n[1]==last){
		last = a;
		a = a->p;
		if(debug)
			fprint(2, " u %p(%p,%p,%p)", a, !a?0:a->p, !a?0:a->n[0], !a?0:a->n[1]);
	}
	if(a == nil){
		if(debug)
			fprint(2, "\n");
		return a;
	}
	if(last!=nil && a->n[0]==last){
		if(debug)
			fprint(2, "\n");
		return a;
	}
	a = a->n[1];
	if(debug)
		fprint(2, " dr %p(%p,%p,%p)", a, !a?0:a->p, !a?0:a->n[0], !a?0:a->n[1]);
	while(a && a->n[0]){
		a = a->n[0];
		if(debug)
			fprint(2, " dl %p(%p,%p,%p)", a, !a?0:a->p, !a?0:a->n[0], !a?0:a->n[1]);
	}
	if(debug)
		fprint(2, "\n");
	return a;
}

static void
walkdel(Avl *a, void *v)
{
	Avl *p;
	Avlwalk *w;
	Avltree *t;

	if(a == nil)
		return;

	p = findpredecessor(a);
	t = v;
	for(w=t->walks; w; w=w->next){
		if(w->node == a){
			/* back pointer to predecessor; not perfect but adequate */
			w->node = p;
			if(p == nil)
				w->started = 0;
		}
	}
}

void
deleteavl(Avltree *t, Avl *key, Avl **oldp)
{
	*oldp = nil;
	_deleteavl(&t->root, nil, key, t->cmp, oldp, walkdel, t);
}

Avlwalk*
avlwalk(Avltree *t)
{
	Avlwalk *w;

	w = emalloc(sizeof(*w));
	w->tree = t;
	w->next = t->walks;
	t->walks = w;
	return w;
}

Avl*
avlnext(Avlwalk *w)
{
	Avl *a;

	if(w->started==0){
		for(a=w->tree->root; a && a->n[0]; a=a->n[0])
			;
		w->node = a;
		w->started = 1;
	}else{
		a = findsuccessor(w->node, 0);
		if(a == w->node){
			findsuccessor(w->node, 1);
			abort();
		}
		w->node = a;
	}
	return w->node;
}

void
endwalk(Avlwalk *w)
{
	Avltree *t;
	Avlwalk **l;

	t = w->tree;
	for(l=&t->walks; *l; l=&(*l)->next){
		if(*l == w){
			*l = w->next;
			break;
		}
	}
	free(w);
}

static void
walkavl(Avl *t, void (*f)(Avl*, void*), void *v)
{
	if(t == nil)
		return;
	walkavl(t->n[0], f, v);
	f(t, v);
	walkavl(t->n[1], f, v);
}

