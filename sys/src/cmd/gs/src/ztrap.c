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

/*$Id: ztrap.c,v 1.1 2000/03/09 08:40:45 lpd Exp $ */
/* Operators for setting trapping parameters and zones */
#include "ghost.h"
#include "oper.h"
#include "ialloc.h"
#include "iparam.h"
#include "gstrap.h"

/* Define the current trap parameters. */
/****** THIS IS BOGUS ******/
gs_trap_params_t i_trap_params;

/* <dict> .settrapparams - */
private int
zsettrapparams(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    dict_param_list list;
    int code;

    check_type(*op, t_dictionary);
    code = dict_param_list_read(&list, op, NULL, false, iimemory);
    if (code < 0)
	return code;
    code = gs_settrapparams(&i_trap_params, (gs_param_list *) & list);
    iparam_list_release(&list);
    if (code < 0)
	return code;
    pop(1);
    return 0;
}

/* - settrapzone - */
private int
zsettrapzone(i_ctx_t *i_ctx_p)
{
/****** NYI ******/
    return_error(e_undefined);
}

/* ------ Initialization procedure ------ */

const op_def ztrap_op_defs[] =
{
    op_def_begin_ll3(),
    {"1.settrapparams", zsettrapparams},
    {"0settrapzone", zsettrapzone},
    op_def_end(0)
};
