/*
 * Hash tables.
 *
 * Keys and values are arbitrary pointer-sized pieces of data.
 * 
 * Hash tables are resized automatically as necessary, so that
 * an empty hash table doesn't take much memory but a large
 * hash table is still efficient.
 */

#include "a.h"

/*
 * Hash table sizes: powers of three.
 * Shouldn't be correlated with pointer values
 * (except on ternary machines).
 */
static int hashsize[] = {
 	9,
 	27,
 	81,
 	243,
 	729,
 	2187,
 	6561,
 	19683,
 	59049,
 	177147,
 	531441,
 	1594323,
 	4782969,
 	14348907,
 	43046721,
 	129140163,
 	387420489,
 	1162261467	/* (getting a little ridiculous...) */
};

typedef struct Xel Xel;
struct Xel
{
	Xel *next;
	void *key;
	void *value;
};

struct Hash
{
	int nel;		/* number of elements */
	Xel **tab;		/* hash chain heads */
	int size;		/* hashsize[size] heads in tab */
};

#define H(h, p)	((uintptr)p%hashsize[h->size])		/* hash value */

/* Allocate a hash table. */
Hash*
mkhash(void)
{
	Hash *h;
	
	h = mallocz(sizeof *h, 1);
	h->tab = mallocz(hashsize[h->size]*sizeof h->tab[0], 1);
	return h;
}

/* Allocate a hash table element. */
static Xel*
mkxel(void *key, void *value)
{
	Xel *x;

	x = malloc(sizeof *x);
	x->key = key;
	x->value = value;
	x->next = nil;
	return x;
}

/* Rehash the table to have size hashsize[size]. */
static void
rehash(Hash *h, int size)
{
	int i;
	Xel *all, *next, *x;
	
	/* Make list of all elements. */
	all = nil;
	for(i=0; i<hashsize[h->size]; i++){
		for(x=h->tab[i]; x; x=next){
			next = x->next;
			x->next = all;
			all = x;
		}
	}
	
	/* Resize. */
	h->size = size;
	free(h->tab);
	h->tab = mallocz(hashsize[h->size]*sizeof h->tab[0], 1);
	
	/* Put the elements back. */
	for(x=all; x; x=next){
		next = x->next;
		i = H(h, x->key);
		x->next = h->tab[i];
		h->tab[i] = x;
	}
}

/* Decide whether to rehash. */
static void
autorehash(Hash *h)
{
	int s;
	
	s = h->size;
	
	/* Grow if we've reached the next level. */
	while(s < nelem(hashsize) && h->nel > hashsize[s+1])
		s++;
	
	/* 
	 * Shrink if we've gotten way too small.
	 * (Can't happen because there is no way to 
	 * remove elements from the hash table yet.)
	 *
	while(s >= 2 && h->nel < hashsize[s-2])
		s--;
	 */
	
	if(s != h->size)
		rehash(h, s);
}

/*
 * Add key=value to the hash table. 
 * If value==nil, could remove the element.
 */
void*
hashput(Hash *h, void *key, void *value)
{
	uint i;
	Xel *e;
	void *old;
	
	assert(h != nil);

	i = H(h, key);
	for(e=h->tab[i]; e; e=e->next){
		if(e->key == key){
			old = e->value;
			e->value = value;
			return old;
		}
	}
	e = mkxel(key, value);
	e->next = h->tab[i];
	h->tab[i] = e;
	h->nel++;
	autorehash(h);
	return nil;
}

/* Retrieve value for key from the hash table. */
void*
hashget(Hash *h, void *key)
{
	uint i;
	Xel *e;

	/* Convenience: nil pointer is an empty hash table. */
	if(h == nil)
		return nil;

	i = H(h, key);
	for(e=h->tab[i]; e; e=e->next)
		if(e->key == key)
			return e->value;
	return nil;
}

List*
hashkeys(Hash *h)
{
	int i;
	Alist al;
	Xel *x;
	
	listreset(&al);
	for(i=0; i<hashsize[h->size]; i++)
		for(x=h->tab[i]; x; x=x->next)
			listadd(&al, x->key);
	return al.first;
}

/* Iterate through the hash table. */
int
hashiterstart(Hashiter *hi, Hash *h)
{
	if(h == nil)
		return 0;

	hi->i = 0;
	hi->j = 0;
	hi->h = h;
	return 1;
}

static Xel*
hashiternext(Hashiter *hi)
{
	int j;
	Xel *x;
	Hash *h;
	
	h = hi->h;
	for(;;){
		if(hi->i >= hashsize[hi->h->size])
			return nil;
		for(j=0, x=h->tab[hi->i]; j<hi->j && x!=nil; j++, x=x->next)
			;
		if(x){
			hi->j++;
			return x;
		}
		hi->j = 0;
		hi->i++;
	}
}

int
hashiternextkey(Hashiter *hi, void **v)
{
	Xel *x;
	
	x = hashiternext(hi);
	if(x == nil)
		return 0;
	*v = x->key;
	return 1;
}

int
hashiternextkv(Hashiter *hi, Hashkv *kv)
{
	Xel *x;
	
	x = hashiternext(hi);
	if(x == nil)
		return 0;
	kv->key = x->key;
	kv->value = x->value;
	return 1;
}

void
hashiterend(Hashiter *hi)
{
	USED(hi);
}
