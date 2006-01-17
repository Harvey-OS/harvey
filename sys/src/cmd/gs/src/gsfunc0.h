/* Copyright (C) 1997, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gsfunc0.h,v 1.7 2005/04/19 07:27:32 igor Exp $ */
/* Definitions for FunctionType 0 (Sampled) Functions */

#ifndef gsfunc0_INCLUDED
#  define gsfunc0_INCLUDED

#include "gsfunc.h"
#include "gsdsrc.h"

/* ---------------- Types and structures ---------------- */

/* Define the Function type. */
#define function_type_Sampled 0

/* Define Sampled functions. */
typedef struct gs_function_Sd_params_s {
    gs_function_params_common;
    int Order;			/* 1 or 3, optional */
    gs_data_source_t DataSource;
    int BitsPerSample;		/* 1, 2, 4, 8, 12, 16, 24, 32 */
    const float *Encode;	/* 2 x m, optional */
    const float *Decode;	/* 2 x n, optional */
    const int *Size;		/* m */
    double *pole;               /* cached poles (internal data) */
    int *array_step;		/* pole array element step by dimensions */
    int *stream_step;		/* sample stream element step by dimensions */
    int array_size;		/* The number of elements in the pole array. */
} gs_function_Sd_params_t;

#define private_st_function_Sd()	/* in gsfunc.c */\
  gs_private_st_composite(st_function_Sd, gs_function_Sd_t,\
    "gs_function_Sd_t", function_Sd_enum_ptrs, function_Sd_reloc_ptrs)

/* ---------------- Procedures ---------------- */

/* Allocate and initialize a Sampled function. */
int gs_function_Sd_init(gs_function_t ** ppfn,
			const gs_function_Sd_params_t * params,
			gs_memory_t * mem);

/* Free the parameters of a Sampled function. */
void gs_function_Sd_free_params(gs_function_Sd_params_t * params,
				gs_memory_t * mem);

#endif /* gsfunc0_INCLUDED */
