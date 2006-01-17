/* Copyright (C) 1989, 1995, 1997, 1998 Aladdin Enterprises.  All rights reserved.
  
  This software is provided AS-IS with no warranty, either express or
  implied.
  
  This software is distributed under license and may not be copied,
  modified or distributed except as expressly authorized under the terms
  of the license contained in the file LICENSE in this distribution.
  
  For more information about licensing, please refer to
  http://www.ghostscript.com/licensing/. For information on
  commercial licensing, go to http://www.artifex.com/licensing/ or
  contact Artifex Software, Inc., 101 Lucas Valley Road #110,
  San Rafael, CA  94903, U.S.A., +1(415)492-9861.
*/

/* $Id: idict.h,v 1.6 2004/08/04 19:36:12 stefan Exp $ */
/* Interfaces for Ghostscript dictionary package */

#ifndef idict_INCLUDED
#  define idict_INCLUDED

#include "iddstack.h"

/*
 * Contrary to our usual practice, we expose the (first-level)
 * representation of a dictionary in the interface file,
 * because it is so important that access checking go fast.
 * The access attributes for the dictionary are stored in
 * the values ref.
 */
struct dict_s {
    ref values;			/* t_array, values */
    ref keys;			/* t_shortarray or t_array, keys */
    ref count;			/* t_integer, count of occupied entries */
    /* (length) */
    ref maxlength;		/* t_integer, maxlength as seen by client. */
    ref memory;			/* foreign t_struct, the allocator that */
    /* created this dictionary */
#define dict_memory(pdict) r_ptr(&(pdict)->memory, gs_ref_memory_t)
#define dict_mem(pdict) r_ptr(&(pdict)->memory, gs_memory_t)
};

/*
 * Define the maximum size of a dictionary.
 */
extern const uint dict_max_size;

/*
 * Define whether dictionaries expand automatically when full.  Note that
 * if dict_auto_expand is true, dict_put, dict_copy, dict_resize, and
 * dict_grow cannot return e_dictfull; however, they can return e_VMerror.
 * (dict_find can return e_dictfull even if dict_auto_expand is true.)
 */
extern bool dict_auto_expand;

/*
 * Create a dictionary.
 */
#ifndef gs_ref_memory_DEFINED
#  define gs_ref_memory_DEFINED
typedef struct gs_ref_memory_s gs_ref_memory_t;
#endif
int dict_alloc(gs_ref_memory_t *, uint maxlength, ref * pdref);

#define dict_create(maxlen, pdref)\
  dict_alloc(iimemory, maxlen, pdref)

/*
 * Return a pointer to a ref that holds the access attributes
 * for a dictionary.
 */
#define dict_access_ref(pdref) (&(pdref)->value.pdict->values)
/*
 * Check a dictionary for read or write permission.
 * Note: this does NOT check the type of its operand!
 */
#define check_dict_read(dref) check_read(*dict_access_ref(&dref))
#define check_dict_write(dref) check_write(*dict_access_ref(&dref))

/*
 * Look up a key in a dictionary.  Store a pointer to the value slot
 * where found, or to the (value) slot for inserting.
 * The caller is responsible for checking that the dictionary is readable.
 * Return 1 if found, 0 if not and there is room for a new entry,
 * Failure returns:
 *      e_typecheck if the key is null;
 *      e_invalidaccess if the key is a string lacking read access;
 *      e_VMerror or e_limitcheck if the key is a string and the corresponding
 *        error occurs from attempting to convert it to a name;
 *      e_dictfull if the dictionary is full and the key is missing.
 */
int dict_find(const ref * pdref, const ref * key, ref ** ppvalue);

/*
 * Look up a (constant) C string in a dictionary.
 * Return 1 if found, <= 0 if not.
 */
int dict_find_string(const ref * pdref, const char *kstr, ref ** ppvalue);

/*
 * Enter a key-value pair in a dictionary.
 * The caller is responsible for checking that the dictionary is writable.
 * Return 1 if this was a new entry, 0 if this replaced an existing entry.
 * Failure returns are as for dict_find, except that e_dictfull doesn't
 * occur if the dictionary is full but expandable, plus:
 *      e_invalidaccess for an attempt to store a younger key or value into
 *        an older dictionary, or as described just below;
 *      e_VMerror if a VMerror occurred while trying to expand the
 *        dictionary.
 * Note that this procedure, and all procedures that may change the
 * contents of a dictionary, take a pointer to a dictionary stack,
 * so they can update the cached 'top' values and also update the cached
 * value pointer in names.  A NULL pointer for the dictionary stack is
 * allowed, but in this case, if the dictionary is present on any dictionary
 * stack, an e_invalidaccess error will occur if cached values need updating.
 * THIS ERROR CHECK IS NOT IMPLEMENTED YET.
 */
int dict_put(ref * pdref, const ref * key, const ref * pvalue,
	     dict_stack_t *pds);

/*
 * Enter a key-value pair where the key is a (constant) C string.
 */
int dict_put_string(ref * pdref, const char *kstr, const ref * pvalue,
		    dict_stack_t *pds);

/*
 * Remove a key-value pair from a dictionary.
 * Return 0 or e_undefined.
 */
int dict_undef(ref * pdref, const ref * key, dict_stack_t *pds);

/*
 * Return the number of elements in a dictionary.
 */
uint dict_length(const ref * pdref);

/*
 * Return the capacity of a dictionary.
 */
uint dict_maxlength(const ref * pdref);

/*
 * Return the maximum index of a slot within a dictionary.
 * Note that this may be greater than maxlength.
 */
uint dict_max_index(const ref * pdref);

/*
 * Copy one dictionary into another.
 * Return 0 or e_dictfull.
 * If new_only is true, only copy entries whose keys
 * aren't already present in the destination.
 */
int dict_copy_entries(const ref * dfrom, ref * dto, bool new_only,
		      dict_stack_t *pds);

#define dict_copy(dfrom, dto, pds) dict_copy_entries(dfrom, dto, false, pds)
#define dict_copy_new(dfrom, dto, pds) dict_copy_entries(dfrom, dto, true, pds)

/*
 * Grow or shrink a dictionary.
 * Return 0, e_dictfull, or e_VMerror.
 */
int dict_resize(ref * pdref, uint newmaxlength, dict_stack_t *pds);

/*
 * Grow a dictionary in the same way as dict_put does.
 * We export this for some special-case code in zop_def.
 */
int dict_grow(ref * pdref, dict_stack_t *pds);

/*
 * Ensure that a dictionary uses the unpacked representation for keys.
 * (This is of no interest to ordinary clients.)
 */
int dict_unpack(ref * pdref, dict_stack_t *pds);

/*
 * Prepare to enumerate a dictionary.
 * Return an integer suitable for the first call to dict_next.
 */
int dict_first(const ref * pdref);

/*
 * Enumerate the next element of a dictionary.
 * index is initially the result of a call on dict_first.
 * Either store a key and value at eltp[0] and eltp[1]
 * and return an updated index, or return -1
 * to signal that there are no more elements in the dictionary.
 */
int dict_next(const ref * pdref, int index, ref * eltp);

/*
 * Given a value pointer return by dict_find, return an index that
 * identifies the entry within the dictionary. (This may, but need not,
 * be the same as the index returned by dict_next.)
 * The index is in the range [0..max_index-1].
 */
int dict_value_index(const ref * pdref, const ref * pvalue);

/*
 * Given an index in [0..max_index-1], as returned by dict_value_index,
 * return the key and value, as returned by dict_next.
 * If the index designates an unoccupied entry, return e_undefined.
 */
int dict_index_entry(const ref * pdref, int index, ref * eltp);

/*
 * The following are some internal details that are used in both the
 * implementation and some high-performance clients.
 */

/* On machines with reasonable amounts of memory, we round up dictionary
 * sizes to the next power of 2 whenever possible, to allow us to use
 * masking rather than division for computing the hash index.
 * Unfortunately, if we required this, it would cut the maximum size of a
 * dictionary in half.  Therefore, on such machines, we distinguish
 * "huge" dictionaries (dictionaries whose size is larger than the largest
 * power of 2 less than dict_max_size) as a special case:
 *
 *      - If the top dictionary on the stack is huge, we set the dtop
 *      parameters so that the fast inline lookup will always fail.
 *
 *      - For general lookup, we use the slower hash_mod algorithm for
 *      huge dictionaries.
 */
#define dict_max_non_huge ((uint)(max_array_size / 2 + 1))

/* Define the hashing function for names. */
/* We don't have to scramble the index, because */
/* indices are assigned in a scattered order (see name_ref in iname.c). */
#define dict_name_index_hash(nidx) (nidx)

/* Hash an arbitrary non-negative or unsigned integer into a dictionary. */
#define dict_hash_mod_rem(hash, size) ((hash) % (size))
#define dict_hash_mod_mask(hash, size) ((hash) & ((size) - 1))
#define dict_hash_mod_small(hash, size) dict_hash_mod_rem(hash, size)
#define dict_hash_mod_inline_small(hash, size) dict_hash_mod_rem(hash, size)
#define dict_hash_mod_large(hash, size)\
  (size > dict_max_non_huge ? dict_hash_mod_rem(hash, size) :\
   dict_hash_mod_mask(hash, size))
#define dict_hash_mod_inline_large(hash, size) dict_hash_mod_mask(hash, size)
/* Round up the requested size of a dictionary.  Return 0 if too big. */
uint dict_round_size_small(uint rsize);
uint dict_round_size_large(uint rsize);

/* Choose the algorithms depending on the size of memory. */
#if arch_small_memory
#  define dict_hash_mod(h, s) dict_hash_mod_small(h, s)
#  define dict_hash_mod_inline(h, s) dict_hash_mod_inline_small(h, s)
#  define dict_round_size(s) dict_round_size_small(s)
#else
#  ifdef DEBUG
#    define dict_hash_mod(h, s)\
       (gs_debug_c('.') ? dict_hash_mod_small(h, s) :\
	dict_hash_mod_large(h, s))
#    define dict_hash_mod_inline(h, s)\
       (gs_debug_c('.') ? dict_hash_mod_inline_small(h, s) :\
	dict_hash_mod_inline_large(h, s))
#    define dict_round_size(s)\
       (gs_debug_c('.') ? dict_round_size_small(s) :\
	dict_round_size_large(s))
#  else
#    define dict_hash_mod(h, s) dict_hash_mod_large(h, s)
#    define dict_hash_mod_inline(h, s) dict_hash_mod_inline_large(h, s)
#    define dict_round_size(s) dict_round_size_large(s)
#  endif
#endif

#endif /* idict_INCLUDED */
