/* Copyright (C) 1993, 1994, 1996, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: zcolor1.c,v 1.1 2000/03/09 08:40:44 lpd Exp $ */
/* Level 1 extended color operators */
#include "ghost.h"
#include "oper.h"
#include "estack.h"
#include "ialloc.h"
#include "igstate.h"
#include "iutil.h"
#include "store.h"
#include "gxfixed.h"
#include "gxmatrix.h"
#include "gzstate.h"
#include "gxdevice.h"
#include "gxcmap.h"
#include "gscolor1.h"
#include "gscssub.h"
#include "gxcspace.h"
#include "icolor.h"
#include "iimage.h"

/* - currentblackgeneration <proc> */
private int
zcurrentblackgeneration(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;

    push(1);
    *op = istate->black_generation;
    return 0;
}

/* - currentcmykcolor <cyan> <magenta> <yellow> <black> */
private int
zcurrentcmykcolor(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    float par[4];

    gs_currentcmykcolor(igs, par);
    push(4);
    make_floats(op - 3, par, 4);
    return 0;
}

/* - currentcolortransfer <redproc> <greenproc> <blueproc> <grayproc> */
private int
zcurrentcolortransfer(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;

    push(4);
    op[-3] = istate->transfer_procs.colored.red;
    op[-2] = istate->transfer_procs.colored.green;
    op[-1] = istate->transfer_procs.colored.blue;
    *op = istate->transfer_procs.colored.gray;
    return 0;
}

/* - currentundercolorremoval <proc> */
private int
zcurrentundercolorremoval(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;

    push(1);
    *op = istate->undercolor_removal;
    return 0;
}

/* <proc> setblackgeneration - */
private int
zsetblackgeneration(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    int code;

    check_proc(*op);
    check_ostack(zcolor_remap_one_ostack - 1);
    check_estack(1 + zcolor_remap_one_estack);
    code = gs_setblackgeneration_remap(igs, gs_mapped_transfer, false);
    if (code < 0)
	return code;
    istate->black_generation = *op;
    pop(1);
    push_op_estack(zcolor_remap_color);
    return zcolor_remap_one(i_ctx_p, &istate->black_generation,
			    igs->black_generation, igs,
			    zcolor_remap_one_finish);
}

/* <cyan> <magenta> <yellow> <black> setcmykcolor - */
private int
zsetcmykcolor(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    double par[4];
    int code;

    if ((code = num_params(op, 4, par)) < 0 ||
	(code = gs_setcmykcolor(igs, par[0], par[1], par[2], par[3])) < 0
	)
	return code;
    make_null(&istate->colorspace.array);
    pop(4);
    return 0;
}

/* <redproc> <greenproc> <blueproc> <grayproc> setcolortransfer - */
private int
zsetcolortransfer(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    int code;

    check_proc(op[-3]);
    check_proc(op[-2]);
    check_proc(op[-1]);
    check_proc(*op);
    check_ostack(zcolor_remap_one_ostack * 4 - 4);
    check_estack(1 + zcolor_remap_one_estack * 4);
    istate->transfer_procs.colored.red = op[-3];
    istate->transfer_procs.colored.green = op[-2];
    istate->transfer_procs.colored.blue = op[-1];
    istate->transfer_procs.colored.gray = *op;
    if ((code = gs_setcolortransfer_remap(igs,
				     gs_mapped_transfer, gs_mapped_transfer,
				     gs_mapped_transfer, gs_mapped_transfer,
					  false)) < 0
	)
	return code;
    /* Use osp rather than op here, because zcolor_remap_one pushes. */
    pop(4);
    push_op_estack(zcolor_reset_transfer);
    if ((code = zcolor_remap_one(i_ctx_p,
				 &istate->transfer_procs.colored.red,
				 igs->set_transfer.colored.red, igs,
				 zcolor_remap_one_finish)) < 0 ||
	(code = zcolor_remap_one(i_ctx_p,
				 &istate->transfer_procs.colored.green,
				 igs->set_transfer.colored.green, igs,
				 zcolor_remap_one_finish)) < 0 ||
	(code = zcolor_remap_one(i_ctx_p,
				 &istate->transfer_procs.colored.blue,
				 igs->set_transfer.colored.blue, igs,
				 zcolor_remap_one_finish)) < 0 ||
	(code = zcolor_remap_one(i_ctx_p, &istate->transfer_procs.colored.gray,
				 igs->set_transfer.colored.gray, igs,
				 zcolor_remap_one_finish)) < 0
	)
	return code;
    return o_push_estack;
}

/* <proc> setundercolorremoval - */
private int
zsetundercolorremoval(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    int code;

    check_proc(*op);
    check_ostack(zcolor_remap_one_ostack - 1);
    check_estack(1 + zcolor_remap_one_estack);
    code = gs_setundercolorremoval_remap(igs, gs_mapped_transfer, false);
    if (code < 0)
	return code;
    istate->undercolor_removal = *op;
    pop(1);
    push_op_estack(zcolor_remap_color);
    return zcolor_remap_one(i_ctx_p, &istate->undercolor_removal,
			    igs->undercolor_removal, igs,
			    zcolor_remap_one_signed_finish);
}

/* <width> <height> <bits/comp> <matrix> */
/*      <datasrc_0> ... <datasrc_ncomp-1> true <ncomp> colorimage - */
/*      <datasrc> false <ncomp> colorimage - */
private int
zcolorimage(i_ctx_t *i_ctx_p)
{
    return zimage_multiple(i_ctx_p, false);
}

/* ------ Initialization procedure ------ */

const op_def zcolor1_op_defs[] =
{
    {"0currentblackgeneration", zcurrentblackgeneration},
    {"0currentcmykcolor", zcurrentcmykcolor},
    {"0currentcolortransfer", zcurrentcolortransfer},
    {"0currentundercolorremoval", zcurrentundercolorremoval},
    {"1setblackgeneration", zsetblackgeneration},
    {"4setcmykcolor", zsetcmykcolor},
    {"4setcolortransfer", zsetcolortransfer},
    {"1setundercolorremoval", zsetundercolorremoval},
    {"7colorimage", zcolorimage},
    op_def_end(0)
};
