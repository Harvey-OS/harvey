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

/* $Id: gdevpdt.c,v 1.1 2002/06/25 01:51:32 lpd Exp $ */
/* Miscellaneous external entry points for pdfwrite text */
#include "gx.h"
#include "memory_.h"
#include "gdevpdfx.h"
#include "gdevpdtx.h"
#include "gdevpdtf.h"
#include "gdevpdti.h"
#include "gdevpdts.h"

/* GC descriptors */
private_st_pdf_text_data();

/* ---------------- Initialization ---------------- */

/*
 * Allocate and initialize the text data structure.
 */
pdf_text_data_t *
pdf_text_data_alloc(gs_memory_t *mem)
{
    pdf_text_data_t *ptd =
	gs_alloc_struct(mem, pdf_text_data_t, &st_pdf_text_data,
			"pdf_text_data_alloc");
    pdf_outline_fonts_t *pofs = pdf_outline_fonts_alloc(mem);
    pdf_bitmap_fonts_t *pbfs = pdf_bitmap_fonts_alloc(mem);
    pdf_text_state_t *pts = pdf_text_state_alloc(mem);

    if (pts == 0 || pbfs == 0 || pofs == 0 || ptd == 0) {
	gs_free_object(mem, pts, "pdf_text_data_alloc");
	gs_free_object(mem, pbfs, "pdf_text_data_alloc");
	gs_free_object(mem, pofs, "pdf_text_data_alloc");
	gs_free_object(mem, ptd, "pdf_text_data_alloc");
	return 0;
    }
    memset(ptd, 0, sizeof(*ptd));
    ptd->outline_fonts = pofs;
    ptd->bitmap_fonts = pbfs;
    ptd->text_state = pts;
    return ptd;
}
