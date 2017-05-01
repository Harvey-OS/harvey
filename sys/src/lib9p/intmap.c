/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>

enum {
	NHASH = 128
};

typedef struct Intlist	Intlist;
struct Intlist
{
	uint32_t	id;
	void*	aux;
	Intlist*	link;
};

struct Intmap
{
	RWLock RWLock;
	Intlist*	hash[NHASH];
	void (*inc)(void*);
};

static uint32_t
hashid(uint32_t id)
{
	return id%NHASH;
}

static void
nop(void* v)
{
}

Intmap*
allocmap(void (*inc)(void*))
{
	Intmap *m;

	m = emalloc9p(sizeof(*m));
	if(inc == nil)
		inc = nop;
	m->inc = inc;
	return m;
}

void
freemap(Intmap *map, void (*destroy)(void*))
{
	int i;
	Intlist *p, *nlink;

	if(destroy == nil)
		destroy = nop;
	for(i=0; i<NHASH; i++){
		for(p=map->hash[i]; p; p=nlink){
			nlink = p->link;
			destroy(p->aux);
			free(p);
		}
	}

	free(map);
}

static Intlist**
llookup(Intmap *map, uint32_t id)
{
	Intlist **lf;

	for(lf=&map->hash[hashid(id)]; *lf; lf=&(*lf)->link)
		if((*lf)->id == id)
			break;
	return lf;
}

/*
 * The RWlock is used as expected except that we allow
 * inc() to be called while holding it.  This is because we're
 * locking changes to the tree structure, not to the references.
 * Inc() is expected to have its own locking.
 */
void*
lookupkey(Intmap *map, uint32_t id)
{
	Intlist *f;
	void *v;

	rlock(&map->RWLock);
	if((f = *llookup(map, id)) != nil){
		v = f->aux;
		map->inc(v);
	}else
		v = nil;
	runlock(&map->RWLock);
	return v;
}

void*
insertkey(Intmap *map, uint32_t id, void *v)
{
	Intlist *f;
	void *ov;
	uint32_t h;

	wlock(&map->RWLock);
	if((f = *llookup(map, id)) != nil){
		/* no decrement for ov because we're returning it */
		ov = f->aux;
		f->aux = v;
	}else{
		f = emalloc9p(sizeof(*f));
		f->id = id;
		f->aux = v;
		h = hashid(id);
		f->link = map->hash[h];
		map->hash[h] = f;
		ov = nil;
	}
	wunlock(&map->RWLock);
	return ov;
}

int
caninsertkey(Intmap *map, uint32_t id, void *v)
{
	Intlist *f;
	int rv;
	uint32_t h;

	wlock(&map->RWLock);
	if(*llookup(map, id))
		rv = 0;
	else{
		f = emalloc9p(sizeof *f);
		f->id = id;
		f->aux = v;
		h = hashid(id);
		f->link = map->hash[h];
		map->hash[h] = f;
		rv = 1;
	}
	wunlock(&map->RWLock);
	return rv;
}

void*
deletekey(Intmap *map, uint32_t id)
{
	Intlist **lf, *f;
	void *ov;

	wlock(&map->RWLock);
	if((f = *(lf = llookup(map, id))) != nil){
		ov = f->aux;
		*lf = f->link;
		free(f);
	}else
		ov = nil;
	wunlock(&map->RWLock);
	return ov;
}
