/* Copyright (C) 1989, 1996, 1997, 1998, 1999, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: idict.c,v 1.12 2004/08/19 19:33:09 stefan Exp $ */
/* Dictionary implementation */
#include "math_.h"		/* for frexp */
#include "string_.h"		/* for strlen */
#include "ghost.h"
#include "gxalloc.h"		/* for accessing masks */
#include "ierrors.h"
#include "imemory.h"
#include "idebug.h"		/* for debug_print_name */
#include "inamedef.h"
#include "iname.h"
#include "ipacked.h"
#include "isave.h"		/* for value cache in names */
#include "store.h"
#include "idict.h"		/* interface definition */
#include "idictdef.h"
#include "iutil.h"
#include "ivmspace.h"		/* for store check */

/*
 * Dictionaries per se aren't supposed to know anything about the
 * dictionary stack, let alone the interpreter's dictionary stack.
 * Unfortunately, there is are two design couplings between them:
 * dictionary stacks cache some of the elements of their top dictionary
 * (requiring updating when that dictionary grows or is unpacked),
 * and names may cache a pointer to their definition (requiring a
 * check whether a dictionary appears on the dictionary stack).
 * Therefore, we need iddstack.h here.
 * We'd really like to fix this, but we don't see how.
 */
#include "iddstack.h"

/*
 * Define the size of the largest valid dictionary.
 * This is limited by the size field of the keys and values refs,
 * and by the enumeration interface, which requires the size to
 * fit in an int.  As it happens, max_array_size will always be
 * smaller than max_int.
 */
const uint dict_max_size = max_array_size - 1;

/* Define whether dictionaries are packed by default. */
bool dict_default_pack = true;

/*
 * Define the check for whether we can set the 1-element cache.
 * We only set the cache if we aren't inside a save.
 * This way, we never have to undo setting the cache.
 */
#define CAN_SET_PVALUE_CACHE(pds, pdref, mem)\
  (pds && dstack_dict_is_permanent(pds, pdref) && !ref_saving_in(mem))

/* Forward references */
private int dict_create_contents(uint size, const ref * pdref, bool pack);

/* Debugging statistics */
#ifdef DEBUG
struct stats_dict_s {
    long lookups;		/* total lookups */
    long probe1;		/* successful lookups on only 1 probe */
    long probe2;		/* successful lookups on 2 probes */
} stats_dict;

/* Wrapper for dict_find */
int real_dict_find(const ref * pdref, const ref * key, ref ** ppvalue);
int
dict_find(const ref * pdref, const ref * pkey, ref ** ppvalue)
{
    dict *pdict = pdref->value.pdict;
    int code = real_dict_find(pdref, pkey, ppvalue);

    stats_dict.lookups++;
    if (r_has_type(pkey, t_name) && dict_is_packed(pdict)) {
	uint nidx = name_index(dict_mem(pdict), pkey);
	uint hash =
	dict_hash_mod(dict_name_index_hash(nidx), npairs(pdict)) + 1;

	if (pdict->keys.value.packed[hash] ==
	    pt_tag(pt_literal_name) + nidx
	    )
	    stats_dict.probe1++;
	else if (pdict->keys.value.packed[hash - 1] ==
		 pt_tag(pt_literal_name) + nidx
	    )
	    stats_dict.probe2++;
    }
    /* Do the cheap flag test before the expensive remainder test. */
    if (gs_debug_c('d') && !(stats_dict.lookups % 1000))
	dlprintf3("[d]lookups=%ld probe1=%ld probe2=%ld\n",
		  stats_dict.lookups, stats_dict.probe1, stats_dict.probe2);
    return code;
}
#define dict_find real_dict_find
#endif

/* Round up the size of a dictionary.  Return 0 if too large. */
uint
dict_round_size_small(uint rsize)
{
    return (rsize > dict_max_size ? 0 : rsize);
}
uint
dict_round_size_large(uint rsize)
{				/* Round up to a power of 2 if not huge. */
    /* If the addition overflows, the new rsize will be zero, */
    /* which will (correctly) be interpreted as a limitcheck. */
    if (rsize > dict_max_non_huge)
	return (rsize > dict_max_size ? 0 : rsize);
    while (rsize & (rsize - 1))
	rsize = (rsize | (rsize - 1)) + 1;
    return (rsize <= dict_max_size ? rsize : dict_max_non_huge);
}

/* Create a dictionary using the given allocator. */
int
dict_alloc(gs_ref_memory_t * mem, uint size, ref * pdref)
{
    ref arr;
    int code =
	gs_alloc_ref_array(mem, &arr, a_all, sizeof(dict) / sizeof(ref),
			   "dict_alloc");
    dict *pdict;
    ref dref;

    if (code < 0)
	return code;
    pdict = (dict *) arr.value.refs;
    make_tav(&dref, t_dictionary,
	     r_space(&arr) | imemory_new_mask(mem) | a_all,
	     pdict, pdict);
    make_struct(&pdict->memory, avm_foreign, mem);
    code = dict_create_contents(size, &dref, dict_default_pack);
    if (code < 0) {
	gs_free_ref_array(mem, &arr, "dict_alloc");
	return code;
    }
    *pdref = dref;
    return 0;
}
/* Create unpacked keys for a dictionary. */
/* The keys are allocated using the same allocator as the dictionary. */
private int
dict_create_unpacked_keys(uint asize, const ref * pdref)
{
    dict *pdict = pdref->value.pdict;
    gs_ref_memory_t *mem = dict_memory(pdict);
    int code;

    code = gs_alloc_ref_array(mem, &pdict->keys, a_all, asize,
			      "dict_create_unpacked_keys");
    if (code >= 0) {
	uint new_mask = imemory_new_mask(mem);
	ref *kp = pdict->keys.value.refs;

	r_set_attrs(&pdict->keys, new_mask);
	refset_null_new(kp, asize, new_mask);
	r_set_attrs(kp, a_executable);	/* wraparound entry */
    }
    return code;
}
/* Create the contents (keys and values) of a newly allocated dictionary. */
/* Allocate in the current VM space, which is assumed to be the same as */
/* the VM space where the dictionary is allocated. */
private int
dict_create_contents(uint size, const ref * pdref, bool pack)
{
    dict *pdict = pdref->value.pdict;
    gs_ref_memory_t *mem = dict_memory(pdict);
    uint new_mask = imemory_new_mask(mem);
    uint asize = dict_round_size((size == 0 ? 1 : size));
    int code;
    register uint i;

    if (asize == 0 || asize > max_array_size - 1)	/* too large */
	return_error(e_limitcheck);
    asize++;			/* allow room for wraparound entry */
    code = gs_alloc_ref_array(mem, &pdict->values, a_all, asize,
			      "dict_create_contents(values)");
    if (code < 0)
	return code;
    r_set_attrs(&pdict->values, new_mask);
    refset_null_new(pdict->values.value.refs, asize, new_mask);
    if (pack) {
	uint ksize = (asize + packed_per_ref - 1) / packed_per_ref;
	ref arr;
	ref_packed *pkp;
	ref_packed *pzp;

	code = gs_alloc_ref_array(mem, &arr, a_all, ksize,
				  "dict_create_contents(packed keys)");
	if (code < 0)
	    return code;
	pkp = (ref_packed *) arr.value.refs;
	make_tasv(&pdict->keys, t_shortarray,
		  r_space(&arr) | a_all | new_mask,
		  asize, packed, pkp);
	for (pzp = pkp, i = 0; i < asize || i % packed_per_ref; pzp++, i++)
	    *pzp = packed_key_empty;
	*pkp = packed_key_deleted;	/* wraparound entry */
    } else {			/* not packed */
	int code = dict_create_unpacked_keys(asize, pdref);

	if (code < 0)
	    return code;
    }
    make_tav(&pdict->count, t_integer, new_mask, intval, 0);
    make_tav(&pdict->maxlength, t_integer, new_mask, intval, size);
    return 0;
}

/*
 * Ensure that a dictionary uses the unpacked representation for keys.
 * We can't just use dict_resize, because the values slots mustn't move.
 */
int
dict_unpack(ref * pdref, dict_stack_t *pds)
{
    dict *pdict = pdref->value.pdict;

    if (!dict_is_packed(pdict))
	return 0;		/* nothing to do */
    {
	gs_ref_memory_t *mem = dict_memory(pdict);
	uint count = nslots(pdict);
	const ref_packed *okp = pdict->keys.value.packed;
	ref old_keys;
	int code;
	ref *nkp;

	old_keys = pdict->keys;
	if (ref_must_save_in(mem, &old_keys))
	    ref_do_save_in(mem, pdref, &pdict->keys, "dict_unpack(keys)");
	code = dict_create_unpacked_keys(count, pdref);
	if (code < 0)
	    return code;
	for (nkp = pdict->keys.value.refs; count--; okp++, nkp++)
	    if (r_packed_is_name(okp)) {
	        packed_get((const gs_memory_t *)mem, okp, nkp);
		ref_mark_new_in(mem, nkp);
	    } else if (*okp == packed_key_deleted)
		r_set_attrs(nkp, a_executable);
	if (!ref_must_save_in(mem, &old_keys))
	    gs_free_ref_array(mem, &old_keys, "dict_unpack(old keys)");
	if (pds)
	    dstack_set_top(pds);	/* just in case */
    }
    return 0;
}

/*
 * Look up a key in a dictionary.  Store a pointer to the value slot
 * where found, or to the (value) slot for inserting.
 * Return 1 if found, 0 if not and there is room for a new entry,
 * or e_dictfull if the dictionary is full and the key is missing.
 * The caller is responsible for ensuring key is not a null.
 */
int
dict_find(const ref * pdref, const ref * pkey,
	  ref ** ppvalue /* result is stored here */ )
{
    dict *pdict = pdref->value.pdict;
    uint size = npairs(pdict);
    register int etype;
    uint nidx;
    ref_packed kpack;
    uint hash;
    int ktype;
    const gs_memory_t *mem = dict_mem(pdict);

    /* Compute hash.  The only types we bother with are strings, */
    /* names, and (unlikely, but worth checking for) integers. */
    switch (r_type(pkey)) {
    case t_name:
	nidx = name_index(mem, pkey);
    nh:
	hash = dict_name_index_hash(nidx);
	kpack = packed_name_key(nidx);
	ktype = t_name;
	break;
    case t_string:		/* convert to a name first */
	{
	    ref nref;
	    int code;

	    if (!r_has_attr(pkey, a_read))
		return_error(e_invalidaccess);
	    code = name_ref(mem, pkey->value.bytes, r_size(pkey), &nref, 1);
	    if (code < 0)
		return code;
	    nidx = name_index(mem, &nref);
	}
	goto nh;
    case t_real:
	/*
	 * Make sure that equal reals and integers hash the same.
	 */
	{
	    int expt, i;
	    double mant = frexp(pkey->value.realval, &expt);
	    /*
	     * The value is mant * 2^expt, where 0.5 <= mant < 1,
	     * or else expt == mant == 0.
	     */

	    if (expt < sizeof(long) * 8 || pkey->value.realval == min_long)
		i = (int)pkey->value.realval;
	    else
		i = (int)(mant * min_long); /* MSVC 6.00.8168.0 cannot compile this */
	    hash = (uint)i * 30503;         /*   with -O2 as a single expression    */
	}
	goto ih;
    case t_integer:
	hash = (uint)pkey->value.intval * 30503;
    ih:
	kpack = packed_key_impossible;
	ktype = -1;
	nidx = 0;		/* only to pacify gcc */
	break;
    case t_null:		/* not allowed as a key */
	return_error(e_typecheck);
    default:
	hash = r_btype(pkey) * 99;	/* yech */
	kpack = packed_key_impossible;
	ktype = -1;
	nidx = 0;		/* only to pacify gcc */
    }
    /* Search the dictionary */
    if (dict_is_packed(pdict)) {
	const ref_packed *pslot = 0;

	packed_search_1(*ppvalue = packed_search_value_pointer,
			return 1,
			if (pslot == 0) pslot = kp, goto miss);
	packed_search_2(*ppvalue = packed_search_value_pointer,
			return 1,
			if (pslot == 0) pslot = kp, goto miss);
	/*
	 * Double wraparound, dict is full.
	 * Note that even if there was an empty slot (pslot != 0),
	 * we must return dictfull if length = maxlength.
	 */
	if (pslot == 0 || d_length(pdict) == d_maxlength(pdict))
	    return_error(e_dictfull);
	*ppvalue = pdict->values.value.refs + (pslot - kbot);
	return 0;
      miss:			/* Key is missing, not double wrap.  See above re dictfull. */
	if (d_length(pdict) == d_maxlength(pdict))
	    return_error(e_dictfull);
	if (pslot == 0)
	    pslot = kp;
	*ppvalue = pdict->values.value.refs + (pslot - kbot);
	return 0;
    } else {
	ref *kbot = pdict->keys.value.refs;
	register ref *kp;
	ref *pslot = 0;
	int wrap = 0;

	for (kp = kbot + dict_hash_mod(hash, size) + 2;;) {
	    --kp;
	    if ((etype = r_type(kp)) == ktype) {	/* Fast comparison if both keys are names */
		if (name_index(mem, kp) == nidx) {
		    *ppvalue = pdict->values.value.refs + (kp - kbot);
		    return 1;
		}
	    } else if (etype == t_null) {	/* Empty, deleted, or wraparound. */
		/* Figure out which. */
		if (kp == kbot) {	/* wrap */
		    if (wrap++) {	/* wrapped twice */
			if (pslot == 0)
			    return_error(e_dictfull);
			break;
		    }
		    kp += size + 1;
		} else if (r_has_attr(kp, a_executable)) {	/* Deleted entry, save the slot. */
		    if (pslot == 0)
			pslot = kp;
		} else		/* key not found */
		    break;
	    } else {
	        if (obj_eq(mem, kp, pkey)) {
		    *ppvalue = pdict->values.value.refs + (kp - kbot);
		    return 1;
		}
	    }
	}
	if (d_length(pdict) == d_maxlength(pdict))
	    return_error(e_dictfull);
	*ppvalue = pdict->values.value.refs +
	    ((pslot != 0 ? pslot : kp) - kbot);
	return 0;
    }
}

/*
 * Look up a (constant) C string in a dictionary.
 * Return 1 if found, <= 0 if not.
 */
int
dict_find_string(const ref * pdref, const char *kstr, ref ** ppvalue)
{
    int code;
    ref kname;
    if ( pdref != 0 ) {
	dict *pdict = pdref->value.pdict;

	if ((code = name_ref(dict_mem(pdict), 
			     (const byte *)kstr, strlen(kstr), &kname, -1)) < 0)
	    return code;
	return dict_find(pdref, &kname, ppvalue);
    }
    return 0;
}

/*
 * Enter a key-value pair in a dictionary.
 * See idict.h for the possible return values.
 */
int
dict_put(ref * pdref /* t_dictionary */ , const ref * pkey, const ref * pvalue,
	 dict_stack_t *pds)
{
    dict *pdict = pdref->value.pdict;
    gs_ref_memory_t *mem = dict_memory(pdict);
    gs_memory_t *pmem = dict_mem(pdict);
    int rcode = 0;
    int code;
    ref *pvslot;

    /* Check the value. */
    store_check_dest(pdref, pvalue);
  top:if ((code = dict_find(pdref, pkey, &pvslot)) <= 0) {	/* not found *//* Check for overflow */
	ref kname;
	uint index;

	switch (code) {
	    case 0:
		break;
	    case e_dictfull:
		if (!pmem->gs_lib_ctx->dict_auto_expand)
		    return_error(e_dictfull);
		code = dict_grow(pdref, pds);
		if (code < 0)
		    return code;
		goto top;	/* keep things simple */
	    default:		/* e_typecheck */
		return code;
	}
	index = pvslot - pdict->values.value.refs;
	/* If the key is a string, convert it to a name. */
	if (r_has_type(pkey, t_string)) {
	    int code;

	    if (!r_has_attr(pkey, a_read))
		return_error(e_invalidaccess);
	    code = name_from_string(pmem, pkey, &kname);
	    if (code < 0)
		return code;
	    pkey = &kname;
	}
	if (dict_is_packed(pdict)) {
	    ref_packed *kp;

	    if (!r_has_type(pkey, t_name) ||
		name_index(pmem, pkey) > packed_name_max_index
		) {		/* Change to unpacked representation. */
		int code = dict_unpack(pdref, pds);

		if (code < 0)
		    return code;
		goto top;
	    }
	    kp = pdict->keys.value.writable_packed + index;
	    if (ref_must_save_in(mem, &pdict->keys)) {	/* See initial comment for why it is safe */
		/* not to save the change if the keys */
		/* array itself is new. */
		ref_do_save_in(mem, &pdict->keys, kp, "dict_put(key)");
	    }
	    *kp = pt_tag(pt_literal_name) + name_index(pmem, pkey);
	} else {
	    ref *kp = pdict->keys.value.refs + index;

	    if_debug2('d', "[d]0x%lx: fill key at 0x%lx\n",
		      (ulong) pdict, (ulong) kp);
	    store_check_dest(pdref, pkey);
	    ref_assign_old_in(mem, &pdict->keys, kp, pkey,
			      "dict_put(key)");	/* set key of pair */
	}
	ref_save_in(mem, pdref, &pdict->count, "dict_put(count)");
	pdict->count.value.intval++;
	/* If the key is a name, update its 1-element cache. */
	if (r_has_type(pkey, t_name)) {
	    name *pname = pkey->value.pname;

	    if (pname->pvalue == pv_no_defn &&
		CAN_SET_PVALUE_CACHE(pds, pdref, mem)
		) {		/* Set the cache. */
		if_debug0('d', "[d]set cache\n");
		pname->pvalue = pvslot;
	    } else {		/* The cache can't be used. */
		if_debug0('d', "[d]no cache\n");
		pname->pvalue = pv_other;
	    }
	}
	rcode = 1;
    }
    if_debug8('d', "[d]0x%lx: put key 0x%lx 0x%lx\n  value at 0x%lx: old 0x%lx 0x%lx, new 0x%lx 0x%lx\n",
	      (ulong) pdref->value.pdict,
	      ((const ulong *)pkey)[0], ((const ulong *)pkey)[1],
	      (ulong) pvslot,
	      ((const ulong *)pvslot)[0], ((const ulong *)pvslot)[1],
	      ((const ulong *)pvalue)[0], ((const ulong *)pvalue)[1]);
    ref_assign_old_in(mem, &pdref->value.pdict->values, pvslot, pvalue,
		      "dict_put(value)");
    return rcode;
}

/*
 * Enter a key-value pair where the key is a (constant) C string.
 */
int
dict_put_string(ref * pdref, const char *kstr, const ref * pvalue,
		dict_stack_t *pds)
{
    int code;
    ref kname;
    dict *pdict = pdref->value.pdict;

    if ((code = name_ref(dict_mem(pdict),
			 (const byte *)kstr, strlen(kstr), &kname, 0)) < 0)
	return code;
    return dict_put(pdref, &kname, pvalue, pds);
}

/* Remove an element from a dictionary. */
int
dict_undef(ref * pdref, const ref * pkey, dict_stack_t *pds)
{
    gs_ref_memory_t *mem;
    ref *pvslot;
    dict *pdict;
    uint index;

    if (dict_find(pdref, pkey, &pvslot) <= 0)
	return (e_undefined);
    /* Remove the entry from the dictionary. */
    pdict = pdref->value.pdict;
    index = pvslot - pdict->values.value.refs;
    mem = dict_memory(pdict);
    if (dict_is_packed(pdict)) {
	ref_packed *pkp = pdict->keys.value.writable_packed + index;

	if_debug3('d', "[d]0x%lx: removing key at 0%lx: 0x%x\n",
		  (ulong)pdict, (ulong)pkp, (uint)*pkp);
	/* See the initial comment for why it is safe not to save */
	/* the change if the keys array itself is new. */
	if (ref_must_save_in(mem, &pdict->keys))
	    ref_do_save_in(mem, &pdict->keys, pkp, "dict_undef(key)");
	/*
	 * Accumulating deleted entries slows down lookup.
	 * Detect the easy case where we can use an empty entry
	 * rather than a deleted one, namely, when the next entry
	 * in the probe order is empty.
	 */
	if (pkp[-1] == packed_key_empty) {
	    /*
	     * In this case we can replace any preceding deleted keys with
	     * empty ones as well.
	     */
	    uint end = nslots(pdict);

	    *pkp = packed_key_empty;
	    while (++index < end && *++pkp == packed_key_deleted)
		*pkp = packed_key_empty;
	} else
	    *pkp = packed_key_deleted;
    } else {			/* not packed */
	ref *kp = pdict->keys.value.refs + index;

	if_debug4('d', "[d]0x%lx: removing key at 0%lx: 0x%lx 0x%lx\n",
		  (ulong)pdict, (ulong)kp, ((ulong *)kp)[0], ((ulong *)kp)[1]);
	make_null_old_in(mem, &pdict->keys, kp, "dict_undef(key)");
	/*
	 * Accumulating deleted entries slows down lookup.
	 * Detect the easy case where we can use an empty entry
	 * rather than a deleted one, namely, when the next entry
	 * in the probe order is empty.
	 */
	if (!r_has_type(kp - 1, t_null) ||	/* full entry */
	    r_has_attr(kp - 1, a_executable)	/* deleted or wraparound */
	    )
	    r_set_attrs(kp, a_executable);	/* mark as deleted */
    }
    ref_save_in(mem, pdref, &pdict->count, "dict_undef(count)");
    pdict->count.value.intval--;
    /* If the key is a name, update its 1-element cache. */
    if (r_has_type(pkey, t_name)) {
	name *pname = pkey->value.pname;

	if (pv_valid(pname->pvalue)) {
#ifdef DEBUG
	    /* Check the the cache is correct. */
	    if (!(pds && dstack_dict_is_permanent(pds, pdref)))
		lprintf1("dict_undef: cached name value pointer 0x%lx is incorrect!\n",
			 (ulong) pname->pvalue);
#endif
	    /* Clear the cache */
	    pname->pvalue = pv_no_defn;
	}
    }
    make_null_old_in(mem, &pdict->values, pvslot, "dict_undef(value)");
    return 0;
}

/* Return the number of elements in a dictionary. */
uint
dict_length(const ref * pdref /* t_dictionary */ )
{
    return d_length(pdref->value.pdict);
}

/* Return the capacity of a dictionary. */
uint
dict_maxlength(const ref * pdref /* t_dictionary */ )
{
    return d_maxlength(pdref->value.pdict);
}

/* Return the maximum index of a slot within a dictionary. */
uint
dict_max_index(const ref * pdref /* t_dictionary */ )
{
    return npairs(pdref->value.pdict) - 1;
}

/*
 * Copy one dictionary into another.
 * If COPY_NEW_ONLY is set, only copy entries whose keys
 * aren't already present in the destination.
 * If COPY_FOR_RESIZE is set, reset any valid name cache entries to
 * pv_no_defn before doing the dict_put.
 */
#define COPY_NEW_ONLY 1
#define COPY_FOR_RESIZE 2
private int
dict_copy_elements(const ref * pdrfrom /* t_dictionary */ ,
		  ref * pdrto /* t_dictionary */ , int options,
		  dict_stack_t *pds)
{
    int space = r_space(pdrto);
    int index;
    ref elt[2];
    ref *pvslot;
    int code;

    if (space != avm_max) {
	/* Do the store check before starting the copy. */
	index = dict_first(pdrfrom);
	while ((index = dict_next(pdrfrom, index, elt)) >= 0)
	    if (!(options & COPY_NEW_ONLY) ||
		dict_find(pdrto, &elt[0], &pvslot) <= 0
		) {
		store_check_space(space, &elt[0]);
		store_check_space(space, &elt[1]);
	    }
    }
    /* Now copy the contents. */
    index = dict_first(pdrfrom);
    while ((index = dict_next(pdrfrom, index, elt)) >= 0) {
	ref *pvalue = pv_no_defn;

	if ((options & COPY_NEW_ONLY) &&
	    dict_find(pdrto, &elt[0], &pvslot) > 0
	    )
	    continue;
	if ((options & COPY_FOR_RESIZE) &&
	    r_has_type(&elt[0], t_name) &&
	    (pvalue = elt[0].value.pname->pvalue, pv_valid(pvalue))
	    )
	    elt[0].value.pname->pvalue = pv_no_defn;
	if ((code = dict_put(pdrto, &elt[0], &elt[1], pds)) < 0) {
	    /*
	     * If COPY_FOR_RESIZE is set, the dict_put isn't supposed to
	     * be able to fail, but we don't want to depend on this.
	     */
	    if (pvalue != pv_no_defn)
		elt[0].value.pname->pvalue = pvalue;
	    return code;
	}
    }
    return 0;
}
int
dict_copy_entries(const ref *pdrfrom, ref *pdrto, bool new_only,
		  dict_stack_t *pds)
{
    return dict_copy_elements(pdrfrom, pdrto, (new_only ? COPY_NEW_ONLY : 0),
			      pds);
}

/* Resize a dictionary. */
int
dict_resize(ref * pdref, uint new_size, dict_stack_t *pds)
{
    dict *pdict = pdref->value.pdict;
    gs_ref_memory_t *mem = dict_memory(pdict);
    uint new_mask = imemory_new_mask(mem);
    dict dnew;
    ref drto;
    int code;

    if (new_size < d_length(pdict)) {
	if (!mem->gs_lib_ctx->dict_auto_expand)
	    return_error(e_dictfull);
	new_size = d_length(pdict);
    }
    make_tav(&drto, t_dictionary, r_space(pdref) | a_all | new_mask,
	     pdict, &dnew);
    dnew.memory = pdict->memory;
    if ((code = dict_create_contents(new_size, &drto, dict_is_packed(pdict))) < 0)
	return code;
    /*
     * We must suppress the store check, in case we are expanding
     * systemdict or another global dictionary that is allowed
     * to reference local objects.
     */
    r_set_space(&drto, avm_local);
    /*
     * If we are expanding a permanent dictionary, we must make sure that
     * dict_put doesn't think this is a second definition for any
     * single-definition names.  This in turn requires that
     * dstack_dict_is_permanent must be true for the second ("to")
     * argument of dict_copy_elements, which requires temporarily
     * setting *pdref = drto.
     */
    if (CAN_SET_PVALUE_CACHE(pds, pdref, mem)) {
	ref drfrom;

	drfrom = *pdref;
	*pdref = drto;
	dict_copy_elements(&drfrom, pdref, COPY_FOR_RESIZE, pds);
	*pdref = drfrom;
    } else {
	dict_copy_elements(pdref, &drto, 0, pds);
    }
    /* Save or free the old dictionary. */
    if (ref_must_save_in(mem, &pdict->values))
	ref_do_save_in(mem, pdref, &pdict->values, "dict_resize(values)");
    else
	gs_free_ref_array(mem, &pdict->values, "dict_resize(old values)");
    if (ref_must_save_in(mem, &pdict->keys))
	ref_do_save_in(mem, pdref, &pdict->keys, "dict_resize(keys)");
    else
	gs_free_ref_array(mem, &pdict->keys, "dict_resize(old keys)");
    ref_assign(&pdict->keys, &dnew.keys);
    ref_assign(&pdict->values, &dnew.values);
    ref_save_in(dict_memory(pdict), pdref, &pdict->maxlength,
		"dict_resize(maxlength)");
    d_set_maxlength(pdict, new_size);
    if (pds)
	dstack_set_top(pds);	/* just in case this is the top dict */
    return 0;
}

/* Grow a dictionary for dict_put. */
int
dict_grow(ref * pdref, dict_stack_t *pds)
{
    dict *pdict = pdref->value.pdict;
    /* We might have maxlength < npairs, if */
    /* dict_round_size increased the size. */
    ulong new_size = (ulong) d_maxlength(pdict) * 3 / 2 + 2;

#if arch_sizeof_int < arch_sizeof_long
    if (new_size > max_uint)
	new_size = max_uint;
#endif
    if (new_size > npairs(pdict)) {
	int code = dict_resize(pdref, (uint) new_size, pds);

	if (code >= 0)
	    return code;
	/* new_size was too big. */
	if (npairs(pdict) < dict_max_size) {
	    code = dict_resize(pdref, dict_max_size, pds);
	    if (code >= 0)
		return code;
	}
	if (npairs(pdict) == d_maxlength(pdict)) {	/* Can't do it. */
	    return code;
	}
	/* We can't grow to new_size, but we can grow to npairs. */
	new_size = npairs(pdict);
    }
    /* maxlength < npairs, we can grow in place */
    ref_save_in(dict_memory(pdict), pdref, &pdict->maxlength,
		"dict_put(maxlength)");
    d_set_maxlength(pdict, new_size);
    return 0;
}

/* Prepare to enumerate a dictionary. */
int
dict_first(const ref * pdref)
{
    return (int)nslots(pdref->value.pdict);
}

/* Enumerate the next element of a dictionary. */
int
dict_next(const ref * pdref, int index, ref * eltp /* ref eltp[2] */ )
{
    dict *pdict = pdref->value.pdict;
    ref *vp = pdict->values.value.refs + index;

    while (vp--, --index >= 0) {
	array_get(dict_mem(pdict), &pdict->keys, (long)index, eltp);
	/* Make sure this is a valid entry. */
	if (r_has_type(eltp, t_name) ||
	    (!dict_is_packed(pdict) && !r_has_type(eltp, t_null))
	    ) {
	    eltp[1] = *vp;
	    if_debug6('d', "[d]0x%lx: index %d: %lx %lx, %lx %lx\n",
		      (ulong) pdict, index,
		      ((ulong *) eltp)[0], ((ulong *) eltp)[1],
		      ((ulong *) vp)[0], ((ulong *) vp)[1]);
	    return index;
	}
    }
    return -1;			/* no more elements */
}

/* Return the index of a value within a dictionary. */
int
dict_value_index(const ref * pdref, const ref * pvalue)
{
    return (int)(pvalue - pdref->value.pdict->values.value.refs - 1);
}

/* Return the entry at a given index within a dictionary. */
/* If the index designates an unoccupied entry, return e_undefined. */
int
dict_index_entry(const ref * pdref, int index, ref * eltp /* ref eltp[2] */ )
{
    const dict *pdict = pdref->value.pdict;

    array_get(dict_mem(pdict), &pdict->keys, (long)(index + 1), eltp);
    if (r_has_type(eltp, t_name) ||
	(!dict_is_packed(pdict) && !r_has_type(eltp, t_null))
	) {
	eltp[1] = pdict->values.value.refs[index + 1];
	return 0;
    }
    return e_undefined;
}
