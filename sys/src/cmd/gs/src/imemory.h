/* Copyright (C) 1993, 1994, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: imemory.h,v 1.5 2002/06/16 04:47:10 lpd Exp $ */
/* Ghostscript memory allocator extensions for interpreter level */

#ifndef imemory_INCLUDED
#  define imemory_INCLUDED

#include "ivmspace.h"

/*
 * The interpreter level of Ghostscript defines a "subclass" extension
 * of the allocator interface in gsmemory.h, by adding the ability to
 * allocate and free arrays of refs, and by adding the distinction
 * between local, global, and system allocation.
 */

#include "gsalloc.h"

#ifndef gs_ref_memory_DEFINED
#  define gs_ref_memory_DEFINED
typedef struct gs_ref_memory_s gs_ref_memory_t;
#endif

	/* Allocate a ref array. */

int gs_alloc_ref_array(gs_ref_memory_t * mem, ref * paref,
		       uint attrs, uint num_refs, client_name_t cname);

	/* Resize a ref array. */
	/* Currently this is only implemented for shrinking, */
	/* not growing. */

int gs_resize_ref_array(gs_ref_memory_t * mem, ref * paref,
			uint new_num_refs, client_name_t cname);

	/* Free a ref array. */

void gs_free_ref_array(gs_ref_memory_t * mem, ref * paref,
		       client_name_t cname);

	/* Allocate a string ref. */

int gs_alloc_string_ref(gs_ref_memory_t * mem, ref * psref,
			uint attrs, uint nbytes, client_name_t cname);

/* Register a ref root.  This just calls gs_register_root. */
/* Note that ref roots are a little peculiar: they assume that */
/* the ref * that they point to points to a *statically* allocated ref. */
int gs_register_ref_root(gs_memory_t *mem, gs_gc_root_t *root,
			 void **pp, client_name_t cname);


/*
 * The interpreter allocator can allocate in either local or global VM,
 * and can switch between the two dynamically.  In Level 1 configurations,
 * global VM is the same as local; however, this is *not* currently true in
 * a Level 2 system running in Level 1 mode.  In addition, there is a third
 * VM space, system VM, that exists in both modes and is used for objects
 * that must not be affected by even the outermost save/restore (stack
 * segments and names).
 *
 * NOTE: since the interpreter's (only) instances of gs_dual_memory_t are
 * embedded in-line in context state structures, pointers to these
 * instances must not be stored anywhere that might persist across a
 * garbage collection.
 */
#ifndef gs_dual_memory_DEFINED
#  define gs_dual_memory_DEFINED
typedef struct gs_dual_memory_s gs_dual_memory_t;
#endif
struct gs_dual_memory_s {
    gs_ref_memory_t *current;	/* = ...global or ...local */
    vm_spaces spaces;		/* system, global, local */
    uint current_space;		/* = current->space */
    /* Garbage collection hook */
    int (*reclaim) (gs_dual_memory_t *, int);
    /* Masks for store checking, see isave.h. */
    uint test_mask;
    uint new_mask;
};

#define public_st_gs_dual_memory()	/* in ialloc.c */\
  gs_public_st_simple(st_gs_dual_memory, gs_dual_memory_t, "gs_dual_memory_t")
#define st_gs_dual_memory_num_ptrs 0

#endif /* imemory_INCLUDED */
