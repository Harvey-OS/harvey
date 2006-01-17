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

/* $Id: gsfunc.c,v 1.12 2005/04/19 14:35:12 igor Exp $ */
/* Generic Function support */
#include "memory_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsparam.h"
#include "gxfunc.h"
#include "stream.h"

/* GC descriptors */
public_st_function();
gs_private_st_ptr(st_function_ptr, gs_function_t *, "gs_function_t *",
		  function_ptr_enum_ptrs, function_ptr_reloc_ptrs);
gs_private_st_element(st_function_ptr_element, gs_function_t *,
		      "gs_function_t *[]", function_ptr_element_enum_ptrs,
		      function_ptr_element_reloc_ptrs, st_function_ptr);

/* Allocate an array of function pointers. */
int
alloc_function_array(uint count, gs_function_t *** pFunctions,
		     gs_memory_t *mem)
{
    gs_function_t **ptr;

    if (count == 0)
	return_error(gs_error_rangecheck);
    ptr = gs_alloc_struct_array(mem, count, gs_function_t *,
				&st_function_ptr_element, "Functions");
    if (ptr == 0)
	return_error(gs_error_VMerror);
    memset(ptr, 0, sizeof(*ptr) * count);
    *pFunctions = ptr;
    return 0;
}

/* Generic free_params implementation. */
void
fn_common_free_params(gs_function_params_t * params, gs_memory_t * mem)
{
    gs_free_const_object(mem, params->Range, "Range");
    gs_free_const_object(mem, params->Domain, "Domain");
}

/* Generic free implementation. */
void
fn_common_free(gs_function_t * pfn, bool free_params, gs_memory_t * mem)
{
    if (free_params)
	gs_function_free_params(pfn, mem);
    gs_free_object(mem, pfn, "fn_common_free");
}

/* Check the values of m, n, Domain, and (if supplied) Range. */
int
fn_check_mnDR(const gs_function_params_t * params, int m, int n)
{
    int i;

    if (m <= 0 || n <= 0)
	return_error(gs_error_rangecheck);
    for (i = 0; i < m; ++i)
	if (params->Domain[2 * i] > params->Domain[2 * i + 1])
	    return_error(gs_error_rangecheck);
    if (params->Range != 0)
	for (i = 0; i < n; ++i)
	    if (params->Range[2 * i] > params->Range[2 * i + 1])
		return_error(gs_error_rangecheck);
    return 0;
}

/* Return default function information. */
void
gs_function_get_info_default(const gs_function_t *pfn, gs_function_info_t *pfi)
{
    pfi->DataSource = 0;
    pfi->Functions = 0;
}

/*
 * Write generic parameters (FunctionType, Domain, Range) on a parameter list.
 */
int
fn_common_get_params(const gs_function_t *pfn, gs_param_list *plist)
{
    int ecode = param_write_int(plist, "FunctionType", &FunctionType(pfn));
    int code;

    if (pfn->params.Domain) {
	code = param_write_float_values(plist, "Domain", pfn->params.Domain,
					2 * pfn->params.m, false);
	if (code < 0)
	    ecode = code;
    }
    if (pfn->params.Range) {
	code = param_write_float_values(plist, "Range", pfn->params.Range,
					2 * pfn->params.n, false);
	if (code < 0)
	    ecode = code;
    }
    return ecode;
}

/*
 * Copy an array of numeric values when scaling a function.
 */
void *
fn_copy_values(const void *pvalues, int count, int size, gs_memory_t *mem)
{
    if (pvalues) {
	void *values = gs_alloc_byte_array(mem, count, size, "fn_copy_values");

	if (values)
	    memcpy(values, pvalues, count * size);
	return values;
    } else
	return 0;		/* caller must check */
}

/*
 * If necessary, scale the Range or Decode array for fn_make_scaled.
 * Note that we must always allocate a new array.
 */
int
fn_scale_pairs(const float **ppvalues, const float *pvalues, int npairs,
	       const gs_range_t *pranges, gs_memory_t *mem)
{
    if (pvalues == 0)
	*ppvalues = 0;
    else {
	float *out = (float *)
	    gs_alloc_byte_array(mem, 2 * npairs, sizeof(*pvalues),
				"fn_scale_pairs");

	*ppvalues = out;
	if (out == 0)
	    return_error(gs_error_VMerror);
	if (pranges) {
	    /* Allocate and compute scaled ranges. */
	    int i;
	    for (i = 0; i < npairs; ++i) {
		double base = pranges[i].rmin, factor = pranges[i].rmax - base;

		out[2 * i] = pvalues[2 * i] * factor + base;
		out[2 * i + 1] = pvalues[2 * i + 1] * factor + base;
	    }
	} else
	    memcpy(out, pvalues, 2 * sizeof(*pvalues) * npairs);
    }
    return 0;
}

/*
 * Scale the generic part of a function (Domain and Range).
 * The client must have copied the parameters already.
 */
int
fn_common_scale(gs_function_t *psfn, const gs_function_t *pfn,
		const gs_range_t *pranges, gs_memory_t *mem)
{
    int code;

    psfn->head = pfn->head;
    psfn->params.Domain = 0;		/* in case of failure */
    psfn->params.Range = 0;
    if ((code = fn_scale_pairs(&psfn->params.Domain, pfn->params.Domain,
			       pfn->params.m, NULL, mem)) < 0 ||
	(code = fn_scale_pairs(&psfn->params.Range, pfn->params.Range,
			       pfn->params.n, pranges, mem)) < 0)
	return code;
    return 0;
}

/* Serialize. */
int
fn_common_serialize(const gs_function_t * pfn, stream *s)
{
    uint n;
    const gs_function_params_t * p = &pfn->params;
    int code = sputs(s, (const byte *)&pfn->head.type, sizeof(pfn->head.type), &n);
    const float dummy[8] = {0, 0, 0, 0,  0, 0, 0, 0};

    if (code < 0)
	return code;
    code = sputs(s, (const byte *)&p->m, sizeof(p->m), &n);
    if (code < 0)
	return code;
    code = sputs(s, (const byte *)&p->Domain[0], sizeof(p->Domain[0]) * p->m * 2, &n);
    if (code < 0)
	return code;
    code = sputs(s, (const byte *)&p->n, sizeof(p->n), &n);
    if (code < 0)
	return code;
    if (p->Range == NULL && p->n * 2 > count_of(dummy))
	return_error(gs_error_unregistered); /* Unimplemented. */
    return sputs(s, (const byte *)(p->Range != NULL ? &p->Range[0] : dummy), 
	    sizeof(p->Range[0]) * p->n * 2, &n);
}

