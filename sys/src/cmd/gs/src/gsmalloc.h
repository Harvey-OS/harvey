/* Copyright (C) 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gsmalloc.h,v 1.1 2000/03/09 08:40:42 lpd Exp $ */
/* Client interface to default (C heap) allocator */
/* Requires gsmemory.h */

#ifndef gsmalloc_INCLUDED
#  define gsmalloc_INCLUDED

/* Define a memory manager that allocates directly from the C heap. */
typedef struct gs_malloc_block_s gs_malloc_block_t;
typedef struct gs_malloc_memory_s {
    gs_memory_common;
    gs_malloc_block_t *allocated;
    long limit;
    long used;
    long max_used;
} gs_malloc_memory_t;

/* Allocate and initialize a malloc memory manager. */
gs_malloc_memory_t *gs_malloc_memory_init(P0());

/* Release all the allocated blocks, and free the memory manager. */
/* The cast is unfortunate, but unavoidable. */
#define gs_malloc_memory_release(mem)\
  gs_memory_free_all((gs_memory_t *)mem, FREE_ALL_EVERYTHING,\
		     "gs_malloc_memory_release")

/*
 * Define a default allocator that allocates from the C heap.
 * (We would really like to get rid of this.)
 */
extern gs_malloc_memory_t *gs_malloc_memory_default;
extern gs_memory_t *gs_memory_t_default;  /* may be locked */
#define gs_memory_default (*gs_memory_t_default)

/*
 * The following procedures are historical artifacts that we hope to
 * get rid of someday.
 */
gs_memory_t * gs_malloc_init(P0());
void gs_malloc_release(P0());
#define gs_malloc(nelts, esize, cname)\
  (void *)gs_alloc_byte_array(&gs_memory_default, nelts, esize, cname)
#define gs_free(data, nelts, esize, cname)\
  gs_free_object(&gs_memory_default, data, cname)

/* Define an accessor for the limit on the total allocated heap space. */
#define gs_malloc_limit (gs_malloc_memory_default->limit)

/* Define an accessor for the maximum amount ever allocated from the heap. */
#define gs_malloc_max (gs_malloc_memory_default->max_used)

/* ---------------- Locking ---------------- */

/* Create a locked wrapper for a heap allocator. */
int gs_malloc_wrap(P2(gs_memory_t **wrapped, gs_malloc_memory_t *contents));

/* Get the wrapped contents. */
gs_malloc_memory_t *gs_malloc_wrapped_contents(P1(gs_memory_t *wrapped));

/* Free the wrapper, and return the wrapped contents. */
gs_malloc_memory_t *gs_malloc_unwrap(P1(gs_memory_t *wrapped));

#endif /* gsmalloc_INCLUDED */
