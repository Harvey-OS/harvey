/* Copyright (C) 1989, 1995, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
  This file is part of AFPL Ghostscript.
  
  AFPL Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author or
  distributor accepts any responsibility for the consequences of using it, or
  for whether it serves any particular purpose or works at all, unless he or
  she says so in writing.  Refer to the Aladdin Free Public License (the
  "License") for full details.
  
  Every copy of AFPL Ghostscript must include a copy of the License, normally
  in a plain ASCII text file named PUBLIC.  The License grants you the right
  to copy, modify and redistribute AFPL Ghostscript, but only under certain
  conditions described in the License.  Among other things, the License
  requires that the copyright notice and this notice be preserved on all
  copies.
*/

/*$Id: iname.h,v 1.2 2000/09/19 19:00:45 lpd Exp $ */
/* Interpreter's name table interface */

#ifndef iname_INCLUDED
#  define iname_INCLUDED

#include "inames.h"

/*
 * This file defines those parts of the name table API that refer to the
 * interpreter's distinguished instance.  Procedures in this file begin
 * with name_.
 */

/* ---------------- Procedural interface ---------------- */

/* Define the interpreter's name table. */
extern name_table *the_gs_name_table;

/* Backward compatibility */
#define the_name_table() ((const name_table *)the_gs_name_table)

/* Get the allocator for the name table. */
#define name_memory()\
  names_memory(the_gs_name_table)

/*
 * Look up and/or enter a name in the name table.
 * See inames.h for the values of enterflag, and the possible return values.
 */
#define name_ref(ptr, size, pnref, enterflag)\
  names_ref(the_gs_name_table, ptr, size, pnref, enterflag)
#define name_string_ref(pnref, psref)\
  names_string_ref(the_gs_name_table, pnref, psref)
/*
 * name_enter_string calls name_ref with a (permanent) C string.
 */
#define name_enter_string(str, pnref)\
  names_enter_string(the_gs_name_table,str, pnref)
/*
 * name_from_string essentially implements cvn.
 * It always enters the name, and copies the executable attribute.
 */
#define name_from_string(psref, pnref)\
  names_from_string(the_gs_name_table, psref, pnref)

/* Compare two names for equality. */
#define name_eq(pnref1, pnref2)\
  names_eq(pnref1, pnref2)

/* Invalidate the value cache for a name. */
#define name_invalidate_value_cache(pnref)\
  names_invalidate_value_cache(the_gs_name_table, pnref)

/* Convert between names and indices. */
#define name_index(pnref)		/* ref => index */\
  names_index(the_gs_name_table, pnref)
#define name_index_ptr(nidx)		/* index => name */\
  names_index_ptr(the_gs_name_table, nidx)
#define name_index_ref(nidx, pnref)	/* index => ref */\
  names_index_ref(the_gs_name_table, nidx, pnref)

/* Get the index of the next valid name. */
/* The argument is 0 or a valid index. */
/* Return 0 if there are no more. */
#define name_next_valid_index(nidx)\
  names_next_valid_index(the_gs_name_table, nidx)

/* Mark a name for the garbage collector. */
/* Return true if this is a new mark. */
#define name_mark_index(nidx)\
  names_mark_index(the_gs_name_table, nidx)

/* Get the object (sub-table) containing a name. */
/* The garbage collector needs this so it can relocate pointers to names. */
#define name_ref_sub_table(pnref)\
  names_ref_sub_table(the_gs_name_table, pnref)

#endif /* iname_INCLUDED */
