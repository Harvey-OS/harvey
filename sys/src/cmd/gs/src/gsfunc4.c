/* Copyright (C) 2000, 2001 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gsfunc4.c,v 1.18 2005/04/19 14:35:12 igor Exp $ */
/* Implementation of FunctionType 4 (PostScript Calculator) Functions */
#include "math_.h"
#include "memory_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsdsrc.h"
#include "gsfunc4.h"
#include "gxfarith.h"
#include "gxfunc.h"
#include "stream.h"
#include "strimpl.h"
#include "sfilter.h"		/* for SubFileDecode */
#include "spprint.h"
#include "stream.h"

typedef struct gs_function_PtCr_s {
    gs_function_head_t head;
    gs_function_PtCr_params_t params;
    /* Define a bogus DataSource for get_function_info. */
    gs_data_source_t data_source;
} gs_function_PtCr_t;

/* GC descriptor */
private_st_function_PtCr();

/* Define the maximum stack depth. */
#define MAX_VSTACK 100		/* per documentation */

/* Define the structure of values on the stack. */
typedef enum {
    CVT_NONE = 0,	/* empty stack slot */
    CVT_BOOL,
    CVT_INT,
    CVT_FLOAT
} calc_value_type_t;
typedef struct calc_value_s {
    calc_value_type_t type;
    union {
	int i;			/* also used for Boolean */
	float f;
    } value;
} calc_value_t;

/* Store a float. */
private inline void
store_float(calc_value_t *vsp, floatp f)
{
    vsp->value.f = f;
    vsp->type = CVT_FLOAT;
}

/*
 * Define extended opcodes with typed operands.  We use the original
 * opcodes for the floating-point case.
 */
typedef enum {

	/* Typed variants */

    PtCr_abs_int = PtCr_NUM_OPCODES,
    PtCr_add_int,
    PtCr_mul_int,
    PtCr_neg_int,
    PtCr_not_bool,		/* default is int */
    PtCr_sub_int,
    PtCr_eq_int,
    PtCr_ge_int,
    PtCr_gt_int,
    PtCr_le_int,
    PtCr_lt_int,
    PtCr_ne_int,

	/* Coerce and re-dispatch */

    PtCr_int_to_float,
    PtCr_2nd_int_to_float,
    PtCr_int2_to_float,

	/* Miscellaneous */

    PtCr_no_op,
    PtCr_typecheck

} gs_PtCr_typed_opcode_t;

/* Evaluate a PostScript Calculator function. */
private int
fn_PtCr_evaluate(const gs_function_t *pfn_common, const float *in, float *out)
{
    const gs_function_PtCr_t *pfn = (const gs_function_PtCr_t *)pfn_common;
    calc_value_t vstack_buf[2 + MAX_VSTACK + 1];
    calc_value_t *vstack = &vstack_buf[1];
    calc_value_t *vsp = vstack + pfn->params.m;
    const byte *p = pfn->params.ops.data;
    int i;

    /*
     * Define the table for mapping explicit opcodes to typed opcodes.
     * We index this table with the opcode and the types of the top 2
     * values on the stack.
     */
    static const struct op_defn_s {
	byte opcode[16];	/* 4 * type[-1] + type[0] */
    } op_defn_table[] = {
	/* Keep this consistent with opcodes in gsfunc4.h! */

#define O4(op) op,op,op,op
#define E PtCr_typecheck
#define E4 O4(E)
#define N PtCr_no_op
	/* 0-operand operators */
#define OP_NONE(op)\
  {{O4(op), O4(op), O4(op), O4(op)}}
	/* 1-operand operators */
#define OP1(b, i, f)\
  {{E,b,i,f, E,b,i,f, E,b,i,f, E,b,i,f}}
#define OP_NUM1(i, f)\
  OP1(E, i, f)
#define OP_MATH1(f)\
  OP1(E, PtCr_int_to_float, f)
#define OP_ANY1(op)\
  OP1(op, op, op)
	/* 2-operand operators */
#define OP_NUM2(i, f)\
  {{E4, E4, E,E,i,PtCr_2nd_int_to_float, E,E,PtCr_int_to_float,f}}
#define OP_INT_BOOL2(i)\
  {{E4, E,i,i,E, E,i,i,E, E4}}
#define OP_MATH2(f)\
  {{E4, E4, E,E,PtCr_int2_to_float,PtCr_2nd_int_to_float,\
    E,E,PtCr_int_to_float,f}}
#define OP_INT2(i)\
  {{E4, E4, E,E,i,E, E4}}
#define OP_REL2(i, f)\
  {{E4, E,i,E,E, E,E,i,PtCr_2nd_int_to_float, E,E,PtCr_int_to_float,f}}
#define OP_ANY2(op)\
  {{E4, E,op,op,op, E,op,op,op, E,op,op,op}}

    /* Arithmetic operators */

	OP_NUM1(PtCr_abs_int, PtCr_abs),	/* abs */
	OP_NUM2(PtCr_add_int, PtCr_add),	/* add */
	OP_INT_BOOL2(PtCr_and),  /* and */
	OP_MATH2(PtCr_atan),	/* atan */
	OP_INT2(PtCr_bitshift),	/* bitshift */
	OP_NUM1(N, PtCr_ceiling),	/* ceiling */
	OP_MATH1(PtCr_cos),	/* cos */
	OP_NUM1(N, PtCr_cvi),	/* cvi */
	OP_NUM1(PtCr_int_to_float, N),	/* cvr */
	OP_MATH2(PtCr_div),	/* div */
	OP_MATH2(PtCr_exp),	/* exp */
	OP_NUM1(N, PtCr_floor),	/* floor */
	OP_INT2(PtCr_idiv),	/* idiv */
	OP_MATH1(PtCr_ln),	/* ln */
	OP_MATH1(PtCr_log),	/* log */
	OP_INT2(PtCr_mod),	/* mod */
	OP_NUM2(PtCr_mul_int, PtCr_mul),	/* mul */
	OP_NUM1(PtCr_neg_int, PtCr_neg),	/* neg */
	OP1(PtCr_not, PtCr_not, E),	/* not */
	OP_INT_BOOL2(PtCr_or),  /* or */
	OP_NUM1(N, PtCr_round),	/* round */
	OP_MATH1(PtCr_sin),	/* sin */
	OP_MATH1(PtCr_sqrt),	/* sqrt */
	OP_NUM2(PtCr_sub_int, PtCr_sub),	/* sub */
	OP_NUM1(N, PtCr_truncate),	/* truncate */
	OP_INT_BOOL2(PtCr_xor),  /* xor */

    /* Comparison operators */

	OP_REL2(PtCr_eq_int, PtCr_eq),	/* eq */
	OP_NUM2(PtCr_ge_int, PtCr_ge),	/* ge */
	OP_NUM2(PtCr_gt_int, PtCr_gt),	/* gt */
	OP_NUM2(PtCr_le_int, PtCr_le),	/* le */
	OP_NUM2(PtCr_lt_int, PtCr_lt),	/* lt */
	OP_REL2(PtCr_ne_int, PtCr_ne),	/* ne */

    /* Stack operators */

	OP1(E, PtCr_copy, E),	/* copy */
	OP_ANY1(PtCr_dup),	/* dup */
	OP_ANY2(PtCr_exch),	/* exch */
	OP1(E, PtCr_index, E),	/* index */
	OP_ANY1(PtCr_pop),	/* pop */
	OP_INT2(PtCr_roll),	/* roll */

    /* Constants */

	OP_NONE(PtCr_byte),		/* byte */
	OP_NONE(PtCr_int),		/* int */
	OP_NONE(PtCr_float),		/* float */
	OP_NONE(PtCr_true),		/* true */
	OP_NONE(PtCr_false),		/* false */

    /* Special */

	OP1(PtCr_if, E, E),		/* if */
	OP_NONE(PtCr_else),		/* else */
	OP_NONE(PtCr_return)		/* return */

    };

    vstack[-1].type = CVT_NONE;  /* for type dispatch in empty stack case */
    vstack[0].type = CVT_NONE;	/* catch underflow */
    for (i = 0; i < pfn->params.m; ++i)
	store_float(&vstack[i + 1], in[i]);

    for (; ; ) {
	int code, n;

    sw:
	switch (op_defn_table[*p++].opcode[(vsp[-1].type << 2) + vsp->type]) {

	    /* Miscellaneous */

	case PtCr_no_op:
	    continue;
	case PtCr_typecheck:
	    return_error(gs_error_typecheck);

	    /* Coerce and re-dispatch */

	case PtCr_int_to_float:
	    store_float(vsp, (floatp)vsp->value.i);
	    --p; goto sw;
	case PtCr_int2_to_float:
	    store_float(vsp, (floatp)vsp->value.i);
	case PtCr_2nd_int_to_float:
	    store_float(vsp - 1, (floatp)vsp[-1].value.i);
	    --p; goto sw;

	    /* Arithmetic operators */

	case PtCr_abs_int:
	    if (vsp->value.i < 0)
		goto neg_int;
	    continue;
	case PtCr_abs:
	    vsp->value.f = fabs(vsp->value.f);
	    continue;
	case PtCr_add_int: {
	    int int1 = vsp[-1].value.i, int2 = vsp->value.i;

	    if ((int1 ^ int2) >= 0 && ((int1 + int2) ^ int1) < 0)
		store_float(vsp - 1, (double)int1 + int2);
	    else
		vsp[-1].value.i = int1 + int2;
	    --vsp; continue;
	}
	case PtCr_add:
	    vsp[-1].value.f += vsp->value.f;
	    --vsp; continue;
	case PtCr_and:
	    vsp[-1].value.i &= vsp->value.i;
	    --vsp; continue;
	case PtCr_atan: {
	    double result;

	    code = gs_atan2_degrees(vsp[-1].value.f, vsp->value.f,
				    &result);
	    if (code < 0)
		return code;
	    vsp[-1].value.f = result;
	    --vsp; continue;
	}
	case PtCr_bitshift:
#define MAX_SHIFT (ARCH_SIZEOF_INT * 8 - 1)
	    if (vsp->value.i < -MAX_SHIFT || vsp->value.i > MAX_SHIFT)
		vsp[-1].value.i = 0;
#undef MAX_SHIFT
	    else if ((n = vsp->value.i) < 0)
		vsp[-1].value.i = ((uint)(vsp[-1].value.i)) >> -n;
	    else
		vsp[-1].value.i <<= n;
	    --vsp; continue;
	case PtCr_ceiling:
	    vsp->value.f = ceil(vsp->value.f);
	    continue;
	case PtCr_cos:
	    vsp->value.f = gs_cos_degrees(vsp->value.f);
	    continue;
	case PtCr_cvi:
	    vsp->value.i = (int)(vsp->value.f);
	    vsp->type = CVT_INT;
	    continue;
	case PtCr_cvr:
	    continue;	/* prepare handled it */
	case PtCr_div:
	    if (vsp->value.f == 0)
		return_error(gs_error_undefinedresult);
	    vsp[-1].value.f /= vsp->value.f;
	    --vsp; continue;
	case PtCr_exp:
	    vsp[-1].value.f = pow(vsp[-1].value.f, vsp->value.f);
	    --vsp; continue;
	case PtCr_floor:
	    vsp->value.f = floor(vsp->value.f);
	    continue;
	case PtCr_idiv:
	    if (vsp->value.i == 0)
		return_error(gs_error_undefinedresult);
	    if ((vsp[-1].value.i /= vsp->value.i) == min_int &&
		vsp->value.i == -1)  /* anomalous boundary case, fail */
		return_error(gs_error_rangecheck);
	    --vsp; continue;
	case PtCr_ln:
	    vsp->value.f = log(vsp->value.f);
	    continue;
	case PtCr_log:
	    vsp->value.f = log10(vsp->value.f);
	    continue;
	case PtCr_mod:
	    if (vsp->value.i == 0)
		return_error(gs_error_undefinedresult);
	    vsp[-1].value.i %= vsp->value.i;
	    --vsp; continue;
	case PtCr_mul_int: {
	    /* We don't bother to optimize this. */
	    double prod = (double)vsp[-1].value.i * vsp->value.i;

	    if (prod < min_int || prod > max_int)
		store_float(vsp - 1, prod);
	    else
		vsp[-1].value.i = (int)prod;
	    --vsp; continue;
	}
	case PtCr_mul:
	    vsp[-1].value.f *= vsp->value.f;
	    --vsp; continue;
	case PtCr_neg_int:
	neg_int:
	    if (vsp->value.i == min_int)
		store_float(vsp, (floatp)vsp->value.i); /* =self negated */
	    else
		vsp->value.i = -vsp->value.i;
	    continue;
	case PtCr_neg:
	    vsp->value.f = -vsp->value.f;
	    continue;
	case PtCr_not_bool:
	    vsp->value.i = !vsp->value.i;
	    continue;
	case PtCr_not:
	    vsp->value.i = ~vsp->value.i;
	    continue;
	case PtCr_or:
	    vsp[-1].value.i |= vsp->value.i;
	    --vsp; continue;
	case PtCr_round:
	    vsp->value.f = floor(vsp->value.f + 0.5);
	    continue;
	case PtCr_sin:
	    vsp->value.f = gs_sin_degrees(vsp->value.f);
	    continue;
	case PtCr_sqrt:
	    vsp->value.f = sqrt(vsp->value.f);
	    continue;
	case PtCr_sub_int: {
	    int int1 = vsp[-1].value.i, int2 = vsp->value.i;

	    if ((int1 ^ int2) < 0 && ((int1 - int2) ^ int1) >= 0)
		store_float(vsp - 1, (double)int1 - int2);
	    else
		vsp[-1].value.i = int1 - int2;
	    --vsp; continue;
	}
	case PtCr_sub:
	    vsp[-1].value.f -= vsp->value.f;
	    --vsp; continue;
	case PtCr_truncate:
	    vsp->value.f = (vsp->value.f < 0 ? ceil(vsp->value.f) :
			    floor(vsp->value.f));
	    continue;
	case PtCr_xor:
	    vsp[-1].value.i ^= vsp->value.i;
	    --vsp; continue;

	    /* Boolean operators */

#define DO_REL(rel, m)\
  vsp[-1].value.i = vsp[-1].value.m rel vsp->value.m

	case PtCr_eq_int:
	    DO_REL(==, i);
	    goto rel;
	case PtCr_eq:
	    DO_REL(==, f);
	rel:
	    vsp[-1].type = CVT_BOOL;
	    --vsp; continue;
	case PtCr_ge_int:
	    DO_REL(>=, i);
	    goto rel;
	case PtCr_ge:
	    DO_REL(>=, f);
	    goto rel;
	case PtCr_gt_int:
	    DO_REL(>, i);
	    goto rel;
	case PtCr_gt:
	    DO_REL(>, f);
	    goto rel;
	case PtCr_le_int:
	    DO_REL(<=, i);
	    goto rel;
	case PtCr_le:
	    DO_REL(<=, f);
	    goto rel;
	case PtCr_lt_int:
	    DO_REL(<, i);
	    goto rel;
	case PtCr_lt:
	    DO_REL(<, f);
	    goto rel;
	case PtCr_ne_int:
	    DO_REL(!=, i);
	    goto rel;
	case PtCr_ne:
	    DO_REL(!=, f);
	    goto rel;

#undef DO_REL

	    /* Stack operators */

	case PtCr_copy:
	    i = vsp->value.i;
	    n = vsp - vstack;
	    if (i < 0 || i >= n)
		return_error(gs_error_rangecheck);
	    if (i > MAX_VSTACK - (n - 1))
		return_error(gs_error_limitcheck);
	    memcpy(vsp, vsp - i, i * sizeof(*vsp));
	    vsp += i - 1;
	    continue;
	case PtCr_dup:
	    vsp[1] = *vsp;
	    goto push;
	case PtCr_exch:
	    vstack[MAX_VSTACK] = *vsp;
	    *vsp = vsp[-1];
	    vsp[-1] = vstack[MAX_VSTACK];
	    continue;
	case PtCr_index:
	    i = vsp->value.i;
	    if (i < 0 || i >= vsp - vstack - 1)
		return_error(gs_error_rangecheck);
	    *vsp = vsp[-i - 1];
	    continue;
	case PtCr_pop:
	    --vsp;
	    continue;
	case PtCr_roll:
	    n = vsp[-1].value.i;
	    i = vsp->value.i;
	    if (n < 0 || n > vsp - vstack - 2)
		return_error(gs_error_rangecheck);
	    /* We don't bother to do this efficiently. */
	    for (; i > 0; i--) {
		memmove(vsp - n, vsp - (n + 1), n * sizeof(*vsp));
		vsp[-(n + 1)] = vsp[-1];
	    }
	    for (; i < 0; i++) {
		vsp[-1] = vsp[-(n + 1)];
		memmove(vsp - (n + 1), vsp - n, n * sizeof(*vsp));
	    }
	    vsp -= 2;
	    continue;

	    /* Constants */

	case PtCr_byte:
	    vsp[1].value.i = *p++, vsp[1].type = CVT_INT;
	    goto push;
	case PtCr_int /* native */:
	    memcpy(&vsp[1].value.i, p, sizeof(int));
	    vsp[1].type = CVT_INT;
	    p += sizeof(int);
	    goto push;
	case PtCr_float /* native */:
	    memcpy(&vsp[1].value.f, p, sizeof(float));
	    vsp[1].type = CVT_FLOAT;
	    p += sizeof(float);
	    goto push;
	case PtCr_true:
	    vsp[1].value.i = true, vsp[1].type = CVT_BOOL;
	    goto push;
	case PtCr_false:
	    vsp[1].value.i = false, vsp[1].type = CVT_BOOL;
	push:
	    if (vsp == &vstack[MAX_VSTACK])
		return_error(gs_error_limitcheck);
	    ++vsp;
	    continue;

	    /* Special */

	case PtCr_if:
	    if ((vsp--)->value.i) {	/* value is true, execute body */
		p += 2;
		continue;
	    }
	    /* falls through */
	case PtCr_else:
	    p += 2 + (p[0] << 8) + p[1];
	    continue;
	case PtCr_return:
	    goto fin;
	}
    }
 fin:

    if (vsp != vstack + pfn->params.n)
	return_error(gs_error_rangecheck);
    for (i = 0; i < pfn->params.n; ++i) {
	switch (vstack[i + 1].type) {
	case CVT_INT:
	    out[i] = (float)vstack[i + 1].value.i;
	    break;
	case CVT_FLOAT:
	    out[i] = vstack[i + 1].value.f;
	    break;
	default:
	    return_error(gs_error_typecheck);
	}
    }
    return 0;
}

/* Test whether a PostScript Calculator function is monotonic. */
private int
fn_PtCr_is_monotonic(const gs_function_t * pfn_common,
		     const float *lower, const float *upper, uint *mask)
{
    /*
     * No reasonable way to tell.  Eventually we should check for
     * functions consisting of only stack-manipulating operations,
     * since these may be common for DeviceN color spaces and *are*
     * monotonic.
     */
    *mask = 0x49249249;
    return 0;
}

/* Write the function definition in symbolic form on a stream. */
private int
calc_put_ops(stream *s, const byte *ops, uint size)
{
    const byte *p;

    spputc(s, '{');
    for (p = ops; p < ops + size; )
	switch (*p++) {
	case PtCr_byte:
	    pprintd1(s, "%d ", *p++);
	    break;
	case PtCr_int: {
	    int i;

	    memcpy(&i, p, sizeof(int));
	    pprintd1(s, "%d ", i);
	    p += sizeof(int);
	    break;
	}
	case PtCr_float: {
	    float f;

	    memcpy(&f, p, sizeof(float));
	    pprintg1(s, "%g ", f);
	    p += sizeof(float);
	    break;
	}
	case PtCr_true:
	    stream_puts(s, "true ");
	    break;
	case PtCr_false:
	    stream_puts(s, "false ");
	    break;
	case PtCr_if: {
	    int skip = (p[0] << 8) + p[1];
	    int code;

	    code = calc_put_ops(s, p += 2, skip);
	    p += skip;
	    if (code < 0)
		return code;
	    if (code > 0) {	/* else */
		skip = (p[-2] << 8) + p[-1];
		code = calc_put_ops(s, p, skip);
		p += skip;
		if (code < 0)
		    return code;
		stream_puts(s, " ifelse ");
	    } else
		stream_puts(s, " if ");
	    break;
	}
	case PtCr_else:
	    if (p != ops + size - 2)
		return_error(gs_error_rangecheck);
	    spputc(s, '}');
	    return 1;
	/*case PtCr_return:*/	/* not possible */
	default: {		/* must be < PtCr_NUM_OPS */
		static const char *const op_names[] = {
		    /* Keep this consistent with opcodes in gsfunc4.h! */
		    "abs", "add", "and", "atan", "bitshift",
		    "ceiling", "cos", "cvi", "cvr", "div", "exp",
		    "floor", "idiv", "ln", "log", "mod", "mul",
		    "neg", "not", "or", "round", "sin", "sqrt", "sub",
		    "truncate", "xor",
		    "eq", "ge", "gt", "le", "lt", "ne",
		    "copy", "dup", "exch", "index", "pop", "roll"
		};

		pprints1(s, "%s ", op_names[p[-1]]);
	    }
	}
    spputc(s, '}');
    return 0;
}
private int
calc_put(stream *s, const gs_function_PtCr_t *pfn)
{
    calc_put_ops(s, pfn->params.ops.data, pfn->params.ops.size - 1);
    return 0;
}

/* Access the symbolic definition as a DataSource. */
private int
calc_access(const gs_data_source_t *psrc, ulong start, uint length,
	    byte *buf, const byte **ptr)
{
    const gs_function_PtCr_t *const pfn =
	(const gs_function_PtCr_t *)
	  ((const char *)psrc - offset_of(gs_function_PtCr_t, data_source));
    /*
     * The caller wants a specific substring of the symbolic definition.
     * Generate the entire definition, using a SubFileDecode filter (in an
     * output pipeline!) to extract the substring.  This is very
     * inefficient, but this code is rarely used, and almost never actually
     * has to break up the definition into pieces to fit in the caller's
     * buffer.
     */
    stream_SFD_state st;
    stream ds, bs;
    byte dbuf[200];		/* arbitrary */
    const stream_template *const template = &s_SFD_template;

    /* Set up the stream that writes into the buffer. */
    s_init(&bs, NULL);
    swrite_string(&bs, buf, length);
    /* Set up the SubFileDecode stream. */
    s_init(&ds, NULL);
    s_init_state((stream_state *)&st, template, NULL);
    template->set_defaults((stream_state *)&st);
    st.skip_count = start;
    s_init_filter(&ds, (stream_state *)&st, dbuf, sizeof(dbuf), &bs);
    calc_put(&ds, pfn);
    sclose(&ds);
    if (ptr)
	*ptr = buf;
    return 0;
}

/* Return PostScript Calculator function information. */
private void
fn_PtCr_get_info(const gs_function_t *pfn_common, gs_function_info_t *pfi)
{
    const gs_function_PtCr_t *const pfn =
	(const gs_function_PtCr_t *)pfn_common;

    gs_function_get_info_default(pfn_common, pfi);
    pfi->DataSource = &pfn->data_source;
    {
	stream s;

	s_init(&s, NULL);
	swrite_position_only(&s);
	calc_put(&s, pfn);
	pfi->data_size = stell(&s);
    }
}

/* Make a scaled copy of a PostScript Calculator function. */
private int
fn_PtCr_make_scaled(const gs_function_PtCr_t *pfn, gs_function_PtCr_t **ppsfn,
		    const gs_range_t *pranges, gs_memory_t *mem)
{
    gs_function_PtCr_t *psfn =
	gs_alloc_struct(mem, gs_function_PtCr_t, &st_function_PtCr,
			"fn_PtCr_make_scaled");
    /* We are adding {<int> 1 roll <float> mul <float> add} for each output. */
    int n = pfn->params.n;
    uint opsize = pfn->params.ops.size + (9 + 2 * sizeof(float)) * n;
    byte *ops = gs_alloc_string(mem, opsize, "fn_PtCr_make_scaled(ops)");
    byte *p;
    int code, i;

    if (psfn == 0 || ops == 0) {
	gs_free_string(mem, ops, opsize, "fn_PtCr_make_scaled(ops)");
	gs_free_object(mem, psfn, "fn_PtCr_make_scaled");
	return_error(gs_error_VMerror);
    }
    psfn->params = pfn->params;
    psfn->params.ops.data = ops;
    psfn->params.ops.size = opsize;
    psfn->data_source = pfn->data_source;
    code = fn_common_scale((gs_function_t *)psfn, (const gs_function_t *)pfn,
			   pranges, mem);
    if (code < 0) {
	gs_function_free((gs_function_t *)psfn, true, mem);
	return code;
    }
    memcpy(ops, pfn->params.ops.data, pfn->params.ops.size - 1); /* minus return */
    p = ops + pfn->params.ops.size - 1;
    for (i = n; --i >= 0; ) {
	float base = pranges[i].rmin;
	float factor = pranges[i].rmax - base;

	if (factor != 1) {
	    p[0] = PtCr_float; memcpy(p + 1, &factor, sizeof(float));
	    p += 1 + sizeof(float);
	    *p++ = PtCr_mul;
	}
	if (base != 0) {
	    p[0] = PtCr_float; memcpy(p + 1, &base, sizeof(float));
	    p += 1 + sizeof(float);
	    *p++ = PtCr_add;
	}
	if (n != 1) {
	    p[0] = PtCr_byte; p[1] = (byte)n;
	    p[2] = PtCr_byte; p[3] = 1;
	    p[4] = PtCr_roll;
	    p += 5;
	}
    }
    *p++ = PtCr_return;
    psfn->params.ops.size = p - ops;
    psfn->params.ops.data =
	gs_resize_string(mem, ops, opsize, psfn->params.ops.size,
			 "fn_PtCr_make_scaled");
    *ppsfn = psfn;
    return 0;
}


/* Free the parameters of a PostScript Calculator function. */
void
gs_function_PtCr_free_params(gs_function_PtCr_params_t * params, gs_memory_t * mem)
{
    gs_free_const_string(mem, params->ops.data, params->ops.size, "ops");
    fn_common_free_params((gs_function_params_t *) params, mem);
}

/* Serialize. */
private int
gs_function_PtCr_serialize(const gs_function_t * pfn, stream *s)
{
    uint n;
    const gs_function_PtCr_params_t * p = (const gs_function_PtCr_params_t *)&pfn->params;
    int code = fn_common_serialize(pfn, s);

    if (code < 0)
	return code;
    code = sputs(s, (const byte *)&p->ops.size, sizeof(p->ops.size), &n);
    if (code < 0)
	return code;
    return sputs(s, p->ops.data, p->ops.size, &n);
}

/* Allocate and initialize a PostScript Calculator function. */
int
gs_function_PtCr_init(gs_function_t ** ppfn,
		  const gs_function_PtCr_params_t * params, gs_memory_t * mem)
{
    static const gs_function_head_t function_PtCr_head = {
	function_type_PostScript_Calculator,
	{
	    (fn_evaluate_proc_t) fn_PtCr_evaluate,
	    (fn_is_monotonic_proc_t) fn_PtCr_is_monotonic,
	    (fn_get_info_proc_t) fn_PtCr_get_info,
	    fn_common_get_params,
	    (fn_make_scaled_proc_t) fn_PtCr_make_scaled,
	    (fn_free_params_proc_t) gs_function_PtCr_free_params,
	    fn_common_free,
	    (fn_serialize_proc_t) gs_function_PtCr_serialize,
	}
    };
    int code;

    *ppfn = 0;			/* in case of error */
    code = fn_check_mnDR((const gs_function_params_t *)params,
			 params->m, params->n);
    if (code < 0)
	return code;
    if (params->m > MAX_VSTACK || params->n > MAX_VSTACK)
	return_error(gs_error_limitcheck);
    /*
     * Pre-validate the operation string to reduce evaluation overhead.
     */
    {
	const byte *p = params->ops.data;

	for (; *p != PtCr_return; ++p)
	    switch ((gs_PtCr_opcode_t)*p) {
	    case PtCr_byte:
		++p; break;
	    case PtCr_int:
		p += sizeof(int); break;
	    case PtCr_float:
		p += sizeof(float); break;
	    case PtCr_if:
	    case PtCr_else:
		p += 2;
	    case PtCr_true:
	    case PtCr_false:
		break;
	    default:
		if (*p >= PtCr_NUM_OPS)
		    return_error(gs_error_rangecheck);
	    }
	if (p != params->ops.data + params->ops.size - 1)
	    return_error(gs_error_rangecheck);
    }
    {
	gs_function_PtCr_t *pfn =
	    gs_alloc_struct(mem, gs_function_PtCr_t, &st_function_PtCr,
			    "gs_function_PtCr_init");

	if (pfn == 0)
	    return_error(gs_error_VMerror);
	pfn->params = *params;
	/*
	 * We claim to have a DataSource, in order to write the function
	 * definition in symbolic form for embedding in PDF files.
	 * ****** THIS IS A HACK. ******
	 */
	data_source_init_string2(&pfn->data_source, NULL, 0);
	pfn->data_source.access = calc_access;
	pfn->head = function_PtCr_head;
	*ppfn = (gs_function_t *) pfn;
    }
    return 0;
}
