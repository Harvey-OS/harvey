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

/*$Id: gsfunc3.c,v 1.1 2000/03/09 08:40:42 lpd Exp $ */
/* Implementation of LL3 Functions */
#include "math_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsfunc3.h"
#include "gxfunc.h"

/* ---------------- Utilities ---------------- */

#define MASK1 ((uint)(~0) / 3)

/*
 * Free an array of subsidiary Functions.  Note that this may be called
 * before the Functions array has been fully initialized.  Note also that
 * its argument conforms to the Functions array in the parameter structure,
 * but it (necessarily) deconstifies it.
 */
private void
fn_free_functions(const gs_function_t *const * Functions, int count,
		  gs_memory_t * mem)
{
    int i;

    for (i = count; --i >= 0;)
	if (Functions[i])
	    gs_function_free((gs_function_t *)Functions[i], true, mem);
    gs_free_const_object(mem, Functions, "Functions");
}

/* ---------------- Exponential Interpolation functions ---------------- */

typedef struct gs_function_ElIn_s {
    gs_function_head_t head;
    gs_function_ElIn_params_t params;
} gs_function_ElIn_t;

private_st_function_ElIn();

/* Evaluate an Exponential Interpolation function. */
private int
fn_ElIn_evaluate(const gs_function_t * pfn_common, const float *in, float *out)
{
    const gs_function_ElIn_t *const pfn =
	(const gs_function_ElIn_t *)pfn_common;
    double arg = in[0], raised;
    int i;

    if (arg < pfn->params.Domain[0])
	arg = pfn->params.Domain[0];
    else if (arg > pfn->params.Domain[1])
	arg = pfn->params.Domain[1];
    raised = pow(arg, pfn->params.N);
    for (i = 0; i < pfn->params.n; ++i) {
	float v0 = (pfn->params.C0 == 0 ? 0.0 : pfn->params.C0[i]);
	float v1 = (pfn->params.C1 == 0 ? 1.0 : pfn->params.C1[i]);
	double value = v0 + raised * (v1 - v0);

	if (pfn->params.Range) {
	    float r0 = pfn->params.Range[2 * i],
		r1 = pfn->params.Range[2 * i + 1];

	    if (value < r0)
		value = r0;
	    else if (value > r1)
		value = r1;
	}
	out[i] = value;
	if_debug3('~', "[~]ElIn %g => [%d]%g\n", arg, i, out[i]);
    }
    return 0;
}

/* Test whether an Exponential function is monotonic.  (They always are.) */
private int
fn_ElIn_is_monotonic(const gs_function_t * pfn_common,
		     const float *lower, const float *upper,
		     gs_function_effort_t effort)
{
    const gs_function_ElIn_t *const pfn =
	(const gs_function_ElIn_t *)pfn_common;
    int i, result;

    if (lower[0] > pfn->params.Domain[1] ||
	upper[0] < pfn->params.Domain[0]
	)
	return_error(gs_error_rangecheck);
    for (i = 0, result = 0; i < pfn->params.n; ++i) {
	double diff =
	    (pfn->params.C1 == 0 ? 1.0 : pfn->params.C1[i]) -
	    (pfn->params.C0 == 0 ? 0.0 : pfn->params.C0[i]);

	if (pfn->params.N < 0)
	    diff = -diff;
	else if (pfn->params.N == 0)
	    diff = 0;
	result |=
	    (diff < 0 ? FN_MONOTONIC_DECREASING :
	     diff > 0 ? FN_MONOTONIC_INCREASING :
	     FN_MONOTONIC_DECREASING | FN_MONOTONIC_INCREASING) <<
	    (2 * i);
    }
    return result;
}

/* Free the parameters of an Exponential Interpolation function. */
void
gs_function_ElIn_free_params(gs_function_ElIn_params_t * params,
			     gs_memory_t * mem)
{
    gs_free_const_object(mem, params->C1, "C1");
    gs_free_const_object(mem, params->C0, "C0");
    fn_common_free_params((gs_function_params_t *) params, mem);
}

/* Allocate and initialize an Exponential Interpolation function. */
int
gs_function_ElIn_init(gs_function_t ** ppfn,
		      const gs_function_ElIn_params_t * params,
		      gs_memory_t * mem)
{
    static const gs_function_head_t function_ElIn_head = {
	function_type_ExponentialInterpolation,
	{
	    (fn_evaluate_proc_t) fn_ElIn_evaluate,
	    (fn_is_monotonic_proc_t) fn_ElIn_is_monotonic,
	    (fn_free_params_proc_t) gs_function_ElIn_free_params,
	    fn_common_free
	}
    };
    int code;

    *ppfn = 0;			/* in case of error */
    code = fn_check_mnDR((const gs_function_params_t *)params, 1, params->n);
    if (code < 0)
	return code;
    if ((params->C0 == 0 || params->C1 == 0) && params->n != 1)
	return_error(gs_error_rangecheck);
    if (params->N != floor(params->N)) {
	/* Non-integral exponent, all inputs must be non-negative. */
	if (params->Domain[0] < 0)
	    return_error(gs_error_rangecheck);
    }
    if (params->N < 0) {
	/* Negative exponent, input must not be zero. */
	if (params->Domain[0] <= 0 && params->Domain[1] >= 0)
	    return_error(gs_error_rangecheck);
    } {
	gs_function_ElIn_t *pfn =
	    gs_alloc_struct(mem, gs_function_ElIn_t, &st_function_ElIn,
			    "gs_function_ElIn_init");

	if (pfn == 0)
	    return_error(gs_error_VMerror);
	pfn->params = *params;
	pfn->params.m = 1;
	pfn->head = function_ElIn_head;
	pfn->head.is_monotonic =
	    fn_domain_is_monotonic((gs_function_t *)pfn, EFFORT_MODERATE);
	*ppfn = (gs_function_t *) pfn;
    }
    return 0;
}

/* ---------------- 1-Input Stitching functions ---------------- */

typedef struct gs_function_1ItSg_s {
    gs_function_head_t head;
    gs_function_1ItSg_params_t params;
} gs_function_1ItSg_t;

private_st_function_1ItSg();

/* Evaluate a 1-Input Stitching function. */
private int
fn_1ItSg_evaluate(const gs_function_t * pfn_common, const float *in, float *out)
{
    const gs_function_1ItSg_t *const pfn =
	(const gs_function_1ItSg_t *)pfn_common;
    float arg = in[0], b0, b1, e0, encoded;
    int k = pfn->params.k;
    int i;

    if (arg < pfn->params.Domain[0]) {
	arg = pfn->params.Domain[0];
	i = 0;
    } else if (arg > pfn->params.Domain[1]) {
	arg = pfn->params.Domain[1];
	i = k - 1;
    } else {
	for (i = 0; i < k - 1; ++i)
	    if (arg <= pfn->params.Bounds[i])
		break;
    }
    b0 = (i == 0 ? pfn->params.Domain[0] : pfn->params.Bounds[i - 1]);
    b1 = (i == k - 1 ? pfn->params.Domain[1] : pfn->params.Bounds[i]);
    e0 = pfn->params.Encode[2 * i];
    encoded =
	(arg - b0) * (pfn->params.Encode[2 * i + 1] - e0) / (b1 - b0) + e0;
    if_debug3('~', "[~]1ItSg %g in %d => %g\n", arg, i, encoded);
    return gs_function_evaluate(pfn->params.Functions[i], &encoded, out);
}

/* Test whether a 1-Input Stitching function is monotonic. */
private int
fn_1ItSg_is_monotonic(const gs_function_t * pfn_common,
		      const float *lower, const float *upper,
		      gs_function_effort_t effort)
{
    const gs_function_1ItSg_t *const pfn =
	(const gs_function_1ItSg_t *)pfn_common;
    float v0 = lower[0], v1 = upper[0];
    float d0 = pfn->params.Domain[0], d1 = pfn->params.Domain[1];
    int k = pfn->params.k;
    int i;
    int result = 0;

    if (v0 > d1 || v1 < d0)
	return_error(gs_error_rangecheck);
    if (v0 < d0)
	v0 = d0;
    if (v1 > d1)
	v1 = d1;
    for (i = 0; i < pfn->params.k; ++i) {
	float b0 = (i == 0 ? d0 : pfn->params.Bounds[i - 1]);
	float b1 = (i == k - 1 ? d1 : pfn->params.Bounds[i]);
	float e0, e1;
	float w0, w1;
	int code;

	if (v0 >= b1 || v1 <= b0)
	    continue;
	e0 = pfn->params.Encode[2 * i];
	e1 = pfn->params.Encode[2 * i + 1];
	w0 = (max(v0, b0) - b0) * (e1 - e0) / (b1 - b0) + e0;
	w1 = (min(v1, b1) - b0) * (e1 - e0) / (b1 - b0) + e0;
	/* Note that w0 > w1 is now possible if e0 > e1. */
	if (w0 > w1) {
	    code = gs_function_is_monotonic(pfn->params.Functions[i],
					    &w1, &w0, effort);
	    if (code <= 0)
		return code;
	    /* Swap the INCREASING and DECREASING flags. */
	    code = ((code & MASK1) << 1) | ((code & (MASK1 << 1)) >> 1);
	} else {
	    code = gs_function_is_monotonic(pfn->params.Functions[i],
					    &w0, &w1, effort);
	    if (code <= 0)
		return code;
	}
	if (result == 0)
	    result = code;
	else {
	    result &= code;
	    /* Check that result is still monotonic in every position. */
	    code = result | ((result & MASK1) << 1) |
		((result & (MASK1 << 1)) >> 1);
	    if (code != (1 << (2 * pfn->params.n)) - 1)
		return 0;
	}
    }
    return result;
}

/* Free the parameters of a 1-Input Stitching function. */
void
gs_function_1ItSg_free_params(gs_function_1ItSg_params_t * params,
			      gs_memory_t * mem)
{
    gs_free_const_object(mem, params->Encode, "Encode");
    gs_free_const_object(mem, params->Bounds, "Bounds");
    fn_free_functions(params->Functions, params->k, mem);
    fn_common_free_params((gs_function_params_t *) params, mem);
}

/* Allocate and initialize a 1-Input Stitching function. */
int
gs_function_1ItSg_init(gs_function_t ** ppfn,
	       const gs_function_1ItSg_params_t * params, gs_memory_t * mem)
{
    static const gs_function_head_t function_1ItSg_head = {
	function_type_1InputStitching,
	{
	    (fn_evaluate_proc_t) fn_1ItSg_evaluate,
	    (fn_is_monotonic_proc_t) fn_1ItSg_is_monotonic,
	    (fn_free_params_proc_t) gs_function_1ItSg_free_params,
	    fn_common_free
	}
    };
    int n = (params->Range == 0 ? 0 : params->n);
    float prev = params->Domain[0];
    int i;

    *ppfn = 0;			/* in case of error */
    for (i = 0; i < params->k; ++i) {
	const gs_function_t *psubfn = params->Functions[i];

	if (psubfn->params.m != 1)
	    return_error(gs_error_rangecheck);
	if (n == 0)
	    n = psubfn->params.n;
	else if (psubfn->params.n != n)
	    return_error(gs_error_rangecheck);
	/* There are only k - 1 Bounds, not k. */
	if (i < params->k - 1) {
	    if (params->Bounds[i] <= prev)
		return_error(gs_error_rangecheck);
	    prev = params->Bounds[i];
	}
    }
    if (params->Domain[1] < prev)
	return_error(gs_error_rangecheck);
    fn_check_mnDR((const gs_function_params_t *)params, 1, n);
    {
	gs_function_1ItSg_t *pfn =
	    gs_alloc_struct(mem, gs_function_1ItSg_t, &st_function_1ItSg,
			    "gs_function_1ItSg_init");

	if (pfn == 0)
	    return_error(gs_error_VMerror);
	pfn->params = *params;
	pfn->params.m = 1;
	pfn->params.n = n;
	pfn->head = function_1ItSg_head;
	pfn->head.is_monotonic =
	    fn_domain_is_monotonic((gs_function_t *)pfn, EFFORT_MODERATE);
	*ppfn = (gs_function_t *) pfn;
    }
    return 0;
}

/* ---------------- Arrayed Output functions ---------------- */

typedef struct gs_function_AdOt_s {
    gs_function_head_t head;
    gs_function_AdOt_params_t params;
} gs_function_AdOt_t;

private_st_function_AdOt();

/* Evaluate an Arrayed Output function. */
private int
fn_AdOt_evaluate(const gs_function_t * pfn_common, const float *in, float *out)
{
    const gs_function_AdOt_t *const pfn =
	(const gs_function_AdOt_t *)pfn_common;
    int i;

    for (i = 0; i < pfn->params.n; ++i) {
	int code =
	    gs_function_evaluate(pfn->params.Functions[i], in, out + i);

	if (code < 0)
	    return code;
    }
    return 0;
}

/* Test whether an Arrayed Output function is monotonic. */
private int
fn_AdOt_is_monotonic(const gs_function_t * pfn_common,
		     const float *lower, const float *upper,
		     gs_function_effort_t effort)
{
    const gs_function_AdOt_t *const pfn =
	(const gs_function_AdOt_t *)pfn_common;
    int i, result;

    for (i = 0, result = 0; i < pfn->params.n; ++i) {
	int code =
	    gs_function_is_monotonic(pfn->params.Functions[i], lower, upper,
				     effort);

	if (code <= 0)
	    return code;
	result |= code << (2 * i);
    }
    return result;
}

/* Free the parameters of an Arrayed Output function. */
void
gs_function_AdOt_free_params(gs_function_AdOt_params_t * params,
			     gs_memory_t * mem)
{
    fn_free_functions(params->Functions, params->n, mem);
    fn_common_free_params((gs_function_params_t *) params, mem);
}

/* Allocate and initialize an Arrayed Output function. */
int
gs_function_AdOt_init(gs_function_t ** ppfn,
		const gs_function_AdOt_params_t * params, gs_memory_t * mem)
{
    static const gs_function_head_t function_AdOt_head = {
	function_type_ArrayedOutput,
	{
	    (fn_evaluate_proc_t) fn_AdOt_evaluate,
	    (fn_is_monotonic_proc_t) fn_AdOt_is_monotonic,
	    (fn_free_params_proc_t) gs_function_AdOt_free_params,
	    fn_common_free
	}
    };
    int m = params->m, n = params->n;
    int i;

    *ppfn = 0;			/* in case of error */
    if (m <= 0 || n <= 0)
	return_error(gs_error_rangecheck);
    for (i = 0; i < n; ++i) {
	const gs_function_t *psubfn = params->Functions[i];

	if (psubfn->params.m != m || psubfn->params.n != 1)
	    return_error(gs_error_rangecheck);
    }
    {
	gs_function_AdOt_t *pfn =
	    gs_alloc_struct(mem, gs_function_AdOt_t, &st_function_AdOt,
			    "gs_function_AdOt_init");

	if (pfn == 0)
	    return_error(gs_error_VMerror);
	pfn->params = *params;
	pfn->params.Domain = 0;
	pfn->params.Range = 0;
	pfn->head = function_AdOt_head;
	pfn->head.is_monotonic =
	    fn_domain_is_monotonic((gs_function_t *)pfn, EFFORT_MODERATE);
	*ppfn = (gs_function_t *) pfn;
    }
    return 0;
}
