/* Copyright (C) 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: zfunc4.c,v 1.1 2000/03/09 08:40:45 lpd Exp $ */
/* PostScript language support for FunctionType 4 (PS Calculator) Functions */
#include "memory_.h"
#include "ghost.h"
#include "oper.h"
#include "opextern.h"
#include "gsfunc.h"
#include "stream.h"		/* for files.h */
#include "files.h"
#include "ialloc.h"
#include "idict.h"
#include "idparam.h"
#include "ifunc.h"
#include "istkparm.h"
#include "istruct.h"
#include "store.h"

/*
 * FunctionType 4 functions are not defined in the PostScript language.
 * We provide support for them because they are needed for PDF 1.3.
 * In addition to the standard FunctionType, Domain, and Range keys,
 * they have a Function key whose value is a procedure in a restricted
 * subset of the PostScript language.  Specifically, the procedure must
 * (recursively) contain only integer, real, Boolean, and procedure
 * constants, and operators chosen from the set given below.  Note that
 * names are not allowed: the procedure must be 'bound'.
 *
 * The following list is taken directly from the PDF 1.3 documentation.
 */
#define XOP(zfn) int zfn(P1(i_ctx_t *))
XOP(zabs); XOP(zand); XOP(zatan); XOP(zbitshift);
XOP(zceiling); XOP(zcos); XOP(zcvi); XOP(zcvr);
XOP(zdiv); XOP(zexp); XOP(zfloor); XOP(zidiv);
XOP(zln); XOP(zlog); XOP(zmod); XOP(zmul);
XOP(zneg); XOP(znot); XOP(zor); XOP(zround);
XOP(zsin); XOP(zsqrt); XOP(ztruncate); XOP(zxor);
XOP(zeq); XOP(zge); XOP(zgt); XOP(zle); XOP(zlt); XOP(zne);
#undef XOP
/*
 * Define operators for true and false, so we don't have to treat them
 * as special cases but can bind them.
 */
private int
ztrue(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;

    push(1);
    make_true(op);
    return 0;
}
private int
zfalse(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;

    push(1);
    make_false(op);
    return 0;
}
static const op_proc_t calc_ops[] = {
    /* Arithmetic operators */
    zabs, zadd, zand, zatan, zbitshift, zceiling, zcos, zcvi, zcvr, zdiv, zexp,
    zfloor, zidiv, zln, zlog, zmod, zmul, zneg, znot, zor, zround,
    zsin, zsqrt, zsub, ztruncate, zxor,

    /* Boolean operators */
    /*zand,*/ zeq, zge, zgt, zif, zifelse, zle, zlt, zne, /*znot, zor,*/
    ztrue, zfalse,

    /* Stack operators */
    zcopy, zdup, zexch, zindex, zpop, zroll
};

/* Define a trivial structure that holds a ref pointer. */
typedef struct ref_ptr_s {
    ref *pref;
} ref_ptr_t;
gs_private_st_composite(st_ref_ptr, ref_ptr_t, "ref_ptr_t",
			ref_ptr_enum_ptrs, ref_ptr_reloc_ptrs);
private
ENUM_PTRS_WITH(ref_ptr_enum_ptrs, ref_ptr_t *prp) return 0;
case 0: ENUM_RETURN_REF(prp->pref);
ENUM_PTRS_END
private
RELOC_PTRS_WITH(ref_ptr_reloc_ptrs, ref_ptr_t *prp)
RELOC_REF_VAR(prp->pref);
RELOC_PTRS_END

/*
 * Evaluate a PostScript Calculator function.  This is a very stripped-down
 * version of the PostScript interpreter loop.  Note that the check we
 * made when constructing the function bounds the recursion depth of
 * psc_interpret.
 */
private int
psc_interpret(i_ctx_t *i_ctx_p, const ref *proc)
{
    uint i;
    int code = 0;

    for (i = 0; i < r_size(proc) && code >= 0; ++i) {
	ref elt;

	array_get(proc, (long)i, &elt);
	if (r_btype(&elt) == t_operator) {
	    if (elt.value.opproc == zif) {
		if (!(r_has_type(osp - 1, t_boolean) && r_is_proc(osp)))
		    return_error(e_typecheck);
		osp -= 2;
		if (osp[1].value.boolval) {
		    ref_assign(&elt, osp + 2);
		    code = psc_interpret(i_ctx_p, &elt);
		}
	    } else if (elt.value.opproc == zifelse) {
		if (!(r_has_type(osp - 2, t_boolean) && r_is_proc(osp - 1) &&
		      r_is_proc(osp))
		    )
		    return_error(e_typecheck);
		osp -= 3;
		ref_assign(&elt, (osp[1].value.boolval ? osp + 2 : osp + 3));
		code = psc_interpret(i_ctx_p, &elt);
	    } else {
		code = elt.value.opproc(i_ctx_p);
	    }
	} else {
	    if (osp >= ostop)
		return_error(e_rangecheck);
	    ++osp;
	    ref_assign(osp, &elt);
	}
    }
    return code;
}
private int
fn_psc_evaluate(const gs_function_t * pfn_common, const float *in, float *out)
{
    const gs_function_Va_t *const pfn =
	(const gs_function_Va_t *)pfn_common;
    const ref *const pref = ((const ref_ptr_t *)pfn->params.eval_data)->pref;
    /*
     * Although operators take an i_ctx_t * (a.k.a. gs_context_state_t *)
     * as their operand, the only operators that we can call here use
     * only the operand stack.  We can get away with fabricating a context
     * state out of whole cloth and only filling in its operand stack.
     */
#define MAX_OSTACK 100		/* per PDF 1.3 spec */
#define OS_GUARD 10
    ref values[OS_GUARD + MAX_OSTACK + OS_GUARD];
    i_ctx_t context;
    i_ctx_t *const i_ctx_p = &context;
    ref_stack_t *const pos = &context.op_stack.stack;
    ref_stack_params_t params;
    ref osref;
    int i;
    int code;

    /* Create the operand stack. */
    make_array(&osref, a_all, countof(values), values);
    ref_stack_init(pos, &osref, OS_GUARD, OS_GUARD, NULL, NULL, &params);
    ref_stack_set_error_codes(pos, e_rangecheck, e_rangecheck);
    ref_stack_set_max_count(pos, MAX_OSTACK);
    ref_stack_allow_expansion(pos, false);

    /* Push the input values on the stack. */
    for (i = 0; i < pfn->params.m; ++i) {
	++osp;
	make_real(osp, in[i]);
    }

    /* Execute the procedure. */
    code = psc_interpret(&context, pref);
    if (code < 0)
	return code;

    /* Pop the results from the stack. */
    if (ref_stack_count_inline(pos) != pfn->params.n)
	return_error(e_rangecheck);
    for (i = pfn->params.n; --i >= 0; --osp) {
	switch (r_type(osp)) {
	case t_real:
	    out[i] = osp->value.realval;
	    break;
	case t_integer:
	    out[i] = osp->value.intval;
	    break;
	default:
	    return_error(e_typecheck);
	}
    }

    return 0;
}

/*
 * Check a calculator function for validity.  Note that we arbitrarily
 * limit the depth of procedure nesting.
 */
#define MAX_PSC_FUNCTION_NESTING 6
private int
check_psc_function(const ref *pref, int depth)
{
    long i;
    ref elt;

    switch (r_btype(pref)) {
    case t_integer: case t_real: case t_boolean:
	return 0;
    case t_operator:
	for (i = 0; i < countof(calc_ops); ++i)
	    if (pref->value.opproc == calc_ops[i])
		return 0;
	return_error(e_rangecheck);
    default:
	if (!r_is_proc(pref))
	    return_error(e_typecheck);
	if (depth == MAX_PSC_FUNCTION_NESTING)
	    return_error(e_limitcheck);
	break;
    }
    for (i = r_size(pref); --i >= 0;) {
	int code;

	array_get(pref, i, &elt);
	code = check_psc_function(&elt, depth + 1);
	if (code < 0)
	    return code;
    }
    return 0;
}
#undef MAX_PSC_FUNCTION_NESTING

/* Check prototype */
build_function_proc(gs_build_function_4);

/* Finish building a FunctionType 4 (PostScript Calculator) function. */
int
gs_build_function_4(const ref *op, const gs_function_params_t * mnDR,
		    int depth, gs_function_t ** ppfn, gs_memory_t *mem)
{
    gs_function_Va_params_t params;
    int code = 0;
    ref_ptr_t *prp;
    ref *proc;

    *(gs_function_params_t *)&params = *mnDR;
    params.eval_data = 0;	/* in case of error */
    if (dict_find_string(op, "Function", &proc) <= 0)
	goto fail;
    code = check_psc_function(proc, 0);
    if (code < 0)
	goto fail;
    params.eval_proc = fn_psc_evaluate;
    /*
     * We would like to simply set eval_data = proc.  Unfortunately,
     * eval_data is an ordinary pointer, not a ref pointer.
     * We allocate an extra level of indirection to handle this.
     */
    prp = gs_alloc_struct(mem, ref_ptr_t, &st_ref_ptr, "gs_build_function_4");
    if (prp == 0) {
	code = gs_note_error(e_VMerror);
	goto fail;
    }
    prp->pref = proc;
    params.eval_data = prp;
    code = gs_function_Va_init(ppfn, &params, mem);
    if (code >= 0)
	return 0;
fail:
    gs_function_Va_free_params(&params, mem);
    return (code < 0 ? code : gs_note_error(e_rangecheck));
}

/* ------ Initialization procedure ------ */

const op_def zfunc4_op_defs[] =
{
    {"0.true", ztrue},
    {"0.false", zfalse},
    op_def_end(0)
};
