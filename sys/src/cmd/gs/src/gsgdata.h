/* Copyright (C) 2001 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gsgdata.h,v 1.5 2004/11/15 01:12:06 ray Exp $ */
/* Interface for glyph data access */

#ifndef gsgdata_INCLUDED
#  define gsgdata_INCLUDED

#include "gsstype.h"		/* for extern_st */

/*
 * Define the structure used to return the data for a glyph upon
 * request.  "Data" currently means the bytes of a Type 1, TrueType, or
 * similar scalable outline, or the bits of a bitmap (not currently used).
 */

/* ------ Information for clients ------ */

/*
 * Clients that get glyph data (for example, using the get_outline procedure
 * of a Type 42 or a CIDFontType 2 font) do so as follows:

	gs_glyph_data_t gdata;
	int code;
	...
	code = ...get_outline(...&gdata...);

 *
 * If code >= 0 (no error), gdata.bits.{data,size} point to the outline data.
 *
 * Since the data may have been allocated in response to the request,
 * when the client is finished with the data, it should call:

	gs_glyph_data_free(&gdata, "client name");

 * This is a no-op if the data are stored in the font, but an actual freeing
 * procedure if they were allocated by get_outline.
 */

/* ------ Structure declaration ------ */

typedef struct gs_glyph_data_procs_s gs_glyph_data_procs_t;
#ifndef gs_glyph_data_DEFINED
#   define gs_glyph_data_DEFINED
typedef struct gs_glyph_data_s gs_glyph_data_t;
#endif
struct gs_glyph_data_s {
    gs_const_bytestring bits;	/* pointer to actual data returned here */
    const gs_glyph_data_procs_t *procs;
    void *proc_data;
    gs_memory_t *memory;	/* allocator to use (may be different than font) */
};
extern_st(st_glyph_data);
#define ST_GLYPH_DATA_NUM_PTRS 2

/*
 * NOTE: Clients must not call these procedures directly.  Use the
 * gs_glyph_data_{substring,free} procedures declared below.
 */
struct gs_glyph_data_procs_s {
#define GS_PROC_GLYPH_DATA_FREE(proc)\
  void proc(gs_glyph_data_t *pgd, client_name_t cname)
    GS_PROC_GLYPH_DATA_FREE((*free));
#define GS_PROC_GLYPH_DATA_SUBSTRING(proc)\
  int proc(gs_glyph_data_t *pgd, uint offset, uint size)
    GS_PROC_GLYPH_DATA_SUBSTRING((*substring));
};

/*
 * Replace glyph data by a substring.  If the data were allocated by
 * get_outline et al, this frees the part of the data outside the substring.
 */
int gs_glyph_data_substring(gs_glyph_data_t *pgd, uint offset, uint size);

/*
 * Free the data for a glyph if they were allocated by get_outline et al.
 * This also does the equivalent of a from_null (see below) so that
 * multiple calls of this procedure are harmless.
 */
void gs_glyph_data_free(gs_glyph_data_t *pgd, client_name_t cname);

/* ------ Information for implementors of get_outline et al ------ */

/*
 * The implementor of get_outline or similar procedures should set the
 * client's glyph_data_t structure as follows:

	...get_outline...(...gs_font *pfont...gs_glyph_data_t *pgd...)
	{
	    ...
	    gs_glyph_data_from_string(pgd, odata, osize, NULL);
   (or)	    gs_glyph_data_from_string(pgd, odata, osize, pfont);
	}

 * If the data are in an object rather then a string, use

	gs_glyph_data_from_bytes(pgd, obase, ooffset, osize, <NULL|pfont>);

 * The last argument of gs_glyph_data_from_{string|bytes} should be pfont
 * iff odata/osize were allocated by this call and will not be retained
 * by the implementor (i.e., should be freed when the client calls
 * gs_glyph_data_free), NULL otherwise.
 */

/*
 * Initialize glyph data from a string or from bytes.
 */
#ifndef gs_font_DEFINED
#  define gs_font_DEFINED
typedef struct gs_font_s gs_font;
#endif
void gs_glyph_data_from_string(gs_glyph_data_t *pgd, const byte *data,
			       uint size, gs_font *font);
void gs_glyph_data_from_bytes(gs_glyph_data_t *pgd, const byte *bytes,
			      uint offset, uint size, gs_font *font);
/* from_null(pgd) is a shortcut for from_string(pgd, NULL, 0, NULL). */
void gs_glyph_data_from_null(gs_glyph_data_t *pgd);

#endif /* gsgdata_INCLUDED */
