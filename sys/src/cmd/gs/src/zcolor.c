/* Copyright (C) 1989, 1992, 1993, 1994, 1996, 1997, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: zcolor.c,v 1.1 2000/03/09 08:40:44 lpd Exp $ */
/* Color operators */
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
#include "icolor.h"

/* Imported from gsht.c */
void gx_set_effective_transfer(P1(gs_state *));

/* Define the number of stack slots needed for zcolor_remap_one. */
const int zcolor_remap_one_ostack = 4;
const int zcolor_remap_one_estack = 3;

/* - currentgray <gray> */
private int
zcurrentgray(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;

    push(1);
    make_real(op, gs_currentgray(igs));
    return 0;
}

/* - currentrgbcolor <red> <green> <blue> */
private int
zcurrentrgbcolor(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    float par[3];

    gs_currentrgbcolor(igs, par);
    push(3);
    make_floats(op - 2, par, 3);
    return 0;
}

/* - currenttransfer <proc> */
private int
zcurrenttransfer(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;

    push(1);
    *op = istate->transfer_procs.colored.gray;
    return 0;
}

/* - processcolors <int> - */
/* Note: this is an undocumented operator that is not supported */
/* in Level 2. */
private int
zprocesscolors(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;

    push(1);
    make_int(op, gs_currentdevice(igs)->color_info.num_components);
    return 0;
}

/* <gray> setgray - */
private int
zsetgray(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    double gray;
    int code;

    if (real_param(op, &gray) < 0)
	return_op_typecheck(op);
    if ((code = gs_setgray(igs, gray)) < 0)
	return code;
    make_null(&istate->colorspace.array);
    pop(1);
    return 0;
}

/* <red> <green> <blue> setrgbcolor - */
private int
zsetrgbcolor(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    double par[3];
    int code;

    if ((code = num_params(op, 3, par)) < 0 ||
	(code = gs_setrgbcolor(igs, par[0], par[1], par[2])) < 0
	)
	return code;
    make_null(&istate->colorspace.array);
    pop(3);
    return 0;
}

/* <proc> settransfer - */
private int
zsettransfer(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    int code;

    check_proc(*op);
    check_ostack(zcolor_remap_one_ostack - 1);
    check_estack(1 + zcolor_remap_one_estack);
    istate->transfer_procs.colored.red =
	istate->transfer_procs.colored.green =
	istate->transfer_procs.colored.blue =
	istate->transfer_procs.colored.gray = *op;
    code = gs_settransfer_remap(igs, gs_mapped_transfer, false);
    if (code < 0)
	return code;
    push_op_estack(zcolor_reset_transfer);
    pop(1);
    return zcolor_remap_one(i_ctx_p, &istate->transfer_procs.colored.gray,
			    igs->set_transfer.colored.gray, igs,
			    zcolor_remap_one_finish);
}

/* ------ Internal routines ------ */

/* Prepare to remap one color component */
/* (also used for black generation and undercolor removal). */
/* Use the 'for' operator to gather the values. */
/* The caller must have done the necessary check_ostack and check_estack. */
int
zcolor_remap_one(i_ctx_t *i_ctx_p, const ref * pproc,
		 gx_transfer_map * pmap, const gs_state * pgs,
		 op_proc_t finish_proc)
{
    os_ptr op;

    /*
     * Detect the identity function, which is a common value for one or
     * more of these functions.
     */
    if (r_size(pproc) == 0) {
	pmap->proc = gs_identity_transfer;
	/*
	 * Even though we don't actually push anything on the e-stack, all
	 * clients do, so we return o_push_estack in this case.  This is
	 * needed so that clients' finishing procedures will get run.
	 */
	return o_push_estack;
    }
    op = osp += 4;
    make_int(op - 3, 0);
    make_int(op - 2, 1);
    make_int(op - 1, transfer_map_size - 1);
    *op = *pproc;
    ++esp;
    make_struct(esp, imemory_space((gs_ref_memory_t *) pgs->memory),
		pmap);
    push_op_estack(finish_proc);
    push_op_estack(zfor_fraction);
    return o_push_estack;
}

/* Store the result of remapping a component. */
private int
zcolor_remap_one_store(i_ctx_t *i_ctx_p, floatp min_value)
{
    int i;
    gx_transfer_map *pmap = r_ptr(esp, gx_transfer_map);

    if (ref_stack_count(&o_stack) < transfer_map_size)
	return_error(e_stackunderflow);
    for (i = 0; i < transfer_map_size; i++) {
	double v;
	int code =
	    real_param(ref_stack_index(&o_stack, transfer_map_size - 1 - i),
		       &v);

	if (code < 0)
	    return code;
	pmap->values[i] =
	    (v < min_value ? float2frac(min_value) :
	     v >= 1.0 ? frac_1 :
	     float2frac(v));
    }
    ref_stack_pop(&o_stack, transfer_map_size);
    esp--;			/* pop pointer to transfer map */
    return o_pop_estack;
}
int
zcolor_remap_one_finish(i_ctx_t *i_ctx_p)
{
    return zcolor_remap_one_store(i_ctx_p, 0.0);
}
int
zcolor_remap_one_signed_finish(i_ctx_t *i_ctx_p)
{
    return zcolor_remap_one_store(i_ctx_p, -1.0);
}

/* Finally, reset the effective transfer functions and */
/* invalidate the current color. */
int
zcolor_reset_transfer(i_ctx_t *i_ctx_p)
{
    gx_set_effective_transfer(igs);
    return zcolor_remap_color(i_ctx_p);
}
int
zcolor_remap_color(i_ctx_t *i_ctx_p)
{
    gx_unset_dev_color(igs);
    return 0;
}

/* ------ Initialization procedure ------ */

const op_def zcolor_op_defs[] =
{
    {"0currentgray", zcurrentgray},
    {"0currentrgbcolor", zcurrentrgbcolor},
    {"0currenttransfer", zcurrenttransfer},
    {"0processcolors", zprocesscolors},
    {"1setgray", zsetgray},
    {"3setrgbcolor", zsetrgbcolor},
    {"1settransfer", zsettransfer},
		/* Internal operators */
    {"1%zcolor_remap_one_finish", zcolor_remap_one_finish},
    {"1%zcolor_remap_one_signed_finish", zcolor_remap_one_signed_finish},
    {"0%zcolor_reset_transfer", zcolor_reset_transfer},
    {"0%zcolor_remap_color", zcolor_remap_color},
    op_def_end(0)
};
