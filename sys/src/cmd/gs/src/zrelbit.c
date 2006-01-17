/* Copyright (C) 1989, 1995, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: zrelbit.c,v 1.6 2004/08/04 19:36:13 stefan Exp $ */
/* Relational, boolean, and bit operators */
#include "ghost.h"
#include "oper.h"
#include "gsutil.h"
#include "idict.h"
#include "store.h"

/*
 * Many of the procedures in this file are public only so they can be
 * called from the FunctionType 4 interpreter (zfunc4.c).
 */

/* ------ Standard operators ------ */

/* Define the type test for eq and its relatives. */
#define EQ_CHECK_READ(opp, dflt)\
    switch ( r_type(opp) ) {\
	case t_string:\
	    check_read(*(opp));\
	    break;\
	default:\
	    dflt;\
  }

/* Forward references */
private int obj_le(os_ptr, os_ptr);

/* <obj1> <obj2> eq <bool> */
int
zeq(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    EQ_CHECK_READ(op - 1, check_op(2));
    EQ_CHECK_READ(op, DO_NOTHING);
    make_bool(op - 1, (obj_eq(imemory, op - 1, op) ? 1 : 0));
    pop(1);
    return 0;
}

/* <obj1> <obj2> ne <bool> */
int
zne(i_ctx_t *i_ctx_p)
{	/* We'll just be lazy and use eq. */
    int code = zeq(i_ctx_p);

    if (!code)
	osp->value.boolval ^= 1;
    return code;
}

/* <num1> <num2> ge <bool> */
/* <str1> <str2> ge <bool> */
int
zge(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    int code = obj_le(op, op - 1);

    if (code < 0)
	return code;
    make_bool(op - 1, code);
    pop(1);
    return 0;
}

/* <num1> <num2> gt <bool> */
/* <str1> <str2> gt <bool> */
int
zgt(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    int code = obj_le(op - 1, op);

    if (code < 0)
	return code;
    make_bool(op - 1, code ^ 1);
    pop(1);
    return 0;
}

/* <num1> <num2> le <bool> */
/* <str1> <str2> le <bool> */
int
zle(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    int code = obj_le(op - 1, op);

    if (code < 0)
	return code;
    make_bool(op - 1, code);
    pop(1);
    return 0;
}

/* <num1> <num2> lt <bool> */
/* <str1> <str2> lt <bool> */
int
zlt(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    int code = obj_le(op, op - 1);

    if (code < 0)
	return code;
    make_bool(op - 1, code ^ 1);
    pop(1);
    return 0;
}

/* <num1> <num2> .max <num> */
/* <str1> <str2> .max <str> */
private int
zmax(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    int code = obj_le(op - 1, op);

    if (code < 0)
	return code;
    if (code) {
	ref_assign(op - 1, op);
    }
    pop(1);
    return 0;
}

/* <num1> <num2> .min <num> */
/* <str1> <str2> .min <str> */
private int
zmin(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    int code = obj_le(op - 1, op);

    if (code < 0)
	return code;
    if (!code) {
	ref_assign(op - 1, op);
    }
    pop(1);
    return 0;
}

/* <bool1> <bool2> and <bool> */
/* <int1> <int2> and <int> */
int
zand(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;

    switch (r_type(op)) {
	case t_boolean:
	    check_type(op[-1], t_boolean);
	    op[-1].value.boolval &= op->value.boolval;
	    break;
	case t_integer:
	    check_type(op[-1], t_integer);
	    op[-1].value.intval &= op->value.intval;
	    break;
	default:
	    return_op_typecheck(op);
    }
    pop(1);
    return 0;
}

/* <bool> not <bool> */
/* <int> not <int> */
int
znot(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;

    switch (r_type(op)) {
	case t_boolean:
	    op->value.boolval = !op->value.boolval;
	    break;
	case t_integer:
	    op->value.intval = ~op->value.intval;
	    break;
	default:
	    return_op_typecheck(op);
    }
    return 0;
}

/* <bool1> <bool2> or <bool> */
/* <int1> <int2> or <int> */
int
zor(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;

    switch (r_type(op)) {
	case t_boolean:
	    check_type(op[-1], t_boolean);
	    op[-1].value.boolval |= op->value.boolval;
	    break;
	case t_integer:
	    check_type(op[-1], t_integer);
	    op[-1].value.intval |= op->value.intval;
	    break;
	default:
	    return_op_typecheck(op);
    }
    pop(1);
    return 0;
}

/* <bool1> <bool2> xor <bool> */
/* <int1> <int2> xor <int> */
int
zxor(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;

    switch (r_type(op)) {
	case t_boolean:
	    check_type(op[-1], t_boolean);
	    op[-1].value.boolval ^= op->value.boolval;
	    break;
	case t_integer:
	    check_type(op[-1], t_integer);
	    op[-1].value.intval ^= op->value.intval;
	    break;
	default:
	    return_op_typecheck(op);
    }
    pop(1);
    return 0;
}

/* <int> <shift> bitshift <int> */
int
zbitshift(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    int shift;

    check_type(*op, t_integer);
    check_type(op[-1], t_integer);
#define MAX_SHIFT (arch_sizeof_long * 8 - 1)
    if (op->value.intval < -MAX_SHIFT || op->value.intval > MAX_SHIFT)
	op[-1].value.intval = 0;
#undef MAX_SHIFT
    else if ((shift = op->value.intval) < 0)
	op[-1].value.intval = ((ulong)(op[-1].value.intval)) >> -shift;
    else
	op[-1].value.intval <<= shift;
    pop(1);
    return 0;
}

/* ------ Extensions ------ */

/* <obj1> <obj2> .identeq <bool> */
private int
zidenteq(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;

    EQ_CHECK_READ(op - 1, check_op(2));
    EQ_CHECK_READ(op, DO_NOTHING);
    make_bool(op - 1, (obj_ident_eq(imemory, op - 1, op) ? 1 : 0));
    pop(1);
    return 0;

}

/* <obj1> <obj2> .identne <bool> */
private int
zidentne(i_ctx_t *i_ctx_p)
{
	/* We'll just be lazy and use .identeq. */
    int code = zidenteq(i_ctx_p);

    if (!code)
	osp->value.boolval ^= 1;
    return code;
}

/* ------ Initialization procedure ------ */

const op_def zrelbit_op_defs[] =
{
    {"2and", zand},
    {"2bitshift", zbitshift},
    {"2eq", zeq},
    {"2ge", zge},
    {"2gt", zgt},
    {"2le", zle},
    {"2lt", zlt},
    {"2.max", zmax},
    {"2.min", zmin},
    {"2ne", zne},
    {"1not", znot},
    {"2or", zor},
    {"2xor", zxor},
		/* Extensions */
    {"2.identeq", zidenteq},
    {"2.identne", zidentne},
    op_def_end(0)
};

/* ------ Internal routines ------ */

/* Compare two operands (both numeric, or both strings). */
/* Return 1 if op[-1] <= op[0], 0 if op[-1] > op[0], */
/* or a (negative) error code. */
private int
obj_le(register os_ptr op1, register os_ptr op)
{
    switch (r_type(op1)) {
	case t_integer:
	    switch (r_type(op)) {
		case t_integer:
		    return (op1->value.intval <= op->value.intval);
		case t_real:
		    return ((double)op1->value.intval <= op->value.realval);
		default:
		    return_op_typecheck(op);
	    }
	case t_real:
	    switch (r_type(op)) {
		case t_real:
		    return (op1->value.realval <= op->value.realval);
		case t_integer:
		    return (op1->value.realval <= (double)op->value.intval);
		default:
		    return_op_typecheck(op);
	    }
	case t_string:
	    check_read(*op1);
	    check_read_type(*op, t_string);
	    return (bytes_compare(op1->value.bytes, r_size(op1),
				  op->value.bytes, r_size(op)) <= 0);
	default:
	    return_op_typecheck(op1);
    }
}
