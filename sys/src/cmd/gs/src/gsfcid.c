/* Copyright (C) 2000 Aladdin Enterprises.  All rights reserved.
  
  This file is part of AFPL Ghostscript.
  
  AFPL Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author or
  distributor accepts any responsibility for the consequences of using it, or
  for whether it serves any particular purpose or works at all, unless he or
  she says so in writing.  Refer to the Aladdin Free Public License (the
  "License") for full details.
  
  Every copy of AFPL Ghostscript must include a copy of the License, normally
  in a plain ASCII text file named PUBLIC.  The License grants you the right
  to copy, modify and redistribute AFPL Ghostscript, but only under certain
  conditions described in the License.  Among other things, the License
  requires that the copyright notice and this notice be preserved on all
  copies.
*/

/*$Id: gsfcid.c,v 1.4 2000/09/19 19:00:28 lpd Exp $ */
/* Support for CID-keyed fonts */
#include "memory_.h"
#include "gx.h"
#include "gsmatrix.h"		/* for gsfont.h */
#include "gsstruct.h"
#include "gxfcid.h"

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
	gs_const_string gstr;
	int fidx;
	gs_glyph glyph = (gs_glyph)(gs_min_cid_glyph + (*pindex)++);
	int code = pfont->cidata.glyph_data((gs_font_base *)pfont, glyph,
					    &gstr, &fidx);

	if (code < 0 || gstr.size == 0)
	    continue;
	*pglyph = glyph;
	return 0;
    }
    *pindex = 0;
    return 0;
}
