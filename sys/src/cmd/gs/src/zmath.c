/* Copyright (C) 1989, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: zmath.c,v 1.5 2002/02/21 22:24:54 giles Exp $ */
/* Mathematical operators */
#include "math_.h"
#include "ghost.h"
#include "gxfarith.h"
#include "oper.h"
#include "store.h"

/*
 * Many of the procedures in this file are public only so they can be
 * called from the FunctionType 4 interpreter (zfunc4.c).
 */

/*
 * Define the current state of random number generator for operators.  We
 * have to implement this ourselves because the Unix rand doesn't provide
 * anything equivalent to rrand.  Note that the value always lies in the
 * range [0..0x7ffffffe], even if longs are longer than 32 bits.
 *
 * The state must be public so that context switching can save and
 * restore it.  (Even though the Red Book doesn't mention this,
 * we verified with Adobe that this is the case.)
 */
#define zrand_state (i_ctx_p->rand_state)

/* Initialize the random number generator. */
const long rand_state_initial = 1;

/****** NOTE: none of these operators currently ******/
/****** check for floating over- or underflow.	******/

/* <num> sqrt <real> */
int
zsqrt(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    double num;
    int code = real_param(op, &num);

    if (code < 0)
	return code;
    if (num < 0.0)
	return_error(e_rangecheck);
    make_real(op, sqrt(num));
    return 0;
}

/* <num> arccos <real> */
private int
zarccos(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    double num, result;
    int code = real_param(op, &num);

    if (code < 0)
	return code;
    result = acos(num) * radians_to_degrees;
    make_real(op, result);
    return 0;
}

/* <num> arcsin <real> */
private int
zarcsin(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    double num, result;
    int code = real_param(op, &num);

    if (code < 0)
	return code;
    result = asin(num) * radians_to_degrees;
    make_real(op, result);
    return 0;
}

/* <num> <denom> atan <real> */
int
zatan(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    double args[2];
    double result;
    int code = num_params(op, 2, args);

    if (code < 0)
	return code;
    code = gs_atan2_degrees(args[0], args[1], &result);
    if (code < 0)
	return code;
    make_real(op - 1, result);
    pop(1);
    return 0;
}

/* <num> cos <real> */
int
zcos(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    double angle;
    int code = real_param(op, &angle);

    if (code < 0)
	return code;
    make_real(op, gs_cos_degrees(angle));
    return 0;
}

/* <num> sin <real> */
int
zsin(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    double angle;
    int code = real_param(op, &angle);

    if (code < 0)
	return code;
    make_real(op, gs_sin_degrees(angle));
    return 0;
}

/* <base> <exponent> exp <real> */
int
zexp(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    double args[2];
    double result;
    double ipart;
    int code = num_params(op, 2, args);

    if (code < 0)
	return code;
    if (args[0] == 0.0 && args[1] == 0.0)
	return_error(e_undefinedresult);
    if (args[0] < 0.0 && modf(args[1], &ipart) != 0.0)
	return_error(e_undefinedresult);
    result = pow(args[0], args[1]);
    make_real(op - 1, result);
    pop(1);
    return 0;
}

/* <posnum> ln <real> */
int
zln(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    double num;
    int code = real_param(op, &num);

    if (code < 0)
	return code;
    if (num <= 0.0)
	return_error(e_rangecheck);
    make_real(op, log(num));
    return 0;
}

/* <posnum> log <real> */
int
zlog(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    double num;
    int code = real_param(op, &num);

    if (code < 0)
	return code;
    if (num <= 0.0)
	return_error(e_rangecheck);
    make_real(op, log10(num));
    return 0;
}

/* - rand <int> */
private int
zrand(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;

	/*
	 * We use an algorithm from CACM 31 no. 10, pp. 1192-1201,
	 * October 1988.  According to a posting by Ed Taft on
	 * comp.lang.postscript, Level 2 (Adobe) PostScript interpreters
	 * use this algorithm too:
	 *      x[n+1] = (16807 * x[n]) mod (2^31 - 1)
	 */
#define A 16807
#define M 0x7fffffff
#define Q 127773		/* M / A */
#define R 2836			/* M % A */
    zrand_state = A * (zrand_state % Q) - R * (zrand_state / Q);
    /* Note that zrand_state cannot be 0 here. */
    if (zrand_state <= 0)
	zrand_state += M;
#undef A
#undef M
#undef Q
#undef R
    push(1);
    make_int(op, zrand_state);
    return 0;
}

/* <int> srand - */
private int
zsrand(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    long state;

    check_type(*op, t_integer);
    state = op->value.intval;
#if arch_sizeof_long > 4
    /* Trim the state back to 32 bits. */
    state = (int)state;
#endif
    /*
     * The following somewhat bizarre adjustments are according to
     * public information from Adobe describing their implementation.
     */
    if (state < 1)
	state = -(state % 0x7ffffffe) + 1;
    else if (state > 0x7ffffffe)
	state = 0x7ffffffe;
    zrand_state = state;
    pop(1);
    return 0;
}

/* - rrand <int> */
private int
zrrand(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;

    push(1);
    make_int(op, zrand_state);
    return 0;
}

/* ------ Initialization procedure ------ */

const op_def zmath_op_defs[] =
{
    {"1arccos", zarccos},	/* extension */
    {"1arcsin", zarcsin},	/* extension */
    {"2atan", zatan},
    {"1cos", zcos},
    {"2exp", zexp},
    {"1ln", zln},
    {"1log", zlog},
    {"0rand", zrand},
    {"0rrand", zrrand},
    {"1sin", zsin},
    {"1sqrt", zsqrt},
    {"1srand", zsrand},
    op_def_end(0)
};
