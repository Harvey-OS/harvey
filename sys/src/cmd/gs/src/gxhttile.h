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

/*$Id: gxhttile.h,v 1.1 2000/03/09 08:40:43 lpd Exp $ */
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
