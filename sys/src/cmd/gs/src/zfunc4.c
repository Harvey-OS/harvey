/* Copyright (C) 1999, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: zfunc4.c,v 1.12 2004/08/04 19:36:13 stefan Exp $ */
/* PostScript language support for FunctionType 4 (PS Calculator) Functions */
#include "memory_.h"
#include "ghost.h"
#include "oper.h"
#include "opextern.h"
#include "gsfunc.h"
#include "gsfunc4.h"
#include "gsutil.h"
#include "idict.h"
#include "ifunc.h"
#include "iname.h"
#include "dstack.h"
#include "ialloc.h"
/*
 * FunctionType 4 functions are not defined in the PostScript language.  We
 * provide support for them because they are needed for PDF 1.3.  In
 * addition to the standard FunctionType, Domain, and Range keys, they have
 * a Function key whose value is a procedure in a restricted subset of the
 * PostScript language.  Specifically, the procedure must (recursively)
 * contain only integer, real, Boolean, and procedure constants (only as
 * literal operands of if and and ifelse), and operators chosen from the set
 * given below.  Note that names other than true and false are not allowed:
 * the procedure must be 'bound'.
 *
 * The following list is taken directly from the PDF 1.3 documentation.
 */
#define XOP(zfn) int zfn(i_ctx_t *)
XOP(zabs); XOP(zand); XOP(zatan); XOP(zbitshift);
XOP(zceiling); XOP(zcos); XOP(zcvi); XOP(zcvr);
XOP(zdiv); XOP(zexp); XOP(zfloor); XOP(zidiv);
XOP(zln); XOP(zlog); XOP(zmod); XOP(zmul);
XOP(zneg); XOP(znot); XOP(zor); XOP(zround);
XOP(zsin); XOP(zsqrt); XOP(ztruncate); XOP(zxor);
XOP(zeq); XOP(zge); XOP(zgt); XOP(zle); XOP(zlt); XOP(zne);
XOP(z2copy);
#undef XOP
typedef struct calc_op_s {
    op_proc_t proc;
    gs_PtCr_opcode_t opcode;
} calc_op_t;
static const calc_op_t calc_ops[] = {

    /* Arithmetic operators */

    {zabs, PtCr_abs},
    {zadd, PtCr_add},
    {zand, PtCr_and},
    {zatan, PtCr_atan},
    {zbitshift, PtCr_bitshift},
    {zceiling, PtCr_ceiling},
    {zcos, PtCr_cos},
    {zcvi, PtCr_cvi},
    {zcvr, PtCr_cvr},
    {zdiv, PtCr_div},
    {zexp, PtCr_exp},
    {zfloor, PtCr_floor},
    {zidiv, PtCr_idiv},
    {zln, PtCr_ln},
    {zlog, PtCr_log},
    {zmod, PtCr_mod},
    {zmul, PtCr_mul},
    {zneg, PtCr_neg},
    {znot, PtCr_not},
    {zor, PtCr_or},
    {zround, PtCr_round},
    {zsin, PtCr_sin},
    {zsqrt, PtCr_sqrt},
    {zsub, PtCr_sub},
    {ztruncate, PtCr_truncate},
    {zxor, PtCr_xor},

    /* Comparison operators */

    {zeq, PtCr_eq},
    {zge, PtCr_ge},
    {zgt, PtCr_gt},
    {zle, PtCr_le},
    {zlt, PtCr_lt},
    {zne, PtCr_ne},

    /* Stack operators */

    {zcopy, PtCr_copy},
    {z2copy, PtCr_copy},
    {zdup, PtCr_dup},
    {zexch, PtCr_exch},
    {zindex, PtCr_index},
    {zpop, PtCr_pop},
    {zroll, PtCr_roll}

    /* Special operators */

    /*{zif, PtCr_if},*/
    /*{zifelse, PtCr_ifelse},*/
    /*{ztrue, PtCr_true},*/
    /*{zfalse, PtCr_false}*/
};

/* Fix up an if or ifelse forward reference. */
private void
psc_fixup(byte *p, byte *to)
{
    int skip = to - (p + 3);

    p[1] = (byte)(skip >> 8);
    p[2] = (byte)skip;
}

/*
 * Check a calculator function for validity, optionally storing its encoded
 * representation and add the size of the encoded representation to *psize.
 * Note that we arbitrarily limit the depth of procedure nesting.  pref is
 * known to be a procedure.
 */
#define MAX_PSC_FUNCTION_NESTING 10
private int
check_psc_function(i_ctx_t *i_ctx_p, const ref *pref, int depth, byte *ops, int *psize)
{
    long i;
    uint size = r_size(pref);

    for (i = 0; i < size; ++i) {
	byte no_ops[1 + max(sizeof(int), sizeof(float))];
	byte *p = (ops ? ops + *psize : no_ops);
	ref elt, elt2, elt3;
	ref * delp;
	int code;

	array_get(imemory, pref, i, &elt);
	switch (r_btype(&elt)) {
	case t_integer: {
	    int i = elt.value.intval;

#if ARCH_SIZEOF_INT < ARCH_SIZEOF_LONG
	    if (i != elt.value.intval) /* check for truncation */
		return_error(e_rangecheck);
#endif
	    if (i == (byte)i) {
		*p = PtCr_byte;
		p[1] = (byte)i;
		*psize += 2;
	    } else {
		*p = PtCr_int;
		memcpy(p + 1, &i, sizeof(i));
		*psize += 1 + sizeof(int);
	    }
	    break;
	}
	case t_real: {
	    float f = elt.value.realval;

	    *p = PtCr_float;
	    memcpy(p + 1, &f, sizeof(f));
	    *psize += 1 + sizeof(float);
	    break;
	}
	case t_boolean:
	    *p = (elt.value.boolval ? PtCr_true : PtCr_false);
	    ++*psize;
	    break;
	case t_name:
	    if (!r_has_attr(&elt, a_executable))
		return_error(e_rangecheck);
	    name_string_ref(imemory, &elt, &elt);
	    if (!bytes_compare(elt.value.bytes, r_size(&elt),
			       (const byte *)"true", 4)) {
		*p = PtCr_true;
	        ++*psize;
	        break;
	    }
	    if (!bytes_compare(elt.value.bytes, r_size(&elt),
				      (const byte *)"false", 5)) {
		*p = PtCr_false;
	        ++*psize;
	        break;
	    }
	    /* Check if the name is a valid operator in systemdict */
	    if (dict_find(systemdict, &elt, &delp) <= 0)
		return_error(e_undefined);
	    if (r_btype(delp) != t_operator)
		return_error(e_typecheck);
	    if (!r_has_attr(delp, a_executable))
		return_error(e_rangecheck);
	    elt = *delp;
	    /* Fall into the operator case */
	case t_operator: {
	    int j;

	    for (j = 0; j < countof(calc_ops); ++j)
		if (elt.value.opproc == calc_ops[j].proc) {
		    *p = calc_ops[j].opcode;
		    ++*psize;
		    goto next;
		}
	    return_error(e_rangecheck);
	}
	default: {
	    if (!r_is_proc(&elt))
		return_error(e_typecheck);
	    if (depth == MAX_PSC_FUNCTION_NESTING)
		return_error(e_limitcheck);
	    if ((code = array_get(imemory, pref, ++i, &elt2)) < 0)
		return code;
	    *psize += 3;
	    code = check_psc_function(i_ctx_p, &elt, depth + 1, ops, psize);
	    if (code < 0)
		return code;
	    /* Check for {proc} if | {proc1} {proc2} ifelse */
#define R_IS_OPER(pref, proc)\
  (r_btype(pref) == t_operator && r_has_attr(pref, a_executable) &&\
   (pref)->value.opproc == proc)
	    if (R_IS_OPER(&elt2, zif)) {
		if (ops) {
		    *p = PtCr_if;
		    psc_fixup(p, ops + *psize);
		}
	    } else if (!r_is_proc(&elt2))
		return_error(e_rangecheck);
	    else if ((code == array_get(imemory, pref, ++i, &elt3)) < 0)
		return code;
	    else if (R_IS_OPER(&elt3, zifelse)) {
		if (ops) {
		    *p = PtCr_if;
		    psc_fixup(p, ops + *psize + 3);
		    p = ops + *psize;
		    *p = PtCr_else;
		}
		*psize += 3;
		code = check_psc_function(i_ctx_p, &elt2, depth + 1, ops, psize);
		if (code < 0)
		    return code;
		if (ops)
		    psc_fixup(p, ops + *psize);
	    } else
		return_error(e_rangecheck);
#undef R_IS_OPER
	}
	}
    next:
	DO_NOTHING;
    }
    return 0;
}
#undef MAX_PSC_FUNCTION_NESTING

/* Check prototype */
build_function_proc(gs_build_function_4);

/* Finish building a FunctionType 4 (PostScript Calculator) function. */
int
gs_build_function_4(i_ctx_t *i_ctx_p, const ref *op, const gs_function_params_t * mnDR,
		    int depth, gs_function_t ** ppfn, gs_memory_t *mem)
{
    gs_function_PtCr_params_t params;
    ref *proc;
    int code;
    byte *ops;
    int size;

    *(gs_function_params_t *)&params = *mnDR;
    params.ops.data = 0;	/* in case of failure */
    params.ops.size = 0;	/* ditto */
    if (dict_find_string(op, "Function", &proc) <= 0) {
	code = gs_note_error(e_rangecheck);
	goto fail;
    }
    if (!r_is_proc(proc)) {
	code = gs_note_error(e_typecheck);
	goto fail;
    }
    size = 0;
    code = check_psc_function(i_ctx_p, proc, 0, NULL, &size);
    if (code < 0)
	goto fail;
    ops = gs_alloc_string(mem, size + 1, "gs_build_function_4(ops)");
    if (ops == 0) {
	code = gs_note_error(e_VMerror);
	goto fail;
    }
    size = 0;
    check_psc_function(i_ctx_p, proc, 0, ops, &size); /* can't fail */
    ops[size] = PtCr_return;
    params.ops.data = ops;
    params.ops.size = size + 1;
    code = gs_function_PtCr_init(ppfn, &params, mem);
    if (code >= 0)
	return 0;
    /* free_params will free the ops string */
fail:
    gs_function_PtCr_free_params(&params, mem);
    return (code < 0 ? code : gs_note_error(e_rangecheck));
}
