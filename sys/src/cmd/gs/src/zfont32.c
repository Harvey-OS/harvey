/* Copyright (C) 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: zfont32.c,v 1.1 2000/03/09 08:40:45 lpd Exp $ */
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
    code = build_proc_name_refs(&build, NULL, "%Type32BuildGlyph");
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
