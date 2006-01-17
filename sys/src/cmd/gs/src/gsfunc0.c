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

/* $Id: gsfunc0.c,v 1.27 2005/07/15 03:36:03 dan Exp $ */
/* Implementation of FunctionType 0 (Sampled) Functions */
#include "math_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsfunc0.h"
#include "gsparam.h"
#include "gxfarith.h"
#include "gxfunc.h"
#include "stream.h"

#define POLE_CACHE_DEBUG 0      /* A temporary development technology need. 
				   Remove after the beta testing. */
#define POLE_CACHE_GENERIC_1D 1 /* A temporary development technology need. 
				   Didn't decide yet - see fn_Sd_evaluate_cubic_cached_1d. */
#define POLE_CACHE_IGNORE 0     /* A temporary development technology need. 
				   Remove after the beta testing. */

#if POLE_CACHE_DEBUG
#   include <assert.h>
#endif

#define MAX_FAST_COMPS 8

typedef struct gs_function_Sd_s {
    gs_function_head_t head;
    gs_function_Sd_params_t params;
} gs_function_Sd_t;

/* GC descriptor */
private_st_function_Sd();
private
ENUM_PTRS_WITH(function_Sd_enum_ptrs, gs_function_Sd_t *pfn)
{
    index -= 6;
    if (index < st_data_source_max_ptrs)
	return ENUM_USING(st_data_source, &pfn->params.DataSource,
			  sizeof(pfn->params.DataSource), index);
    return ENUM_USING_PREFIX(st_function, st_data_source_max_ptrs);
}
ENUM_PTR3(0, gs_function_Sd_t, params.Encode, params.Decode, params.Size);
ENUM_PTR3(3, gs_function_Sd_t, params.pole, params.array_step, params.stream_step);
ENUM_PTRS_END
private
RELOC_PTRS_WITH(function_Sd_reloc_ptrs, gs_function_Sd_t *pfn)
{
    RELOC_PREFIX(st_function);
    RELOC_USING(st_data_source, &pfn->params.DataSource,
		sizeof(pfn->params.DataSource));
    RELOC_PTR3(gs_function_Sd_t, params.Encode, params.Decode, params.Size);
    RELOC_PTR3(gs_function_Sd_t, params.pole, params.array_step, params.stream_step);
}
RELOC_PTRS_END

/* Define the maximum plausible number of inputs and outputs */
/* for a Sampled function. */
#define max_Sd_m 16
#define max_Sd_n 16

/* Get one set of sample values. */
#define SETUP_SAMPLES(bps, nbytes)\
	int n = pfn->params.n;\
	byte buf[max_Sd_n * ((bps + 7) >> 3)];\
	const byte *p;\
	int i;\
\
	data_source_access(&pfn->params.DataSource, offset >> 3,\
			   nbytes, buf, &p)

private int
fn_gets_1(const gs_function_Sd_t * pfn, ulong offset, uint * samples)
{
    SETUP_SAMPLES(1, ((offset & 7) + n + 7) >> 3);
    for (i = 0; i < n; ++i) {
	samples[i] = (*p >> (~offset & 7)) & 1;
	if (!(++offset & 7))
	    p++;
    }
    return 0;
}
private int
fn_gets_2(const gs_function_Sd_t * pfn, ulong offset, uint * samples)
{
    SETUP_SAMPLES(2, (((offset & 7) >> 1) + n + 3) >> 2);
    for (i = 0; i < n; ++i) {
	samples[i] = (*p >> (6 - (offset & 7))) & 3;
	if (!((offset += 2) & 7))
	    p++;
    }
    return 0;
}
private int
fn_gets_4(const gs_function_Sd_t * pfn, ulong offset, uint * samples)
{
    SETUP_SAMPLES(4, (((offset & 7) >> 2) + n + 1) >> 1);
    for (i = 0; i < n; ++i) {
	samples[i] = ((offset ^= 4) & 4 ? *p >> 4 : *p++ & 0xf);
    }
    return 0;
}
private int
fn_gets_8(const gs_function_Sd_t * pfn, ulong offset, uint * samples)
{
    SETUP_SAMPLES(8, n);
    for (i = 0; i < n; ++i) {
	samples[i] = *p++;
    }
    return 0;
}
private int
fn_gets_12(const gs_function_Sd_t * pfn, ulong offset, uint * samples)
{
    SETUP_SAMPLES(12, (((offset & 7) >> 2) + 3 * n + 1) >> 1);
    for (i = 0; i < n; ++i) {
	if (offset & 4)
	    samples[i] = ((*p & 0xf) << 8) + p[1], p += 2;
	else
	    samples[i] = (*p << 4) + (p[1] >> 4), p++;
	offset ^= 4;
    }
    return 0;
}
private int
fn_gets_16(const gs_function_Sd_t * pfn, ulong offset, uint * samples)
{
    SETUP_SAMPLES(16, n * 2);
    for (i = 0; i < n; ++i) {
	samples[i] = (*p << 8) + p[1];
	p += 2;
    }
    return 0;
}
private int
fn_gets_24(const gs_function_Sd_t * pfn, ulong offset, uint * samples)
{
    SETUP_SAMPLES(24, n * 3);
    for (i = 0; i < n; ++i) {
	samples[i] = (*p << 16) + (p[1] << 8) + p[2];
	p += 3;
    }
    return 0;
}
private int
fn_gets_32(const gs_function_Sd_t * pfn, ulong offset, uint * samples)
{
    SETUP_SAMPLES(32, n * 4);
    for (i = 0; i < n; ++i) {
	samples[i] = (*p << 24) + (p[1] << 16) + (p[2] << 8) + p[3];
	p += 4;
    }
    return 0;
}

private int (*const fn_get_samples[]) (const gs_function_Sd_t * pfn,
				       ulong offset, uint * samples) =
{
    0, fn_gets_1, fn_gets_2, 0, fn_gets_4, 0, 0, 0,
	fn_gets_8, 0, 0, 0, fn_gets_12, 0, 0, 0,
	fn_gets_16, 0, 0, 0, 0, 0, 0, 0,
	fn_gets_24, 0, 0, 0, 0, 0, 0, 0,
	fn_gets_32
};

/*
 * Compute a value by cubic interpolation.
 * f[] = f(0), f(1), f(2), f(3); 1 < x < 2.
 * The formula is derived from those presented in
 * http://www.cs.uwa.edu.au/undergraduate/units/233.413/Handouts/Lecture04.html
 * (thanks to Raph Levien for the reference).
 */
private double
interpolate_cubic(floatp x, floatp f0, floatp f1, floatp f2, floatp f3)
{
    /*
     * The parameter 'a' affects the contribution of the high-frequency
     * components.  The abovementioned source suggests a = -0.5.
     */
#define a (-0.5) 
#define SQR(v) ((v) * (v))
#define CUBE(v) ((v) * (v) * (v))
    const double xm1 = x - 1, m2x = 2 - x, m3x = 3 - x;
    const double c =
	(a * CUBE(x) - 5 * a * SQR(x) + 8 * a * x - 4 * a) * f0 +
	((a+2) * CUBE(xm1) - (a+3) * SQR(xm1) + 1) * f1 +
	((a+2) * CUBE(m2x) - (a+3) * SQR(m2x) + 1) * f2 +
	(a * CUBE(m3x) - 5 * a * SQR(m3x) + 8 * a * m3x - 4 * a) * f3;

    if_debug6('~', "[~](%g, %g, %g, %g)order3(%g) => %g\n",
	      f0, f1, f2, f3, x, c);
    return c;
#undef a
#undef SQR
#undef CUBE
}

/*
 * Compute a value by quadratic interpolation.
 * f[] = f(0), f(1), f(2); 0 < x < 1.
 *
 * We used to use a quadratic formula for this, derived from
 * f(0) = f0, f(1) = f1, f'(1) = (f2 - f0) / 2, but now we
 * match what we believe is Acrobat Reader's behavior.
 */
inline private double
interpolate_quadratic(floatp x, floatp f0, floatp f1, floatp f2)
{
    return interpolate_cubic(x + 1, f0, f0, f1, f2);
}

/* Calculate a result by multicubic interpolation. */
private void
fn_interpolate_cubic(const gs_function_Sd_t *pfn, const float *fparts,
		     const int *iparts, const ulong *factors,
		     float *samples, ulong offset, int m)
{
    int j;

top:
    if (m == 0) {
	uint sdata[max_Sd_n];

	(*fn_get_samples[pfn->params.BitsPerSample])(pfn, offset, sdata);
	for (j = pfn->params.n - 1; j >= 0; --j)
	    samples[j] = (float)sdata[j];
    } else {
	float fpart = *fparts++;
	int ipart = *iparts++;
	ulong delta = *factors++;
	int size = pfn->params.Size[pfn->params.m - m];
	float samples1[max_Sd_n], samplesm1[max_Sd_n], samples2[max_Sd_n];

	--m;
	if (is_fzero(fpart))
	    goto top;
	fn_interpolate_cubic(pfn, fparts, iparts, factors, samples,
			     offset, m);
	fn_interpolate_cubic(pfn, fparts, iparts, factors, samples1,
			     offset + delta, m);
	/* Ensure we don't try to access out of bounds. */
	/*
	 * If size == 1, the only possible value for ipart and fpart is
	 * 0, so we've already handled this case.
	 */
	if (size == 2) {	/* ipart = 0 */
	    /* Use linear interpolation. */
	    for (j = pfn->params.n - 1; j >= 0; --j)
		samples[j] += (samples1[j] - samples[j]) * fpart;
	    return;
	}
	if (ipart == 0) {
	    /* Use quadratic interpolation. */
	    fn_interpolate_cubic(pfn, fparts, iparts, factors,
				 samples2, offset + delta * 2, m);
	    for (j = pfn->params.n - 1; j >= 0; --j)
		samples[j] =
		    interpolate_quadratic(fpart, samples[j],
					  samples1[j], samples2[j]);
	    return;
	}
	/* At this point we know ipart > 0, size >= 3. */
	fn_interpolate_cubic(pfn, fparts, iparts, factors, samplesm1,
			     offset - delta, m);
	if (ipart == size - 2) {
	    /* Use quadratic interpolation. */
	    for (j = pfn->params.n - 1; j >= 0; --j)
		samples[j] =
		    interpolate_quadratic(1 - fpart, samples1[j],
					  samples[j], samplesm1[j]);
	    return;
	}
	/* Now we know 0 < ipart < size - 2, size > 3. */
	fn_interpolate_cubic(pfn, fparts, iparts, factors,
			     samples2, offset + delta * 2, m);
	for (j = pfn->params.n - 1; j >= 0; --j)
	    samples[j] =
		interpolate_cubic(fpart + 1, samplesm1[j], samples[j],
				  samples1[j], samples2[j]);
    }
}

/* Calculate a result by multilinear interpolation. */
private void
fn_interpolate_linear(const gs_function_Sd_t *pfn, const float *fparts,
		 const ulong *factors, float *samples, ulong offset, int m)
{
    int j;

top:
    if (m == 0) {
	uint sdata[max_Sd_n];

	(*fn_get_samples[pfn->params.BitsPerSample])(pfn, offset, sdata);
	for (j = pfn->params.n - 1; j >= 0; --j)
	    samples[j] = (float)sdata[j];
    } else {
	float fpart = *fparts++;
	float samples1[max_Sd_n];

	if (is_fzero(fpart)) {
	    ++factors;
	    --m;
	    goto top;
	}
	fn_interpolate_linear(pfn, fparts, factors + 1, samples,
			      offset, m - 1);
	fn_interpolate_linear(pfn, fparts, factors + 1, samples1,
			      offset + *factors, m - 1);
	for (j = pfn->params.n - 1; j >= 0; --j)
	    samples[j] += (samples1[j] - samples[j]) * fpart;
    }
}

private inline double 
fn_Sd_encode(const gs_function_Sd_t *pfn, int i, double sample)
{
    float d0, d1, r0, r1;
    double value;
    int bps = pfn->params.BitsPerSample;
    /* x86 machines have problems with shifts if bps >= 32 */
    uint max_samp = (bps < (sizeof(uint) * 8)) ? ((1 << bps) - 1) : max_uint;

    if (pfn->params.Range)
	r0 = pfn->params.Range[2 * i], r1 = pfn->params.Range[2 * i + 1];
    else
	r0 = 0, r1 = (float)((1 << bps) - 1);
    if (pfn->params.Decode)
	d0 = pfn->params.Decode[2 * i], d1 = pfn->params.Decode[2 * i + 1];
    else
	d0 = r0, d1 = r1;

    value = sample * (d1 - d0) / max_samp + d0;
    if (value < r0)
	value = r0;
    else if (value > r1)
	value = r1;
    return value;
}

/* Evaluate a Sampled function. */
/* A generic algorithm with a recursion by dimentions. */
private int
fn_Sd_evaluate_general(const gs_function_t * pfn_common, const float *in, float *out)
{
    const gs_function_Sd_t *pfn = (const gs_function_Sd_t *)pfn_common;
    int bps = pfn->params.BitsPerSample;
    ulong offset = 0;
    int i;
    float encoded[max_Sd_m];
    int iparts[max_Sd_m];	/* only needed for cubic interpolation */
    ulong factors[max_Sd_m];
    float samples[max_Sd_n];

    /* Encode the input values. */

    for (i = 0; i < pfn->params.m; ++i) {
	float d0 = pfn->params.Domain[2 * i],
	    d1 = pfn->params.Domain[2 * i + 1];
	float arg = in[i], enc;

	if (arg < d0)
	    arg = d0;
	else if (arg > d1)
	    arg = d1;
	if (pfn->params.Encode) {
	    float e0 = pfn->params.Encode[2 * i];
	    float e1 = pfn->params.Encode[2 * i + 1];

	    enc = (arg - d0) * (e1 - e0) / (d1 - d0) + e0;
	    if (enc < 0)
		encoded[i] = 0;
	    else if (enc >= pfn->params.Size[i] - 1)
		encoded[i] = (float)pfn->params.Size[i] - 1;
	    else
		encoded[i] = enc;
	} else {
	    /* arg is guaranteed to be in bounds, ergo so is enc */
	    encoded[i] = (arg - d0) * (pfn->params.Size[i] - 1) / (d1 - d0);
	}
    }

    /* Look up and interpolate the output values. */

    {
	ulong factor = bps * pfn->params.n;

	for (i = 0; i < pfn->params.m; factor *= pfn->params.Size[i++]) {
	    int ipart = (int)encoded[i];

	    offset += (factors[i] = factor) * ipart;
	    iparts[i] = ipart;	/* only needed for cubic interpolation */
	    encoded[i] -= ipart;
	}
    }
    if (pfn->params.Order == 3)
	fn_interpolate_cubic(pfn, encoded, iparts, factors, samples,
			     offset, pfn->params.m);
    else
	fn_interpolate_linear(pfn, encoded, factors, samples, offset,
			      pfn->params.m);

    /* Encode the output values. */

    for (i = 0; i < pfn->params.n; ++i)
	out[i] = (float)fn_Sd_encode(pfn, i, samples[i]);

    return 0;
}

static const double double_stub = 1e90;

private inline void
fn_make_cubic_poles(double *p, double f0, double f1, double f2, double f3, 
	    const int pole_step_minor)
{   /* The following is poles of the polinomial,
       which represents interpolate_cubic in [1,2]. */
    const double a = -0.5;

    p[pole_step_minor * 1] = (a*f0 + 3*f1 - a*f2)/3.0;
    p[pole_step_minor * 2] = (-a*f1 + 3*f2 + a*f3)/3.0;
}

private void
fn_make_poles(double *p, const int pole_step, int power, int bias)
{
    const int pole_step_minor = pole_step / 3;
    switch(power) {
	case 1: /* A linear 3d power curve. */
	    /* bias must be 0. */
	    p[pole_step_minor * 1] = (2 * p[pole_step * 0] + 1 * p[pole_step * 1]) / 3;
	    p[pole_step_minor * 2] = (1 * p[pole_step * 0] + 2 * p[pole_step * 1]) / 3;
	    break;
	case 2:
	    /* bias may be be 0 or 1. */
	    /* Duplicate the beginning or the ending pole (the old code compatible). */
	    fn_make_cubic_poles(p + pole_step * bias, 
		    p[pole_step * 0], p[pole_step * bias], 
		    p[pole_step * (1 + bias)], p[pole_step * 2], 
		    pole_step_minor);
	    break;
	case 3:
	    /* bias must be 1. */
	    fn_make_cubic_poles(p + pole_step * bias, 
		    p[pole_step * 0], p[pole_step * 1], p[pole_step * 2], p[pole_step * 3], 
		    pole_step_minor);
	    break;
	default: /* Must not happen. */
	   DO_NOTHING; 
    }
}

/* Evaluate a Sampled function.
   A cubic interpolation with a pole cache.
   Allows a fast check for extreme suspection. */
/* This implementation is a particular case of 1 dimension. 
   maybe we'll use as an optimisation of the generic case,
   so keep it for a while. */
private int
fn_Sd_evaluate_cubic_cached_1d(const gs_function_Sd_t *pfn, const float *in, float *out)
{
    float d0 = pfn->params.Domain[2 * 0];
    float d1 = pfn->params.Domain[2 * 0 + 1];
    float x0 = in[0];
    const int pole_step_minor = pfn->params.n;
    const int pole_step = 3 * pole_step_minor;
    int i0; /* A cell index. */
    int ib, ie, i, k;
    double *p, t0, t1, tt;

    if (x0 < d0)
	x0 = d0;
    if (x0 > d1)
	x0 = d1;
    tt = (in[0] - d0) * (pfn->params.Size[0] - 1) / (d1 - d0);
    i0 = (int)floor(tt);
    ib = max(i0 - 1, 0);
    ie = min(pfn->params.Size[0], i0 + 3);
    for (i = ib; i < ie; i++) {
	if (pfn->params.pole[i * pole_step] == double_stub) {
	    uint sdata[max_Sd_n];
	    int bps = pfn->params.BitsPerSample;

	    p = &pfn->params.pole[i * pole_step];
	    fn_get_samples[pfn->params.BitsPerSample](pfn, i * bps * pfn->params.n, sdata);
	    for (k = 0; k < pfn->params.n; k++, p++)
		*p = fn_Sd_encode(pfn, k, (double)sdata[k]);
	}
    }
    p = &pfn->params.pole[i0 * pole_step];
    t0 = tt - i0;
    if (t0 == 0) {
	for (k = 0; k < pfn->params.n; k++, p++)
	    out[k] = *p;
    } else {
	if (p[1 * pole_step_minor] == double_stub) {
	    for (k = 0; k < pfn->params.n; k++)
		fn_make_poles(&pfn->params.pole[ib * pole_step + k], pole_step, 
			ie - ib - 1, i0 - ib);
	}
	t1 = 1 - t0;
	for (k = 0; k < pfn->params.n; k++, p++) {
	    double y = p[0 * pole_step_minor] * t1 * t1 * t1 +
		       p[1 * pole_step_minor] * t1 * t1 * t0 * 3 +
		       p[2 * pole_step_minor] * t1 * t0 * t0 * 3 +
		       p[3 * pole_step_minor] * t0 * t0 * t0;
	    if (y < pfn->params.Range[0])
		y = pfn->params.Range[0];
	    if (y > pfn->params.Range[1])
		y = pfn->params.Range[1];
	    out[k] = y;
	}
    }
    return 0;
}

private inline void
decode_argument(const gs_function_Sd_t *pfn, const float *in, double T[max_Sd_m], int I[max_Sd_m])
{
    int i;

    for (i = 0; i < pfn->params.m; i++) {
	float xi = in[i];
	float d0 = pfn->params.Domain[2 * i + 0];
	float d1 = pfn->params.Domain[2 * i + 1];
	double t;

	if (xi < d0)
	    xi = d0;
	if (xi > d1)
	    xi = d1;
	t = (xi - d0) * (pfn->params.Size[i] - 1) / (d1 - d0);
	I[i] = (int)floor(t);
	T[i] = t - I[i];
    }
}

private inline void
index_span(const gs_function_Sd_t *pfn, int *I, double *T, int ii, int *Ii, int *ib, int *ie)
{
    *Ii = I[ii];
    if (T[ii] != 0) {
	*ib = max(*Ii - 1, 0);
	*ie = min(pfn->params.Size[ii], *Ii + 3);
    } else {
	*ib = *Ii; 
	*ie = *Ii + 1;
    }
}

private inline int
load_vector_to(const gs_function_Sd_t *pfn, int s_offset, double *V)
{
    uint sdata[max_Sd_n];
    int k, code;

    code = fn_get_samples[pfn->params.BitsPerSample](pfn, s_offset, sdata);
    if (code < 0)
	return code;
    for (k = 0; k < pfn->params.n; k++)
	V[k] = fn_Sd_encode(pfn, k, (double)sdata[k]);
    return 0;
}

private inline int
load_vector(const gs_function_Sd_t *pfn, int a_offset, int s_offset)
{
    if (*(pfn->params.pole + a_offset) == double_stub) {
	uint sdata[max_Sd_n];
	int k, code;

	code = fn_get_samples[pfn->params.BitsPerSample](pfn, s_offset, sdata);
	if (code < 0)
	    return code;
	for (k = 0; k < pfn->params.n; k++)
	    *(pfn->params.pole + a_offset + k) = fn_Sd_encode(pfn, k, (double)sdata[k]);
    }
    return 0;
}

private inline void
interpolate_vector(const gs_function_Sd_t *pfn, int offset, int pole_step, int power, int bias)
{
    int k;

    for (k = 0; k < pfn->params.n; k++)
	fn_make_poles(pfn->params.pole + offset + k, pole_step, power, bias);
}

private inline void
interpolate_tensors(const gs_function_Sd_t *pfn, int *I, double *T, 
	int offset, int pole_step, int power, int bias, int ii)
{
    if (ii < 0)
	interpolate_vector(pfn, offset, pole_step, power, bias);
    else {
	int s = pfn->params.array_step[ii];
	int Ii = I[ii];

	if (T[ii] == 0) {
	    interpolate_tensors(pfn, I, T, offset + Ii * s, pole_step, power, bias, ii - 1);	
	} else {
	    int l;

	    for (l = 0; l < 4; l++) 
		interpolate_tensors(pfn, I, T, offset + Ii * s + l * s / 3, pole_step, power, bias, ii - 1);	
	}
    }
}

private inline bool 
is_tensor_done(const gs_function_Sd_t *pfn, int *I, double *T, int a_offset, int ii)
{
    /* Check an inner pole of the cell. */
    int i, o = 0;

    for (i = ii; i >= 0; i--) {
	o += I[i] * pfn->params.array_step[i];
	if (T[i] != 0)
	    o += pfn->params.array_step[i] / 3;
    }
    if (*(pfn->params.pole + a_offset + o) != double_stub)
	return true;
    return false;
}

/* Creates a tensor of Bezier coefficients by node interpolation. */
private inline int
make_interpolation_tensor(const gs_function_Sd_t *pfn, int *I, double *T,
			    int a_offset, int s_offset, int ii)
{
    /* Well, this function isn't obvious. Trying to explain what it does.

       Suppose we have a 4x4x4...x4 hypercube of nodes, and we want to build
       a multicubic interpolation function for the inner 2x2x2...x2 hypercube.
       We represent the multicubic function with a tensor of Besier poles,
       and the size of the tensor is 4x4x....x4. Note that the outer corners 
       of the tensor are equal to the corners of the 2x2x...x2 hypercube.

       We organized the 'pole' array so that a tensor of a cell 
       occupies the cell, and tensors for neighbour cells have a common hyperplane.

       For a 1-dimentional case the let the nodes are p0, p1, p2, p3,
       and let the tensor coefficients are q0, q1, q2, q3.
       We choose a cubic approximation, in which tangents at nodes p1, p2
       are parallel to (p2 - p0) and (p3 - p1) correspondingly.
       (Well, this doesn't give a the minimal curvity, but likely it is
       what Adobe implementations do, see the bug 687352, 
       and we agree that it's some reasonable).

       For a 2-dimensional case we apply the 1-dimentional case through
       the first dimension, and then construct a surface by varying the
       second coordinate as a parameter. It gives a bicubic surface,
       and the result doesn't depend on the order of coordinates 
       (I proved the latter with Matematica 3.0).
       Then we know that an interpolation by one coordinate and
       a differentiation by another coordinate are interchangeble operators.
       Due to that poles of the interpolated function are same as 
       interpolated poles of the function (well, we didn't spend time
       for a strong proof, but this fact was confirmed with testing the 
       implementation with POLE_CACHE_DEBUG).

       Then we apply the 2-dimentional considerations recursively 
       to all dimensions. This is exactly what this function does.

       When the source node array have an insufficient nomber of nodes
       along a dimension, we duplicate ending nodes. This solution is done 
       choosen to the compatibility to the old code, but definitely 
       there exists a better one.
     */
    int code;
    
    if (ii < 0) {
	if (POLE_CACHE_IGNORE || *(pfn->params.pole + a_offset) == double_stub) {
	    code = load_vector(pfn, a_offset, s_offset);
	    if (code < 0)
		return code;
	}
    } else {
	int Ii, ib, ie, i;
	int sa = pfn->params.array_step[ii];
	int ss = pfn->params.stream_step[ii];

	index_span(pfn, I, T, ii, &Ii, &ib, &ie);
	if (POLE_CACHE_IGNORE || !is_tensor_done(pfn, I, T, a_offset, ii)) {
	    for (i = ib; i < ie; i++) {
		code = make_interpolation_tensor(pfn, I, T, 
				a_offset + i * sa, s_offset + i * ss, ii - 1);
		if (code < 0)
		    return code;
	    }
	    if (T[ii] != 0)
		interpolate_tensors(pfn, I, T, a_offset + ib * sa, sa, ie - ib - 1, 
				Ii - ib, ii - 1);
	}
    }
    return 0;
}

/* Creates a subarray of samples. */
private inline int
make_interpolation_nodes(const gs_function_Sd_t *pfn, double *T0, double *T1,
			    int *I, double *T,
			    int a_offset, int s_offset, int ii)
{
    int code;

    if (ii < 0) {
	if (POLE_CACHE_IGNORE || *(pfn->params.pole + a_offset) == double_stub) {
	    code = load_vector(pfn, a_offset, s_offset);
	    if (code < 0)
		return code;
	}
	if (pfn->params.Order == 3) {
	    code = make_interpolation_tensor(pfn, I, T, 0, 0, pfn->params.m - 1);
	}
    } else {
	int i;
	int i0 = (int)floor(T0[ii]);
	int i1 = (int)ceil(T1[ii]);
	int sa = pfn->params.array_step[ii];
	int ss = pfn->params.stream_step[ii];

	if (i0 < 0 || i0 >= pfn->params.Size[ii])
	    return_error(gs_error_unregistered); /* Must not happen. */
	if (i1 < 0 || i1 >= pfn->params.Size[ii])
	    return_error(gs_error_unregistered); /* Must not happen. */
	I[ii] = i0;
	T[ii] = (i1 > i0 ? 1 : 0);
	for (i = i0; i <= i1; i++) {
	    code = make_interpolation_nodes(pfn, T0, T1, I, T,
			    a_offset + i * sa, s_offset + i * ss, ii - 1);
	    if (code < 0)
		return code;
	}
    }
    return 0;
}

private inline int
evaluate_from_tenzor(const gs_function_Sd_t *pfn, int *I, double *T, int offset, int ii, double *y)
{
    int s = pfn->params.array_step[ii], k, l, code;

    if (ii < 0) {
	for (k = 0; k < pfn->params.n; k++)
	    y[k] = *(pfn->params.pole + offset + k);
    } else if (T[ii] == 0) {
	return evaluate_from_tenzor(pfn, I, T, offset + s * I[ii], ii - 1, y);
    } else {
	double t0 = T[ii], t1 = 1 - t0;
	double p[4][max_Sd_n];

	for (l = 0; l < 4; l++) {
	    code = evaluate_from_tenzor(pfn, I, T, offset + s * I[ii] + l * (s / 3), ii - 1, p[l]);
	    if (code < 0)
		return code;
	}
	for (k = 0; k < pfn->params.n; k++)
	    y[k] = p[0][k] * t1 * t1 * t1 +
		   p[1][k] * t1 * t1 * t0 * 3 +
		   p[2][k] * t1 * t0 * t0 * 3 +
	   p[3][k] * t0 * t0 * t0;
    }
    return 0;
}


/* Evaluate a Sampled function. */
/* A cubic interpolation with pole cache. */
/* Allows a fast check for extreme suspection with is_tensor_monotonic. */
private int
fn_Sd_evaluate_multicubic_cached(const gs_function_Sd_t *pfn, const float *in, float *out)
{
    double T[max_Sd_m], y[max_Sd_n];
    int I[max_Sd_m], k, code;

    decode_argument(pfn, in, T, I);
    code = make_interpolation_tensor(pfn, I, T, 0, 0, pfn->params.m - 1);
    if (code < 0)
	return code;
    evaluate_from_tenzor(pfn, I, T, 0, pfn->params.m - 1, y);
    for (k = 0; k < pfn->params.n; k++) {
	double yk = y[k];

	if (yk < pfn->params.Range[k * 2 + 0])
	    yk = pfn->params.Range[k * 2 + 0];
	if (yk > pfn->params.Range[k * 2 + 1])
	    yk = pfn->params.Range[k * 2 + 1];
	out[k] = yk;
    }
    return 0;
}

/* Evaluate a Sampled function. */
private int
fn_Sd_evaluate(const gs_function_t * pfn_common, const float *in, float *out)
{
    const gs_function_Sd_t *pfn = (const gs_function_Sd_t *)pfn_common;
    int code;

    if (pfn->params.Order == 3) {
	if (POLE_CACHE_GENERIC_1D || pfn->params.m > 1)
	    code = fn_Sd_evaluate_multicubic_cached(pfn, in, out);
	else
	    code = fn_Sd_evaluate_cubic_cached_1d(pfn, in, out);
#	if POLE_CACHE_DEBUG
	{   float y[max_Sd_n];
	    int k, code1;

	    code1 = fn_Sd_evaluate_general(pfn_common, in, y);
	    assert(code == code1);
	    for (k = 0; k < pfn->params.n; k++)
		assert(any_abs(y[k] - out[k]) <= 1e-6 * (pfn->params.Range[k * 2 + 1] - pfn->params.Range[k * 2 + 0]));
	}
#	endif
    } else
	code = fn_Sd_evaluate_general(pfn_common, in, out);
    return code;
}

/* Map a function subdomain to the sample index subdomain. */
private inline int
get_scaled_range(const gs_function_Sd_t *const pfn,
		   const float *lower, const float *upper, 
		   int i, float *pw0, float *pw1)
{
    float d0 = pfn->params.Domain[i * 2 + 0], d1 = pfn->params.Domain[i * 2 + 1];
    float v0 = lower[i], v1 = upper[i];
    float e0, e1, w0, w1, w;
    const float small_noize = (float)1e-6;

    if (v0 < d0 || v0 > d1)
	return gs_error_rangecheck;
    if (pfn->params.Encode)
	e0 = pfn->params.Encode[i * 2 + 0], e1 = pfn->params.Encode[i * 2 + 1];
    else
	e0 = 0, e1 = (float)pfn->params.Size[i] - 1;
    w0 = (v0 - d0) * (e1 - e0) / (d1 - d0) + e0;
    if (w0 < 0)
	w0 = 0;
    else if (w0 >= pfn->params.Size[i] - 1)
	w0 = (float)pfn->params.Size[i] - 1;
    w1 = (v1 - d0) * (e1 - e0) / (d1 - d0) + e0;
    if (w1 < 0)
	w1 = 0;
    else if (w1 >= pfn->params.Size[i] - 1)
	w1 = (float)pfn->params.Size[i] - 1;
    if (w0 > w1) {
	w = w0; w0 = w1; w1 = w;
    }
    if (floor(w0 + 1) - w0 < small_noize * any_abs(e1 - e0))
	w0 = (floor(w0) + 1);
    if (w1 - floor(w1) < small_noize * any_abs(e1 - e0))
	w1 = floor(w1);
    if (w0 > w1)
	w0 = w1;
    *pw0 = w0;
    *pw1 = w1;
    return 0;
}

/* Copy a tensor to a differently indexed pole array. */
private int
copy_poles(const gs_function_Sd_t *pfn, int *I, double *T0, double *T1, int a_offset,
		int ii, double *pole, int p_offset, int pole_step)
{
    int i, ei, sa, code;
    int order = pfn->params.Order;

    if (pole_step <= 0)
	return_error(gs_error_limitcheck); /* Too small buffer. */
    ei = (T0[ii] == T1[ii] ? 1 : order + 1);
    sa = pfn->params.array_step[ii];
    if (ii == 0) {
	for (i = 0; i < ei; i++)
	    *(pole + p_offset + i * pole_step) = 
		    *(pfn->params.pole + a_offset + I[ii] * sa + i * (sa / order));
    } else {
	for (i = 0; i < ei; i++) {
	    code = copy_poles(pfn, I, T0, T1, a_offset + I[ii] * sa + i * (sa / order), ii - 1, 
			    pole, p_offset + i * pole_step, pole_step / 4);
	    if (code < 0)
		return code;
	}
    }
    return 0;
}

private inline void
subcurve(double *pole, int pole_step, double t0, double t1)
{
    /* Generated with subcurve.nb using Mathematica 3.0. */
    double q0 = pole[pole_step * 0];
    double q1 = pole[pole_step * 1];
    double q2 = pole[pole_step * 2];
    double q3 = pole[pole_step * 3];
    double t01 = t0 - 1, t11 = t1 - 1;
    double small = 1e-13;

#define Power2(a) (a) * (a)
#define Power3(a) (a) * (a) * (a)
    pole[pole_step * 0] = t0*(t0*(q3*t0 - 3*q2*t01) + 3*q1*Power2(t01)) - q0*Power3(t01);
    pole[pole_step * 1] = q1*t01*(-2*t0 - t1 + 3*t0*t1) + t0*(q2*t0 + 2*q2*t1 - 
			    3*q2*t0*t1 + q3*t0*t1) - q0*t11*Power2(t01);
    pole[pole_step * 2] = t1*(2*q2*t0 + q2*t1 - 3*q2*t0*t1 + q3*t0*t1) + 
			    q1*(-t0 - 2*t1 + 3*t0*t1)*t11 - q0*t01*Power2(t11);
    pole[pole_step * 3] = t1*(t1*(3*q2 - 3*q2*t1 + q3*t1) + 
			    3*q1*Power2(t11)) - q0*Power3(t11);
#undef Power2
#undef Power3
    if (any_abs(pole[pole_step * 1] - pole[pole_step * 0]) < small)
	pole[pole_step * 1] = pole[pole_step * 0];
    if (any_abs(pole[pole_step * 2] - pole[pole_step * 3]) < small)
	pole[pole_step * 2] = pole[pole_step * 3];
}

private inline void
subline(double *pole, int pole_step, double t0, double t1)
{
    double q0 = pole[pole_step * 0];
    double q1 = pole[pole_step * 1];

    pole[pole_step * 0] = (1 - t0) * q0 + t0 * q1;
    pole[pole_step * 1] = (1 - t1) * q0 + t1 * q1;
}

private void
clamp_poles(double *T0, double *T1, int ii, int i, double * pole, 
		int p_offset, int pole_step, int pole_step_i, int order)
{
    if (ii < 0) {
	if (order == 3)
	    subcurve(pole + p_offset, pole_step_i, T0[i], T1[i]);
	else
	    subline(pole + p_offset, pole_step_i, T0[i], T1[i]);
    } else if (i == ii) {
	clamp_poles(T0, T1, ii - 1, i, pole, p_offset, pole_step / 4, pole_step, order);
    } else {
	int j, ei = (T0[ii] == T1[ii] ? 1 : order + 1);

	for (j = 0; j < ei; j++)
	    clamp_poles(T0, T1, ii - 1, i, pole, p_offset + j * pole_step, 
			    pole_step / 4, pole_step_i, order);
    }
}

private inline int /* 3 - don't know, 2 - decreesing, 0 - constant, 1 - increasing. */
curve_monotonity(double *pole, int pole_step)
{
    double p0 = pole[pole_step * 0];
    double p1 = pole[pole_step * 1];
    double p2 = pole[pole_step * 2];
    double p3 = pole[pole_step * 3];

    if (p0 == p1 && any_abs(p1 - p2) < 1e-13 && p2 == p3)
	return 0;
    if (p0 <= p1 && p1 <= p2 && p2 <= p3)
	return 1;
    if (p0 >= p1 && p1 >= p2 && p2 >= p3)
	return 2;
    /* Maybe not monotonic. 
       Don't want to solve quadratic equations, so return "don't know". 
       This case should be rare.
     */
    return 3;
}

private inline int /* 2 - decreesing, 0 - constant, 1 - increasing. */
line_monotonity(double *pole, int pole_step)
{
    double p0 = pole[pole_step * 0];
    double p1 = pole[pole_step * 1];

    if (p1 - p0 > 1e-13)
	return 1;
    if (p0 - p1 > 1e-13)
	return 2;
    return 0;
}

private int /* 3 bits per guide : 3 - non-monotonic or don't know, 
		    2 - decreesing, 0 - constant, 1 - increasing. 
		    The number of guides is order+1. */
tensor_dimension_monotonity(const double *T0, const double *T1, int ii, int i0, double *pole, 
		int p_offset, int pole_step, int pole_step_i, int order)
{
    if (ii < 0) {
	if (order == 3)
	    return curve_monotonity(pole + p_offset, pole_step_i);
	else
	    return line_monotonity(pole + p_offset, pole_step_i);
    } else if (i0 == ii) {
	/* Delay the dimension till the end, and adjust pole_step. */
	return tensor_dimension_monotonity(T0, T1, ii - 1, i0, pole, p_offset, 
			    pole_step / 4, pole_step, order);
    } else {
	int j, ei = (T0[ii] == T1[ii] ? 1 : order + 1), m = 0, mm;

	for (j = 0; j < ei; j++) {
	    mm = tensor_dimension_monotonity(T0, T1, ii - 1, i0, pole, p_offset + j * pole_step, 
			    pole_step/ 4, pole_step_i, order);
	    m |= mm << (j * 3);
	    if (mm == 3) {
		/* If one guide is not monotonic, the dimension is not monotonic. 
		   Can return early. */
		break; 
	    }
	}
	return m;
    }
}

private inline int
is_tensor_monotonic_by_dimension(const gs_function_Sd_t *pfn, int *I, double *T0, double *T1, int i0, int k,
		    uint *mask /* 3 bits per guide : 3 - non-monotonic or don't know, 
		    2 - decreesing, 0 - constant, 1 - increasing. 
		    The number of guides is order+1. */)
{
    double pole[4*4*4]; /* For a while restricting with 3-in cubic functions. 
                 More arguments need a bigger buffer, but the rest of code is same. */
    int i, code, ii = pfn->params.m - 1;
    double TT0[3], TT1[3];

    *mask = 0;
    if (ii >= 3) {
	 /* Unimplemented. We don't know practical cases,
	    because currently it is only called while decomposing a shading.  */
	return_error(gs_error_limitcheck);
    }
    code = copy_poles(pfn, I, T0, T1, k, ii, pole, 0, count_of(pole) / 4);
    if (code < 0)
	return code;
    for (i = ii; i >= 0; i--) {
	TT0[i] = 0;
	if (T0[i] != T1[i]) {
	    if (T0[i] != 0 || T1[i] != 1)
		clamp_poles(T0, T1, ii, i, pole, 0, count_of(pole) / 4, -1, pfn->params.Order);
	    TT1[i] = 1;
	} else
	    TT1[i] = 0;
    }
    *mask = tensor_dimension_monotonity(TT0, TT1, ii, i0, pole, 0, 
			count_of(pole) / 4, -1, pfn->params.Order);
    return 0;
}

private int /* error code */
is_lattice_monotonic_by_dimension(const gs_function_Sd_t *pfn, const double *T0, const double *T1, 
	int *I, double *S0, double *S1, int ii, int i0, int k,
	uint *mask /* 3 bits per guide : 1 - non-monotonic or don't know, 0 - monotonic;
		      The number of guides is order+1. */)
{
    if (ii == -1) {
	/* fixme : could cache the cell monotonity against redundant evaluation. */
	return is_tensor_monotonic_by_dimension(pfn, I, S0, S1, i0, k, mask);
    } else {
	int i1 = (ii > i0 ? ii : ii == 0 ? i0 : ii - 1); /* Delay the dimension i0 till the end of recursion. */
	int j, code;
	int bi = (int)floor(T0[i1]);
	int ei = (int)floor(T1[i1]);
	uint m, mm, m1 = 0x49249249 & ((1 << ((pfn->params.Order + 1) * 3)) - 1);

	if (floor(T1[i1]) == T1[i1])
	    ei --;
	m = 0;
	for (j = bi; j <= ei; j++) {
	    /* fixme : A better performance may be obtained with comparing central nodes with side ones. */
	    I[i1] = j;
	    S0[i1] = max(T0[i1] - j, 0);
	    S1[i1] = min(T1[i1] - j, 1);
	    code = is_lattice_monotonic_by_dimension(pfn, T0, T1, I, S0, S1, ii - 1, i0, k, &mm);
	    if (code < 0)
		return code;
	    m |= mm;
	    if (m == m1) /* Don't return early - shadings need to know about all dimensions. */
		break;
	}
	if (ii == 0) {
	    /* Detect non-monotonic guides. */
	    m = m & (m >> 1);
	}
	*mask = m;
	return 0;
    }
}

private inline int /* error code */
is_lattice_monotonic(const gs_function_Sd_t *pfn, const double *T0, const double *T1, 
	 int *I, double *S0, double *S1,
	 int k, uint *mask /* 1 bit per dimension : 1 - non-monotonic or don't know, 
		      0 - monotonic. */)
{
    uint m, mm = 0;
    int i, code;

    for (i = 0; i < pfn->params.m; i++) {
	if (T0[i] != T1[i]) {
	    code = is_lattice_monotonic_by_dimension(pfn, T0, T1, I, S0, S1, pfn->params.m - 1, i, k, &m);
	    if (code < 0)
		return code;
	    if (m)
		mm |= 1 << i;
	}
    }
    *mask = mm;
    return 0;
}

private int /* 3 bits per result : 3 - non-monotonic or don't know, 		    
	       2 - decreesing, 0 - constant, 1 - increasing,
	       <0 - error. */
fn_Sd_1arg_linear_monotonic_rec(const gs_function_Sd_t *const pfn, int i0, int i1, 
				const double *V0, const double *V1)
{
    if (i1 - i0 <= 1) {
	int code = 0, i;

	for (i = 0; i < pfn->params.n; i++) {
	    if (V0[i] < V1[i])
		code |= 1 << (i * 3);
	    else if (V0[i] > V1[i])
		code |= 2 << (i * 3);
	}
	return code;
    } else {
	double VV[MAX_FAST_COMPS];
	int ii = (i0 + i1) / 2, code, cod1;

	code = load_vector_to(pfn, ii * pfn->params.n * pfn->params.BitsPerSample, VV);
	if (code < 0)
	    return code;
	if (code & (code >>1))
	    return code; /* Not monotonic by some component of the result. */
	code = fn_Sd_1arg_linear_monotonic_rec(pfn, i0, ii, V0, VV);
	if (code < 0)
	    return code;
	cod1 = fn_Sd_1arg_linear_monotonic_rec(pfn, ii, i1, VV, V1);
	if (cod1 < 0)
	    return cod1;
	return code | cod1;	
    }
}

private int
fn_Sd_1arg_linear_monotonic(const gs_function_Sd_t *const pfn, double T0, double T1,
			    uint *mask /* 1 - non-monotonic or don't know, 0 - monotonic. */)
{
    int i0 = (int)floor(T0);
    int i1 = (int)ceil(T1), code;
    double V0[MAX_FAST_COMPS], V1[MAX_FAST_COMPS];

    if (i1 - i0 > 1) {
	code = load_vector_to(pfn, i0 * pfn->params.n * pfn->params.BitsPerSample, V0);
	if (code < 0)
	    return code;
	code = load_vector_to(pfn, i1 * pfn->params.n * pfn->params.BitsPerSample, V1);
	if (code < 0)
	    return code;
	code = fn_Sd_1arg_linear_monotonic_rec(pfn, i0, i1, V0, V1);
	if (code < 0)
	    return code;
	if (code & (code >> 1)) {
	    *mask = 1;
	    return 0;
	}
    }
    *mask = 0;
    return 1;
}

#define DEBUG_Sd_1arg 0

/* Test whether a Sampled function is monotonic. */
private int /* 1 = monotonic, 0 = not or don't know, <0 = error. */
fn_Sd_is_monotonic_aux(const gs_function_Sd_t *const pfn,
		   const float *lower, const float *upper, 
		   uint *mask /* 1 bit per dimension : 1 - non-monotonic or don't know, 
		      0 - monotonic. */)
{
    int i, code, ii = pfn->params.m - 1;
    int I[4];
    double T0[count_of(I)], T1[count_of(I)];
    double S0[count_of(I)], S1[count_of(I)]; 
    uint m, mm, m1;
#   if DEBUG_Sd_1arg
    int code1, mask1;
#   endif

    if (ii >= count_of(T0)) {
	 /* Unimplemented. We don't know practical cases,
	    because currently it is only called while decomposing a shading.  */
	return_error(gs_error_limitcheck);
    }
    for (i = 0; i <= ii; i++) {
	float w0, w1;

	code = get_scaled_range(pfn, lower, upper, i, &w0, &w1);
	if (code < 0)
	    return code;
	T0[i] = w0;
	T1[i] = w1;
    }
    if (pfn->params.m == 1 && pfn->params.Order == 1 && pfn->params.n <= MAX_FAST_COMPS) {
	code = fn_Sd_1arg_linear_monotonic(pfn, T0[0], T1[0], mask);
#	if !DEBUG_Sd_1arg
	    return code;
#	else
	    mask1 = *mask;
	    code1 = code;
#	endif
    }
    m1 = (1 << pfn->params.m )- 1;
    code = make_interpolation_nodes(pfn, T0, T1, I, S0, 0, 0, ii);
    if (code < 0)
	return code;
    mm = 0;
    for (i = 0; i < pfn->params.n; i++) {
	code = is_lattice_monotonic(pfn, T0, T1, I, S0, S1, i, &m);
	if (code < 0)
	    return code;
	mm |= m;
	if (mm == m1) /* Don't return early - shadings need to know about all dimensions. */
	    break; 
    }
#   if DEBUG_Sd_1arg
	if (mask1 != mm)
	    return_error(gs_error_unregistered);
	if (code1 != !mm)
	    return_error(gs_error_unregistered);
#   endif
    *mask = mm;
    return !mm;
}

/* Test whether a Sampled function is monotonic. */
/* 1 = monotonic, 0 = don't know, <0 = error. */
private int
fn_Sd_is_monotonic(const gs_function_t * pfn_common,
		   const float *lower, const float *upper, uint *mask)
{
    const gs_function_Sd_t *const pfn =
	(const gs_function_Sd_t *)pfn_common;

    return fn_Sd_is_monotonic_aux(pfn, lower, upper, mask);
}

/* Return Sampled function information. */
private void
fn_Sd_get_info(const gs_function_t *pfn_common, gs_function_info_t *pfi)
{
    const gs_function_Sd_t *const pfn =
	(const gs_function_Sd_t *)pfn_common;
    long size;
    int i;

    gs_function_get_info_default(pfn_common, pfi);
    pfi->DataSource = &pfn->params.DataSource;
    for (i = 0, size = 1; i < pfn->params.m; ++i)
	size *= pfn->params.Size[i];
    pfi->data_size =
	(size * pfn->params.n * pfn->params.BitsPerSample + 7) >> 3;
}

/* Write Sampled function parameters on a parameter list. */
private int
fn_Sd_get_params(const gs_function_t *pfn_common, gs_param_list *plist)
{
    const gs_function_Sd_t *const pfn =
	(const gs_function_Sd_t *)pfn_common;
    int ecode = fn_common_get_params(pfn_common, plist);
    int code;

    if (pfn->params.Order != 1) {
	if ((code = param_write_int(plist, "Order", &pfn->params.Order)) < 0)
	    ecode = code;
    }
    if ((code = param_write_int(plist, "BitsPerSample",
				&pfn->params.BitsPerSample)) < 0)
	ecode = code;
    if (pfn->params.Encode) {
	if ((code = param_write_float_values(plist, "Encode",
					     pfn->params.Encode,
					     2 * pfn->params.m, false)) < 0)
	    ecode = code;
    }
    if (pfn->params.Decode) {
	if ((code = param_write_float_values(plist, "Decode",
					     pfn->params.Decode,
					     2 * pfn->params.n, false)) < 0)
	    ecode = code;
    }
    if (pfn->params.Size) {
	if ((code = param_write_int_values(plist, "Size", pfn->params.Size,
					   pfn->params.m, false)) < 0)
	    ecode = code;
    }
    return ecode;
}

/* Make a scaled copy of a Sampled function. */
private int
fn_Sd_make_scaled(const gs_function_Sd_t *pfn, gs_function_Sd_t **ppsfn,
		  const gs_range_t *pranges, gs_memory_t *mem)
{
    gs_function_Sd_t *psfn =
	gs_alloc_struct(mem, gs_function_Sd_t, &st_function_Sd,
			"fn_Sd_make_scaled");
    int code;

    if (psfn == 0)
	return_error(gs_error_VMerror);
    psfn->params = pfn->params;
    psfn->params.Encode = 0;		/* in case of failure */
    psfn->params.Decode = 0;
    psfn->params.Size =
	fn_copy_values(pfn->params.Size, pfn->params.m, sizeof(int), mem);
    if ((code = (psfn->params.Size == 0 ?
		 gs_note_error(gs_error_VMerror) : 0)) < 0 ||
	(code = fn_common_scale((gs_function_t *)psfn,
				(const gs_function_t *)pfn,
				pranges, mem)) < 0 ||
	(code = fn_scale_pairs(&psfn->params.Encode, pfn->params.Encode,
			       pfn->params.m, NULL, mem)) < 0 ||
	(code = fn_scale_pairs(&psfn->params.Decode, pfn->params.Decode,
			       pfn->params.n, pranges, mem)) < 0) {
	gs_function_free((gs_function_t *)psfn, true, mem);
    } else
	*ppsfn = psfn;
    return code;
}

/* Free the parameters of a Sampled function. */
void
gs_function_Sd_free_params(gs_function_Sd_params_t * params, gs_memory_t * mem)
{
    gs_free_const_object(mem, params->Size, "Size");
    gs_free_const_object(mem, params->Decode, "Decode");
    gs_free_const_object(mem, params->Encode, "Encode");
    fn_common_free_params((gs_function_params_t *) params, mem);
    gs_free_object(mem, params->pole, "gs_function_Sd_free_params");
    gs_free_object(mem, params->array_step, "gs_function_Sd_free_params");
    gs_free_object(mem, params->stream_step, "gs_function_Sd_free_params");
}

/* aA helper for gs_function_Sd_serialize. */
private int serialize_array(const float *a, int half_size, stream *s)
{
    uint n;
    const float dummy[2] = {0, 0};
    int i, code;

    if (a != NULL)
	return sputs(s, (const byte *)a, sizeof(a[0]) * half_size * 2, &n);
    for (i = 0; i < half_size; i++) {
	code = sputs(s, (const byte *)dummy, sizeof(dummy), &n);
	if (code < 0)
	    return code;
    }
    return 0;
}

/* Serialize. */
private int
gs_function_Sd_serialize(const gs_function_t * pfn, stream *s)
{
    uint n;
    const gs_function_Sd_params_t * p = (const gs_function_Sd_params_t *)&pfn->params;
    gs_function_info_t info;
    int code = fn_common_serialize(pfn, s);
    ulong pos;
    uint count;
    byte buf[100];
    const byte *ptr;

    if (code < 0)
	return code;
    code = sputs(s, (const byte *)&p->Order, sizeof(p->Order), &n);
    if (code < 0)
	return code;
    code = sputs(s, (const byte *)&p->BitsPerSample, sizeof(p->BitsPerSample), &n);
    if (code < 0)
	return code;
    code = serialize_array(p->Encode, p->m, s);
    if (code < 0)
	return code;
    code = serialize_array(p->Decode, p->n, s);
    if (code < 0)
	return code;
    gs_function_get_info(pfn, &info);
    code = sputs(s, (const byte *)&info.data_size, sizeof(info.data_size), &n);
    if (code < 0)
	return code;
    for (pos = 0; pos < info.data_size; pos += count) {
	count = min(sizeof(buf), info.data_size - pos);
	data_source_access_only(info.DataSource, pos, count, buf, &ptr);
	code = sputs(s, ptr, count, &n);
	if (code < 0)
	    return code;
    }
    return 0;
}

/* Allocate and initialize a Sampled function. */
int
gs_function_Sd_init(gs_function_t ** ppfn,
		  const gs_function_Sd_params_t * params, gs_memory_t * mem)
{
    static const gs_function_head_t function_Sd_head = {
	function_type_Sampled,
	{
	    (fn_evaluate_proc_t) fn_Sd_evaluate,
	    (fn_is_monotonic_proc_t) fn_Sd_is_monotonic,
	    (fn_get_info_proc_t) fn_Sd_get_info,
	    (fn_get_params_proc_t) fn_Sd_get_params,
	    (fn_make_scaled_proc_t) fn_Sd_make_scaled,
	    (fn_free_params_proc_t) gs_function_Sd_free_params,
	    fn_common_free,
	    (fn_serialize_proc_t) gs_function_Sd_serialize,
	}
    };
    int code;
    int i;

    *ppfn = 0;			/* in case of error */
    code = fn_check_mnDR((const gs_function_params_t *)params,
			 params->m, params->n);
    if (code < 0)
	return code;
    if (params->m > max_Sd_m)
	return_error(gs_error_limitcheck);
    switch (params->Order) {
	case 0:		/* use default */
	case 1:
	case 3:
	    break;
	default:
	    return_error(gs_error_rangecheck);
    }
    switch (params->BitsPerSample) {
	case 1:
	case 2:
	case 4:
	case 8:
	case 12:
	case 16:
	case 24:
	case 32:
	    break;
	default:
	    return_error(gs_error_rangecheck);
    }
    for (i = 0; i < params->m; ++i)
	if (params->Size[i] <= 0)
	    return_error(gs_error_rangecheck);
    {
	gs_function_Sd_t *pfn =
	    gs_alloc_struct(mem, gs_function_Sd_t, &st_function_Sd,
			    "gs_function_Sd_init");
	int bps, sa, ss, i, order;

	if (pfn == 0)
	    return_error(gs_error_VMerror);
	pfn->params = *params;
	if (params->Order == 0)
	    pfn->params.Order = 1;	/* default */
	pfn->params.pole = NULL;
	pfn->params.array_step = NULL;
	pfn->params.stream_step = NULL;
	pfn->head = function_Sd_head;
	pfn->params.array_size = 0;
	if (pfn->params.m == 1 && pfn->params.Order == 1 && pfn->params.n <= MAX_FAST_COMPS && !DEBUG_Sd_1arg) {
	    /* Won't use pole cache. Call fn_Sd_1arg_linear_monotonic instead. */
	} else {
	    pfn->params.array_step = (int *)gs_alloc_byte_array(mem, 
				    max_Sd_m, sizeof(int), "gs_function_Sd_init");
	    pfn->params.stream_step = (int *)gs_alloc_byte_array(mem, 
				    max_Sd_m, sizeof(int), "gs_function_Sd_init");
	    if (pfn->params.array_step == NULL || pfn->params.stream_step == NULL)
		return_error(gs_error_VMerror);
	    bps = pfn->params.BitsPerSample;
	    sa = pfn->params.n;
	    ss = pfn->params.n * bps;
	    order = pfn->params.Order;
	    for (i = 0; i < pfn->params.m; i++) {
		pfn->params.array_step[i] = sa * order;
		sa = (pfn->params.Size[i] * order - (order - 1)) * sa;
		pfn->params.stream_step[i] = ss;
		ss = pfn->params.Size[i] * ss;
	    }
	    pfn->params.pole = (double *)gs_alloc_byte_array(mem, 
				    sa, sizeof(double), "gs_function_Sd_init");
	    if (pfn->params.pole == NULL)
		return_error(gs_error_VMerror);
	    for (i = 0; i < sa; i++)
		pfn->params.pole[i] = double_stub;
	    pfn->params.array_size = sa;
	}
	*ppfn = (gs_function_t *) pfn;
    }
    return 0;
}
