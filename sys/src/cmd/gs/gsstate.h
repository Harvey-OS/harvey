/* Copyright (C) 1989, 1992, 1993, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* gsstate.h */
/* Graphics state API for Ghostscript library */

#ifndef gsstate_INCLUDED
#  define gsstate_INCLUDED

/* Opaque type for a graphics state */
#ifndef gs_state_DEFINED
#  define gs_state_DEFINED
typedef struct gs_state_s gs_state;
#endif

/* Allocation, freeing, initialization, and copying */
gs_state *gs_state_alloc(P1(gs_memory_t *)); /* 0 if fails */
int	gs_state_free(P1(gs_state *));
int	gs_gsave(P1(gs_state *)),
	gs_grestore(P1(gs_state *)),
	gs_grestoreall(P1(gs_state *));
gs_state *gs_gstate(P1(gs_state *));
int	gs_copygstate(P2(gs_state * /*to*/, const gs_state * /*from*/)),
	gs_currentgstate(P2(gs_state * /*to*/, const gs_state * /*from*/)),
	gs_setgstate(P2(gs_state * /*to*/, const gs_state * /*from*/));
gs_memory_t *gs_state_memory(P1(const gs_state *));	/* special for interpreter */
gs_state *gs_state_saved(P1(const gs_state *));		/* special for Level 2 grestore and save/restore */
gs_state *gs_state_swap_saved(P2(gs_state *, gs_state *));	/* special for save/restore */
gs_memory_t *gs_state_swap_memory(P2(gs_state *, gs_memory_t *)); /* special for interpreter */
int	gs_initgraphics(P1(gs_state *));

/* Device control */
#include "gsdevice.h"

/* Line parameters and quality */
#include "gsline.h"

/* Color and gray */
#include "gscolor.h"

/* Halftone screen */
#include "gsht.h"
int	gs_sethalftonephase(P3(gs_state *, int, int));
int	gs_currenthalftonephase(P2(const gs_state *, gs_int_point *));

/* Miscellaneous */
int	gs_setfilladjust(P2(gs_state *, floatp));
float	gs_currentfilladjust(P1(const gs_state *));

/* "Client data" interface for graphics states. */
typedef void *(*gs_state_alloc_proc_t)(P1(gs_memory_t *mem));
typedef int (*gs_state_copy_proc_t)(P2(void *to, const void *from));
typedef void (*gs_state_free_proc_t)(P2(void *old, gs_memory_t *mem));
typedef struct gs_state_client_procs_s {
	gs_state_alloc_proc_t alloc;
	gs_state_copy_proc_t copy;
	gs_state_free_proc_t free;
} gs_state_client_procs;
void	gs_state_set_client(P3(gs_state *, void *, const gs_state_client_procs *));
void *	gs_state_client_data(P1(const gs_state *));
/* gzstate.h redefines the following: */
#ifndef gs_state_client_data_inline
#  define gs_state_client_data_inline(pgs) gs_state_client_data(pgs)
#endif

#endif					/* gsstate_INCLUDED */
