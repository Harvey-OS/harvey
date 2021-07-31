/* Copyright (C) 1994, 1997, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: zht1.c,v 1.1 2000/03/09 08:40:45 lpd Exp $ */
/* setcolorscreen operator */
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
#include "iht.h"
#include "store.h"

/* Dummy spot function */
private float
spot_dummy(floatp x, floatp y)
{
    return (x + y) / 2;
}

/* <red_freq> ... <gray_proc> setcolorscreen - */
private int setcolorscreen_finish(P1(i_ctx_t *));
private int setcolorscreen_cleanup(P1(i_ctx_t *));
private int
zsetcolorscreen(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    gs_colorscreen_halftone cscreen;
    ref sprocs[4];
    gs_halftone *pht;
    gx_device_halftone *pdht;
    int i;
    int code = 0;
    int space = 0;
    gs_memory_t *mem;

    for (i = 0; i < 4; i++) {
	os_ptr op1 = op - 9 + i * 3;
	int code = zscreen_params(op1, &cscreen.screens.indexed[i]);

	if (code < 0)
	    return code;
	cscreen.screens.indexed[i].spot_function = spot_dummy;
	sprocs[i] = *op1;
	space = max(space, r_space_index(op1));
    }
    mem = (gs_memory_t *)idmemory->spaces_indexed[space];
    check_estack(8);		/* for sampling screens */
    rc_alloc_struct_0(pht, gs_halftone, &st_halftone,
		      mem, pht = 0, "setcolorscreen(halftone)");
    rc_alloc_struct_0(pdht, gx_device_halftone, &st_device_halftone,
		      mem, pdht = 0, "setcolorscreen(device halftone)");
    if (pht == 0 || pdht == 0)
	code = gs_note_error(e_VMerror);
    else {
	pht->type = ht_type_colorscreen;
	pht->params.colorscreen = cscreen;
	code = gs_sethalftone_prepare(igs, pht, pdht);
    }
    if (code >= 0) {		/* Schedule the sampling of the screens. */
	es_ptr esp0 = esp;	/* for backing out */

	esp += 8;
	make_mark_estack(esp - 7, es_other, setcolorscreen_cleanup);
	memcpy(esp - 6, sprocs, sizeof(ref) * 4);	/* procs */
	make_istruct(esp - 2, 0, pht);
	make_istruct(esp - 1, 0, pdht);
	make_op_estack(esp, setcolorscreen_finish);
	for (i = 0; i < 4; i++) {
	    /* Shuffle the indices to correspond to */
	    /* the component order. */
	    code = zscreen_enum_init(i_ctx_p,
				     &pdht->components[(i + 1) & 3].corder,
				&pht->params.colorscreen.screens.indexed[i],
				     &sprocs[i], 0, 0, mem);
	    if (code < 0) {
		esp = esp0;
		break;
	    }
	}
    }
    if (code < 0) {
	gs_free_object(mem, pdht, "setcolorscreen(device halftone)");
	gs_free_object(mem, pht, "setcolorscreen(halftone)");
	return code;
    }
    pop(12);
    return o_push_estack;
}
/* Install the color screen after sampling. */
private int
setcolorscreen_finish(i_ctx_t *i_ctx_p)
{
    gx_device_halftone *pdht = r_ptr(esp, gx_device_halftone);
    int code;

    pdht->order = pdht->components[0].corder;
    code = gx_ht_install(igs, r_ptr(esp - 1, gs_halftone), pdht);
    if (code < 0)
	return code;
    memcpy(istate->screen_procs.indexed, esp - 5, sizeof(ref) * 4);
    make_null(&istate->halftone);
    esp -= 7;
    setcolorscreen_cleanup(i_ctx_p);
    return o_pop_estack;
}
/* Clean up after installing the color screen. */
private int
setcolorscreen_cleanup(i_ctx_t *i_ctx_p)
{
    gs_halftone *pht = r_ptr(esp + 6, gs_halftone);
    gx_device_halftone *pdht = r_ptr(esp + 7, gx_device_halftone);

    gs_free_object(pdht->rc.memory, pdht,
		   "setcolorscreen_cleanup(device halftone)");
    gs_free_object(pht->rc.memory, pht,
		   "setcolorscreen_cleanup(halftone)");
    return 0;
}

/* ------ Initialization procedure ------ */

const op_def zht1_op_defs[] =
{
    {"<setcolorscreen", zsetcolorscreen},
		/* Internal operators */
    {"0%setcolorscreen_finish", setcolorscreen_finish},
    op_def_end(0)
};
