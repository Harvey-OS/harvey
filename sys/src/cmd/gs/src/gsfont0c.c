/* Copyright (C) 2002 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gsfont0c.c,v 1.4 2002/09/08 20:29:07 igor Exp $ */
/* Create a FontType 0 wrapper for a CIDFont. */
#include "memory_.h"
#include "gx.h"
#include "gserrors.h"
#include "gxfont.h"
#include "gxfcid.h"
#include "gxfcmap.h"
#include "gxfont0.h"
#include "gxfont0c.h"

/* ---------------- Private ---------------- */

/* Create a Type 0 wrapper for a CIDFont/CMap pair. */
private int
type0_from_cidfont_cmap(gs_font_type0 **ppfont0, gs_font *font,
			gs_cmap_t *pcmap, int wmode, const gs_matrix *psmat,
			gs_memory_t *mem)
{
    gs_font_type0 *font0 = (gs_font_type0 *)
	gs_font_alloc(mem, &st_gs_font_type0, &gs_font_procs_default, NULL,
		      "gs_type0_from_cidfont(font)");
    /* We allocate Encoding dynamically only for the sake of the GC. */
    uint *encoding = (uint *)
	gs_alloc_bytes(mem, sizeof(uint), "gs_type0_from_cidfont(Encoding)");
    gs_font **fdep =
	gs_alloc_struct_array(mem, 1, gs_font *, &st_gs_font_ptr_element,
			      "gs_type0_from_cidfont(FDepVector)");
    int code;

    if (font0 == 0 || encoding == 0 || fdep == 0) {
	gs_free_object(mem, fdep, "gs_type0_from_cidfont(FDepVector)");
	gs_free_object(mem, encoding, "gs_type0_from_cidfont(Encoding)");
	gs_free_object(mem, font0, "gs_type0_from_cidfont(font)");
	return_error(gs_error_VMerror);
    }
    if (psmat)
	font0->FontMatrix = *psmat;
    else
	gs_make_identity(&font0->FontMatrix);
    font0->FontType = ft_composite;
    font0->procs.init_fstack = gs_type0_init_fstack;
    font0->procs.define_font = gs_no_define_font;
    font0->procs.make_font = 0; /* not called */
    font0->procs.next_char_glyph = gs_type0_next_char_glyph;
    font0->key_name = font->key_name;
    font0->font_name = font->font_name;
    font0->data.FMapType = fmap_CMap;
    encoding[0] = 0;
    font0->data.Encoding = encoding;
    font0->data.encoding_size = 1;
    fdep[0] = font;
    font0->data.FDepVector = fdep;
    font0->data.fdep_size = 1;
    font0->data.CMap = pcmap;
    font0->data.SubsVector.data = 0;
    font0->data.SubsVector.size = 0;
    code = gs_definefont(font->dir, (gs_font *)font0);
    if (code < 0)
	return code;
    *ppfont0 = font0;
    return 0;
}

/* ---------------- Public entries ---------------- */

/* Create a Type 0 wrapper for a CIDFont. */
int
gs_font_type0_from_cidfont(gs_font_type0 **ppfont0, gs_font *font, int wmode,
			   const gs_matrix *psmat, gs_memory_t *mem)
{
    gs_cmap_t *pcmap;
    int code = gs_cmap_create_identity(&pcmap, 2, wmode, mem);

    if (code < 0)
	return code;
    code = type0_from_cidfont_cmap(ppfont0, font, pcmap, wmode, psmat, mem);
    if (code < 0) {
	gs_free_object(mem, pcmap, "gs_font_type0_from_cidfont(CMap)");
	/****** FREE SUBSTRUCTURES -- HOW? ******/
    }
    return code;
}

/*
 * Create a Type 0 font wrapper for a Type 42 font (converted to a Type 2
 * CIDFont), optionally using the TrueType cmap as the CMap.
 * See gs_cmap_from_type42_cmap for details.
 */
int
gs_font_type0_from_type42(gs_font_type0 **ppfont0, gs_font_type42 *pfont42,
			  int wmode, bool use_cmap, gs_memory_t *mem)
{
    gs_font_cid2 *pfcid;
    gs_font_type0 *pfont0;
    int code = gs_font_cid2_from_type42(&pfcid, pfont42, wmode, mem);

    if (code < 0)
	return code;
    if (use_cmap) {
	gs_cmap_t *pcmap;

	code = gs_cmap_from_type42_cmap(&pcmap, pfont42, wmode, mem);
	if (code < 0)
	    return code;
	code = type0_from_cidfont_cmap(&pfont0, (gs_font *)pfcid, pcmap,
				       wmode, NULL, mem);
    } else {
	code = gs_font_type0_from_cidfont(&pfont0, (gs_font *)pfcid, wmode,
					  NULL, mem);
    }
    if (code < 0) {
	gs_free_object(mem, pfcid, "gs_type0_from_type42(CIDFont)");
	/****** FREE SUBSTRUCTURES -- HOW? ******/
	return code;
    }

    *ppfont0 = pfont0;
    return 0;
}
