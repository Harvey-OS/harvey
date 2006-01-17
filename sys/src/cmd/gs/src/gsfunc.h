/* Copyright (C) 1997, 2000, 2002 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gsfunc.h,v 1.13 2005/04/19 14:35:12 igor Exp $ */
/* Generic definitions for Functions */

#ifndef gsfunc_INCLUDED
#  define gsfunc_INCLUDED

#include "gstypes.h"		/* for gs_range_t */

#ifndef stream_DEFINED
#  define stream_DEFINED
typedef struct stream_s stream;
#endif

/* ---------------- Types and structures ---------------- */

/*
 * gs_function_type_t is defined as equivalent to int, rather than as an
 * enum type, because we can't enumerate all its possible values here in the
 * generic definitions.
 */
typedef int gs_function_type_t;

/*
 * Define information common to all Function types.
 * We separate the private part from the parameters so that
 * clients can create statically initialized parameter structures.
 */
#define gs_function_params_common\
    int m;			/* # of inputs */\
    const float *Domain;	/* 2 x m */\
    int n;			/* # of outputs */\
    const float *Range		/* 2 x n, optional except for type 0 */

/* Define abstract types. */
#ifndef gs_data_source_DEFINED
#  define gs_data_source_DEFINED
typedef struct gs_data_source_s gs_data_source_t;
#endif
#ifndef gs_param_list_DEFINED
#  define gs_param_list_DEFINED
typedef struct gs_param_list_s gs_param_list;
#endif

/* Define a generic function, for use as the target type of pointers. */
typedef struct gs_function_params_s {
    gs_function_params_common;
} gs_function_params_t;
#ifndef gs_function_DEFINED
typedef struct gs_function_s gs_function_t;
#  define gs_function_DEFINED
#endif
typedef struct gs_function_info_s {
    const gs_data_source_t *DataSource;
    ulong data_size;
    const gs_function_t *const *Functions;
    int num_Functions;
} gs_function_info_t;

/* Evaluate a function. */
#define FN_EVALUATE_PROC(proc)\
  int proc(const gs_function_t * pfn, const float *in, float *out)
typedef FN_EVALUATE_PROC((*fn_evaluate_proc_t));

/* Test whether a function is monotonic. */
#define FN_IS_MONOTONIC_PROC(proc)\
  int proc(const gs_function_t * pfn, const float *lower,\
	   const float *upper, uint *mask)
typedef FN_IS_MONOTONIC_PROC((*fn_is_monotonic_proc_t));

/* Get function information. */
#define FN_GET_INFO_PROC(proc)\
  void proc(const gs_function_t *pfn, gs_function_info_t *pfi)
typedef FN_GET_INFO_PROC((*fn_get_info_proc_t));

/* Put function parameters on a parameter list. */
#define FN_GET_PARAMS_PROC(proc)\
  int proc(const gs_function_t *pfn, gs_param_list *plist)
typedef FN_GET_PARAMS_PROC((*fn_get_params_proc_t));

/*
 * Create a new function with scaled output.  The i'th output value is
 * transformed linearly so that [0 .. 1] are mapped to [pranges[i].rmin ..
 * pranges[i].rmax].  Any necessary parameters or subfunctions of the
 * original function are copied, not shared, even if their values aren't
 * changed, so that the new function can be freed without having to worry
 * about freeing data that should be kept.  Note that if there is a "data
 * source", it is shared, not copied: this should not be a problem, since
 * gs_function_free does not free the data source.
 */
#define FN_MAKE_SCALED_PROC(proc)\
  int proc(const gs_function_t *pfn, gs_function_t **ppsfn,\
	   const gs_range_t *pranges, gs_memory_t *mem)
typedef FN_MAKE_SCALED_PROC((*fn_make_scaled_proc_t));

/* Free function parameters. */
#define FN_FREE_PARAMS_PROC(proc)\
  void proc(gs_function_params_t * params, gs_memory_t * mem)
typedef FN_FREE_PARAMS_PROC((*fn_free_params_proc_t));

/* Free a function. */
#define FN_FREE_PROC(proc)\
  void proc(gs_function_t * pfn, bool free_params, gs_memory_t * mem)
typedef FN_FREE_PROC((*fn_free_proc_t));

/* Serialize a function. */
#define FN_SERIALIZE_PROC(proc)\
  int proc(const gs_function_t * pfn, stream *s)
typedef FN_SERIALIZE_PROC((*fn_serialize_proc_t));

/* Define the generic function structures. */
typedef struct gs_function_procs_s {
    fn_evaluate_proc_t evaluate;
    fn_is_monotonic_proc_t is_monotonic;
    fn_get_info_proc_t get_info;
    fn_get_params_proc_t get_params;
    fn_make_scaled_proc_t make_scaled;
    fn_free_params_proc_t free_params;
    fn_free_proc_t free;
    fn_serialize_proc_t serialize;
} gs_function_procs_t;
typedef struct gs_function_head_s {
    gs_function_type_t type;
    gs_function_procs_t procs;
} gs_function_head_t;
struct gs_function_s {
    gs_function_head_t head;
    gs_function_params_t params;
};

#define FunctionType(pfn) ((pfn)->head.type)

/*
 * Each specific function type has a definition in its own header file
 * for its parameter record.  In order to keep names from overflowing
 * various compilers' limits, we take the name of the function type and
 * reduce it to the first and last letter of each word, e.g., for
 * Sampled functions, XxYy is Sd.

typedef struct gs_function_XxYy_params_s {
     gs_function_params_common;
    << P additional members >>
} gs_function_XxYy_params_t;
#define private_st_function_XxYy()\
  gs_private_st_suffix_addP(st_function_XxYy, gs_function_XxYy_t,\
    "gs_function_XxYy_t", function_XxYy_enum_ptrs, function_XxYy_reloc_ptrs,\
    st_function, <<params.additional_members>>)

 */

/* ---------------- Procedures ---------------- */

/*
 * Each specific function type has a pair of procedures in its own
 * header file, one to allocate and initialize an instance of that type,
 * and one to free the parameters of that type.

int gs_function_XxYy_init(gs_function_t **ppfn,
			  const gs_function_XxYy_params_t *params,
			  gs_memory_t *mem));

void gs_function_XxYy_free_params(gs_function_XxYy_params_t *params,
				  gs_memory_t *mem);

 */

/* Allocate an array of function pointers. */
int alloc_function_array(uint count, gs_function_t *** pFunctions,
			 gs_memory_t *mem);

/* Evaluate a function. */
#define gs_function_evaluate(pfn, in, out)\
  ((pfn)->head.procs.evaluate)(pfn, in, out)

/*
 * Test whether a function is monotonic on a given (closed) interval.
 * return 1 = monotonic, 0 = not or don't know, <0 = error..
 * Sets mask : 1 bit per dimension : 
 *    1 - non-monotonic or don't know, 
 *    0 - monotonic.
 * If lower[i] > upper[i], the result may be not defined.
 */
#define gs_function_is_monotonic(pfn, lower, upper, mask)\
  ((pfn)->head.procs.is_monotonic)(pfn, lower, upper, mask)

/* Get function information. */
#define gs_function_get_info(pfn, pfi)\
  ((pfn)->head.procs.get_info(pfn, pfi))

/* Write function parameters. */
#define gs_function_get_params(pfn, plist)\
  ((pfn)->head.procs.get_params(pfn, plist))

/* Create a scaled function. */
#define gs_function_make_scaled(pfn, ppsfn, pranges, mem)\
  ((pfn)->head.procs.make_scaled(pfn, ppsfn, pranges, mem))

/* Free function parameters. */
#define gs_function_free_params(pfn, mem)\
  ((pfn)->head.procs.free_params(&(pfn)->params, mem))

/* Free a function's implementation, optionally including its parameters. */
#define gs_function_free(pfn, free_params, mem)\
  ((pfn)->head.procs.free(pfn, free_params, mem))

/* Serialize a function. */
#define gs_function_serialize(pfn, s)\
  ((pfn)->head.procs.serialize(pfn, s))

#endif /* gsfunc_INCLUDED */
