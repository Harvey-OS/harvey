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

/* $Id: zfunc.c,v 1.14 2004/08/04 19:36:13 stefan Exp $ */
/* Generic PostScript language interface to Functions */
#include "memory_.h"
#include "ghost.h"
#include "oper.h"
#include "gscdefs.h"
#include "gsfunc.h"
#include "gsstruct.h"
#include "ialloc.h"
#include "idict.h"
#include "idparam.h"
#include "ifunc.h"
#include "store.h"

/*#define TEST*/

/* Define the maximum depth of nesting of subsidiary functions. */
#define MAX_SUB_FUNCTION_DEPTH 3

/* ------ Operators ------ */

/* Create a function procedure from a function structure. */
private int
make_function_proc(i_ctx_t *i_ctx_p, ref *op, gs_function_t *pfn)
{
    ref cref;			/* closure */
    int code;

    code = ialloc_ref_array(&cref, a_executable | a_execute, 2,
			    ".buildfunction");
    if (code < 0)
	return code;
    make_istruct_new(cref.value.refs, a_executable | a_execute, pfn);
    make_oper_new(cref.value.refs + 1, 0, zexecfunction);
    ref_assign(op, &cref);
    return 0;
}

/* <dict> .buildfunction <function_proc> */
private int
zbuildfunction(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    gs_function_t *pfn;
    int code = fn_build_function(i_ctx_p, op, &pfn, imemory);

    if (code < 0)
	return code;
    code = make_function_proc(i_ctx_p, op, pfn);
    if (code < 0)
	gs_function_free(pfn, true, imemory);
    return 0;
}

#ifdef TEST

/* <function_proc> <array> .scalefunction <function_proc> */
private int
zscalefunction(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    gs_function_t *pfn;
    gs_function_t *psfn;
    gs_range_t *ranges;
    int code;
    uint i;

    check_proc(op[-1]);
    pfn = ref_function(op - 1);
    if (pfn == 0 || !r_is_array(op))
	return_error(e_typecheck);
    if (r_size(op) != 2 * pfn->params.n)
	return_error(e_rangecheck);
    ranges = (gs_range_t *)
	gs_alloc_byte_array(imemory, pfn->params.n, sizeof(gs_range_t),
			    "zscalefunction");
    if (ranges == 0)
	return_error(e_VMerror);
    for (i = 0; i < pfn->params.n; ++i) {
	ref rval[2];
	float val[2];

	if ((code = array_get(op, 2 * i, &rval[0])) < 0 ||
	    (code = array_get(op, 2 * i + 1, &rval[1])) < 0 ||
	    (code = float_params(rval + 1, 2, val)) < 0)
	    return code;
	ranges[i].rmin = val[0];
	ranges[i].rmax = val[1];
    }
    code = gs_function_make_scaled(pfn, &psfn, ranges, imemory);
    gs_free_object(imemory, ranges, "zscalefunction");
    if (code < 0 ||
	(code = make_function_proc(i_ctx_p, op - 1, psfn)) < 0) {
	gs_function_free(psfn, true, imemory);
	return code;
    }
    pop(1);
    return 0;
}

#endif /* TEST */

/* <in1> ... <function_struct> %execfunction <out1> ... */
int
zexecfunction(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;

	/*
	 * Since this operator's name begins with %, the name is not defined
	 * in systemdict.  The only place this operator can ever appear is
	 * in the execute-only closure created by .buildfunction.
	 * Therefore, in principle it is unnecessary to check the argument.
	 * However, we do a little checking anyway just on general
	 * principles.  Note that since the argument may be an instance of
	 * any subclass of gs_function_t, we currently have no way to check
	 * its type.
	 */
    if (!r_is_struct(op) ||
	!r_has_masked_attrs(op, a_executable | a_execute, a_executable | a_all)
	)
	return_error(e_typecheck);
    {
	gs_function_t *pfn = (gs_function_t *) op->value.pstruct;
	int m = pfn->params.m, n = pfn->params.n;
	int diff = n - (m + 1);

	if (diff > 0)
	    check_ostack(diff);
	{
	    float params[20];	/* arbitrary size, just to avoid allocs */
	    float *in;
	    float *out;
	    int code = 0;

	    if (m + n <= countof(params)) {
		in = params;
	    } else {
		in = (float *)ialloc_byte_array(m + n, sizeof(float),
						"%execfunction(in/out)");
		if (in == 0)
		    code = gs_note_error(e_VMerror);
	    }
	    out = in + m;
	    if (code < 0 ||
		(code = float_params(op - 1, m, in)) < 0 ||
		(code = gs_function_evaluate(pfn, in, out)) < 0
		)
		DO_NOTHING;
	    else {
		if (diff > 0)
		    push(diff);	/* can't fail */
		else if (diff < 0) {
		    pop(-diff);
		    op = osp;
		}
		code = make_floats(op + 1 - n, out, n);
	    }
	    if (in != params)
		ifree_object(in, "%execfunction(in)");
	    return code;
	}
    }
}

/*
 * <proc> .isencapfunction <bool>
 *
 * This routine checks if a given Postscript procedure is an "encapsulated"
 * function of the type made by .buildfunction.  These functions can then
 * be executed without executing the interpreter.  These functions can be
 * executed directly from within C code inside the graphics library.
 */
private int
zisencapfunction(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    gs_function_t *pfn;

    check_proc(*op);
    pfn = ref_function(op);
    make_bool(op, pfn != NULL);
    return 0;
}

/* ------ Procedures ------ */

/* Build a function structure from a PostScript dictionary. */
int
fn_build_function(i_ctx_t *i_ctx_p, const ref * op, gs_function_t ** ppfn, gs_memory_t *mem)
{
    return fn_build_sub_function(i_ctx_p, op, ppfn, 0, mem);
}
int
fn_build_sub_function(i_ctx_t *i_ctx_p, const ref * op, gs_function_t ** ppfn,
		      int depth, gs_memory_t *mem)
{
    int code, type, i;
    gs_function_params_t params;

    if (depth > MAX_SUB_FUNCTION_DEPTH)
	return_error(e_limitcheck);
    check_type(*op, t_dictionary);
    code = dict_int_param(op, "FunctionType", 0, max_int, -1, &type);
    if (code < 0)
	return code;
    for (i = 0; i < build_function_type_table_count; ++i)
	if (build_function_type_table[i].type == type)
	    break;
    if (i == build_function_type_table_count)
	return_error(e_rangecheck);
    /* Collect parameters common to all function types. */
    params.Domain = 0;
    params.Range = 0;
    code = fn_build_float_array(op, "Domain", true, true, &params.Domain, mem);
    if (code < 0)
	goto fail;
    params.m = code >> 1;
    code = fn_build_float_array(op, "Range", false, true, &params.Range, mem);
    if (code < 0)
	goto fail;
    params.n = code >> 1;
    /* Finish building the function. */
    /* If this fails, it will free all the parameters. */
    return (*build_function_type_table[i].proc)
	(i_ctx_p, op, &params, depth + 1, ppfn, mem);
fail:
    gs_free_const_object(mem, params.Range, "Range");
    gs_free_const_object(mem, params.Domain, "Domain");
    return code;
}

/*
 * Collect a heap-allocated array of floats.  If the key is missing, set
 * *pparray = 0 and return 0; otherwise set *pparray and return the number
 * of elements.  Note that 0-length arrays are acceptable, so if the value
 * returned is 0, the caller must check whether *pparray == 0.
 */
int
fn_build_float_array(const ref * op, const char *kstr, bool required,
		     bool even, const float **pparray, gs_memory_t *mem)
{
    ref *par;
    int code;

    *pparray = 0;
    if (dict_find_string(op, kstr, &par) <= 0)
	return (required ? gs_note_error(e_rangecheck) : 0);
    if (!r_is_array(par))
	return_error(e_typecheck);
    {
	uint size = r_size(par);
	float *ptr = (float *)
	    gs_alloc_byte_array(mem, size, sizeof(float), kstr);

	if (ptr == 0)
	    return_error(e_VMerror);
	code = dict_float_array_check_param(mem, op, kstr, size, 
					    ptr, NULL,
					    0, e_rangecheck);
	if (code < 0 || (even && (code & 1) != 0)) {
	    gs_free_object(mem, ptr, kstr);
	    return(code < 0 ? code : gs_note_error(e_rangecheck));
	}
	*pparray = ptr;
    }
    return code;
}

/*
 * Similar to fn_build_float_array() except
 * - numeric parameter is accepted and converted to 1-element array
 * - number of elements is not checked for even/odd
 */
int
fn_build_float_array_forced(const ref * op, const char *kstr, bool required,
		     const float **pparray, gs_memory_t *mem)
{
    ref *par;
    int code;
    uint size;
    float *ptr;

    *pparray = 0;
    if (dict_find_string(op, kstr, &par) <= 0)
	return (required ? gs_note_error(e_rangecheck) : 0);

    if( r_is_array(par) )
	size = r_size(par);
    else if(r_type(par) == t_integer || r_type(par) == t_real)
        size = 1;
    else
	return_error(e_typecheck);
    ptr = (float *)gs_alloc_byte_array(mem, size, sizeof(float), kstr);

    if (ptr == 0)
        return_error(e_VMerror);
    if(r_is_array(par) )    
        code = dict_float_array_check_param(mem, op, kstr, 
					    size, ptr, NULL,
					    0, e_rangecheck);
    else {
        code = dict_float_param(op, kstr, 0., ptr); /* defailt cannot happen */
        if( code == 0 )
            code = 1;
    }

    if (code < 0 ) {
        gs_free_object(mem, ptr, kstr);                          
        return code;
    }
    *pparray = ptr;
    return code;
}

/*
 * If a PostScript object is a Function procedure, return the function
 * object, otherwise return 0.
 */
gs_function_t *
ref_function(const ref *op)
{
    if (r_has_type(op, t_array) &&
	r_has_masked_attrs(op, a_executable | a_execute,
			   a_executable | a_all) &&
	r_size(op) == 2 &&
	r_has_type_attrs(op->value.refs + 1, t_operator, a_executable) &&
	op->value.refs[1].value.opproc == zexecfunction &&
	r_is_struct(op->value.refs) &&
	r_has_masked_attrs(op->value.refs, a_executable | a_execute,
			   a_executable | a_all)
	)
	return (gs_function_t *)op->value.refs->value.pstruct;
    return 0;
}

/* ------ Initialization procedure ------ */

const op_def zfunc_op_defs[] =
{
    {"1.buildfunction", zbuildfunction},
#ifdef TEST
    {"2.scalefunction", zscalefunction},
#endif /* TEST */
    {"1%execfunction", zexecfunction},
    {"1.isencapfunction", zisencapfunction},
    op_def_end(0)
};
