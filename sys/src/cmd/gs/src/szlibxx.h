/* Copyright (C) 1998 Aladdin Enterprises.  All rights reserved.
  
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

/*$Id: szlibxx.h,v 1.2 2000/09/19 19:00:51 lpd Exp $ */
/* Implementation definitions for zlib interface */
/* Must be compiled with -I$(ZSRCDIR) */

#ifndef szlibxx_INCLUDED
#  define szlibxx_INCLUDED

#include "szlibx.h"
#include "zlib.h"

/*
 * We don't want to allocate zlib's private data directly from
 * the C heap, but we must allocate it as immovable; and to avoid
 * garbage collection issues, we must keep GC-traceable pointers
 * to every block allocated.  Since the stream state itself is movable,
 * we have to allocate an immovable block for the z_stream state as well.
 */
typedef struct zlib_block_s zlib_block_t;
struct zlib_block_s {
    void *data;
    zlib_block_t *next;
    zlib_block_t *prev;
};
#define private_st_zlib_block()	/* in szlibc.c */\
  gs_private_st_ptrs3(st_zlib_block, zlib_block_t, "zlib_block_t",\
    zlib_block_enum_ptrs, zlib_block_reloc_ptrs, next, prev, data)
/* The typedef is in szlibx.h */
/*typedef*/ struct zlib_dynamic_state_s {
    gs_memory_t *memory;
    zlib_block_t *blocks;
    z_stream zstate;
} /*zlib_dynamic_state_t*/;
#define private_st_zlib_dynamic_state()	/* in szlibc.c */\
  gs_private_st_ptrs1(st_zlib_dynamic_state, zlib_dynamic_state_t,\
    "zlib_dynamic_state_t", zlib_dynamic_enum_ptrs, zlib_dynamic_reloc_ptrs,\
    blocks)

/*
 * Provide zlib-compatible allocation and freeing functions.
 * The mem pointer actually points to the dynamic state.
 */
void *s_zlib_alloc(P3(void *mem, uint items, uint size));
void s_zlib_free(P2(void *mem, void *address));

/* Internal procedure to allocate and free the dynamic state. */
int s_zlib_alloc_dynamic_state(P1(stream_zlib_state *ss));
void s_zlib_free_dynamic_state(P1(stream_zlib_state *ss));

#endif /* szlibxx_INCLUDED */
