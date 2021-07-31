/* Copyright (C) 1989, 1992, 1993, 1994, 1995 Aladdin Enterprises.  All rights reserved.
  
  This file is part of Aladdin Ghostscript.
  
  Aladdin Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author
  or distributor accepts any responsibility for the consequences of using it,
  or for whether it serves any particular purpose or works at all, unless he
  or she says so in writing.  Refer to the Aladdin Ghostscript Free Public
  License (the "License") for full details.
  
  Every copy of Aladdin Ghostscript must include a copy of the License,
  normally in a plain ASCII text file named PUBLIC.  The License grants you
  the right to copy, modify and redistribute Aladdin Ghostscript, but only
  under certain conditions described in the License.  Among other things, the
  License requires that the copyright notice and this notice be preserved on
  all copies.
*/

/* idict.h */
/* Interfaces for Ghostscript dictionary package */

/*
 * Contrary to our usual practice, we expose the (first-level)
 * representation of a dictionary in the interface file,
 * because it is so important that access checking go fast.
 * The access attributes for the dictionary are stored in
 * the values ref.
 */
struct dict_s {
	ref values;		/* t_array, values */
	ref keys;		/* t_shortarray or t_array, keys */
	ref count;		/* t_integer, count of occupied entries */
				/* (length) */
	ref maxlength;		/* t_integer, maxlength as seen by client. */
};

/* Define the maximum size of a dictionary. */
extern const uint dict_max_size;

/*
 * Define whether dictionaries expand automatically when full.  Note that
 * if dict_auto_expand is true, dict_put, dict_copy, dict_resize, and
 * dict_grow cannot return e_dictfull; however, they can return e_VMerror.
 * (dict_find can return e_dictfull even if dict_auto_expand is true.)
 */
extern bool dict_auto_expand;

/* Create a dictionary. */
int dict_create(P2(uint maxlength, ref *pdref));

/* Return a pointer to a ref that holds the access attributes */
/* for a dictionary. */
#define dict_access_ref(pdref) (&(pdref)->value.pdict->values)
#define check_dict_read(dref) check_read(*dict_access_ref(&dref))
#define check_dict_write(dref) check_write(*dict_access_ref(&dref))

/*
 * Look up a key in a dictionary.  Store a pointer to the value slot
 * where found, or to the (value) slot for inserting.
 * Return e_typecheck if the key is null, 1 if found, 0 if not and
 * there is room for a new entry, or e_dictfull if the dictionary is full
 * and the key is missing.
 */
int dict_find(P3(const ref *pdref, const ref *key, ref **ppvalue));

/*
 * Look up a (constant) C string in a dictionary.
 * Return 1 if found, <= 0 if not.
 */
int dict_find_string(P3(const ref *pdref, const char _ds *kstr,
			ref **ppvalue));

/*
 * Enter a key-value pair in a dictionary.
 * Return e_typecheck if the key is null, 1 if this was a new entry,
 * 0 if this replaced an existing entry, e_dictfull as for dict_find,
 * e_invalidaccess for an attempt to store a younger key or value into
 * an older dictionary, or e_VMerror if the key was a string
 * and a VMerror occurred when converting it to a name.
 * The caller is responsible for checking that the dictionary is writable.
 */
int dict_put(P3(ref *pdref, const ref *key, const ref *pvalue));

/*
 * Enter a key-value pair where the key is a (constant) C string.
 */
int dict_put_string(P3(ref *pdref, const char *kstr, const ref *pvalue));

/* Remove a key-value pair from a dictionary. */
/* Return 0 or e_undefined. */
int dict_undef(P2(ref *pdref, const ref *key));

/* Return the number of elements in a dictionary. */
uint dict_length(P1(const ref *pdref));

/* Return the capacity of a dictionary. */
uint dict_maxlength(P1(const ref *pdref));

/* Copy one dictionary into another. */
/* Return 0 or e_dictfull. */
/* If new_only is true, only copy entries whose keys */
/* aren't already present in the destination. */
int dict_copy_entries(P3(const ref *dfrom, ref *dto, bool new_only));
#define dict_copy(dfrom, dto) dict_copy_entries(dfrom, dto, false)
#define dict_copy_new(dfrom, dto) dict_copy_entries(dfrom, dto, true)

/* Grow or shrink a dictionary. */
/* Return 0, e_dictfull, or e_VMerror. */
int dict_resize(P2(ref *pdref, uint newmaxlength));

/* Grow a dictionary in the same way as dict_put does. */
/* We export this for some special-case code in zop_def. */
int dict_grow(P1(ref *pdref));

/* Ensure that a dictionary uses the unpacked representation for keys. */
/* (This is of no interest to ordinary clients.) */
int dict_unpack(P1(ref *pdref));

/* Prepare to enumerate a dictionary. */
/* Return an integer suitable for the first call to dict_next. */
int dict_first(P1(const ref *pdref));

/* Enumerate the next element of a dictionary. */
/* index is initially the result of a call on dict_first. */
/* Either store a key and value at eltp[0] and eltp[1] */
/* and return an updated index, or return -1 */
/* to signal that there are no more elements in the dictionary. */
int dict_next(P3(const ref *pdref, int index, ref *eltp));

/* Given a value pointer return by dict_find, return an index that */
/* identifies the entry within the dictionary. (This may, but need not, */
/* be the same as the index returned by dict_next.) */
/* The index is in the range [0..maxlength-1]. */
int dict_value_index(P2(const ref *pdref, const ref *pvalue));

/* Given an index in [0..maxlength-1], as returned by dict_value_index, */
/* return the key and value, as returned by dict_next. */
/* If the index designates an unoccupied entry, return e_undefined. */
int dict_index_entry(P3(const ref *pdref, int index, ref *eltp));
