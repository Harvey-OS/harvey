/* Copyright (C) 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: inames.h,v 1.5 2002/06/16 04:47:10 lpd Exp $ */
/* Name table interface */

#ifndef inames_INCLUDED
#  define inames_INCLUDED

/*
 * This file defines those parts of the name table API that depend neither
 * on the implementation nor on the existence of a single distinguished
 * instance.  Procedures in this file begin with names_.
 */

/* ---------------- Interface types ---------------- */

#ifndef name_table_DEFINED
#  define name_table_DEFINED
typedef struct name_table_s name_table;
#endif

typedef uint name_index_t;

/* ---------------- Constant values ---------------- */

extern const uint name_max_string;

/* ---------------- Procedural interface ---------------- */

#ifndef gs_ref_memory_DEFINED
#  define gs_ref_memory_DEFINED
typedef struct gs_ref_memory_s gs_ref_memory_t;
#endif

/* Allocate and initialize a name table. */
name_table *names_init(ulong size, gs_ref_memory_t *imem);

/* Get the allocator for a name table. */
gs_memory_t *names_memory(const name_table * nt);

/*
 * Look up and/or enter a name in the name table.
 * The values of enterflag are:
 *      -1 -- don't enter (return an error) if missing;
 *       0 -- enter if missing, don't copy the string, which was allocated
 *              statically;
 *       1 -- enter if missing, copy the string;
 *       2 -- enter if missing, don't copy the string, which was already
 *              allocated dynamically (using the names_memory allocator).
 * Possible errors: VMerror, limitcheck (if string is too long or if
 * we have assigned all possible name indices).
 */
int names_ref(name_table * nt, const byte * ptr, uint size, ref * pnref,
	      int enterflag);
void names_string_ref(const name_table * nt, const ref * pnref, ref * psref);

/*
 * names_enter_string calls names_ref with a (permanent) C string.
 */
int names_enter_string(name_table * nt, const char *str, ref * pnref);

/*
 * names_from_string essentially implements cvn.
 * It always enters the name, and copies the executable attribute.
 */
int names_from_string(name_table * nt, const ref * psref, ref * pnref);

/* Compare two names for equality. */
#define names_eq(pnref1, pnref2)\
  ((pnref1)->value.pname == (pnref2)->value.pname)

/* Invalidate the value cache for a name. */
void names_invalidate_value_cache(name_table * nt, const ref * pnref);

/* Convert between names and indices. */
name_index_t names_index(const name_table * nt, const ref * pnref);		/* ref => index */
name *names_index_ptr(const name_table * nt, name_index_t nidx);	/* index => name */
void names_index_ref(const name_table * nt, name_index_t nidx, ref * pnref);	/* index => ref */

/* Get the index of the next valid name. */
/* The argument is 0 or a valid index. */
/* Return 0 if there are no more. */
name_index_t names_next_valid_index(name_table * nt, name_index_t nidx);

/* Mark a name for the garbage collector. */
/* Return true if this is a new mark. */
bool names_mark_index(name_table * nt, name_index_t nidx);

/* Get the object (sub-table) containing a name. */
/* The garbage collector needs this so it can relocate pointers to names. */
void /*obj_header_t */ *
    names_ref_sub_table(name_table * nt, const ref * pnref);
void /*obj_header_t */ *
    names_index_sub_table(name_table * nt, name_index_t nidx);
void /*obj_header_t */ *
    names_index_string_sub_table(name_table * nt, name_index_t nidx);

#endif /* inames_INCLUDED */
