/* Copyright (C) 1995, 1998 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gsalloc.h,v 1.1 2000/03/09 08:40:42 lpd Exp $ */
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
void gs_memory_gc_status(P2(const gs_ref_memory_t *, gs_memory_gc_status_t *));
void gs_memory_set_gc_status(P2(gs_ref_memory_t *, const gs_memory_gc_status_t *));

/* ------ Initialization ------ */

/*
 * Allocate and mostly initialize the state of an allocator (system, global,
 * or local).  Does not initialize global or space.
 */
gs_ref_memory_t *ialloc_alloc_state(P2(gs_raw_memory_t *, uint));

/*
 * Add a chunk to an externally controlled allocator.  Such allocators
 * allocate all objects as immovable, are not garbage-collected, and
 * don't attempt to acquire additional memory (or free chunks) on their own.
 */
int ialloc_add_chunk(P3(gs_ref_memory_t *, ulong, client_name_t));

/* ------ Internal routines ------ */

/* Prepare for a GC. */
void ialloc_gc_prepare(P1(gs_ref_memory_t *));

/* Initialize after a save. */
void ialloc_reset(P1(gs_ref_memory_t *));

/* Initialize after a save or GC. */
void ialloc_reset_free(P1(gs_ref_memory_t *));

/* Set the cached allocation limit of an alloctor from its GC parameters. */
void ialloc_set_limit(P1(gs_ref_memory_t *));

/* Consolidate free objects. */
void ialloc_consolidate_free(P1(gs_ref_memory_t *));

#endif /* gsalloc_INCLUDED */
