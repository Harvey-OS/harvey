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

/*$Id: gsfunc.c,v 1.1 2000/03/09 08:40:42 lpd Exp $ */
/* Generic Function support */
#include "gx.h"
#include "gserrors.h"
#include "gxfunc.h"

/* GC descriptors */
public_st_function();

/* Generic free_params implementation. */
void
fn_common_free_params(gs_function_params_t * params, gs_memory_t * mem)
{
    gs_free_const_object(mem, params->Range, "Range");
    gs_free_const_object(mem, params->Domain, "Domain");
}

/* Generic free implementation. */
void
fn_common_free(gs_function_t * pfn, bool free_params, gs_memory_t * mem)
{
    if (free_params)
	gs_function_free_params(pfn, mem);
    gs_free_object(mem, pfn, "fn_common_free");
}

/* Check the values of m, n, Domain, and (if supplied) Range. */
int
fn_check_mnDR(const gs_function_params_t * params, int m, int n)
{
    int i;

    if (m <= 0 || n <= 0)
	return_error(gs_error_rangecheck);
    for (i = 0; i < m; ++i)
	if (params->Domain[2 * i] > params->Domain[2 * i + 1])
	    return_error(gs_error_rangecheck);
    if (params->Range != 0)
	for (i = 0; i < n; ++i)
	    if (params->Range[2 * i] > params->Range[2 * i + 1])
		return_error(gs_error_rangecheck);
    return 0;
}

/* Get the monotonicity of a function over its Domain. */
int
fn_domain_is_monotonic(const gs_function_t *pfn, gs_function_effort_t effort)
{
#define MAX_M 16		/* arbitrary */
    float lower[MAX_M], upper[MAX_M];
    int i;

    if (pfn->params.m > MAX_M)
	return gs_error_undefined;
    for (i = 0; i < pfn->params.m; ++i) {
	lower[i] = pfn->params.Domain[2 * i];
	upper[i] = pfn->params.Domain[2 * i + 1];
    }
    return gs_function_is_monotonic(pfn, lower, upper, effort);
}

/* ---------------- Vanilla functions ---------------- */

/* GC descriptor */
private_st_function_Va();

/*
 * Test whether a Vanilla function is monotonic.  (This information is
 * provided at function definition time.)
 */
private int
fn_Va_is_monotonic(const gs_function_t * pfn_common,
		   const float *lower, const float *upper,
		   gs_function_effort_t effort)
{
    const gs_function_Va_t *const pfn =
	(const gs_function_Va_t *)pfn_common;

    return pfn->params.is_monotonic;
}

/* Free the parameters of a Vanilla function. */
void
gs_function_Va_free_params(gs_function_Va_params_t * params,
			   gs_memory_t * mem)
{
    gs_free_object(mem, params->eval_data, "eval_data");
    fn_common_free_params((gs_function_params_t *) params, mem);
}

/* Allocate and initialize a Vanilla function. */
int
gs_function_Va_init(gs_function_t ** ppfn,
		    const gs_function_Va_params_t * params,
		    gs_memory_t * mem)
{
    static const gs_function_head_t function_Va_head = {
	function_type_Vanilla,
	{
	    NULL,			/* filled in from params */
	    (fn_is_monotonic_proc_t) fn_Va_is_monotonic,
	    (fn_free_params_proc_t) gs_function_Va_free_params,
	    fn_common_free
	}
    };
    int code;

    *ppfn = 0;			/* in case of error */
    code = fn_check_mnDR((const gs_function_params_t *)params, 1, params->n);
    if (code < 0)
	return code;
    {
	gs_function_Va_t *pfn =
	    gs_alloc_struct(mem, gs_function_Va_t, &st_function_Va,
			    "gs_function_Va_init");

	if (pfn == 0)
	    return_error(gs_error_VMerror);
	pfn->params = *params;
	pfn->head = function_Va_head;
	pfn->head.procs.evaluate = params->eval_proc;
	pfn->head.is_monotonic = params->is_monotonic;
	*ppfn = (gs_function_t *) pfn;
    }
    return 0;
}
