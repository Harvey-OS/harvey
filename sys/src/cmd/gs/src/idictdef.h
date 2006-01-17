/* Copyright (C) 1998 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: idictdef.h,v 1.4 2002/02/21 22:24:53 giles Exp $ */
/* Internals of dictionary implementation */

#ifndef idictdef_INCLUDED
#  define idictdef_INCLUDED

/*
 * A dictionary of capacity M is a structure containing the following
 * elements (refs):
 *
 *      keys - a t_shortarray or t_array of M+1 elements, containing
 *      the keys.
 *
 *      values - a t_array of M+1 elements, containing the values.
 *
 *      count - a t_integer whose value tells how many entries are
 *      occupied (N).
 *
 *      maxlength - a t_integer whose value gives the client's view of
 *      the capacity (C).  C may be less than M (see below).
 *
 *      memory - a foreign t_struct referencing the allocator used to
 *      create this dictionary, which will also be used to expand or
 *      unpack it if necessary.
 *
 * C < M is possible because on large-memory systems, we usually round up M
 * so that M is a power of 2 (see idict.h for details); this allows us to
 * use masking rather than division for computing the initial hash probe.
 * However, C is always the maxlength specified by the client, so clients
 * get a consistent story.
 *
 * As noted above, the keys may be either in packed or unpacked form.
 * The markers for unused and deleted entries are different in the two forms.
 * In the packed form:
 *      unused entries contain packed_key_empty;
 *      deleted entries contain packed_key_deleted.
 * In the unpacked form:
 *      unused entries contain a literal null;
 *      deleted entries contain an executable null.
 *
 * The first entry is always marked deleted, to reduce the cost of the
 * wrap-around check.
 *
 * Note that if the keys slot in the dictionary is new,
 * all the key slots are new (more recent than the last save).
 * We use this fact to avoid saving stores into packed keys
 * for newly created dictionaries.
 *
 * Note that name keys with indices above packed_name_max_index require using
 * the unpacked form.  */
#define dict_is_packed(dct) r_has_type(&(dct)->keys, t_shortarray)
#define packed_key_empty (pt_tag(pt_integer) + 0)
#define packed_key_deleted (pt_tag(pt_integer) + 1)
#define packed_key_impossible pt_tag(pt_full_ref)	/* never matches */
#define packed_name_key(nidx)\
  ((nidx) <= packed_name_max_index ? pt_tag(pt_literal_name) + (nidx) :\
   packed_key_impossible)
/*
 * Using a special mark for deleted entries causes lookup time to degrade
 * as entries are inserted and deleted.  This is not a problem, because
 * entries are almost never deleted.
 */
#define d_maxlength(dct) ((uint)((dct)->maxlength.value.intval))
#define d_set_maxlength(dct,siz) ((dct)->maxlength.value.intval = (siz))
#define nslots(dct) r_size(&(dct)->values)
#define npairs(dct) (nslots(dct) - 1)
#define d_length(dct) ((uint)((dct)->count.value.intval))

/*
 * Define macros for searching a packed dictionary.  Free variables:
 *      ref_packed kpack - holds the packed key.
 *      uint hash - holds the hash of the name.
 *      dict *pdict - points to the dictionary.
 *      uint size - holds npairs(pdict).
 * Note that the macro is *not* enclosed in {}, so that we can access
 * the values of kbot and kp after leaving the loop.
 *
 * We break the macro into two to avoid overflowing some preprocessors.
 */
/* packed_search_body also uses kp and kbot as free variables. */
#define packed_search_value_pointer (pdict->values.value.refs + (kp - kbot))
#define packed_search_body(found1,found2,del,miss)\
    { if_debug2('D', "[D]probe 0x%lx: 0x%x\n", (ulong)kp, *kp);\
      if ( *kp == kpack )\
       { found1;\
	 found2;\
       }\
      else if ( !r_packed_is_name(kp) )\
       { /* Empty, deleted, or wraparound. Figure out which. */\
	 if ( *kp == packed_key_empty ) miss;\
	 if ( kp == kbot ) break;	/* wrap */\
	 else { del; }\
       }\
    }
#define packed_search_1(found1,found2,del,miss)\
   const ref_packed *kbot = pdict->keys.value.packed;\
   register const ref_packed *kp;\
   for ( kp = kbot + dict_hash_mod(hash, size) + 1; ; kp-- )\
     packed_search_body(found1,found2,del,miss)
#define packed_search_2(found1,found2,del,miss)\
   for ( kp += size; ; kp-- )\
     packed_search_body(found1,found2,del,miss)

#endif /* idictdef_INCLUDED */
