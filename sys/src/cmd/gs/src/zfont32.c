/* Copyright (C) 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: zfont32.c,v 1.5 2004/08/04 19:36:13 stefan Exp $ */
/* Type 32 font operators */
#include "ghost.h"
#include "oper.h"
#include "gsccode.h"		/* for gxfont.h */
#include "gsmatrix.h"
#include "gsutil.h"
#include "gxfont.h"
#include "bfont.h"
#include "store.h"

/* The encode_char procedure of a Type 32 font should never be called. */
private gs_glyph
zfont_no_encode_char(gs_font *pfont, gs_char chr, gs_glyph_space_t ignored)
{
    return gs_no_glyph;
}

/* <string|name> <font_dict> .buildfont32 <string|name> <font> */
/* Build a type 32 (bitmap) font. */
private int
zbuildfont32(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    int code;
    build_proc_refs build;
    gs_font_base *pfont;

    check_type(*op, t_dictionary);
    code = build_proc_name_refs(imemory, &build, NULL, "%Type32BuildGlyph");
    if (code < 0)
	return code;
    code = build_gs_simple_font(i_ctx_p, op, &pfont, ft_CID_bitmap,
				&st_gs_font_base, &build,
				bf_Encoding_optional);
    if (code < 0)
	return code;
    /* Always transform cached bitmaps. */
    pfont->BitmapWidths = true;
    pfont->ExactSize = fbit_transform_bitmaps;
    pfont->InBetweenSize = fbit_transform_bitmaps;
    pfont->TransformedChar = fbit_transform_bitmaps;
    /* The encode_char procedure of a Type 32 font */
    /* should never be called. */
    pfont->procs.encode_char = zfont_no_encode_char;
    return define_gs_font((gs_font *) pfont);
}

/* ------ Initialization procedure ------ */

const op_def zfont32_op_defs[] =
{
    {"2.buildfont32", zbuildfont32},
    op_def_end(0)
};
