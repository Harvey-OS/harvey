/* Copyright (C) 1997 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gxpcache.h,v 1.1 2000/03/09 08:40:43 lpd Exp $ */
/* Definition of Pattern cache */

#ifndef gxpcache_INCLUDED
#  define gxpcache_INCLUDED

/*
 * Define a cache for rendered Patterns.  This is currently an open
 * hash table with single probing (no reprobing) and round-robin
 * replacement.  Obviously, we can do better in both areas.
 */
#ifndef gx_pattern_cache_DEFINED
#  define gx_pattern_cache_DEFINED
typedef struct gx_pattern_cache_s gx_pattern_cache;

#endif
#ifndef gx_color_tile_DEFINED
#  define gx_color_tile_DEFINED
typedef struct gx_color_tile_s gx_color_tile;

#endif
struct gx_pattern_cache_s {
    gs_memory_t *memory;
    gx_color_tile *tiles;
    uint num_tiles;
    uint tiles_used;
    uint next;			/* round-robin index */
    ulong bits_used;
    ulong max_bits;
    void (*free_all) (P1(gx_pattern_cache *));
};

#define private_st_pattern_cache() /* in gxpcmap.c */\
  gs_private_st_ptrs1(st_pattern_cache, gx_pattern_cache,\
    "gx_pattern_cache", pattern_cache_enum, pattern_cache_reloc, tiles)

#endif /* gxpcache_INCLUDED */
