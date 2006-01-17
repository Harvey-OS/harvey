/* Copyright (C) 1996 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gxstate.h,v 1.11 2004/08/31 13:23:16 igor Exp $ */
/* Internal graphics state API */

#ifndef gxstate_INCLUDED
#  define gxstate_INCLUDED

/* Opaque type for a graphics state */
#ifndef gs_state_DEFINED
#  define gs_state_DEFINED
typedef struct gs_state_s gs_state;

#endif
#include "gscspace.h"


/*
 * The interfaces in this file are for internal use only, primarily by the
 * interpreter.  They are not guaranteed to remain stable from one release
 * to another.
 */

/* Memory and save/restore management */
gs_memory_t *gs_state_memory(const gs_state *);
gs_state *gs_state_saved(const gs_state *);
gs_state *gs_state_swap_saved(gs_state *, gs_state *);
gs_memory_t *gs_state_swap_memory(gs_state *, gs_memory_t *);

/*
 * "Client data" interface for graphics states.
 *
 * As of release 4.36, the copy procedure is superseded by copy_for
 * (although it will still be called if there is no copy_for procedure).
 */
typedef void *(*gs_state_alloc_proc_t) (gs_memory_t * mem);
typedef int (*gs_state_copy_proc_t) (void *to, const void *from);
typedef void (*gs_state_free_proc_t) (void *old, gs_memory_t * mem);

typedef enum {
    copy_for_gsave,		/* from = current, to = new(saved) */
    copy_for_grestore,		/* from = saved, to = current */
    copy_for_gstate,		/* from = current, to = new(copy) */
    copy_for_setgstate,		/* from = stored, to = current */
    copy_for_copygstate,	/* from & to are specified explicitly */
    copy_for_currentgstate	/* from = current, to = stored */
} gs_state_copy_reason_t;

/* Note that the 'from' argument of copy_for is not const. */
/* This is deliberate -- some clients need this. */
typedef int (*gs_state_copy_for_proc_t) (void *to, void *from,
					 gs_state_copy_reason_t reason);
typedef struct gs_state_client_procs_s {
    gs_state_alloc_proc_t alloc;
    gs_state_copy_proc_t copy;
    gs_state_free_proc_t free;
    gs_state_copy_for_proc_t copy_for;
} gs_state_client_procs;
void gs_state_set_client(gs_state *, void *, const gs_state_client_procs *, 
			    bool client_has_pattern_streams);

/* gzstate.h redefines the following: */
#ifndef gs_state_client_data
void *gs_state_client_data(const gs_state *);
#endif

/* Accessories. */
gs_id gx_get_clip_path_id(gs_state *);

#endif /* gxstate_INCLUDED */
