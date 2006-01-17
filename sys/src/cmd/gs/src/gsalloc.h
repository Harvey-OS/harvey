/* Copyright (C) 1995, 1998 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gsalloc.h,v 1.7 2004/08/04 19:36:12 stefan Exp $ */
/* Memory allocator extensions for standard allocator */

#ifndef gsalloc_INCLUDED
#  define gsalloc_INCLUDED

/* The following should not be needed at this level! */

#ifndef gs_ref_memory_DEFINED
#  define gs_ref_memory_DEFINED
typedef struct gs_ref_memory_s gs_ref_memory_t;
#endif

/*
 * Define a structure and interface for GC-related allocator state.
 */
typedef struct gs_memory_gc_status_s {
	/* Set by client */
    long vm_threshold;		/* GC interval */
    long max_vm;		/* maximum allowed allocation */
    int *psignal;		/* if not NULL, store signal_value */
				/* here if we go over the vm_threshold */
    int signal_value;		/* value to store in *psignal */
    bool enabled;		/* auto GC enabled if true */
	/* Set by allocator */
    long requested;		/* amount of last failing request */
} gs_memory_gc_status_t;
void gs_memory_gc_status(const gs_ref_memory_t *, gs_memory_gc_status_t *);
void gs_memory_set_gc_status(gs_ref_memory_t *, const gs_memory_gc_status_t *);
void gs_memory_set_vm_threshold(gs_ref_memory_t * mem, long val);
void gs_memory_set_vm_reclaim(gs_ref_memory_t * mem, bool enabled);

/* ------ Initialization ------ */

/*
 * Allocate and mostly initialize the state of an allocator (system, global,
 * or local).  Does not initialize global or space.
 */
gs_ref_memory_t *ialloc_alloc_state(gs_memory_t *, uint);

/*
 * Add a chunk to an externally controlled allocator.  Such allocators
 * allocate all objects as immovable, are not garbage-collected, and
 * don't attempt to acquire additional memory (or free chunks) on their own.
 */
int ialloc_add_chunk(gs_ref_memory_t *, ulong, client_name_t);

/* ------ Internal routines ------ */

/* Prepare for a GC. */
void ialloc_gc_prepare(gs_ref_memory_t *);

/* Initialize after a save. */
void ialloc_reset(gs_ref_memory_t *);

/* Initialize after a save or GC. */
void ialloc_reset_free(gs_ref_memory_t *);

/* Set the cached allocation limit of an alloctor from its GC parameters. */
void ialloc_set_limit(gs_ref_memory_t *);

/* Consolidate free objects. */
void ialloc_consolidate_free(gs_ref_memory_t *);

#endif /* gsalloc_INCLUDED */
