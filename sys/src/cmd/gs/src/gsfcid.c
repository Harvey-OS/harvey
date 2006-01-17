/* Copyright (C) 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gsfcid.c,v 1.14 2005/01/13 16:58:07 igor Exp $ */
/* Support for CID-keyed fonts */
#include "memory_.h"
#include "gx.h"
#include "gsmatrix.h"		/* for gsfont.h */
#include "gsstruct.h"
#include "gxfcid.h"
#include "gserrors.h"

/* CIDSystemInfo structure descriptors */
public_st_cid_system_info();
public_st_cid_system_info_element();

/* CID-keyed font structure descriptors */
public_st_gs_font_cid_data();
public_st_gs_font_cid0();
private
ENUM_PTRS_WITH(font_cid0_enum_ptrs, gs_font_cid0 *pfcid0)
{
    index -= 2;
    if (index < st_gs_font_cid_data_num_ptrs)
	return ENUM_USING(st_gs_font_cid_data, &pfcid0->cidata.common,
			  sizeof(gs_font_cid_data), index);
    ENUM_PREFIX(st_gs_font_base, st_gs_font_cid_data_num_ptrs);
}
ENUM_PTR(0, gs_font_cid0, cidata.FDArray);
ENUM_PTR(1, gs_font_cid0, cidata.proc_data);
ENUM_PTRS_END
private
RELOC_PTRS_WITH(font_cid0_reloc_ptrs, gs_font_cid0 *pfcid0);
    RELOC_PREFIX(st_gs_font_base);
    RELOC_USING(st_gs_font_cid_data, &pfcid0->cidata.common,
		sizeof(st_gs_font_cid_data));
    RELOC_VAR(pfcid0->cidata.FDArray);
    RELOC_VAR(pfcid0->cidata.proc_data);
RELOC_PTRS_END
public_st_gs_font_cid1();
private
ENUM_PTRS_WITH(font_cid1_enum_ptrs, gs_font_cid1 *pfcid1)
{
    if (index < st_cid_system_info_num_ptrs)
	return ENUM_USING(st_cid_system_info, &pfcid1->cidata.CIDSystemInfo,
			  sizeof(st_cid_system_info), index);
    ENUM_PREFIX(st_gs_font_base, st_cid_system_info_num_ptrs);
}
ENUM_PTRS_END
private
RELOC_PTRS_WITH(font_cid1_reloc_ptrs, gs_font_cid1 *pfcid1);
    RELOC_PREFIX(st_gs_font_base);
    RELOC_USING(st_cid_system_info, &pfcid1->cidata.CIDSystemInfo,
		sizeof(st_cid_system_info));
RELOC_PTRS_END
public_st_gs_font_cid2();
private
ENUM_PTRS_WITH(font_cid2_enum_ptrs, gs_font_cid2 *pfcid2)
{
    if (index < st_gs_font_cid_data_num_ptrs)
	return ENUM_USING(st_gs_font_cid_data, &pfcid2->cidata.common,
			  sizeof(gs_font_cid_data), index);
    ENUM_PREFIX(st_gs_font_type42, st_gs_font_cid_data_num_ptrs);
}
ENUM_PTRS_END
private
RELOC_PTRS_WITH(font_cid2_reloc_ptrs, gs_font_cid2 *pfcid2);
    RELOC_PREFIX(st_gs_font_type42);
    RELOC_USING(st_gs_font_cid_data, &pfcid2->cidata.common,
		sizeof(st_gs_font_cid_data));
RELOC_PTRS_END

/* GC descriptor for allocating FDArray for CIDFontType 0 fonts. */
gs_private_st_ptr(st_gs_font_type1_ptr, gs_font_type1 *, "gs_font_type1 *",
  font1_ptr_enum_ptrs, font1_ptr_reloc_ptrs);
gs_public_st_element(st_gs_font_type1_ptr_element, gs_font_type1 *,
  "gs_font_type1 *[]", font1_ptr_element_enum_ptrs,
  font1_ptr_element_reloc_ptrs, st_gs_font_type1_ptr);

/*
 * The CIDSystemInfo of a CMap may be null.  We represent this by setting
 * Registry and Ordering to empty strings, and Supplement to 0.
 */
void
cid_system_info_set_null(gs_cid_system_info_t *pcidsi)
{
    memset(pcidsi, 0, sizeof(*pcidsi));
}
bool
cid_system_info_is_null(const gs_cid_system_info_t *pcidsi)
{
    return (pcidsi->Registry.size == 0 && pcidsi->Ordering.size == 0 &&
	    pcidsi->Supplement == 0);
}

/*
 * Get the CIDSystemInfo of a font.  If the font is not a CIDFont,
 * return NULL.
 */
const gs_cid_system_info_t *
gs_font_cid_system_info(const gs_font *pfont)
{
    switch (pfont->FontType) {
    case ft_CID_encrypted:
	return &((const gs_font_cid0 *)pfont)->cidata.common.CIDSystemInfo;
    case ft_CID_user_defined:
	return &((const gs_font_cid1 *)pfont)->cidata.CIDSystemInfo;
    case ft_CID_TrueType:
	return &((const gs_font_cid2 *)pfont)->cidata.common.CIDSystemInfo;
    default:
	return 0;
    }
}

/*
 * Check CIDSystemInfo compatibility.
 */
bool 
gs_is_CIDSystemInfo_compatible(const gs_cid_system_info_t *info0, 
			       const gs_cid_system_info_t *info1)
{
    if (info0 == NULL || info1 == NULL)
	return false;
    if (info0->Registry.size != info1->Registry.size)
	return false;
    if (info0->Ordering.size !=	info1->Ordering.size)
	return false;
    if (memcmp(info0->Registry.data, info1->Registry.data, 
	       info0->Registry.size))
	return false;
    if (memcmp(info0->Ordering.data, info1->Ordering.data,
	       info0->Ordering.size))
	return false;
    return true;
}

/*
 * Provide a default enumerate_glyph procedure for CIDFontType 0 fonts.
 * Built for simplicity, not for speed.
 */
font_proc_enumerate_glyph(gs_font_cid0_enumerate_glyph); /* check prototype */
int
gs_font_cid0_enumerate_glyph(gs_font *font, int *pindex,
			     gs_glyph_space_t ignore_glyph_space,
			     gs_glyph *pglyph)
{
    gs_font_cid0 *const pfont = (gs_font_cid0 *)font;

    while (*pindex < pfont->cidata.common.CIDCount) {
	gs_glyph_data_t gdata;
	int fidx;
	gs_glyph glyph = (gs_glyph)(gs_min_cid_glyph + (*pindex)++);
	int code;

	gdata.memory = pfont->memory;
	code = pfont->cidata.glyph_data((gs_font_base *)pfont, glyph,
					    &gdata, &fidx);
	if (code < 0 || gdata.bits.size == 0)
	    continue;
	*pglyph = glyph;
	gs_glyph_data_free(&gdata, "gs_font_cid0_enumerate_glyphs");
	return 0;
    }
    *pindex = 0;
    return 0;
}

/* Return the font from the FDArray at the given index */
const gs_font *
gs_cid0_indexed_font(const gs_font *font, int fidx)
{
    gs_font_cid0 *const pfont = (gs_font_cid0 *)font;

    if (font->FontType != ft_CID_encrypted) {
	eprintf1("Unexpected font type: %d\n", font->FontType);
        return 0;
    }
    return (const gs_font*) (pfont->cidata.FDArray[fidx]);
}

/* Check whether a CID font has a Type 2 subfont. */
bool
gs_cid0_has_type2(const gs_font *font)
{
    gs_font_cid0 *const pfont = (gs_font_cid0 *)font;
    int i;

    if (font->FontType != ft_CID_encrypted) {
	eprintf1("Unexpected font type: %d\n", font->FontType);
        return false;
    }
    for (i = 0; i < pfont->cidata.FDArray_size; i++)
	if (((const gs_font *)pfont->cidata.FDArray[i])->FontType == ft_encrypted2)
	    return true;
    return false;
}
