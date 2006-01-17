/* Copyright (C) 1991, 1995, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: isave.h,v 1.7 2004/08/04 19:36:13 stefan Exp $ */
/* Procedures for save/restore */
/* Requires imemory.h */

#ifndef isave_INCLUDED
#  define isave_INCLUDED

#include "idosave.h"

/*
 * According to the PostScript language definition, save objects are simple,
 * not composite.  Consequently, we cannot use their natural representation,
 * namely a t_struct pointing to an alloc_save_t, since we aren't willing to
 * allocate them all in global VM and rely on garbage collection to clean
 * them up.  Instead, we assign each one a unique "save ID", and store this
 * in the alloc_save_t object.  Mapping the number to the object requires
 * at most searching the local save chain for the current gs_dual_memory_t,
 * and this approach means we don't have to do anything to invalidate
 * save objects when we do a restore.
 */
#ifndef alloc_save_t_DEFINED	/* also in inamedef.h */
typedef struct alloc_save_s alloc_save_t;
#  define alloc_save_t_DEFINED
#endif

/* Initialize the save machinery. */
extern void alloc_save_init(gs_dual_memory_t *);

/* Map a save ID to its save object.  Return 0 if the ID is invalid. */
alloc_save_t *alloc_find_save(const gs_dual_memory_t *, ulong);

/*
 * Save the state.  Return 0 if we can't allocate the save object,
 * otherwise return the save ID.  The second argument is a client data
 * pointer, assumed to point to an object.
 */
ulong alloc_save_state(gs_dual_memory_t *, void *);

/* Get the client pointer passed to alloc_saved_state. */
void *alloc_save_client_data(const alloc_save_t *);

/* Return (the id of) the innermost externally visible save object. */
ulong alloc_save_current_id(const gs_dual_memory_t *);
alloc_save_t *alloc_save_current(const gs_dual_memory_t *);

/* Check whether a pointer refers to an object allocated since a given save. */
bool alloc_is_since_save(const void *, const alloc_save_t *);

/* Check whether a name was created since a given save. */
bool alloc_name_is_since_save(const gs_memory_t *mem, const ref *, const alloc_save_t *);
bool alloc_name_index_is_since_save(const gs_memory_t *mem, uint, const alloc_save_t *);

/*
 * Check whether any names have been created since a given save
 * that might be released by the restore.
 */
bool alloc_any_names_since_save(const alloc_save_t *);

/*
 * Do one step of restoring the state.  Return true if the argument
 * was the innermost save, in which case this is the last (or only) step.
 * Assume the caller obtained the argument by calling alloc_find_save;
 * if this is the case, the operation cannot fail.
 */
bool alloc_restore_step_in(gs_dual_memory_t *, alloc_save_t *);
/* Backward compatibility */
#define alloc_restore_state_step(save) alloc_restore_step_in(idmemory, save)

/*
 * Forget a save -- like committing a transaction (restore is like
 * aborting a transaction).  Assume the caller obtained the argument
 * by calling alloc_find_save.  Note that forgetting a save does not
 * require checking pointers for recency.
 */
void alloc_forget_save_in(gs_dual_memory_t *, alloc_save_t *);
/* Backward compatibility */
#define alloc_forget_save(save) alloc_forget_save_in(idmemory, save)

/* Release all memory -- like doing a restore "past the bottom". */
void alloc_restore_all(gs_dual_memory_t *);

/* ------ Internals ------ */

/*
 * If we are in a save, we want to save the old contents if l_new is
 * not set; if we are not in a save, we never want to save old contents.
 * We can test this quickly with a single mask that is l_new if we are
 * in a save, and -1 if we are not, since type_attrs of a valid ref
 * cannot be 0; this is the test_mask in a gs_dual_memory_t.  Similarly,
 * we want to set the l_new bit in newly allocated objects iff we are in
 * a save; this is the new_mask in a gs_dual_memory_t.
 */

/* Record that we are in a save. */
void alloc_set_in_save(gs_dual_memory_t *);

/* Record that we are not in a save. */
void alloc_set_not_in_save(gs_dual_memory_t *);

/* Remove entries from font and character caches. */
void font_restore(const alloc_save_t * save);

#endif /* isave_INCLUDED */
