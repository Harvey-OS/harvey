/* Copyright (C) 2000 Aladdin Enterprises.  All rights reserved.
  
  This file is part of AFPL Ghostscript.
  
  AFPL Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author or
  distributor accepts any responsibility for the consequences of using it, or
  for whether it serves any particular purpose or works at all, unless he or
  she says so in writing.  Refer to the Aladdin Free Public License (the
  "License") for full details.
  
  Every copy of AFPL Ghostscript must include a copy of the License, normally
  in a plain ASCII text file named PUBLIC.  The License grants you the right
  to copy, modify and redistribute AFPL Ghostscript, but only under certain
  conditions described in the License.  Among other things, the License
  requires that the copyright notice and this notice be preserved on all
  copies.
*/

/*$Id: gsfuncv.h,v 1.2 2000/09/19 19:00:28 lpd Exp $ */
/* Definitions for "Vanilla" Functions */

#ifndef gsfuncv_INCLUDED
#  define gsfuncv_INCLUDED

#include "gsfunc.h"

/*
 * The simplest type of Function, "Vanilla" Functions just store closure
 * data.  The client provides the evaluation procedure.
 */

/* ---------------- Types and structures ---------------- */

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

/* ---------------- Procedures ---------------- */

/* Allocate and initialize a Vanilla function. */
int gs_function_Va_init(P3(gs_function_t ** ppfn,
			   const gs_function_Va_params_t * params,
			   gs_memory_t * mem));

/* Free the parameters of a Vanilla function. */
void gs_function_Va_free_params(P2(gs_function_Va_params_t * params,
				   gs_memory_t * mem));

#endif /* gsfuncv_INCLUDED */
