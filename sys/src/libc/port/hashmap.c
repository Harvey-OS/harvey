/*
 *	NB: The two least significant bits of key are used for
 *	book keeping, meaning that those input bits are ignored
 *	by all lookups and inserts.
 *
 *	A key should typically be an aligned address of some sort,
 *	like a disk block or a virtual address of a page.
 *
 *	The underlying table is resized based on utilization.
 *	Size is halved when utilization falls below 25%, and
 *	doubled when it goes over 75%.
 *
 *	Deletes are lazy, so looking for non-existent keys may
 *	be slow unless resizes are triggered frequently enough.
 *
 *	Probing is linear. It'll do primary clustering, which
 *	is bad, but we'll have less cache misses which is good.
 *
 *	Locking is up to the user (you).
 */

#include <u.h>
#include <libc.h>
#include <hashmap.h>

enum {
	MinCap = 16,
};

#define makekey(va) ((va) | 3)
#define isfree(he) (((he)->key & 1) == 0)
#define ischain(he) (((he)->key & 2) != 0)
#define setfree(he) ((he)->key &= ~(uint64_t)1)

/*
 *	http://burtleburtle.net/bob/hash/integer.mapml
 *	https://gist.github.com/badboy/6267743
 *	https://github.com/google/cityhash/blob/master/src/city.h
 */
size_t
scramble64(uint64_t a)
{
	// this is from cityhash. ok avalance, quick.
	uint64_t kMul = 0x9ddfea08eb382d69ULL;
	a *= kMul;
	a ^= (a >> 47);
	a *= kMul;
	return (size_t)a;
}

static int
hmapput1(Hashtable *cur, uint64_t key, uint64_t val)
{
	Hashentry *tab = cur->tab;
	size_t cap = cur->cap;
	size_t hash = scramble64(key)&(cap-1);
	for(size_t i = 0; i < cap; i++, hash = (hash+1)&(cap-1)){
		Hashentry *he = tab + hash;
		if(isfree(he)){
			he->val = val;
			he->key = key;
			cur->len++;
			return 0;
		}
		if(he->key == key)
			return -1; // key already in hmap, bail out.
	}
	return -1; // hmap full
}

static int
hmapget1(Hashtable *cur, uint64_t key, Hashentry **hep)
{
	Hashentry *tab = cur->tab;
	size_t cap = cur->cap;
	key = makekey(key);
	size_t hash = scramble64(key)&(cap-1);
	for(size_t i = 0; i < cap; i++, hash = (hash+1)&(cap-1)){
		Hashentry *he = tab + hash;
		if(he->key == key){
			*hep = he;
			return 0;
		}
		if(!ischain(he))
			return -1; // not found
	}
	return -1; // hmap full
}

static int
hmapresize(Hashmap *map, int isgrow)
{
	Hashtable *cur = map->cur;
	Hashtable *next = map->next;

	next->len = 0;
	next->cap = isgrow ? cur->cap*2 : cur->cap/2;
	next->cap = next->cap < MinCap ? MinCap : next->cap;
	next->tab = malloc(next->cap * sizeof cur->tab[0]);
	memset(next->tab, 0, next->cap * sizeof cur->tab[0]);
	if(cur->tab !=nil){
		Hashentry *tab = cur->tab;
		size_t cap = cur->cap;
		for(size_t i = 0; i < cap; i++){
			Hashentry *he = tab + i;
			if(!isfree(he)){
				int err;
				if((err = hmapput1(next, he->key, he->val)) != 0)
					return err;
			}
		}
	}
	// rehash is done: set current pointer to the new table.
	map->cur = next;
	free(cur->tab);
	cur->tab = nil;
	map->next = cur;
	return 0;
}

int
hmapinit(Hashmap *map)
{
	memset(map, 0, sizeof map[0]);
	map->cur = map->tabs+0;
	map->next = map->tabs+1;
	return 0;
}

int
hmapfree(Hashmap *map)
{
	if(map->cur->tab != nil)
		free(map->cur->tab);
	if(map->next->tab != nil)
		free(map->next->tab);
	return 0;
}

int
hmapdel(Hashmap *map, uint64_t key, uint64_t *valp)
{
	Hashtable *cur = map->cur;
	Hashentry *he;
	int err;
	if((err = hmapget1(cur, key, &he)) != 0)
		return err;
	*valp = he->val;
	setfree(he);
	cur->len--;
	if(cur->cap > MinCap && cur->len < cur->cap/4)
		if((err = hmapresize(map, 0)) != 0)
			return err;
	return 0;
}

int
hmapget(Hashmap *map, uint64_t key, uint64_t *valp)
{
	Hashtable *cur = map->cur;
	Hashentry *he;
	int err;
	if((err = hmapget1(cur, key, &he)) != 0)
		return err;
	*valp = he->val;
	return 0;
}

int
hmapput(Hashmap *map, uint64_t key, uint64_t val)
{
	Hashtable *cur = map->cur;
	int err;
	if(cur->len >= 3*(cur->cap/4))
		if((err = hmapresize(map, 1)) != 0)
			return err;
	return hmapput1(map->cur, makekey(key), val);
}

int
hmapstats(Hashmap *map, size_t *chains, size_t nchains)
{
	Hashtable *cur = map->cur;

	memset(chains, 0, nchains * sizeof chains[0]);
	for(size_t i = 0; i < cur->cap; i++){
		Hashentry *he = cur->tab + i;
		if(!isfree(he)){
			size_t offset = i - (scramble64(he->key) & (cur->cap-1));
			if(offset < nchains){
				chains[offset]++;
			} else {
				chains[nchains-1]++;
			}
		}
	}
	return 0;
}
