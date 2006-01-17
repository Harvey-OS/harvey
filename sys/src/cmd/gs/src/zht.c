/* Copyright (C) 1989, 1991, 1993, 1994, 1997, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: zht.c,v 1.8 2005/10/11 10:04:28 leonardo Exp $ */
/* Halftone definition operators */
#include "ghost.h"
#include "memory_.h"
#include "oper.h"
#include "estack.h"
#include "gsstruct.h"		/* must precede igstate.h, */
					/* because of #ifdef in gsht.h */
#include "ialloc.h"
#include "igstate.h"
#include "gsmatrix.h"
#include "gxdevice.h"		/* for gzht.h */
#include "gzht.h"
#include "gsstate.h"
#include "iht.h"		/* prototypes */
#include "store.h"

/* Forward references */
private int screen_sample(i_ctx_t *);
private int set_screen_continue(i_ctx_t *);
private int screen_cleanup(i_ctx_t *);

/* - .currenthalftone <dict> 0 */
/* - .currenthalftone <frequency> <angle> <proc> 1 */
/* - .currenthalftone <red_freq> ... <gray_proc> 2 */
private int
zcurrenthalftone(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    gs_halftone ht;

    gs_currenthalftone(igs, &ht);
    switch (ht.type) {
	case ht_type_screen:
	    push(4);
	    make_real(op - 3, ht.params.screen.frequency);
	    make_real(op - 2, ht.params.screen.angle);
	    op[-1] = istate->screen_procs.gray;
	    make_int(op, 1);
	    break;
	case ht_type_colorscreen:
	    push(13);
	    {
		os_ptr opc = op - 12;
		gs_screen_halftone *pht = 
		    &ht.params.colorscreen.screens.colored.red;

		make_real(opc, pht->frequency);
		make_real(opc + 1, pht->angle);
		opc[2] = istate->screen_procs.red;

		opc = op - 9;
		pht = &ht.params.colorscreen.screens.colored.green;
		make_real(opc, pht->frequency);
		make_real(opc + 1, pht->angle);
		opc[2] = istate->screen_procs.green;

		opc = op - 6;
		pht = &ht.params.colorscreen.screens.colored.blue;
		make_real(opc, pht->frequency);
		make_real(opc + 1, pht->angle);
		opc[2] = istate->screen_procs.blue;

		opc = op - 3;
		pht = &ht.params.colorscreen.screens.colored.gray;
		make_real(opc, pht->frequency);
		make_real(opc + 1, pht->angle);
		opc[2] = istate->screen_procs.gray;
	    }
	    make_int(op, 2);
	    break;
	default:		/* Screen was set by sethalftone. */
	    push(2);
	    op[-1] = istate->halftone;
	    make_int(op, 0);
	    break;
    }
    return 0;
}

/* - .currentscreenlevels <int> */
private int
zcurrentscreenlevels(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;

    push(1);
    make_int(op, gs_currentscreenlevels(igs));
    return 0;
}

/* The setscreen operator is complex because it has to sample */
/* each pixel in the pattern cell, calling a procedure, and then */
/* sort the result into a whitening order. */

/* Layout of stuff pushed on estack: */
/*      Control mark, */
/*      [other stuff for other screen-setting operators], */
/*      finishing procedure (or 0), */
/*      spot procedure, */
/*      enumeration structure (as bytes). */
#define snumpush 4
#define sproc esp[-1]
#define senum r_ptr(esp, gs_screen_enum)

/* Forward references */
private int setscreen_finish(i_ctx_t *);

/* <frequency> <angle> <proc> setscreen - */
private int
zsetscreen(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    gs_screen_halftone screen;
    gx_ht_order order;
    int code = zscreen_params(op, &screen);
    gs_memory_t *mem;
    int space_index = r_space_index(op);

    if (code < 0)
	return code;
    mem = (gs_memory_t *)idmemory->spaces_indexed[space_index];
    /*
     * Allocate the halftone in the same VM space as the procedure.
     * This keeps the space relationships consistent.
     */
    code = gs_screen_order_init_memory(&order, igs, &screen,
				       gs_currentaccuratescreens(), mem);
    if (code < 0)
	return code;
    return zscreen_enum_init(i_ctx_p, &order, &screen, op, 3,
			     setscreen_finish, space_index);
}
/* We break out the body of this operator so it can be shared with */
/* the code for Type 1 halftones in sethalftone. */
int
zscreen_enum_init(i_ctx_t *i_ctx_p, const gx_ht_order * porder,
		  gs_screen_halftone * psp, ref * pproc, int npop,
		  int (*finish_proc)(i_ctx_t *), int space_index)
{
    gs_screen_enum *penum;
    gs_memory_t * mem = (gs_memory_t *)idmemory->spaces_indexed[space_index]; 
    int code;

    check_estack(snumpush + 1);
    penum = gs_screen_enum_alloc(mem, "setscreen");
    if (penum == 0)
	return_error(e_VMerror);
    make_struct(esp + snumpush, space_index << r_space_shift, penum);	/* do early for screen_cleanup in case of error */
    code = gs_screen_enum_init_memory(penum, porder, igs, psp, mem);
    if (code < 0) {
	screen_cleanup(i_ctx_p);
	return code;
    }
    /* Push everything on the estack */
    make_mark_estack(esp + 1, es_other, screen_cleanup);
    esp += snumpush;
    make_op_estack(esp - 2, finish_proc);
    sproc = *pproc;
    push_op_estack(screen_sample);
    pop(npop);
    return o_push_estack;
}
/* Set up the next sample */
private int
screen_sample(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    gs_screen_enum *penum = senum;
    gs_point pt;
    int code = gs_screen_currentpoint(penum, &pt);
    ref proc;

    switch (code) {
	default:
	    return code;
	case 1:
	    /* All done */
	    if (real_opproc(esp - 2) != 0)
		code = (*real_opproc(esp - 2)) (i_ctx_p);
	    esp -= snumpush;
	    screen_cleanup(i_ctx_p);
	    return (code < 0 ? code : o_pop_estack);
	case 0:
	    ;
    }
    push(2);
    make_real(op - 1, pt.x);
    make_real(op, pt.y);
    proc = sproc;
    push_op_estack(set_screen_continue);
    *++esp = proc;
    return o_push_estack;
}
/* Continuation procedure for processing sampled pixels. */
private int
set_screen_continue(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    double value;
    int code = real_param(op, &value);

    if (code < 0)
	return code;
    code = gs_screen_next(senum, value);
    if (code < 0)
	return code;
    pop(1);
    return screen_sample(i_ctx_p);
}
/* Finish setscreen. */
private int
setscreen_finish(i_ctx_t *i_ctx_p)
{
    gs_screen_install(senum);
    istate->screen_procs.red = sproc;
    istate->screen_procs.green = sproc;
    istate->screen_procs.blue = sproc;
    istate->screen_procs.gray = sproc;
    make_null(&istate->halftone);
    return 0;
}
/* Clean up after screen enumeration */
private int
screen_cleanup(i_ctx_t *i_ctx_p)
{
    gs_screen_enum *penum = r_ptr(esp + snumpush, gs_screen_enum);

    gs_free_object(penum->halftone.rc.memory, penum, "screen_cleanup");
    return 0;
}

/* ------ Utility procedures ------ */

/* Get parameters for a single screen. */
int
zscreen_params(os_ptr op, gs_screen_halftone * phs)
{
    double fa[2];
    int code = num_params(op - 1, 2, fa);

    if (code < 0)
	return code;
    check_proc(*op);
    phs->frequency = fa[0];
    phs->angle = fa[1];
    return 0;
}

/* ------ Initialization procedure ------ */

const op_def zht_op_defs[] =
{
    {"0.currenthalftone", zcurrenthalftone},
    {"0.currentscreenlevels", zcurrentscreenlevels},
    {"3setscreen", zsetscreen},
		/* Internal operators */
    {"0%screen_sample", screen_sample},
    {"1%set_screen_continue", set_screen_continue},
    {"0%setscreen_finish", setscreen_finish},
    op_def_end(0)
};
