/* Copyright (C) 1996, 2000, 2001 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gsgcache.h,v 1.2 2004/11/15 01:12:06 ray Exp $ */
/* Glyph data cache definition. */

#ifndef gxgcache_INCLUDED
#  define gxgcache_INCLUDED

/* Using : */

#ifndef gs_font_type42_DEFINED
#  define gs_font_type42_DEFINED
typedef struct gs_font_type42_s gs_font_type42;
#endif

#ifndef gs_glyph_data_DEFINED
#   define gs_glyph_data_DEFINED
typedef struct gs_glyph_data_s gs_glyph_data_t;
#endif

#ifndef stream_DEFINED
#  define stream_DEFINED
typedef struct stream_s stream;
#endif

/* Data type definition : */

#ifndef gs_glyph_cache_DEFINED
#  define gs_glyph_cache_DEFINED
typedef struct gs_glyph_cache_s gs_glyph_cache;
#endif

typedef int (*get_glyph_data_from_file)(gs_font_type42 *pfont, stream *s, uint glyph_index,
		gs_glyph_data_t *pgd);


/* Methods : */

gs_glyph_cache *gs_glyph_cache__alloc(gs_font_type42 *pfont, stream *s,
			get_glyph_data_from_file read_data);
int gs_glyph_cache__release(void /* gs_glyph_cache */ *data, void*);
int gs_get_glyph_data_cached(gs_font_type42 *pfont, uint glyph_index, gs_glyph_data_t *pgd);

#endif /* gxgcache_INCLUDED */
