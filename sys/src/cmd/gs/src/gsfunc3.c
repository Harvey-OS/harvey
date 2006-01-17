/* Copyright (C) 1997, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gsfunc3.c,v 1.26 2005/05/03 10:50:48 igor Exp $ */
/* Implementation of LL3 Functions */
#include "math_.h"
#include "memory_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsfunc3.h"
#include "gsparam.h"
#include "gxfunc.h"
#include "stream.h"

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

/*
 * Scale an array of subsidiary functions.  Note that the scale may either
 * be propagated unchanged (step_ranges = false) or divided among the
 * (1-output) subfunctions (step_ranges = true).
 */
private int
fn_scale_functions(gs_function_t ***ppsfns, const gs_function_t *const *pfns,
		   int count, const gs_range_t *pranges, bool step_ranges,
		   gs_memory_t *mem)
{
    gs_function_t **psfns;
    int code = alloc_function_array(count, &psfns, mem);
    const gs_range_t *ranges = pranges;
    int i;
    
    if (code < 0)
	return code;
    for (i = 0; i < count; ++i) {
	int code = gs_function_make_scaled(pfns[i], &psfns[i], ranges, mem);

	if (code < 0) {
	    fn_free_functions((const gs_function_t *const *)psfns, count, mem);
	    return code;
	}
	if (step_ranges)
	    ++ranges;
    }
    *ppsfns = psfns;
    return 0;
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
		     const float *lower, const float *upper, uint *mask)
{
    const gs_function_ElIn_t *const pfn =
	(const gs_function_ElIn_t *)pfn_common;

    if (lower[0] > pfn->params.Domain[1] ||
	upper[0] < pfn->params.Domain[0]
	)
	return_error(gs_error_rangecheck);
    *mask = 0;
    return 1;
}

/* Write Exponential Interpolation function parameters on a parameter list. */
private int
fn_ElIn_get_params(const gs_function_t *pfn_common, gs_param_list *plist)
{
    const gs_function_ElIn_t *const pfn =
	(const gs_function_ElIn_t *)pfn_common;
    int ecode = fn_common_get_params(pfn_common, plist);
    int code;

    if (pfn->params.C0) {
	if ((code = param_write_float_values(plist, "C0", pfn->params.C0,
					     pfn->params.n, false)) < 0)
	    ecode = code;
    }
    if (pfn->params.C1) {
	if ((code = param_write_float_values(plist, "C1", pfn->params.C1,
					     pfn->params.n, false)) < 0)
	    ecode = code;
    }
    if ((code = param_write_float(plist, "N", &pfn->params.N)) < 0)
	ecode = code;
    return ecode;
}

/* Make a scaled copy of an Exponential Interpolation function. */
private int
fn_ElIn_make_scaled(const gs_function_ElIn_t *pfn,
		     gs_function_ElIn_t **ppsfn,
		     const gs_range_t *pranges, gs_memory_t *mem)
{
    gs_function_ElIn_t *psfn =
	gs_alloc_struct(mem, gs_function_ElIn_t, &st_function_ElIn,
			"fn_ElIn_make_scaled");
    float *c0;
    float *c1;
    int code, i;

    if (psfn == 0)
	return_error(gs_error_VMerror);
    psfn->params = pfn->params;
    psfn->params.C0 = c0 =
	fn_copy_values(pfn->params.C0, pfn->params.n, sizeof(float), mem);
    psfn->params.C1 = c1 =
	fn_copy_values(pfn->params.C1, pfn->params.n, sizeof(float), mem);
    if ((code = ((c0 == 0 && pfn->params.C0 != 0) ||
		 (c1 == 0 && pfn->params.C1 != 0) ?
		 gs_note_error(gs_error_VMerror) : 0)) < 0 ||
	(code = fn_common_scale((gs_function_t *)psfn,
				(const gs_function_t *)pfn,
				pranges, mem)) < 0) {
	gs_function_free((gs_function_t *)psfn, true, mem);
	return code;
    }
    for (i = 0; i < pfn->params.n; ++i) {
	double base = pranges[i].rmin, factor = pranges[i].rmax - base;

	c1[i] = c1[i] * factor + base;
	c0[i] = c0[i] * factor + base;
    }
    *ppsfn = psfn;
    return 0;
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

/* Serialize. */
private int
gs_function_ElIn_serialize(const gs_function_t * pfn, stream *s)
{
    uint n;
    const gs_function_ElIn_params_t * p = (const gs_function_ElIn_params_t *)&pfn->params;
    int code = fn_common_serialize(pfn, s);

    if (code < 0)
	return code;
    code = sputs(s, (const byte *)&p->C0[0], sizeof(p->C0[0]) * p->n, &n);
    if (code < 0)
	return code;
    code = sputs(s, (const byte *)&p->C1[0], sizeof(p->C1[0]) * p->n, &n);
    if (code < 0)
	return code;
    return sputs(s, (const byte *)&p->N, sizeof(p->N), &n);
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
	    gs_function_get_info_default,
	    (fn_get_params_proc_t) fn_ElIn_get_params,
	    (fn_make_scaled_proc_t) fn_ElIn_make_scaled,
	    (fn_free_params_proc_t) gs_function_ElIn_free_params,
	    fn_common_free,
	    (fn_serialize_proc_t) gs_function_ElIn_serialize,
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
    if (b1 == b0)
	encoded = e0;
    else
	encoded =
	    (arg - b0) * (pfn->params.Encode[2 * i + 1] - e0) / (b1 - b0) + e0;
    if_debug3('~', "[~]1ItSg %g in %d => %g\n", arg, i, encoded);
    return gs_function_evaluate(pfn->params.Functions[i], &encoded, out);
}

/* Test whether a 1-Input Stitching function is monotonic. */
private int
fn_1ItSg_is_monotonic(const gs_function_t * pfn_common,
		      const float *lower, const float *upper, uint *mask)
{
    const gs_function_1ItSg_t *const pfn =
	(const gs_function_1ItSg_t *)pfn_common;
    float v0 = lower[0], v1 = upper[0];
    float d0 = pfn->params.Domain[0], d1 = pfn->params.Domain[1];
    int k = pfn->params.k;
    int i;

    *mask = 0;
    if (v0 > v1) {
	v0 = v1; v1 = lower[0];
    }
    if (v0 > d1 || v1 < d0)
	return_error(gs_error_rangecheck);
    if (v0 < d0)
	v0 = d0;
    if (v1 > d1)
	v1 = d1;
    for (i = 0; i < pfn->params.k; ++i) {
	float b0 = (i == 0 ? d0 : pfn->params.Bounds[i - 1]);
	float b1 = (i == k - 1 ? d1 : pfn->params.Bounds[i]);
	const float small = 0.0000001 * (b1 - b0);
	float e0, e1;
	float w0, w1;
	float vv0, vv1;
	double vb0, vb1;

	if (v0 >= b1)
	    continue;
	if (v0 >= b1 - small)
	    continue; /* Ignore a small noize */
	vv0 = max(b0, v0);
	vv1 = v1;
	if (vv1 > b1 && v1 < b1 + small)
	    vv1 = b1; /* Ignore a small noize */
	if (vv0 == vv1)
	    return 1;
	if (vv0 < b1 && vv1 > b1)
	    return 0; /* Consider stitches as monotonity beraks. */
	e0 = pfn->params.Encode[2 * i];
	e1 = pfn->params.Encode[2 * i + 1];
	vb0 = max(vv0, b0);
	vb1 = min(vv1, b1);
	w0 = (float)(vb0 - b0) * (e1 - e0) / (b1 - b0) + e0;
	w1 = (float)(vb1 - b0) * (e1 - e0) / (b1 - b0) + e0;
	/* Note that w0 > w1 is now possible if e0 > e1. */
	if (e0 > e1) {
	    if (w0 > e0 && w0 - small <= e0)
		w0 = e0; /* Suppress a small noize */
	    if (w1 < e1 && w1 + small >= e1)
		w1 = e1; /* Suppress a small noize */
	} else {
	    if (w0 < e0 && w0 + small >= e0)
		w0 = e0; /* Suppress a small noize */
	    if (w1 > e1 && w1 - small <= e1)
		w1 = e1; /* Suppress a small noize */
	}
	if (w0 > w1)
	    return gs_function_is_monotonic(pfn->params.Functions[i],
					    &w1, &w0, mask);
	else
	    return gs_function_is_monotonic(pfn->params.Functions[i],
					    &w0, &w1, mask);
    }
    /* v0 is equal to the range end. */
    *mask = 0;
    return 1; 
}

/* Return 1-Input Stitching function information. */
private void
fn_1ItSg_get_info(const gs_function_t *pfn_common, gs_function_info_t *pfi)
{
    const gs_function_1ItSg_t *const pfn =
	(const gs_function_1ItSg_t *)pfn_common;

    gs_function_get_info_default(pfn_common, pfi);
    pfi->Functions = pfn->params.Functions;
    pfi->num_Functions = pfn->params.k;
}

/* Write 1-Input Stitching function parameters on a parameter list. */
private int
fn_1ItSg_get_params(const gs_function_t *pfn_common, gs_param_list *plist)
{
    const gs_function_1ItSg_t *const pfn =
	(const gs_function_1ItSg_t *)pfn_common;
    int ecode = fn_common_get_params(pfn_common, plist);
    int code;

    if ((code = param_write_float_values(plist, "Bounds", pfn->params.Bounds,
					 pfn->params.k - 1, false)) < 0)
	ecode = code;
    if ((code = param_write_float_values(plist, "Encode", pfn->params.Encode,
					 2 * pfn->params.k, false)) < 0)
	ecode = code;
    return ecode;
}

/* Make a scaled copy of a 1-Input Stitching function. */
private int
fn_1ItSg_make_scaled(const gs_function_1ItSg_t *pfn,
		     gs_function_1ItSg_t **ppsfn,
		     const gs_range_t *pranges, gs_memory_t *mem)
{
    gs_function_1ItSg_t *psfn =
	gs_alloc_struct(mem, gs_function_1ItSg_t, &st_function_1ItSg,
			"fn_1ItSg_make_scaled");
    int code;

    if (psfn == 0)
	return_error(gs_error_VMerror);
    psfn->params = pfn->params;
    psfn->params.Functions = 0;	/* in case of failure */
    psfn->params.Bounds =
	fn_copy_values(pfn->params.Bounds, pfn->params.k - 1, sizeof(float),
		       mem);
    psfn->params.Encode =
	fn_copy_values(pfn->params.Encode, 2 * pfn->params.k, sizeof(float),
		       mem);
    if ((code = (psfn->params.Bounds == 0 || psfn->params.Encode == 0 ?
		 gs_note_error(gs_error_VMerror) : 0)) < 0 ||
	(code = fn_common_scale((gs_function_t *)psfn,
				(const gs_function_t *)pfn,
				pranges, mem)) < 0 ||
	(code = fn_scale_functions((gs_function_t ***)&psfn->params.Functions,
				   pfn->params.Functions,
				   pfn->params.n, pranges, false, mem)) < 0) {
	gs_function_free((gs_function_t *)psfn, true, mem);
	return code;
    }
    *ppsfn = psfn;
    return 0;
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

/* Serialize. */
private int
gs_function_1ItSg_serialize(const gs_function_t * pfn, stream *s)
{
    uint n;
    const gs_function_1ItSg_params_t * p = (const gs_function_1ItSg_params_t *)&pfn->params;
    int code = fn_common_serialize(pfn, s);
    int k;

    if (code < 0)
	return code;
    code = sputs(s, (const byte *)&p->k, sizeof(p->k), &n);
    if (code < 0)
	return code;

    for (k = 0; k < p->k && code >= 0; k++) 
	code = gs_function_serialize(p->Functions[k], s);
    if (code < 0)
	return code;
    code = sputs(s, (const byte *)&p->Bounds[0], sizeof(p->Bounds[0]) * (p->k - 1), &n);
    if (code < 0)
	return code;
    return sputs(s, (const byte *)&p->Encode[0], sizeof(p->Encode[0]) * (p->k * 2), &n);
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
	    (fn_get_info_proc_t) fn_1ItSg_get_info,
	    (fn_get_params_proc_t) fn_1ItSg_get_params,
	    (fn_make_scaled_proc_t) fn_1ItSg_make_scaled,
	    (fn_free_params_proc_t) gs_function_1ItSg_free_params,
	    fn_common_free,
	    (fn_serialize_proc_t) gs_function_1ItSg_serialize,
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
	    if (params->Bounds[i] < prev)
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
fn_AdOt_evaluate(const gs_function_t *pfn_common, const float *in0, float *out)
{
    const gs_function_AdOt_t *const pfn =
	(const gs_function_AdOt_t *)pfn_common;
    const float *in = in0;
#define MAX_ADOT_IN 16
    float in_buf[MAX_ADOT_IN];
    int i;

    /*
     * We have to take special care to handle the case where in and out
     * overlap.  For the moment, handle it only for a limited number of
     * input values.
     */
    if (in <= out + (pfn->params.n - 1) && out <= in + (pfn->params.m - 1)) {
	if (pfn->params.m > MAX_ADOT_IN)
	    return_error(gs_error_rangecheck);
	memcpy(in_buf, in, pfn->params.m * sizeof(*in));
	in = in_buf;
    }
    for (i = 0; i < pfn->params.n; ++i) {
	int code =
	    gs_function_evaluate(pfn->params.Functions[i], in, out + i);

	if (code < 0)
	    return code;
    }
    return 0;
#undef MAX_ADOT_IN
}

/* Test whether an Arrayed Output function is monotonic. */
private int
fn_AdOt_is_monotonic(const gs_function_t * pfn_common,
		     const float *lower, const float *upper, uint *mask)
{
    const gs_function_AdOt_t *const pfn =
	(const gs_function_AdOt_t *)pfn_common;
    int i;

    for (i = 0; i < pfn->params.n; ++i) {
	int code =
	    gs_function_is_monotonic(pfn->params.Functions[i], lower, upper, mask);

	if (code <= 0)
	    return code;
    }
    return 1;
}

/* Return Arrayed Output function information. */
private void
fn_AdOt_get_info(const gs_function_t *pfn_common, gs_function_info_t *pfi)
{
    const gs_function_AdOt_t *const pfn =
	(const gs_function_AdOt_t *)pfn_common;

    gs_function_get_info_default(pfn_common, pfi);
    pfi->Functions = pfn->params.Functions;
    pfi->num_Functions = pfn->params.n;
}

/* Make a scaled copy of an Arrayed Output function. */
private int
fn_AdOt_make_scaled(const gs_function_AdOt_t *pfn, gs_function_AdOt_t **ppsfn,
		    const gs_range_t *pranges, gs_memory_t *mem)
{
    gs_function_AdOt_t *psfn =
	gs_alloc_struct(mem, gs_function_AdOt_t, &st_function_AdOt,
			"fn_AdOt_make_scaled");
    int code;

    if (psfn == 0)
	return_error(gs_error_VMerror);
    psfn->params = pfn->params;
    psfn->params.Functions = 0;	/* in case of failure */
    if ((code = fn_common_scale((gs_function_t *)psfn,
				(const gs_function_t *)pfn,
				pranges, mem)) < 0 ||
	(code = fn_scale_functions((gs_function_t ***)&psfn->params.Functions,
				   pfn->params.Functions,
				   pfn->params.n, pranges, true, mem)) < 0) {
	gs_function_free((gs_function_t *)psfn, true, mem);
	return code;
    }
    *ppsfn = psfn;
    return 0;
}

/* Free the parameters of an Arrayed Output function. */
void
gs_function_AdOt_free_params(gs_function_AdOt_params_t * params,
			     gs_memory_t * mem)
{
    fn_free_functions(params->Functions, params->n, mem);
    fn_common_free_params((gs_function_params_t *) params, mem);
}

/* Serialize. */
private int
gs_function_AdOt_serialize(const gs_function_t * pfn, stream *s)
{
    const gs_function_AdOt_params_t * p = (const gs_function_AdOt_params_t *)&pfn->params;
    int code = fn_common_serialize(pfn, s);
    int k;

    if (code < 0)
	return code;
    for (k = 0; k < p->n && code >= 0; k++) 
	code = gs_function_serialize(p->Functions[k], s);
    return code;
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
	    (fn_get_info_proc_t) fn_AdOt_get_info,
	    fn_common_get_params,	/****** WHAT TO DO ABOUT THIS? ******/
	    (fn_make_scaled_proc_t) fn_AdOt_make_scaled,
	    (fn_free_params_proc_t) gs_function_AdOt_free_params,
	    fn_common_free,
	    (fn_serialize_proc_t) gs_function_AdOt_serialize,
	}
    };
    int m = params->m, n = params->n;

    *ppfn = 0;			/* in case of error */
    if (m <= 0 || n <= 0)
	return_error(gs_error_rangecheck);
    {
	gs_function_AdOt_t *pfn =
	    gs_alloc_struct(mem, gs_function_AdOt_t, &st_function_AdOt,
			    "gs_function_AdOt_init");
	float *domain = (float *)
	    gs_alloc_byte_array(mem, 2 * m, sizeof(float),
				"gs_function_AdOt_init(Domain)");
	int i, j;

	if (pfn == 0)
	    return_error(gs_error_VMerror);
	pfn->params = *params;
	pfn->params.Domain = domain;
	pfn->params.Range = 0;
	pfn->head = function_AdOt_head;
	if (domain == 0) {
	    gs_function_free((gs_function_t *)pfn, true, mem);
	    return_error(gs_error_VMerror);
	}
	/*
	 * We compute the Domain as the intersection of the Domains of
	 * the individual subfunctions.  This isn't quite right: some
	 * subfunction might actually make use of a larger domain of
	 * input values.  However, the only place that Arrayed Output
	 * functions are used is in Shading and similar dictionaries,
	 * where the input values are clamped to the intersection of
	 * the individual Domains anyway.
	 */
	memcpy(domain, params->Functions[0]->params.Domain,
	       2 * sizeof(float) * m);
	for (i = 1; i < n; ++i) {
	    const float *dom = params->Functions[i]->params.Domain;

	    for (j = 0; j < 2 * m; j += 2, dom += 2) {
		domain[j] = max(domain[j], dom[0]);
		domain[j + 1] = min(domain[j + 1], dom[1]);
	    }
	}
	*ppfn = (gs_function_t *) pfn;
    }
    return 0;
}
