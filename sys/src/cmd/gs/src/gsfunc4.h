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

/*$Id: gsfunc4.h,v 1.2 2000/09/19 19:00:28 lpd Exp $ */
/* Definitions for "PostScript Calculator" Functions */

#ifndef gsfunc4_INCLUDED
#  define gsfunc4_INCLUDED

#include "gsfunc.h"

/* ---------------- Types and structures ---------------- */

/* Define the Function type. */
#define function_type_PostScript_Calculator 4

/* Define the opcodes. */
typedef enum {

    /* Arithmetic operators */

    PtCr_abs, PtCr_add, PtCr_and, PtCr_atan, PtCr_bitshift,
    PtCr_ceiling, PtCr_cos, PtCr_cvi, PtCr_cvr, PtCr_div, PtCr_exp,
    PtCr_floor, PtCr_idiv, PtCr_ln, PtCr_log, PtCr_mod, PtCr_mul,
    PtCr_neg, PtCr_not, PtCr_or, PtCr_round,
    PtCr_sin, PtCr_sqrt, PtCr_sub, PtCr_truncate, PtCr_xor,

    /* Comparison operators */

    PtCr_eq, PtCr_ge, PtCr_gt, PtCr_le, PtCr_lt, PtCr_ne,

    /* Stack operators */

    PtCr_copy, PtCr_dup, PtCr_exch, PtCr_index, PtCr_pop, PtCr_roll,

    /* Constants */

    PtCr_byte, PtCr_int /* native */, PtCr_float /* native */,
    PtCr_true, PtCr_false,

    /* Special operators */

    PtCr_if, PtCr_else, PtCr_return

} gs_PtCr_opcode_t;
#define PtCr_NUM_OPS ((int)PtCr_byte)
#define PtCr_NUM_OPCODES ((int)PtCr_return + 1)

/* Define PostScript Calculator functions. */
typedef struct gs_function_PtCr_params_s {
    gs_function_params_common;
    gs_const_string ops;	/* gs_PtCr_opcode_t[] */
} gs_function_PtCr_params_t;

/****** NEEDS TO INCLUDE data_source ******/
#define private_st_function_PtCr()	/* in gsfunc4.c */\
  gs_private_st_suffix_add_strings1(st_function_PtCr, gs_function_PtCr_t,\
    "gs_function_PtCr_t", function_PtCr_enum_ptrs, function_PtCr_reloc_ptrs,\
    st_function, params.ops)

/* ---------------- Procedures ---------------- */

/* Allocate and initialize a PostScript Calculator function. */
int gs_function_PtCr_init(P3(gs_function_t ** ppfn,
			     const gs_function_PtCr_params_t * params,
			     gs_memory_t * mem));

/* Free the parameters of a PostScript Calculator function. */
void gs_function_PtCr_free_params(P2(gs_function_PtCr_params_t * params,
				     gs_memory_t * mem));

#endif /* gsfunc4_INCLUDED */
