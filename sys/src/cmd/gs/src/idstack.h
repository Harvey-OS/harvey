/* Copyright (C) 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: idstack.h,v 1.1 2000/03/09 08:40:43 lpd Exp $ */
/* Generic dictionary stack API */

#ifndef idstack_INCLUDED
#  define idstack_INCLUDED

#include "iddstack.h"
#include "idsdata.h"
#include "istack.h"

/* Define the type of pointers into the dictionary stack. */
typedef s_ptr ds_ptr;
typedef const_s_ptr const_ds_ptr;

/* Clean up a dictionary stack after a garbage collection. */
void dstack_gc_cleanup(P1(dict_stack_t *));

/*
 * Define a special fast entry for name lookup on a dictionary stack.
 * The key is known to be a name; search the entire dict stack.
 * Return the pointer to the value slot.
 * If the name isn't found, just return 0.
 */
ref *dstack_find_name_by_index(P2(dict_stack_t *, uint));

/*
 * Define an extra-fast macro for name lookup, optimized for
 * a single-probe lookup in the top dictionary on the stack.
 * Amazingly enough, this seems to hit over 90% of the time
 * (aside from operators, of course, which are handled either with
 * the special cache pointer or with 'bind').
 */
#define dstack_find_name_by_index_inline(pds,nidx,htemp)\
  ((pds)->top_keys[htemp = dict_hash_mod_inline(dict_name_index_hash(nidx),\
     (pds)->top_npairs) + 1] == pt_tag(pt_literal_name) + (nidx) ?\
   (pds)->top_values + htemp : dstack_find_name_by_index(pds, nidx))
/*
 * Define a similar macro that only checks the top dictionary on the stack.
 */
#define if_dstack_find_name_by_index_top(pds,nidx,htemp,pvslot)\
  if ( (((pds)->top_keys[htemp = dict_hash_mod_inline(dict_name_index_hash(nidx),\
	 (pds)->top_npairs) + 1] == pt_tag(pt_literal_name) + (nidx)) ?\
	((pvslot) = (pds)->top_values + (htemp), 1) :\
	0)\
     )

#endif /* idstack_INCLUDED */
