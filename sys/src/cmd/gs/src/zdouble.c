/* Copyright (C) 1995, 1996, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: zdouble.c,v 1.5 2002/06/16 03:43:50 lpd Exp $ */
/* Double-precision floating point arithmetic operators */
#include "math_.h"
#include "memory_.h"
#include "string_.h"
#include "ctype_.h"
#include "ghost.h"
#include "gxfarith.h"
#include "oper.h"
#include "store.h"

/*
 * Thanks to Jean-Pierre Demailly of the Institut Fourier of the
 * Universit\'e de Grenoble I <demailly@fourier.grenet.fr> for proposing
 * this package and for arranging the funding for its creation.
 *
 * These operators work with doubles represented as 8-byte strings.  When
 * applicable, they write their result into a string supplied as an argument.
 * They also accept ints and reals as arguments.
 */

/* Forward references */
private int double_params_result(os_ptr, int, double *);
private int double_params(os_ptr, int, double *);
private int double_result(i_ctx_t *, int, double);
private int double_unary(i_ctx_t *, double (*)(double));

#define dbegin_unary()\
	os_ptr op = osp;\
	double num;\
	int code = double_params_result(op, 1, &num);\
\
	if ( code < 0 )\
	  return code

#define dbegin_binary()\
	os_ptr op = osp;\
	double num[2];\
	int code = double_params_result(op, 2, num);\
\
	if ( code < 0 )\
	  return code

/* ------ Arithmetic ------ */

/* <dnum1> <dnum2> <dresult> .dadd <dresult> */
private int
zdadd(i_ctx_t *i_ctx_p)
{
    dbegin_binary();
    return double_result(i_ctx_p, 2, num[0] + num[1]);
}

/* <dnum1> <dnum2> <dresult> .ddiv <dresult> */
private int
zddiv(i_ctx_t *i_ctx_p)
{
    dbegin_binary();
    if (num[1] == 0.0)
	return_error(e_undefinedresult);
    return double_result(i_ctx_p, 2, num[0] / num[1]);
}

/* <dnum1> <dnum2> <dresult> .dmul <dresult> */
private int
zdmul(i_ctx_t *i_ctx_p)
{
    dbegin_binary();
    return double_result(i_ctx_p, 2, num[0] * num[1]);
}

/* <dnum1> <dnum2> <dresult> .dsub <dresult> */
private int
zdsub(i_ctx_t *i_ctx_p)
{
    dbegin_binary();
    return double_result(i_ctx_p, 2, num[0] - num[1]);
}

/* ------ Simple functions ------ */

/* <dnum> <dresult> .dabs <dresult> */
private int
zdabs(i_ctx_t *i_ctx_p)
{
    return double_unary(i_ctx_p, fabs);
}

/* <dnum> <dresult> .dceiling <dresult> */
private int
zdceiling(i_ctx_t *i_ctx_p)
{
    return double_unary(i_ctx_p, ceil);
}

/* <dnum> <dresult> .dfloor <dresult> */
private int
zdfloor(i_ctx_t *i_ctx_p)
{
    return double_unary(i_ctx_p, floor);
}

/* <dnum> <dresult> .dneg <dresult> */
private int
zdneg(i_ctx_t *i_ctx_p)
{
    dbegin_unary();
    return double_result(i_ctx_p, 1, -num);
}

/* <dnum> <dresult> .dround <dresult> */
private int
zdround(i_ctx_t *i_ctx_p)
{
    dbegin_unary();
    return double_result(i_ctx_p, 1, floor(num + 0.5));
}

/* <dnum> <dresult> .dsqrt <dresult> */
private int
zdsqrt(i_ctx_t *i_ctx_p)
{
    dbegin_unary();
    if (num < 0.0)
	return_error(e_rangecheck);
    return double_result(i_ctx_p, 1, sqrt(num));
}

/* <dnum> <dresult> .dtruncate <dresult> */
private int
zdtruncate(i_ctx_t *i_ctx_p)
{
    dbegin_unary();
    return double_result(i_ctx_p, 1, (num < 0 ? ceil(num) : floor(num)));
}

/* ------ Transcendental functions ------ */

private int
darc(i_ctx_t *i_ctx_p, double (*afunc)(double))
{
    dbegin_unary();
    return double_result(i_ctx_p, 1, (*afunc)(num) * radians_to_degrees);
}
/* <dnum> <dresult> .darccos <dresult> */
private int
zdarccos(i_ctx_t *i_ctx_p)
{
    return darc(i_ctx_p, acos);
}
/* <dnum> <dresult> .darcsin <dresult> */
private int
zdarcsin(i_ctx_t *i_ctx_p)
{
    return darc(i_ctx_p, asin);
}

/* <dnum> <ddenom> <dresult> .datan <dresult> */
private int
zdatan(i_ctx_t *i_ctx_p)
{
    double result;

    dbegin_binary();
    if (num[0] == 0) {		/* on X-axis, special case */
	if (num[1] == 0)
	    return_error(e_undefinedresult);
	result = (num[1] < 0 ? 180 : 0);
    } else {
	result = atan2(num[0], num[1]) * radians_to_degrees;
	if (result < 0)
	    result += 360;
    }
    return double_result(i_ctx_p, 2, result);
}

/* <dnum> <dresult> .dcos <dresult> */
private int
zdcos(i_ctx_t *i_ctx_p)
{
    return double_unary(i_ctx_p, gs_cos_degrees);
}

/* <dbase> <dexponent> <dresult> .dexp <dresult> */
private int
zdexp(i_ctx_t *i_ctx_p)
{
    double ipart;

    dbegin_binary();
    if (num[0] == 0.0 && num[1] == 0.0)
	return_error(e_undefinedresult);
    if (num[0] < 0.0 && modf(num[1], &ipart) != 0.0)
	return_error(e_undefinedresult);
    return double_result(i_ctx_p, 2, pow(num[0], num[1]));
}

private int
dlog(i_ctx_t *i_ctx_p, double (*lfunc)(double))
{
    dbegin_unary();
    if (num <= 0.0)
	return_error(e_rangecheck);
    return double_result(i_ctx_p, 1, (*lfunc)(num));
}
/* <dposnum> <dresult> .dln <dresult> */
private int
zdln(i_ctx_t *i_ctx_p)
{
    return dlog(i_ctx_p, log);
}
/* <dposnum> <dresult> .dlog <dresult> */
private int
zdlog(i_ctx_t *i_ctx_p)
{
    return dlog(i_ctx_p, log10);
}

/* <dnum> <dresult> .dsin <dresult> */
private int
zdsin(i_ctx_t *i_ctx_p)
{
    return double_unary(i_ctx_p, gs_sin_degrees);
}

/* ------ Comparison ------ */

private int
dcompare(i_ctx_t *i_ctx_p, int mask)
{
    os_ptr op = osp;
    double num[2];
    int code = double_params(op, 2, num);

    if (code < 0)
	return code;
    make_bool(op - 1,
	      (mask & (num[0] < num[1] ? 1 : num[0] > num[1] ? 4 : 2))
	      != 0);
    pop(1);
    return 0;
}
/* <dnum1> <dnum2> .deq <bool> */
private int
zdeq(i_ctx_t *i_ctx_p)
{
    return dcompare(i_ctx_p, 2);
}
/* <dnum1> <dnum2> .dge <bool> */
private int
zdge(i_ctx_t *i_ctx_p)
{
    return dcompare(i_ctx_p, 6);
}
/* <dnum1> <dnum2> .dgt <bool> */
private int
zdgt(i_ctx_t *i_ctx_p)
{
    return dcompare(i_ctx_p, 4);
}
/* <dnum1> <dnum2> .dle <bool> */
private int
zdle(i_ctx_t *i_ctx_p)
{
    return dcompare(i_ctx_p, 3);
}
/* <dnum1> <dnum2> .dlt <bool> */
private int
zdlt(i_ctx_t *i_ctx_p)
{
    return dcompare(i_ctx_p, 1);
}
/* <dnum1> <dnum2> .dne <bool> */
private int
zdne(i_ctx_t *i_ctx_p)
{
    return dcompare(i_ctx_p, 5);
}

/* ------ Conversion ------ */

/* Take the easy way out.... */
#define MAX_CHARS 50

/* <dnum> <dresult> .cvd <dresult> */
private int
zcvd(i_ctx_t *i_ctx_p)
{
    dbegin_unary();
    return double_result(i_ctx_p, 1, num);
}

/* <string> <dresult> .cvsd <dresult> */
private int
zcvsd(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    int code = double_params_result(op, 0, NULL);
    double num;
    char buf[MAX_CHARS + 2];
    char *str = buf;
    uint len;
    char end;

    if (code < 0)
	return code;
    check_read_type(op[-1], t_string);
    len = r_size(op - 1);
    if (len > MAX_CHARS)
	return_error(e_limitcheck);
    memcpy(str, op[-1].value.bytes, len);
    /*
     * We check syntax in the following way: we remove whitespace,
     * verify that the string contains only [0123456789+-.dDeE],
     * then append a $ and then check that the next character after
     * the scanned number is a $.
     */
    while (len > 0 && isspace(*str))
	++str, --len;
    while (len > 0 && isspace(str[len - 1]))
	--len;
    str[len] = 0;
    if (strspn(str, "0123456789+-.dDeE") != len)
	return_error(e_syntaxerror);
    strcat(str, "$");
    if (sscanf(str, "%lf%c", &num, &end) != 2 || end != '$')
	return_error(e_syntaxerror);
    return double_result(i_ctx_p, 1, num);
}

/* <dnum> .dcvi <int> */
private int
zdcvi(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
#define alt_min_long (-1L << (arch_sizeof_long * 8 - 1))
#define alt_max_long (~(alt_min_long))
    static const double min_int_real = (alt_min_long * 1.0 - 1);
    static const double max_int_real = (alt_max_long * 1.0 + 1);
    double num;
    int code = double_params(op, 1, &num);

    if (code < 0)
	return code;

    if (num < min_int_real || num > max_int_real)
	return_error(e_rangecheck);
    make_int(op, (long)num);	/* truncates toward 0 */
    return 0;
}

/* <dnum> .dcvr <real> */
private int
zdcvr(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
#define b30 (0x40000000L * 1.0)
#define max_mag (0xffffff * b30 * b30 * b30 * 0x4000)
    static const float min_real = -max_mag;
    static const float max_real = max_mag;
#undef b30
#undef max_mag
    double num;
    int code = double_params(op, 1, &num);

    if (code < 0)
	return code;
    if (num < min_real || num > max_real)
	return_error(e_rangecheck);
    make_real(op, (float)num);
    return 0;
}

/* <dnum> <string> .dcvs <substring> */
private int
zdcvs(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    double num;
    int code = double_params(op - 1, 1, &num);
    char str[MAX_CHARS + 1];
    int len;

    if (code < 0)
	return code;
    check_write_type(*op, t_string);
    /*
     * To get fully accurate output results for IEEE double-
     * precision floats (53 bits of mantissa), the ANSI
     * %g default of 6 digits is not enough; 16 are needed.
     * Unfortunately, using %.16g produces unfortunate artifacts such as
     * 1.2 printing as 1.200000000000005.  Therefore, we print using %g,
     * and if the result isn't accurate enough, print again
     * using %.16g.
     */
    {
	double scanned;

	sprintf(str, "%g", num);
	sscanf(str, "%lf", &scanned);
	if (scanned != num)
	    sprintf(str, "%.16g", num);
    }
    len = strlen(str);
    if (len > r_size(op))
	return_error(e_rangecheck);
    memcpy(op->value.bytes, str, len);
    op[-1] = *op;
    r_set_size(op - 1, len);
    pop(1);
    return 0;
}

/* ------ Initialization table ------ */

/* We need to split the table because of the 16-element limit. */
const op_def zdouble1_op_defs[] = {
		/* Arithmetic */
    {"3.dadd", zdadd},
    {"3.ddiv", zddiv},
    {"3.dmul", zdmul},
    {"3.dsub", zdsub},
		/* Comparison */
    {"2.deq", zdeq},
    {"2.dge", zdge},
    {"2.dgt", zdgt},
    {"2.dle", zdle},
    {"2.dlt", zdlt},
    {"2.dne", zdne},
		/* Conversion */
    {"2.cvd", zcvd},
    {"2.cvsd", zcvsd},
    {"1.dcvi", zdcvi},
    {"1.dcvr", zdcvr},
    {"2.dcvs", zdcvs},
    op_def_end(0)
};
const op_def zdouble2_op_defs[] = {
		/* Simple functions */
    {"2.dabs", zdabs},
    {"2.dceiling", zdceiling},
    {"2.dfloor", zdfloor},
    {"2.dneg", zdneg},
    {"2.dround", zdround},
    {"2.dsqrt", zdsqrt},
    {"2.dtruncate", zdtruncate},
		/* Transcendental functions */
    {"2.darccos", zdarccos},
    {"2.darcsin", zdarcsin},
    {"3.datan", zdatan},
    {"2.dcos", zdcos},
    {"3.dexp", zdexp},
    {"2.dln", zdln},
    {"2.dlog", zdlog},
    {"2.dsin", zdsin},
    op_def_end(0)
};

/* ------ Internal procedures ------ */

/* Get some double arguments. */
private int
double_params(os_ptr op, int count, double *pval)
{
    pval += count;
    while (--count >= 0) {
	switch (r_type(op)) {
	    case t_real:
		*--pval = op->value.realval;
		break;
	    case t_integer:
		*--pval = op->value.intval;
		break;
	    case t_string:
		if (!r_has_attr(op, a_read) ||
		    r_size(op) != sizeof(double)
		)
		           return_error(e_typecheck);
		--pval;
		memcpy(pval, op->value.bytes, sizeof(double));
		break;
	    case t__invalid:
		return_error(e_stackunderflow);
	    default:
		return_error(e_typecheck);
	}
	op--;
    }
    return 0;
}

/* Get some double arguments, and check for a double result. */
private int
double_params_result(os_ptr op, int count, double *pval)
{
    check_write_type(*op, t_string);
    if (r_size(op) != sizeof(double))
	return_error(e_typecheck);
    return double_params(op - 1, count, pval);
}

/* Return a double result. */
private int
double_result(i_ctx_t *i_ctx_p, int count, double result)
{
    os_ptr op = osp;
    os_ptr op1 = op - count;

    ref_assign_inline(op1, op);
    memcpy(op1->value.bytes, &result, sizeof(double));
    pop(count);
    return 0;
}

/* Apply a unary function to a double operand. */
private int
double_unary(i_ctx_t *i_ctx_p, double (*func)(double))
{
    dbegin_unary();
    return double_result(i_ctx_p, 1, (*func)(num));
}
