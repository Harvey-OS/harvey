/* Copyright (C) 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: idsdata.h,v 1.1 2000/03/09 08:40:43 lpd Exp $ */
/* Generic dictionary stack structure definition */

#ifndef idsdata_INCLUDED
#  define idsdata_INCLUDED

#include "isdata.h"

/* Define the dictionary stack structure. */
#ifndef dict_stack_DEFINED
#  define dict_stack_DEFINED
typedef struct dict_stack_s dict_stack_t;
#endif
struct dict_stack_s {

    ref_stack_t stack;		/* the actual stack of dictionaries */

/*
 * Switching between Level 1 and Level 2 involves inserting and removing
 * globaldict on the dictionary stack.  Instead of truly inserting and
 * removing entries, we replace globaldict by a copy of systemdict in
 * Level 1 mode.  min_dstack_size, the minimum number of entries, does not
 * change depending on language level; the countdictstack and dictstack
 * operators must take this into account.
 */
    uint min_size;		/* size of stack after clearing */

    int userdict_index;		/* index of userdict on stack */

/*
 * Cache a value for fast checking of def operations.
 * If the top entry on the dictionary stack is a writable dictionary,
 * dsspace is the space of the dictionary; if it is a non-writable
 * dictionary, dsspace = -1.  Then def is legal precisely if
 * r_space(pvalue) <= dsspace.  Note that in order for this trick to work,
 * the result of r_space must be a signed integer; some compilers treat
 * enums as unsigned, probably in violation of the ANSI standard.
 */
    int def_space;

/*
 * Cache values for fast name lookup.  If the top entry on the dictionary
 * stack is a readable dictionary with packed keys, dtop_keys, dtop_npairs,
 * and dtop_values are keys.value.packed, npairs, and values.value.refs
 * for that dictionary; otherwise, these variables point to a dummy
 * empty dictionary.
 */
    const ref_packed *top_keys;
    uint top_npairs;
    ref *top_values;

/*
 * Cache a copy of the bottom entry on the stack, which is never deleted.
 */
    ref system_dict;

};

/*
 * The top-entry pointers are recomputed after garbage collection, so we
 * don't declare them as pointers.
 */
#define public_st_dict_stack()	/* in interp.c */\
  gs_public_st_suffix_add0(st_dict_stack, dict_stack_t, "dict_stack_t",\
    dict_stack_enum_ptrs, dict_stack_reloc_ptrs, st_ref_stack)
#define st_dict_stack_num_ptrs st_ref_stack_num_ptrs

#endif /* idsdata_INCLUDED */
