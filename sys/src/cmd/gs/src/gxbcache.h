/* Copyright (C) 1995, 1996 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gxbcache.h,v 1.1 2000/03/09 08:40:42 lpd Exp $ */
/* Bitmap cache structures */

#ifndef gxbcache_INCLUDED
#  define gxbcache_INCLUDED

#include "gxbitmap.h"

/*
 * These structures are superclasses for a cache in which the 'value' is
 * a bitmap.  The structures defined here don't take any position about
 * the nature of the 'key'.
 */

/* ---------------- Bitmap cache entry ---------------- */

/*
 * The cache may contain both used and free blocks.
 * All blocks have a common header; free blocks have ONLY the header.
 */
typedef struct gx_cached_bits_head_s {
    uint size;			/* total block size in bytes */
    uint depth;			/* bits per pixel, free block if 0 */
} gx_cached_bits_head;

#define cb_head_is_free(cbh) ((cbh)->depth == 0)
#define cb_head_set_free(cbh) ((cbh)->depth = 0)
#define gx_cached_bits_common\
	gx_cached_bits_head head;	/* must be first */\
		/* The rest of the entry is an abbreviation of */\
		/* gx_strip_bitmap, sans data. */\
	ushort width, height, shift;\
	ushort raster;\
	gx_bitmap_id id
/* Define aliases for head members. */
#define cb_depth head.depth
/* Define aliases for common members formerly in the head. */
#define cb_raster raster
typedef struct gx_cached_bits_s {
    gx_cached_bits_common;
} gx_cached_bits;

#define cb_is_free(cb) cb_head_is_free(&(cb)->head)
/*
 * Define the alignment of the gx_cached_bits structure.  We must ensure
 * that an immediately following bitmap will be properly aligned.
 */
#define align_cached_bits_mod\
  (max(align_bitmap_mod, max(arch_align_ptr_mod, arch_align_long_mod)))

/*
 * We may allocate a bitmap cache in chunks, so as not to tie up memory
 * prematurely if it isn't needed (or something else needs it more).
 * Thus there is a structure for managing an entire cache, and another
 * structure for managing each chunk.
 */
typedef struct gx_bits_cache_chunk_s gx_bits_cache_chunk;
struct gx_bits_cache_chunk_s {
    gx_bits_cache_chunk *next;
    byte *data;			/* gx_cached_bits_head * */
    uint size;
    uint allocated;		/* amount of allocated data */
};

/* ---------------- Bitmap cache ---------------- */

#define gx_bits_cache_common\
	gx_bits_cache_chunk *chunks;	/* current chunk in circular list */\
	uint cnext;			/* rover for allocating entries */\
					/* in current chunk */\
	uint bsize;			/* total # of bytes for all entries */\
	uint csize		/* # of entries */
typedef struct gx_bits_cache_s {
    gx_bits_cache_common;
} gx_bits_cache;

/* ---------------- Procedural interface ---------------- */

/* ------ Entire cache ------ */

/* Initialize a cache.  The caller must allocate and initialize */
/* the first chunk. */
void gx_bits_cache_init(P2(gx_bits_cache *, gx_bits_cache_chunk *));

/* ------ Chunks ------ */

/* Initialize a chunk.  The caller must allocate it and its data. */
void gx_bits_cache_chunk_init(P3(gx_bits_cache_chunk *, byte *, uint));

/* ------ Individual entries ------ */

/* Attempt to allocate an entry.  If successful, set *pcbh and return 0. */
/* If there isn't enough room, set *pcbh to an entry requiring freeing, */
/* or to 0 if we are at the end of the chunk, and return -1. */
int gx_bits_cache_alloc(P3(gx_bits_cache *, ulong, gx_cached_bits_head **));

/* Shorten an entry by a given amount. */
void gx_bits_cache_shorten(P4(gx_bits_cache *, gx_cached_bits_head *,
			      uint, gx_bits_cache_chunk *));

/* Free an entry.  The caller is responsible for removing the entry */
/* from any other structures (like a hash table). */
void gx_bits_cache_free(P3(gx_bits_cache *, gx_cached_bits_head *,
			   gx_bits_cache_chunk *));

#endif /* gxbcache_INCLUDED */
