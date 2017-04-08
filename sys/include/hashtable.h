/* Copyright (C) 2002, 2004 Christopher Clark <firstname.lastname@cl.cam.ac.uk>
 *
 * Modified 2009 by Barret Rhoden <brho@cs.berkeley.edu>
 * Changes include:
 *   - No longer frees keys or values.  It's up to the client to do that.
 *   - Provides common hash and equality functions (meant for longs)
 *   - Uses the slab allocator for hash entry allocation.
 *   - Merges the iterator code with the main hash table code, mostly to avoid
 *   externing the hentry cache.
 *   - hash for each */

/*****************************************************************************/
typedef struct hash_entry
{
    void *k, *v;
    size_t h;
    struct hash_entry *next;
} hash_entry_t;

typedef struct hashtable {
    size_t tablelength;
    hash_entry_t **table;
    size_t entrycount;
    size_t loadlimit;
    size_t primeindex;
    size_t (*hashfn) (void *k);
    ssize_t (*eqfn) (void *k1, void *k2);
} hashtable_t;

static inline size_t indexFor(unsigned int tablelength, unsigned int hashvalue)
{
	return (hashvalue % tablelength);
};

/*****************************************************************************/

/* Example of use:
 *		hashtable_init(); // Do this once during kernel initialization
 *
 *      struct hashtable  *h;
 *      struct some_key   *k;
 *      struct some_value *v;
 *
 *      static size_t         hash_from_key_fn( void *k );
 *      static ssize_t        keys_equal_fn ( void *key1, void *key2 );
 *
 *      h = create_hashtable(16, hash_from_key_fn, keys_equal_fn);
 *      k = (struct some_key *)     kmalloc(sizeof(struct some_key));
 *      v = (struct some_value *)   kmalloc(sizeof(struct some_value));
 *
 *      (initialise k and v to suitable values)
 *
 *      if (! hashtable_insert(h,k,v) )
 *      {     panic("Hashtable broken...\n");       }
 *
 *      if (NULL == (found = hashtable_search(h,k) ))
 *      {    printk("not found!");                  }
 *
 *      if (NULL == (found = hashtable_remove(h,k) ))
 *      {    printk("Not found\n");                 }
 *
 */

/* Macros may be used to define type-safe(r) hashtable access functions, with
 * methods specialized to take known key and value types as parameters.
 *
 * Example:
 *
 * Insert this at the start of your file:
 *
 * DEFINE_HASHTABLE_INSERT(insert_some, struct some_key, struct some_value);
 * DEFINE_HASHTABLE_SEARCH(search_some, struct some_key, struct some_value);
 * DEFINE_HASHTABLE_REMOVE(remove_some, struct some_key, struct some_value);
 *
 * This defines the functions 'insert_some', 'search_some' and 'remove_some'.
 * These operate just like hashtable_insert etc., with the same parameters,
 * but their function signatures have 'struct some_key *' rather than
 * 'void *', and hence can generate compile time errors if your program is
 * supplying incorrect data as a key (and similarly for value).
 *
 * Note that the hash and key equality functions passed to create_hashtable
 * still take 'void *' parameters instead of 'some key *'. This shouldn't be
 * a difficult issue as they're only defined and passed once, and the other
 * functions will ensure that only valid keys are supplied to them.
 *
 * The cost for this checking is increased code size and runtime overhead
 * - if performance is important, it may be worth switching back to the
 * unsafe methods once your program has been debugged with the safe methods.
 * This just requires switching to some simple alternative defines - eg:
 * #define insert_some hashtable_insert
 *
 */

/* Call this once on bootup, after initializing the slab allocator.  */
void hashtable_init(void);

/* Common hash/equals functions.  Don't call these directly. */
size_t __generic_hash(void *k);
ssize_t __generic_eq(void *k1, void *k2);

/*****************************************************************************
 * create_hashtable

 * @name                    create_hashtable
 * @param   minsize         minimum initial size of hashtable
 * @param   hashfunction    function for hashing keys
 * @param   key_eq_fn       function for determining key equality
 * @return                  newly created hashtable or NULL on failure
 */

hashtable_t *
create_hashtable(size_t minsize,
                 size_t (*hashfunction) (void*),
                 ssize_t (*key_eq_fn) (void*,void*));

/*****************************************************************************
 * hashtable_insert

 * @name        hashtable_insert
 * @param   h   the hashtable to insert into
 * @param   k   the key
 * @param   v   the value
 * @return      non-zero for successful insertion
 *
 * This function will cause the table to expand if the insertion would take
 * the ratio of entries to table size over the maximum load factor.
 *
 * This function does not check for repeated insertions with a duplicate key.
 * The value returned when using a duplicate key is undefined -- when
 * the hashtable changes size, the order of retrieval of duplicate key
 * entries is reversed.
 * If in doubt, remove before insert.
 */

ssize_t
hashtable_insert(hashtable_t *h, void *k, void *v);

#define DEFINE_HASHTABLE_INSERT(fnname, keytype, valuetype) \
ssize_t fnname (hashtable_t *h, keytype *k, valuetype *v) \
{ \
    return hashtable_insert(h,k,v); \
}

/*****************************************************************************
 * hashtable_search

 * @name        hashtable_search
 * @param   h   the hashtable to search
 * @param   k   the key to search for
 * @return      the value associated with the key, or NULL if none found
 */

void *
hashtable_search(hashtable_t *h, void *k);

#define DEFINE_HASHTABLE_SEARCH(fnname, keytype, valuetype) \
valuetype * fnname (hashtable_t *h, keytype *k) \
{ \
    return (valuetype *) (hashtable_search(h,k)); \
}

/*****************************************************************************
 * hashtable_remove

 * @name        hashtable_remove
 * @param   h   the hashtable to remove the item from
 * @param   k   the key to search for
 * @return      the value associated with the key, or NULL if none found
 *
 * Caller ought to free the key, if appropriate.
 */

void * /* returns value */
hashtable_remove(hashtable_t *h, void *k);

#define DEFINE_HASHTABLE_REMOVE(fnname, keytype, valuetype) \
valuetype * fnname (hashtable_t *h, keytype *k) \
{ \
    return (valuetype *) (hashtable_remove(h,k)); \
}


/*****************************************************************************
 * hashtable_count

 * @name        hashtable_count
 * @param   h   the hashtable
 * @return      the number of items stored in the hashtable
 */

size_t
hashtable_count(hashtable_t *h);


/*****************************************************************************
 * hashtable_destroy

 * @name        hashtable_destroy
 * @param   h   the hashtable
 *
 * This will not free the values or the keys.  Each user of the hashtable may
 * have a different way of freeing (such as the slab allocator, kfree, or
 * nothing (key is just an integer)).
 *
 * If the htable isn't empty, you ought to iterate through and destroy manually.
 * This will warn if you try to destroy an non-empty htable.
 */

void
hashtable_destroy(hashtable_t *h);

/***************************** Hashtable Iterator ****************************/
/*****************************************************************************/
/* This struct is only concrete here to allow the inlining of two of the
 * accessor functions. */
typedef struct hashtable_itr {
    hashtable_t *h;
    hash_entry_t *e;
    hash_entry_t *parent;
    size_t index;
} hashtable_itr_t;

/*****************************************************************************/
/* hashtable_iterator.  Be sure to kfree this when you are done.
 */

hashtable_itr_t *
hashtable_iterator(hashtable_t *h);

/*****************************************************************************/
/* hashtable_iterator_key
 * - return the key of the (key,value) pair at the current position
 *
 * Keep this in sync with the non-externed version in the .c */

extern inline void *
hashtable_iterator_key(hashtable_itr_t *i)
{
    return i->e->k;
}

/*****************************************************************************/
/* value - return the value of the (key,value) pair at the current position
 *
 * Keep this in sync with the non-externed version in the .c */

extern inline void *
hashtable_iterator_value(hashtable_itr_t *i)
{
    return i->e->v;
}

/*****************************************************************************/
/* advance - advance the iterator to the next element
 *           returns zero if advanced to end of table */

ssize_t
hashtable_iterator_advance(hashtable_itr_t *itr);

/*****************************************************************************/
/* remove - remove current element and advance the iterator to the next element
 *          NB: if you need the key or value to free it, read it before
 *          removing. ie: beware memory leaks!  this will not remove the key.
 *          returns zero if advanced to end of table */

ssize_t
hashtable_iterator_remove(hashtable_itr_t *itr);

/*****************************************************************************/
/* search - overwrite the supplied iterator, to point to the entry
 *          matching the supplied key.
 *          h points to the hashtable to be searched.
 *          returns zero if not found. */
ssize_t
hashtable_iterator_search(hashtable_itr_t *itr,
                          hashtable_t *h, void *k);

#define DEFINE_HASHTABLE_ITERATOR_SEARCH(fnname, keytype) \
ssize_t fnname (hashtable_itr_t *i, hashtable_t *h, keytype *k) \
{ \
    return (hashtable_iterator_search(i,h,k)); \
}

/* Runs func on each member of the hash table */
void hash_for_each(struct hashtable *hash, void func(void *, void *),
				   void *opaque);
/* Same, but removes the item too */
void hash_for_each_remove(struct hashtable *hash, void func(void *, void *),
						  void *opaque);

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
