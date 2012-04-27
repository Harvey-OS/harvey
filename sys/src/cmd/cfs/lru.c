/*
 *  lru lists are circular with a list head
 *  pointing to the start and end of the list
 */
#include <u.h>
#include "lru.h"

/*
 *  Create an lru chain of buffers
 */
void
lruinit(Lru *h)
{
	h->lprev = h->lnext = h;
}

/*
 *  Add a member to an lru chain
 */
void
lruadd(Lru *h, Lru *m)
{
	h->lprev->lnext = m;
	m->lprev = h->lprev;
	h->lprev = m;
	m->lnext = h;
}

/*
 *  Move to end of lru list
 */
void
lruref(Lru *h, Lru *m)
{
	if(h->lprev == m)
		return;		/* alread at end of list */

	/*
	 *  remove from list
	 */
	m->lprev->lnext = m->lnext;
	m->lnext->lprev = m->lprev;

	/*
	 *  add in at end
	 */
	h->lprev->lnext = m;
	m->lprev = h->lprev;
	h->lprev = m;
	m->lnext = h;
}

/*
 *  Move to head of lru list
 */
void
lruderef(Lru *h, Lru *m)
{
	if(h->lnext == m)
		return;		/* alread at head of list */

	/*
	 *  remove from list
	 */
	m->lprev->lnext = m->lnext;
	m->lnext->lprev = m->lprev;

	/*
	 *  add in at head
	 */
	h->lnext->lprev = m;
	m->lnext = h->lnext;
	h->lnext = m;
	m->lprev = h;
}
