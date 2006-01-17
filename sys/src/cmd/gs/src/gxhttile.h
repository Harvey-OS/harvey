/* Copyright (C) 1995, 1996 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gxhttile.h,v 1.4 2002/02/21 22:24:53 giles Exp $ */
/* Halftone tile definition */
/* Requires gxbitmap.h */

#ifndef gxhttile_INCLUDED
#  define gxhttile_INCLUDED

/*
 * A halftone tile is just an ordinary bitmap tile, plus a couple of other
 * items associated with managing its existence in a tile cache.
 * (See gzht.h for details.)  We define this in its own file so that
 * clients of gx_device_color can access it.
 */

#ifndef gx_ht_tile_DEFINED
#  define gx_ht_tile_DEFINED
typedef struct gx_ht_tile_s gx_ht_tile;

#endif

struct gx_ht_tile_s {
    gx_strip_bitmap tiles;	/* the currently rendered tile */
    int level;			/* the cached gray level, i.e. */
    /* the number of spots whitened, */
    /* or -1 if the cache is empty */
    uint index;			/* the index of the tile within */
    /* the cache (for GC) */
};

#endif /* gxhttile_INCLUDED */
