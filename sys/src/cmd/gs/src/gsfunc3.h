/* Copyright (C) 1997, 1998 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gsfunc3.h,v 1.1 2000/03/09 08:40:42 lpd Exp $ */
/* Definitions for LL3 Functions */

#ifndef gsfunc3_INCLUDED
#  define gsfunc3_INCLUDED

#include "gsfunc.h"
#include "gsdsrc.h"

/* ---------------- Types and structures ---------------- */

/*
 * Define the Function types.
 * See gsfunc.h for why gs_function_type_t can't be an enum type.
 */
enum {
    function_type_ExponentialInterpolation = 2,
    function_type_1InputStitching = 3,
    /* For internal use only */
    function_type_ArrayedOutput = -1
};

/* Define Exponential Interpolation functions. */
typedef struct gs_function_ElIn_params_s {
    gs_function_params_common;
    const float *C0;		/* n, optional */
    const float *C1;		/* n, optional */
    float N;
} gs_function_ElIn_params_t;

#define private_st_function_ElIn()	/* in gsfunc.c */\
  gs_private_st_suffix_add2(st_function_ElIn, gs_function_ElIn_t,\
    "gs_function_ElIn_t", function_ElIn_enum_ptrs, function_ElIn_reloc_ptrs,\
    st_function, params.C0, params.C1)

/* Define 1-Input Stitching functions. */
typedef struct gs_function_1ItSg_params_s {
    gs_function_params_common;
    int k;
    const gs_function_t *const *Functions;	/* k */
    const float *Bounds;	/* k - 1 */
    const float *Encode;	/* 2 x k */
} gs_function_1ItSg_params_t;

#define private_st_function_1ItSg()	/* in gsfunc.c */\
  gs_private_st_suffix_add3(st_function_1ItSg, gs_function_1ItSg_t,\
    "gs_function_1ItSg_t", function_1ItSg_enum_ptrs, function_1ItSg_reloc_ptrs,\
    st_function, params.Functions, params.Bounds, params.Encode)

/*
 * Define Arrayed Output functions.  These consist of n m x 1 functions
 * whose outputs are assembled into the output of the arrayed function.
 * We use them to handle certain PostScript constructs that can accept
 * either a single n-output function or n 1-output functions.
 *
 * Note that for this type, and only this type, both Domain and Range
 * are ignored (0).
 */
typedef struct gs_function_AdOt_params_s {
    gs_function_params_common;
    const gs_function_t *const *Functions;	/* n */
} gs_function_AdOt_params_t;

#define private_st_function_AdOt()	/* in gsfunc.c */\
  gs_private_st_suffix_add1(st_function_AdOt, gs_function_AdOt_t,\
    "gs_function_AdOt_t", function_AdOt_enum_ptrs, function_AdOt_reloc_ptrs,\
    st_function, params.Functions)

/* ---------------- Procedures ---------------- */

/* Allocate and initialize functions of specific types. */
int gs_function_ElIn_init(P3(gs_function_t ** ppfn,
			     const gs_function_ElIn_params_t * params,
			     gs_memory_t * mem));
int gs_function_1ItSg_init(P3(gs_function_t ** ppfn,
			      const gs_function_1ItSg_params_t * params,
			      gs_memory_t * mem));
int gs_function_AdOt_init(P3(gs_function_t ** ppfn,
			     const gs_function_AdOt_params_t * params,
			     gs_memory_t * mem));

/* Free parameters of specific types. */
void gs_function_ElIn_free_params(P2(gs_function_ElIn_params_t * params,
				     gs_memory_t * mem));
void gs_function_1ItSg_free_params(P2(gs_function_1ItSg_params_t * params,
				      gs_memory_t * mem));
void gs_function_AdOt_free_params(P2(gs_function_AdOt_params_t * params,
				     gs_memory_t * mem));

#endif /* gsfunc3_INCLUDED */
