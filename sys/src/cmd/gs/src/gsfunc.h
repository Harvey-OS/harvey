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

/*$Id: gsfunc.h,v 1.1 2000/03/09 08:40:42 lpd Exp $ */
/* Generic definitions for Functions */

#ifndef gsfunc_INCLUDED
#  define gsfunc_INCLUDED

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

/* Define calculation effort values (currently only used for monotonicity). */
typedef enum {
    EFFORT_EASY = 0,
    EFFORT_MODERATE = 1,
    EFFORT_ESSENTIAL = 2
} gs_function_effort_t;

/* Define a generic function, for use as the target type of pointers. */
typedef struct gs_function_params_s {
    gs_function_params_common;
} gs_function_params_t;
typedef struct gs_function_s gs_function_t;
typedef int (*fn_evaluate_proc_t)(P3(const gs_function_t * pfn,
				     const float *in, float *out));
typedef int (*fn_is_monotonic_proc_t)(P4(const gs_function_t * pfn,
					 const float *lower,
					 const float *upper,
					 gs_function_effort_t effort));
typedef void (*fn_free_params_proc_t)(P2(gs_function_params_t * params,
					 gs_memory_t * mem));
typedef void (*fn_free_proc_t)(P3(gs_function_t * pfn,
				  bool free_params, gs_memory_t * mem));
typedef struct gs_function_procs_s {
    fn_evaluate_proc_t evaluate;
    fn_is_monotonic_proc_t is_monotonic;
    fn_free_params_proc_t free_params;
    fn_free_proc_t free;
} gs_function_procs_t;
typedef struct gs_function_head_s {
    gs_function_type_t type;
    gs_function_procs_t procs;
    int is_monotonic;		/* cached when function is created */
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

int gs_function_XxYy_init(P3(gs_function_t **ppfn,
			     const gs_function_XxYy_params_t *params,
			     gs_memory_t *mem));

void gs_function_XxYy_free_params(P2(gs_function_XxYy_params_t *params,
				     gs_memory_t *mem));

 */

/* Evaluate a function. */
#define gs_function_evaluate(pfn, in, out)\
  (*(pfn)->head.procs.evaluate)(pfn, in, out)

/*
 * Test whether a function is monotonic on a given (closed) interval.  If
 * the test requires too much effort, the procedure may return
 * gs_error_undefined; normally, it returns 0 for false, >0 for true,
 * gs_error_rangecheck if any part of the interval is outside the function's
 * domain.  If lower[i] > upper[i], the result is not defined.
 */
#define gs_function_is_monotonic(pfn, lower, upper, effort)\
  (*(pfn)->head.procs.is_monotonic)(pfn, lower, upper, effort)
/*
 * If the function is monotonic, is_monotonic returns the direction of
 * monotonicity for output value N in bits 2N and 2N+1.  (Functions with
 * more than sizeof(int) * 4 - 1 outputs are never identified as monotonic.)
 */
#define FN_MONOTONIC_INCREASING 1
#define FN_MONOTONIC_DECREASING 2

/* Free function parameters. */
#define gs_function_free_params(pfn, mem)\
  (*(pfn)->head.procs.free_params)(&(pfn)->params, mem)

/* Free a function's implementation, optionally including its parameters. */
#define gs_function_free(pfn, free_params, mem)\
  (*(pfn)->head.procs.free)(pfn, free_params, mem)

/* ---------------- Vanilla functions ---------------- */

/*
 * The simplest type of functions, these just store closure data.
 * The client provides the evaluation procedure.
 */

#define function_type_Vanilla (-1)

typedef struct gs_function_Va_params_s {
    gs_function_params_common;
    fn_evaluate_proc_t eval_proc;
    void *eval_data;
    int is_monotonic;
} gs_function_Va_params_t;

typedef struct gs_function_Va_s {
    gs_function_head_t head;
    gs_function_Va_params_t params;
} gs_function_Va_t;

#define private_st_function_Va()	/* in gsfunc.c */\
  gs_private_st_suffix_add1(st_function_Va, gs_function_Va_t,\
    "gs_function_Va_t", function_Va_enum_ptrs, function_Va_reloc_ptrs,\
    st_function, params.eval_data)

int gs_function_Va_init(P3(gs_function_t ** ppfn,
			   const gs_function_Va_params_t * params,
			   gs_memory_t * mem));

void gs_function_Va_free_params(P2(gs_function_Va_params_t * params,
				   gs_memory_t * mem));

#endif /* gsfunc_INCLUDED */
