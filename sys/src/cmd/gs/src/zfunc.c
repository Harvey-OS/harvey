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

/*$Id: zfunc.c,v 1.1 2000/03/09 08:40:45 lpd Exp $ */
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

/* Define the maximum depth of nesting of subsidiary functions. */
#define MAX_SUB_FUNCTION_DEPTH 3

/* GC descriptors */
gs_private_st_ptr(st_function_ptr, gs_function_t *, "gs_function_t *",
		  function_ptr_enum_ptrs, function_ptr_reloc_ptrs);
gs_private_st_element(st_function_ptr_element, gs_function_t *,
		      "gs_function_t *[]", function_ptr_element_enum_ptrs,
		      function_ptr_element_reloc_ptrs, st_function_ptr);

/* ------ Operators ------ */

private int zexecfunction(P1(i_ctx_t *));

/* <dict> .buildfunction <function_struct> */
private int
zbuildfunction(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    gs_function_t *pfn;
    ref cref;			/* closure */
    int code;

    code = ialloc_ref_array(&cref, a_executable | a_execute, 2,
			    ".buildfunction");
    if (code < 0)
	return code;
    code = fn_build_function(op, &pfn, imemory);
    if (code < 0) {
	ifree_ref_array(&cref, ".buildfunction");
	return code;
    }
    make_istruct_new(cref.value.refs, a_executable | a_execute, pfn);
    make_oper_new(cref.value.refs + 1, 0, zexecfunction);
    ref_assign(op, &cref);
    return 0;
}

/* <in1> ... <function_struct> %execfunction <out1> ... */
private int
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
	r_has_masked_attrs(op, a_executable | a_execute, a_all)
	)
	return_error(e_typecheck);
    {
	gs_function_t *pfn = (gs_function_t *) op->value.pstruct;
	int m = pfn->params.m, n = pfn->params.n;
	int diff = n - (m + 1);

	if (diff > 0)
	    check_ostack(diff);
	{
	    float *in = (float *)ialloc_byte_array(m, sizeof(float),
						   "%execfunction(in)");
	    float *out = (float *)ialloc_byte_array(n, sizeof(float),
						    "%execfunction(out)");
	    int code;

	    if (in == 0 || out == 0)
		code = gs_note_error(e_VMerror);
	    else if ((code = float_params(op - 1, m, in)) < 0 ||
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
	    ifree_object(out, "%execfunction(out)");
	    ifree_object(in, "%execfunction(in)");
	    return code;
	}
    }
}

/* ------ Procedures ------ */

/* Build a function structure from a PostScript dictionary. */
int
fn_build_function(const ref * op, gs_function_t ** ppfn, gs_memory_t *mem)
{
    return fn_build_sub_function(op, ppfn, 0, mem);
}
int
fn_build_sub_function(const ref * op, gs_function_t ** ppfn, int depth,
		      gs_memory_t *mem)
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
	(op, &params, depth + 1, ppfn, mem);
fail:
    gs_free_const_object(mem, params.Range, "Range");
    gs_free_const_object(mem, params.Domain, "Domain");
    return code;
}

/* Allocate an array of function objects. */
int
alloc_function_array(uint count, gs_function_t *** pFunctions,
		     gs_memory_t *mem)
{
    gs_function_t **ptr;

    if (count == 0)
	return_error(e_rangecheck);
    ptr = gs_alloc_struct_array(mem, count, gs_function_t *,
				&st_function_ptr_element, "Functions");
    if (ptr == 0)
	return_error(e_VMerror);
    memset(ptr, 0, sizeof(*ptr) * count);
    *pFunctions = ptr;
    return 0;
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
	code = dict_float_array_param(op, kstr, size, ptr, NULL);
	if (code < 0 || (even && (code & 1) != 0)) {
	    gs_free_object(mem, ptr, kstr);
	    return(code < 0 ? code : gs_note_error(e_rangecheck));
	}
	*pparray = ptr;
    }
    return code;
}

/* ------ Initialization procedure ------ */

const op_def zfunc_op_defs[] =
{
    {"1.buildfunction", zbuildfunction},
    {"1%execfunction", zexecfunction},
    op_def_end(0)
};
