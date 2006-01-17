/* Copyright (C) 1997, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gsfcmap.h,v 1.9 2003/02/06 09:40:00 igor Exp $ */
/* Public interface to CMaps */

#ifndef gsfcmap_INCLUDED
#  define gsfcmap_INCLUDED

#include "gsccode.h"

/* Define the abstract type for a CMap. */
#ifndef gs_cmap_DEFINED
#  define gs_cmap_DEFINED
typedef struct gs_cmap_s gs_cmap_t;
#endif

/* ---------------- Procedural interface ---------------- */

/*
 * Create an Identity CMap.
 */
int gs_cmap_create_identity(gs_cmap_t **ppcmap, int num_bytes, int wmode,
			    gs_memory_t *mem);

/*
 * Create an Identity CMap where each entry varies only in the lowest byte,
 * and that maps to characters rather than to CIDs.
 * (This is suitable for use as a ToUnicode CMap, for example.)
 */
int gs_cmap_create_char_identity(gs_cmap_t **ppcmap, int num_bytes,
				 int wmode, gs_memory_t *mem);

/*
 * Decode a character from a string using a CMap, updating the index.
 * Return 0 for a CID or name, N > 0 for a character code where N is the
 * number of bytes in the code, or an error.  Store the decoded bytes in
 * *pchr.  For undefined characters, set *pglyph = gs_no_glyph and return 0.
 */
int gs_cmap_decode_next(const gs_cmap_t *pcmap, const gs_const_string *str,
			uint *pindex, uint *pfidx,
			gs_char *pchr, gs_glyph *pglyph);

/*
 * Allocate and initialize a ToUnicode CMap.
 */
int gs_cmap_ToUnicode_alloc(gs_memory_t *mem, int id, int num_codes, int key_size, 
			    gs_cmap_t **ppcmap);

/*
 * Write a code pair to ToUnicode CMap.
 */
void gs_cmap_ToUnicode_add_pair(gs_cmap_t *pcmap, int code0, int code2);

#endif /* gsfcmap_INCLUDED */
