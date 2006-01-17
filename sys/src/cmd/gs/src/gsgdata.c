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

/* $Id: gsgdata.c,v 1.3 2002/02/21 22:24:52 giles Exp $ */
/* Support for glyph data access */

#include "memory_.h"
#include "gx.h"
#include "gsgdata.h"
#include "gserrors.h"
#include "gsmatrix.h"		/* for gsfont.h */
#include "gsstruct.h"
#include "gxfont.h"

/* GC structure descriptor */
private ENUM_PTRS_WITH(gs_glyph_data_enum_ptrs, gs_glyph_data_t *pgd)
    case 0: return ENUM_CONST_BYTESTRING(&pgd->bits);
    case 1: return ENUM_OBJ(pgd->proc_data);
ENUM_PTRS_END
private RELOC_PTRS_WITH(gs_glyph_data_reloc_ptrs, gs_glyph_data_t *pgd)
{
    RELOC_CONST_BYTESTRING_VAR(pgd->bits);
    RELOC_OBJ_VAR(pgd->proc_data);
} RELOC_PTRS_END
gs_public_st_composite(st_glyph_data, gs_glyph_data_t, "gs_glyph_data_t",
		       gs_glyph_data_enum_ptrs, gs_glyph_data_reloc_ptrs);

/* ------ Client facilities ------ */

/* Replace glyph data by a substring. */
int
gs_glyph_data_substring(gs_glyph_data_t *pgd, uint offset, uint size)
{
    if (offset > pgd->bits.size || size > pgd->bits.size - offset)
	return_error(gs_error_rangecheck);
    return pgd->procs->substring(pgd, offset, size);
}

/* Free the data for a glyph. */
void
gs_glyph_data_free(gs_glyph_data_t *pgd, client_name_t cname)
{
    pgd->procs->free(pgd, cname);
    gs_glyph_data_from_null(pgd);
}

/* ------ Implementor support ------ */

/* Don't manage the glyph data. */
private void
glyph_data_free_permanent(gs_glyph_data_t *pgd, client_name_t cname)
{
}
private int
glyph_data_substring_permanent(gs_glyph_data_t *pgd, uint offset, uint size)
{
    pgd->bits.data += offset;
    pgd->bits.size = size;
    return 0;
}			       

/* Manage the glyph data using the font's allocator. */
private void
glyph_data_free_by_font(gs_glyph_data_t *pgd, client_name_t cname)
{
    gs_free_const_bytestring(((gs_font *)pgd->proc_data)->memory,
			     &pgd->bits, cname);
}
private int
glyph_data_substring_by_font(gs_glyph_data_t *pgd, uint offset, uint size)
{
    gs_font *const font = pgd->proc_data;
    byte *data = (byte *)pgd->bits.data; /* break const */

    if (pgd->bits.bytes)	/* object, not string */
	return glyph_data_substring_permanent(pgd, offset, size);
    if (offset > 0)
	memmove(data, data + offset, size);
    pgd->bits.data = 
	gs_resize_string(font->memory, data, pgd->bits.size, size,
			 "glyph_data_substring"); /* shortening, can't fail */
    pgd->bits.size = size;
    return 0;
}			       

private const gs_glyph_data_procs_t no_free_procs = {
    glyph_data_free_permanent, glyph_data_substring_permanent
};
private const gs_glyph_data_procs_t free_by_font_procs = {
    glyph_data_free_by_font, glyph_data_substring_by_font
};

/*
 * Initialize glyph data from a string or from bytes.  If the font is NULL
 * (for glyph data that is part of the font), set the glyph data freeing
 * procedure to "do not free"; if the font is not NULL (for just-allocated
 * glyph data), set the freeing procedure to "free using the font's
 * allocator."
 */
void
gs_glyph_data_from_string(gs_glyph_data_t *pgd, const byte *data,
			  uint size, gs_font *font)
{
    gs_bytestring_from_string(&pgd->bits, data, size);
    pgd->proc_data = font;
    pgd->procs = (font ? &free_by_font_procs : &no_free_procs);
}
void
gs_glyph_data_from_bytes(gs_glyph_data_t *pgd, const byte *bytes,
			 uint offset, uint size, gs_font *font)
{
    gs_bytestring_from_bytes(&pgd->bits, bytes, offset, size);
    pgd->proc_data = font;
    pgd->procs = (font ? &free_by_font_procs : &no_free_procs);
}
void
gs_glyph_data_from_null(gs_glyph_data_t *pgd)
{
    gs_glyph_data_from_string(pgd, NULL, 0, NULL);
}
