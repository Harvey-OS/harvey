/* Copyright (C) 2002, 2004 Christopher Clark  <firstname.lastname@cl.cam.ac.uk>
 *
 * Modified 2009 by Barret Rhoden <brho@cs.berkeley.edu>
 * Changes include:
 *   - No longer frees keys or values.  It's up to the client to do that.
 *   - Uses the slab allocator for hash entry allocation.
 *   - Merges the iterator code with the main hash table code, mostly to avoid
 *   externing the hentry cache. */

#include <u.h>
#include <libc.h>
#include <hashtable.h>

/*
Credit for primes table: Aaron Krowne
 http://br.endernet.org/~akrowne/
 http://planetmath.org/encyclopedia/GoodHashTablePrimes.html
*/
static const unsigned int primes[] = {
53, 97, 193, 389,
769, 1543, 3079, 6151,
12289, 24593, 49157, 98317,
196613, 393241, 786433, 1572869,
3145739, 6291469, 12582917, 25165843,
50331653, 100663319, 201326611, 402653189,
805306457, 1610612741
};
const unsigned int prime_table_length = sizeof(primes)/sizeof(primes[0]);
#define APPLY_MAX_LOAD_FACTOR(size) \
    ((size * 13)/20)
//const float max_load_factor = 0.65;

struct kmem_cache *hentry_cache;

/* Call this once on bootup, after initializing the slab allocator.  */
void hashtable_init(void)
{
	hentry_cache = kmem_cache_create("hash_entry", sizeof(struct hash_entry),
	                                 __alignof__(struct hash_entry), 0, 0, 0);
}

/* Common hash/equals functions.  Don't call these directly. */
size_t __generic_hash(void *k)
{
	/* 0x9e370001UL used by Linux (32 bit)
	 * (prime approx to the golden ratio to the max integer, IAW Knuth)
	 */
	return (size_t)k * 0x9e370001UL;
}

ssize_t __generic_eq(void *k1, void *k2)
{
	return k1 == k2;
}

/*****************************************************************************/
hashtable_t *
create_hashtable(size_t minsize,
                 size_t (*hashf) (void*),
                 ssize_t (*eqf) (void*,void*))
{
    hashtable_t *h;
    size_t pindex, size = primes[0];
    /* Check requested hashtable isn't too large */
    if (minsize > (1u << 30)) return nil;
    /* Enforce size as prime */
    for (pindex=0; pindex < prime_table_length; pindex++) {
        if (primes[pindex] > minsize) { size = primes[pindex]; break; }
    }
    h = (hashtable_t *)mallocz(sizeof(hashtable_t), 1);
    if (nil == h) return nil; /*oom*/
    h->table = (hash_entry_t **)mallocz(sizeof(hash_entry_t*) * size, 1);
    if (nil == h->table) { free(h); return nil; } /*oom*/
    memset(h->table, 0, size * sizeof(hash_entry_t *));
    h->tablelength  = size;
    h->primeindex   = pindex;
    h->entrycount   = 0;
    h->hashfn       = hashf;
    h->eqfn         = eqf;
    h->loadlimit    = APPLY_MAX_LOAD_FACTOR(size);
    return h;
}

/*****************************************************************************/
static size_t
hash(hashtable_t *h, void *k)
{
    /* Aim to protect against poor hash functions by adding logic here
     * - logic taken from java 1.4 hashtable source */
    size_t i = h->hashfn(k);
    i += ~(i << 9);
    i ^=  ((i >> 14) | (i << 18)); /* >>> */
    i +=  (i << 4);
    i ^=  ((i >> 10) | (i << 22)); /* >>> */
    return i;
}

/*****************************************************************************/
static ssize_t
hashtable_expand(hashtable_t *h)
{
    /* Double the size of the table to accomodate more entries */
    hash_entry_t **newtable;
    hash_entry_t *e;
    hash_entry_t **pE;
    size_t newsize, i, index;
    /* Check we're not hitting max capacity */
    if (h->primeindex == (prime_table_length - 1)) return 0;
    newsize = primes[++(h->primeindex)];

    newtable = (hash_entry_t **)mallocz(sizeof(hash_entry_t*) * newsize, 1);
    if (nil != newtable)
    {
        memset(newtable, 0, newsize * sizeof(hash_entry_t*));
        /* This algorithm is not 'stable'. ie. it reverses the list
         * when it transfers entries between the tables */
        for (i = 0; i < h->tablelength; i++) {
            while (nil != (e = h->table[i])) {
                h->table[i] = e->next;
                index = indexFor(newsize,e->h);
                e->next = newtable[index];
                newtable[index] = e;
            }
        }
        kfree(h->table);
        h->table = newtable;
    }
    /* Plan B: realloc instead */
    else
    {
        newtable = (hash_entry_t**)
                   realloc(h->table, newsize*sizeof(hash_entry_t*), 0);
        if (nil == newtable) { (h->primeindex)--; return 0; }
        h->table = newtable;
        memset(newtable[h->tablelength], 0, newsize - h->tablelength);
        for (i = 0; i < h->tablelength; i++) {
            for (pE = &(newtable[i]), e = *pE; e != nil; e = *pE) {
                index = indexFor(newsize,e->h);
                if (index == i)
                {
                    pE = &(e->next);
                }
                else
                {
                    *pE = e->next;
                    e->next = newtable[index];
                    newtable[index] = e;
                }
            }
        }
    }
    h->tablelength = newsize;
    h->loadlimit   = APPLY_MAX_LOAD_FACTOR(newsize);
    return -1;
}

/*****************************************************************************/
size_t
hashtable_count(hashtable_t *h)
{
    return h->entrycount;
}

/*****************************************************************************/
ssize_t
hashtable_insert(hashtable_t *h, void *k, void *v)
{
    /* This method allows duplicate keys - but they shouldn't be used */
    size_t index;
    hash_entry_t *e;
    if (++(h->entrycount) > h->loadlimit)
    {
        /* Ignore the return value. If expand fails, we should
         * still try cramming just this value into the existing table
         * -- we may not have memory for a larger table, but one more
         * element may be ok. Next time we insert, we'll try expanding again.*/
        hashtable_expand(h);
    }
    e = (hash_entry_t *)kmem_cache_alloc(hentry_cache, 0);
    if (nil == e) { --(h->entrycount); return 0; } /*oom*/
    e->h = hash(h,k);
    index = indexFor(h->tablelength,e->h);
    e->k = k;
    e->v = v;
    e->next = h->table[index];
    h->table[index] = e;
    return -1;
}

/*****************************************************************************/
void * /* returns value associated with key */
hashtable_search(hashtable_t *h, void *k)
{
    hash_entry_t *e;
    size_t hashvalue, index;
    hashvalue = hash(h,k);
    index = indexFor(h->tablelength,hashvalue);
    e = h->table[index];
    while (nil != e)
    {
        /* Check hash value to short circuit heavier comparison */
        if ((hashvalue == e->h) && (h->eqfn(k, e->k))) return e->v;
        e = e->next;
    }
    return nil;
}

/*****************************************************************************/
void * /* returns value associated with key */
hashtable_remove(hashtable_t *h, void *k)
{
    /* TODO: consider compacting the table when the load factor drops enough,
     *       or provide a 'compact' method. */

    hash_entry_t *e;
    hash_entry_t **pE;
    void *v;
    size_t hashvalue, index;

    hashvalue = hash(h,k);
    index = indexFor(h->tablelength,hash(h,k));
    pE = &(h->table[index]);
    e = *pE;
    while (nil != e)
    {
        /* Check hash value to short circuit heavier comparison */
        if ((hashvalue == e->h) && (h->eqfn(k, e->k)))
        {
            *pE = e->next;
            h->entrycount--;
            v = e->v;
			kmem_cache_free(hentry_cache, e);
            return v;
        }
        pE = &(e->next);
        e = e->next;
    }
    return nil;
}

/*****************************************************************************/
/* destroy */
void
hashtable_destroy(hashtable_t *h)
{
	if (hashtable_count(h))
		print("Destroying a non-empty hashtable, clean up after yourself!\n");

    size_t i;
    hash_entry_t *e, *f;
    hash_entry_t **table = h->table;
	for (i = 0; i < h->tablelength; i++) {
		e = table[i];
		while (nil != e) {
			f = e;
			e = e->next;
			kmem_cache_free(hentry_cache, f);
		}
	}
    kfree(h->table);
    kfree(h);
}

/*****************************************************************************/
/* hashtable_iterator    - iterator constructor, be sure to kfree this when
 * you're done.
 *
 * If the htable isn't empty, e and index will refer to the first entry. */

hashtable_itr_t *
hashtable_iterator(hashtable_t *h)
{
    size_t i, tablelength;
    hashtable_itr_t *itr = (hashtable_itr_t *)
	    mallocz(sizeof(hashtable_itr_t), 1);
    if (nil == itr) return nil;
    itr->h = h;
    itr->e = nil;
    itr->parent = nil;
    tablelength = h->tablelength;
    itr->index = tablelength;
    if (0 == h->entrycount) return itr;

    for (i = 0; i < tablelength; i++)
    {
        if (nil != h->table[i])
        {
            itr->e = h->table[i];
            itr->index = i;
            break;
        }
    }
    return itr;
}
#if 0
/*****************************************************************************/
/* key      - return the key of the (key,value) pair at the current position */
/* value    - return the value of the (key,value) pair at the current position */

void *
hashtable_iterator_key(hashtable_itr_t *i)
{ return i->e->k; }

void *
hashtable_iterator_value(hashtable_itr_t *i)
{ return i->e->v; }
#endif
/*****************************************************************************/
/* advance - advance the iterator to the next element
 *           returns zero if advanced to end of table */

ssize_t
hashtable_iterator_advance(hashtable_itr_t *itr)
{
    size_t j,tablelength;
    hash_entry_t **table;
    hash_entry_t *next;
    if (nil == itr->e) return 0; /* stupidity check */

    next = itr->e->next;
    if (nil != next)
    {
        itr->parent = itr->e;
        itr->e = next;
        return -1;
    }
    tablelength = itr->h->tablelength;
    itr->parent = nil;
    if (tablelength <= (j = ++(itr->index)))
    {
        itr->e = nil;
        return 0;
    }
    table = itr->h->table;
    while (nil == (next = table[j]))
    {
        if (++j >= tablelength)
        {
            itr->index = tablelength;
            itr->e = nil;
            return 0;
        }
    }
    itr->index = j;
    itr->e = next;
    return -1;
}

/*****************************************************************************/
/* remove - remove the entry at the current iterator position
 *          and advance the iterator, if there is a successive
 *          element.
 *          If you want the value, read it before you remove:
 *          beware memory leaks if you don't.
 *          Returns zero if end of iteration. */

ssize_t
hashtable_iterator_remove(hashtable_itr_t *itr)
{
    hash_entry_t *remember_e, *remember_parent;
    ssize_t ret;

    /* Do the removal */
    if (nil == (itr->parent))
    {
        /* element is head of a chain */
        itr->h->table[itr->index] = itr->e->next;
    } else {
        /* element is mid-chain */
        itr->parent->next = itr->e->next;
    }
    /* itr->e is now outside the hashtable */
    remember_e = itr->e;
    itr->h->entrycount--;

    /* Advance the iterator, correcting the parent */
    remember_parent = itr->parent;
    ret = hashtable_iterator_advance(itr);
    if (itr->parent == remember_e) { itr->parent = remember_parent; }
	kmem_cache_free(hentry_cache, remember_e);
    return ret;
}

/*****************************************************************************/
ssize_t /* returns zero if not found */
hashtable_iterator_search(hashtable_itr_t *itr,
                          hashtable_t *h, void *k)
{
    hash_entry_t *e, *parent;
    size_t hashvalue, index;

    hashvalue = hash(h,k);
    index = indexFor(h->tablelength,hashvalue);

    e = h->table[index];
    parent = nil;
    while (nil != e)
    {
        /* Check hash value to short circuit heavier comparison */
        if ((hashvalue == e->h) && (h->eqfn(k, e->k)))
        {
            itr->index = index;
            itr->e = e;
            itr->parent = parent;
            itr->h = h;
            return -1;
        }
        parent = e;
        e = e->next;
    }
    return 0;
}

/* Runs func on each member of the hash table */
void hash_for_each(struct hashtable *hash, void func(void *, void *),
				   void *opaque)
{
	if (hashtable_count(hash)) {
		struct hashtable_itr *iter = hashtable_iterator(hash);
		do {
			void *item = hashtable_iterator_value(iter);
			func(item, opaque);
		} while (hashtable_iterator_advance(iter));
		kfree(iter);
	}
}

/* Runs func on each member of the hash table, removing the item after
 * processing it.  Make sure func frees the item, o/w you'll leak. */
void hash_for_each_remove(struct hashtable *hash, void func(void *, void *),
						  void *opaque)
{
	if (hashtable_count(hash)) {
		struct hashtable_itr *iter = hashtable_iterator(hash);
		do {
			void *item = hashtable_iterator_value(iter);
			func(item, opaque);
		} while (hashtable_iterator_remove(iter));
		kfree(iter);
	}
}

/*
 * Copyright (c) 2002, 2004, Christopher Clark
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * * Neither the name of the original author; nor the names of any contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
