/* Copyright (C) 1993, 1994, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: imemory.h,v 1.1 2000/03/09 08:40:43 lpd Exp $ */
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

int gs_alloc_ref_array(P5(gs_ref_memory_t * mem, ref * paref,
			  uint attrs, uint num_refs, client_name_t cname));

	/* Resize a ref array. */
	/* Currently this is only implemented for shrinking, */
	/* not growing. */

int gs_resize_ref_array(P4(gs_ref_memory_t * mem, ref * paref,
			   uint new_num_refs, client_name_t cname));

	/* Free a ref array. */

void gs_free_ref_array(P3(gs_ref_memory_t * mem, ref * paref,
			  client_name_t cname));

	/* Allocate a string ref. */

int gs_alloc_string_ref(P5(gs_ref_memory_t * mem, ref * psref,
			   uint attrs, uint nbytes, client_name_t cname));

/* Register a ref root.  This just calls gs_register_root. */
/* Note that ref roots are a little peculiar: they assume that */
/* the ref * that they point to points to a *statically* allocated ref. */
int gs_register_ref_root(P4(gs_memory_t *mem, gs_gc_root_t *root,
			    void **pp, client_name_t cname));


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
    int (*reclaim) (P2(gs_dual_memory_t *, int));
    /* Masks for store checking, see isave.h. */
    uint test_mask;
    uint new_mask;
};

#define public_st_gs_dual_memory()	/* in ialloc.c */\
  gs_public_st_simple(st_gs_dual_memory, gs_dual_memory_t, "gs_dual_memory_t")
#define st_gs_dual_memory_num_ptrs 0

#endif /* imemory_INCLUDED */
