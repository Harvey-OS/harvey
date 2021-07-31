/* Copyright (C) 1996, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: zcid.c,v 1.1 2000/03/09 08:40:44 lpd Exp $ */
/* CID-keyed font operators */
#include "ghost.h"
#include "oper.h"
#include "gsmatrix.h"
#include "gsccode.h"
#include "gxfont.h"
#include "bfont.h"
#include "iname.h"
#include "store.h"

/* Imported from zfont42.c */
int build_gs_TrueType_font(P6(i_ctx_t *i_ctx_p, os_ptr op, font_type ftype,
			      const char *bcstr, const char *bgstr,
			      build_font_options_t options));

/* <string|name> <font_dict> .buildfont9/10 <string|name> <font> */
/* Build a type 9 or 10 (CID-keyed) font. */
/* Right now, we treat these like type 3 (with a BuildGlyph procedure). */
private int
build_gs_cid_font(i_ctx_t *i_ctx_p, os_ptr op, font_type ftype,
		  const build_proc_refs * pbuild)
{
    int code;
    gs_font_base *pfont;

    check_type(*op, t_dictionary);
    code = build_gs_simple_font(i_ctx_p, op, &pfont, ftype, &st_gs_font_base,
				pbuild,
				bf_Encoding_optional |
				bf_FontBBox_required |
				bf_UniqueID_ignored);
    if (code < 0)
	return code;
    return define_gs_font((gs_font *) pfont);
}
private int
zbuildfont9(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    build_proc_refs build;
    int code = build_proc_name_refs(&build, NULL, "%Type9BuildGlyph");

    if (code < 0)
	return code;
    return build_gs_cid_font(i_ctx_p, op, ft_CID_encrypted, &build);
}
private int
zbuildfont10(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    build_proc_refs build;
    int code = build_gs_font_procs(op, &build);

    if (code < 0)
	return code;
    make_null(&build.BuildChar);	/* only BuildGlyph */
    return build_gs_cid_font(i_ctx_p, op, ft_CID_user_defined, &build);
}

/* <string|name> <font_dict> .buildfont11 <string|name> <font> */
private int
zbuildfont11(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    return build_gs_TrueType_font(i_ctx_p, op, ft_CID_TrueType,
				  (const char *)0, "%Type11BuildGlyph",
				  bf_Encoding_optional |
				  bf_FontBBox_required |
				  bf_UniqueID_ignored |
				  bf_CharStrings_optional);
}

/* ------ Initialization procedure ------ */

const op_def zcid_op_defs[] =
{
    {"2.buildfont9", zbuildfont9},
    {"2.buildfont10", zbuildfont10},
    {"2.buildfont11", zbuildfont11},
    op_def_end(0)
};
