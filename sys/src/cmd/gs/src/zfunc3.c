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

/*$Id: zfunc3.c,v 1.1 2000/03/09 08:40:45 lpd Exp $ */
/* PostScript language interface to LL3 Functions */
#include "memory_.h"
#include "ghost.h"
#include "oper.h"
#include "gsfunc3.h"
#include "gsstruct.h"
#include "stream.h"		/* for files.h */
#include "files.h"
#include "ialloc.h"
#include "idict.h"
#include "idparam.h"
#include "ifunc.h"
#include "store.h"

/* Check prototypes */
build_function_proc(gs_build_function_2);
build_function_proc(gs_build_function_3);

/* Finish building a FunctionType 2 (ExponentialInterpolation) function. */
int
gs_build_function_2(const ref *op, const gs_function_params_t * mnDR,
		    int depth, gs_function_t ** ppfn, gs_memory_t *mem)
{
    gs_function_ElIn_params_t params;
    int code, n0, n1;

    *(gs_function_params_t *)&params = *mnDR;
    params.C0 = 0;
    params.C1 = 0;
    if ((code = dict_float_param(op, "N", 0.0, &params.N)) != 0 ||
	(code = n0 = fn_build_float_array(op, "C0", false, false, &params.C0, mem)) < 0 ||
	(code = n1 = fn_build_float_array(op, "C1", false, false, &params.C1, mem)) < 0
	)
	goto fail;
    if (params.C0 == 0)
	n0 = 1;			/* C0 defaulted */
    if (params.C1 == 0)
	n1 = 1;			/* C1 defaulted */
    if (params.Range == 0)
	params.n = n0;		/* either one will do */
    if (n0 != n1 || n0 != params.n)
	goto fail;
    code = gs_function_ElIn_init(ppfn, &params, mem);
    if (code >= 0)
	return 0;
fail:
    gs_function_ElIn_free_params(&params, mem);
    return (code < 0 ? code : gs_note_error(e_rangecheck));
}

/* Finish building a FunctionType 3 (1-Input Stitching) function. */
int
gs_build_function_3(const ref *op, const gs_function_params_t * mnDR,
		    int depth, gs_function_t ** ppfn, gs_memory_t *mem)
{
    gs_function_1ItSg_params_t params;
    int code;

    *(gs_function_params_t *) & params = *mnDR;
    params.Functions = 0;
    params.Bounds = 0;
    params.Encode = 0;
    {
	ref *pFunctions;
	gs_function_t **ptr;
	int i;

	if ((code = dict_find_string(op, "Functions", &pFunctions)) <= 0)
	    return (code < 0 ? code : gs_note_error(e_rangecheck));
	check_array_only(*pFunctions);
	params.k = r_size(pFunctions);
	code = alloc_function_array(params.k, &ptr, mem);
	if (code < 0)
	    return code;
	params.Functions = (const gs_function_t * const *)ptr;
	for (i = 0; i < params.k; ++i) {
	    ref subfn;

	    array_get(pFunctions, (long)i, &subfn);
	    code = fn_build_sub_function(&subfn, &ptr[i], depth, mem);
	    if (code < 0)
		goto fail;
	}
    }
    if ((code = fn_build_float_array(op, "Bounds", true, false, &params.Bounds, mem)) != params.k - 1 ||
	(code = fn_build_float_array(op, "Encode", true, true, &params.Encode, mem)) != 2 * params.k
	)
	goto fail;
    if (params.Range == 0)
	params.n = params.Functions[0]->params.n;
    code = gs_function_1ItSg_init(ppfn, &params, mem);
    if (code >= 0)
	return 0;
fail:
    gs_function_1ItSg_free_params(&params, mem);
    return (code < 0 ? code : gs_note_error(e_rangecheck));
}
