/* Copyright (C) 1997 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gxpcache.h,v 1.5 2002/06/16 08:45:43 lpd Exp $ */
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
    void (*free_all) (gx_pattern_cache *);
};

#define private_st_pattern_cache() /* in gxpcmap.c */\
  gs_private_st_ptrs1(st_pattern_cache, gx_pattern_cache,\
    "gx_pattern_cache", pattern_cache_enum, pattern_cache_reloc, tiles)

#endif /* gxpcache_INCLUDED */
