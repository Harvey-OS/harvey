/* Copyright (C) 1989, 1995, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: inamedef.h,v 1.5 2002/06/16 04:47:10 lpd Exp $ */
/* Name table definition */

#ifndef inamedef_INCLUDED
#  define inamedef_INCLUDED

#include "inameidx.h"
#include "inamestr.h"
#include "inames.h"
#include "gsstruct.h"		/* for gc_state_t */

/*
 * The name table machinery has two slightly different configurations:
 * a faster one that limits the total number of names to 64K and allows
 * names up to 16K in size, and a slightly slower one that limits
 * the total to 4M and restricts names to 256 characters.
 * The maximum number of names is 2^(16+EXTEND_NAMES)-1.
 */
#define max_name_extension_bits 6
#if EXTEND_NAMES > max_name_extension_bits
#  undef EXTEND_NAMES
#  define EXTEND_NAMES max_name_extension_bits
#endif
/*       
 * We capture the small algorithmic differences between these two
 * configurations entirely in this header file;
 * the implementation doesn't need any conditionals on EXTEND_NAMES.
 */
#define max_name_index (uint)((0x10000 << EXTEND_NAMES) - 1)
/* As explained below, we distinguish name indices from name counts. */
#define max_name_count max_name_index

/* ---------------- Structure definitions ---------------- */

/*
 * Define the structure of a name.  The pvalue member implements an
 * important optimization to avoid lookup for operator and other global
 * names.
 */
struct name_s {
/* pvalue specifies the definition status of the name: */
/*      pvalue == pv_no_defn: no definitions */
#define pv_no_defn ((ref *)0)
/*      pvalue == pv_other: other status */
#define pv_other ((ref *)1)
/*      pvalue != pv_no_defn, pvalue != pv_other: pvalue is valid */
#define pv_valid(pvalue) ((unsigned long)(pvalue) > 1)
    ref *pvalue;		/* if only defined in systemdict or */
				/* userdict, this points to the value */
};

/*typedef struct name_s name; *//* in iref.h */

/*
 * Define the structure of a name table.  Normally we would make this
 * an opaque type, but we want to be able to in-line some of the
 * access procedures.
 *
 * The name table is a two-level indexed table, consisting of
 * sub-tables of size nt_sub_size each.
 *
 * First we define the name sub-table structure.
 */
#define nt_log2_sub_size NT_LOG2_SUB_SIZE /* in inameidx.h */
# define nt_sub_size (1 << nt_log2_sub_size)
# define nt_sub_index_mask (nt_sub_size - 1)
typedef struct name_sub_table_s {
    name names[NT_SUB_SIZE];	/* must be first */
#ifdef EXTEND_NAMES
    uint high_index;		/* sub-table base index & (-1 << 16) */
#endif
} name_sub_table;

/*
 * Now define the name table itself.
 * This must be made visible so that the interpreter can use the
 * inline macros defined below.
 */
struct name_table_s {
    uint free;			/* head of free list, which is sorted in */
				/* increasing count (not index) order */
    uint sub_next;		/* index of next sub-table to allocate */
				/* if not already allocated */
    uint perm_count;		/* # of permanent (read-only) strings */
    uint sub_count;		/* index of highest allocated sub-table +1 */
    uint max_sub_count;		/* max allowable value of sub_count */
    uint name_string_attrs;	/* imemory_space(memory) | a_readonly */
    gs_memory_t *memory;
    uint hash[NT_HASH_SIZE];
    struct sub_ {		/* both ptrs are 0 or both are non-0 */
	name_sub_table *names;
	name_string_sub_table_t *strings;
    } sub[max_name_index / nt_sub_size + 1];
};
/*typedef struct name_table_s name_table; *//* in inames.h */

/* ---------------- Procedural interface ---------------- */

/*
 * Convert between names, indices, and strings.  Note that the inline
 * versions, but not the procedure versions, take a name_table argument.
 */
		/* index => string */
#define names_index_string_inline(nt, nidx)\
  ((nt)->sub[(nidx) >> nt_log2_sub_size].strings->strings +\
   ((nidx) & nt_sub_index_mask))
		/* ref => string */
#define names_string_inline(nt, pnref)\
  names_index_string_inline(nt, names_index_inline(nt, pnref))
		/* ref => index */
#if EXTEND_NAMES
#  define names_index_inline(nt_ignored, pnref)\
     ( ((const name_sub_table *)\
	((pnref)->value.pname - (r_size(pnref) & nt_sub_index_mask)))->high_index + r_size(pnref) )
#else
#  define names_index_inline(nt_ignored, pnref) r_size(pnref)
#endif
#define names_index(nt_ignored, pnref) names_index_inline(nt_ignored, pnref)
		/* index => name */
#define names_index_ptr_inline(nt, nidx)\
  ((nt)->sub[(nidx) >> nt_log2_sub_size].names->names +\
   ((nidx) & nt_sub_index_mask))
		/* index => ref */
#define names_index_ref_inline(nt, nidx, pnref)\
  make_name(pnref, nidx, names_index_ptr_inline(nt, nidx));
/* Backward compatibility */
#define name_index_inline(pnref) names_index_inline(ignored, pnref)
#define name_index_ptr_inline(nt, pnref) names_index_ptr_inline(nt, pnref)
#define name_index_ref_inline(nt, nidx, pnref)\
  names_index_ref_inline(nt, nidx, pnref)
		/* name => ref */
/* We have to set the space to system so that the garbage collector */
/* won't think names are foreign and therefore untraceable. */
#define make_name(pnref, nidx, pnm)\
  make_tasv(pnref, t_name, avm_system, (ushort)(nidx), pname, pnm)

/* ------ Garbage collection ------ */

/* Unmark all non-permanent names before a garbage collection. */
void names_unmark_all(name_table * nt);

/* Finish tracing the name table by putting free names on the free list. */
void names_trace_finish(name_table * nt, gc_state_t * gcst);

/* ------ Save/restore ------ */

/* Clean up the name table before a restore. */
#ifndef alloc_save_t_DEFINED	/* also in isave.h */
typedef struct alloc_save_s alloc_save_t;
#  define alloc_save_t_DEFINED
#endif
void names_restore(name_table * nt, alloc_save_t * save);

#endif /* inamedef_INCLUDED */
