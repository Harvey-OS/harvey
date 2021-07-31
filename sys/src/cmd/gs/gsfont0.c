/* Copyright (C) 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* gsfont0.c */
/* Composite font operations for Ghostscript library */
#include "memory_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsstruct.h"
#include "gxfixed.h"
#include "gsmatrix.h"
#include "gxdevice.h"
#include "gxdevmem.h"
#include "gxchar.h"
#include "gxfcache.h"			/* gs_font_dir */
#include "gxfont.h"
#include "gxfont0.h"

/* Structure descriptor */
public_st_gs_font_type0();
private ENUM_PTRS_BEGIN(font_type0_enum_ptrs) ENUM_PREFIX(st_gs_font, 3);
#define pfont ((gs_font_type0 *)vptr)
	ENUM_PTR(0, gs_font_type0, data.Encoding);
	ENUM_PTR(1, gs_font_type0, data.FDepVector);
	case 2:
		if ( pfont->data.FMapType == fmap_SubsVector )
		  {	*pep = (void *)&pfont->data.SubsVector;
			return ptr_const_string_type;
		  }
		else
		  {	*pep = 0;
			break;
		  }
ENUM_PTRS_END
private RELOC_PTRS_BEGIN(font_type0_reloc_ptrs) RELOC_PREFIX(st_gs_font);
	RELOC_PTR(gs_font_type0, data.Encoding);
	RELOC_PTR(gs_font_type0, data.FDepVector);
	if ( pfont->data.FMapType == fmap_SubsVector )
	  RELOC_CONST_STRING_PTR(gs_font_type0, data.SubsVector);
#undef pfont
RELOC_PTRS_END

/* Adjust a composite font by concatenating a given matrix */
/* to the FontMatrix of all descendant composite fonts. */
private int
gs_type0_adjust_matrix(gs_font_dir *pdir, gs_font_type0 *pfont,
  const gs_matrix *pmat)
{	gs_font **pdep = pfont->data.FDepVector;
	uint fdep_size = pfont->data.fdep_size;
	gs_font **ptdep;
	uint i;
	/* Check for any descendant composite fonts. */
	for ( i = 0; i < fdep_size; i++ )
	  if ( pdep[i]->FontType == ft_composite )
	    break;
	if ( i == fdep_size )
		return 0;
	ptdep = gs_alloc_struct_array(pfont->memory, fdep_size, gs_font *,
				      &st_gs_font_ptr_element,
				      "gs_type0_adjust_font(FDepVector)");
	if ( ptdep == 0 )
		return_error(gs_error_VMerror);
	memcpy(ptdep, pdep, sizeof(gs_font *) * fdep_size);
	for ( ; i < fdep_size; i++ )
	  if ( pdep[i]->FontType == ft_composite )
	{	int code = gs_makefont(pdir, pdep[i], pmat, &ptdep[i]);
		if ( code < 0 )
			return code;
	}
	pfont->data.FDepVector = ptdep;
	return 0;
}

/* Finish defining a composite font, */
/* by adjusting its descendants' FontMatrices. */
int
gs_type0_define_font(gs_font_dir *pdir, gs_font *pfont)
{	const gs_matrix *pmat = &pfont->FontMatrix;
	/* Check for the identity matrix, which is common in root fonts. */
	if ( pmat->xx == 1.0 && pmat->yy == 1.0 &&
	     pmat->xy == 0.0 && pmat->yx == 0.0 &&
	     pmat->tx == 0.0 && pmat->ty == 0.0
	   )
		return 0;
	return gs_type0_adjust_matrix(pdir, (gs_font_type0 *)pfont, pmat);
}

/* Finish scaling a composite font similarly. */
int
gs_type0_make_font(gs_font_dir *pdir, const gs_font *pfont,
  const gs_matrix *pmat, gs_font **ppfont)
{	return gs_type0_adjust_matrix(pdir, (gs_font_type0 *)*ppfont, pmat);
}
